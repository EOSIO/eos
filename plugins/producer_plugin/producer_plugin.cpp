/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/net_plugin/net_plugin.hpp>
#include <eosio/chain/producer_object.hpp>

#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>

using std::string;
using std::vector;

namespace eosio {

static appbase::abstract_plugin& _producer_plugin = app().register_plugin<producer_plugin>();

using namespace eosio::chain;

class producer_plugin_impl {
public:
   producer_plugin_impl(boost::asio::io_service& io)
      : _timer(io) {}

   void schedule_production_loop();
   block_production_condition::block_production_condition_enum block_production_loop();
   block_production_condition::block_production_condition_enum maybe_produce_block(fc::mutable_variant_object& capture);

   boost::program_options::variables_map _options;
   bool     _production_enabled                 = false;
   uint32_t _required_producer_participation    = uint32_t(config::required_producer_participation);
   uint32_t _production_skip_flags              = eosio::chain::skip_nothing;

   std::map<chain::public_key_type, chain::private_key_type> _private_keys;
   std::set<chain::account_name>                             _producers;
   boost::asio::deadline_timer                               _timer;
};

void new_chain_banner(const eosio::chain::chain_controller& db)
{
   std::cerr << "\n"
      "*******************************\n"
      "*                             *\n"
      "*   ------ NEW CHAIN ------   *\n"
      "*   -  Welcome to EOSIO!  -   *\n"
      "*   -----------------------   *\n"
      "*                             *\n"
      "*******************************\n"
      "\n";
   if(db.get_slot_at_time(fc::time_point::now()) > 200)
   {
      std::cerr << "Your genesis seems to have an old timestamp\n"
         "Please consider using the --genesis-timestamp option to give your genesis a recent timestamp\n"
         "\n"
         ;
   }
   return;
}

producer_plugin::producer_plugin()
   : my(new producer_plugin_impl(app().get_io_service())){}

producer_plugin::~producer_plugin() {}

void producer_plugin::set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   auto default_priv_key = private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("nathan")));
   auto private_key_default = std::make_pair(default_priv_key.get_public_key(), default_priv_key );

   boost::program_options::options_description producer_options;

   producer_options.add_options()
         ("enable-stale-production", boost::program_options::bool_switch()->notifier([this](bool e){my->_production_enabled = e;}), "Enable block production, even if the chain is stale.")
         ("required-participation", boost::program_options::bool_switch()->notifier([this](int e){my->_required_producer_participation = uint32_t(e*config::percent_1);}), "Percent of producers (0-99) that must be participating in order to produce blocks")
         ("producer-name,p", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          ("ID of producer controlled by this node (e.g. inita; may specify multiple times)"))
         ("private-key", boost::program_options::value<vector<string>>()->composing()->multitoken()->default_value({fc::json::to_string(private_key_default)},
                                                                                                fc::json::to_string(private_key_default)),
          "Tuple of [public key, WIF private key] (may specify multiple times)")
         ;
   config_file_options.add(producer_options);
}

chain::public_key_type producer_plugin::first_producer_public_key() const
{
  chain::chain_controller& chain = app().get_plugin<chain_plugin>().chain();
  try {
    return chain.get_producer(*my->_producers.begin()).signing_key;
  } catch(std::out_of_range) {
    return chain::public_key_type();
  }
}

bool producer_plugin::is_producer_key(const chain::public_key_type& key) const
{
  auto private_key_itr = my->_private_keys.find(key);
  if(private_key_itr != my->_private_keys.end())
    return true;
  return false;
}

chain::signature_type producer_plugin::sign_compact(const chain::public_key_type& key, const fc::sha256& digest) const
{
  if(key != chain::public_key_type()) {
    auto private_key_itr = my->_private_keys.find(key);
    FC_ASSERT(private_key_itr != my->_private_keys.end(), "Local producer has no private key in config.ini corresponding to public key ${key}", ("key", key));

    return private_key_itr->second.sign(digest);
  }
  else {
    return chain::signature_type();
  }
}

template<typename T>
T dejsonify(const string& s) {
   return fc::json::from_string(s).as<T>();
}

#define LOAD_VALUE_SET(options, name, container, type) \
if( options.count(name) ) { \
   const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
   std::copy(ops.begin(), ops.end(), std::inserter(container, container.end())); \
}

void producer_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   my->_options = &options;
   LOAD_VALUE_SET(options, "producer-name", my->_producers, types::account_name)

   if( options.count("private-key") )
   {
      const std::vector<std::string> key_id_to_wif_pair_strings = options["private-key"].as<std::vector<std::string>>();
      for (const std::string& key_id_to_wif_pair_string : key_id_to_wif_pair_strings)
      {
         auto key_id_to_wif_pair = dejsonify<std::pair<public_key_type, private_key_type>>(key_id_to_wif_pair_string);
         my->_private_keys[key_id_to_wif_pair.first] = key_id_to_wif_pair.second;
      }
   }
} FC_LOG_AND_RETHROW() }

void producer_plugin::plugin_startup()
{ try {
   ilog("producer plugin:  plugin_startup() begin");
   chain::chain_controller& chain = app().get_plugin<chain_plugin>().chain();

   if (!my->_producers.empty())
   {
      ilog("Launching block production for ${n} producers.", ("n", my->_producers.size()));
      if(my->_production_enabled)
      {
         if(chain.head_block_num() == 0)
            new_chain_banner(chain);
         my->_production_skip_flags |= eosio::chain::skip_undo_history_check;
      }
      my->schedule_production_loop();
   } else
      elog("No producers configured! Please add producer IDs and private keys to configuration.");
   ilog("producer plugin:  plugin_startup() end");
   } FC_CAPTURE_AND_RETHROW() }

void producer_plugin::plugin_shutdown() {
   try {
      my->_timer.cancel();
   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
   }
}

void producer_plugin_impl::schedule_production_loop() {
   //Schedule for the next second's tick regardless of chain state
   // If we would wait less than 50ms (1/10 of block_interval), wait for the whole block interval.
   fc::time_point now = fc::time_point::now();
   int64_t time_to_next_block_time = (config::block_interval_us) - (now.time_since_epoch().count() % (config::block_interval_us) );
   if(time_to_next_block_time < config::block_interval_us/10 ) {     // we must sleep for at least 50ms
      ilog("Less than ${t}us to next block time, time_to_next_block_time ${bt}us",
           ("t", config::block_interval_us/10)("bt", time_to_next_block_time));
      time_to_next_block_time += config::block_interval_us;
   }

   //_timer.expires_from_now(boost::posix_time::microseconds(time_to_next_block_time));
   _timer.expires_from_now( boost::posix_time::microseconds(time_to_next_block_time) );
   //_timer.async_wait(boost::bind(&producer_plugin_impl::block_production_loop, this));
   _timer.async_wait( [&](const boost::system::error_code&){ block_production_loop(); } );
}

block_production_condition::block_production_condition_enum producer_plugin_impl::block_production_loop() {
   block_production_condition::block_production_condition_enum result;
   fc::mutable_variant_object capture;
   try
   {
      result = maybe_produce_block(capture);
   }
   catch( const fc::canceled_exception& )
   {
      //We're trying to exit. Go ahead and let this one out.
      throw;
   }
   catch( const fc::exception& e )
   {
      elog("Got exception while generating block:\n${e}", ("e", e.to_detail_string()));
      result = block_production_condition::exception_producing_block;
   }

   switch(result)
   {
      case block_production_condition::produced: {
         const auto& db = app().get_plugin<chain_plugin>().chain();
         auto producer  = db.head_block_producer();
 //        auto pending   = db.pending().size();

         wlog("\r${p} generated block ${id}... #${n} @ ${t} with ${count} trxs", ("p", producer)(capture) );
         break;
      }
      case block_production_condition::not_synced:
         ilog("Not producing block because production is disabled until we receive a recent block (see: --enable-stale-production)");
         break;
      case block_production_condition::not_my_turn:
         ilog("Not producing block because it isn't my turn, its ${scheduled_producer}", (capture) );
         break;
      case block_production_condition::not_time_yet:
         ilog("Not producing block because slot has not yet arrived");
         break;
      case block_production_condition::no_private_key:
         ilog("Not producing block because I don't have the private key for ${scheduled_key}", (capture) );
         break;
      case block_production_condition::low_participation:
         elog("Not producing block because node appears to be on a minority fork with only ${pct}% producer participation", (capture) );
         break;
      case block_production_condition::lag:
         elog("Not producing block because node didn't wake up within 500ms of the slot time.");
         break;
      case block_production_condition::consecutive:
         elog("Not producing block because the last block was generated by the same producer.\nThis node is probably disconnected from the network so block production has been disabled.\nDisable this check with --allow-consecutive option.");
         break;
      case block_production_condition::exception_producing_block:
         elog( "exception producing block" );
         break;
   }

   schedule_production_loop();
   return result;
}

block_production_condition::block_production_condition_enum producer_plugin_impl::maybe_produce_block(fc::mutable_variant_object& capture) {
   chain::chain_controller& chain = app().get_plugin<chain_plugin>().chain();
   fc::time_point now = fc::time_point::now();

   if (app().get_plugin<chain_plugin>().is_skipping_transaction_signatures()) {
      _production_skip_flags |= skip_transaction_signatures;
   }
   // If the next block production opportunity is in the present or future, we're synced.
   if( !_production_enabled )
   {
      if( chain.get_slot_time(1) >= now )
         _production_enabled = true;
      else
         return block_production_condition::not_synced;
   }

   // is anyone scheduled to produce now or one second in the future?
   uint32_t slot = chain.get_slot_at_time( now );
   if( slot == 0 )
   {
      capture("next_time", chain.get_slot_time(1));
      return block_production_condition::not_time_yet;
   }

   //
   // this assert should not fail, because now <= db.head_block_time()
   // should have resulted in slot == 0.
   //
   // if this assert triggers, there is a serious bug in get_slot_at_time()
   // which would result in allowing a later block to have a timestamp
   // less than or equal to the previous block
   //
   assert( now > chain.head_block_time() );

   auto scheduled_producer = chain.get_scheduled_producer( slot );
   // we must control the producer scheduled to produce the next block.
   if( _producers.find( scheduled_producer ) == _producers.end() )
   {
      capture("scheduled_producer", scheduled_producer);
      return block_production_condition::not_my_turn;
   }

   auto scheduled_time = chain.get_slot_time( slot );
   eosio::chain::public_key_type scheduled_key = chain.get_producer(scheduled_producer).signing_key;
   auto private_key_itr = _private_keys.find( scheduled_key );

   if( private_key_itr == _private_keys.end() )
   {
      capture("scheduled_key", scheduled_key);
      return block_production_condition::no_private_key;
   }

   uint32_t prate = chain.producer_participation_rate();
   if( prate < _required_producer_participation )
   {
      capture("pct", uint32_t(100*uint64_t(prate) / config::percent_1));
      return block_production_condition::low_participation;
   }

   if( llabs(( time_point(scheduled_time) - now).count()) > fc::milliseconds( config::block_interval_ms ).count() )
   {
      capture("scheduled_time", scheduled_time)("now", now);
      return block_production_condition::lag;
   }


   auto block = chain.generate_block(
      scheduled_time,
      scheduled_producer,
      private_key_itr->second,
      _production_skip_flags
      );

   capture("n", block.block_num())("t", block.timestamp)("c", now)("count",block.input_transactions.size())("id",string(block.id()).substr(8,8));

   app().get_plugin<net_plugin>().broadcast_block(block);
   return block_production_condition::produced;
}

} // namespace eosio
