/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/chain/producer_object.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <eosio/chain/global_property_object.hpp>

#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/scoped_exit.hpp>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>
#include <algorithm>
#include <boost/range/adaptor/map.hpp>
#include <boost/function_output_iterator.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/signals2/connection.hpp>

namespace bmi = boost::multi_index;
using bmi::indexed_by;
using bmi::ordered_non_unique;
using bmi::member;
using bmi::tag;
using bmi::hashed_unique;

using boost::multi_index_container;

using std::string;
using std::vector;
using boost::signals2::scoped_connection;

// HACK TO EXPOSE LOGGER MAP

namespace fc {
   extern std::unordered_map<std::string,logger>& get_logger_map();
}

const fc::string logger_name("producer_plugin");
fc::logger _log;

namespace eosio {

static appbase::abstract_plugin& _producer_plugin = app().register_plugin<producer_plugin>();

using namespace eosio::chain;
using namespace eosio::chain::plugin_interface;

namespace {
   bool failure_is_subjective(const fc::exception& e, bool deadline_is_subjective) {
      auto code = e.code();
      return (code == block_cpu_usage_exceeded::code_value) ||
             (code == block_net_usage_exceeded::code_value) ||
             (code == deadline_exception::code_value && deadline_is_subjective) ||
             (code == leeway_deadline_exception::code_value && deadline_is_subjective);
   }
}

struct transaction_id_with_expiry {
   transaction_id_type     trx_id;
   fc::time_point          expiry;
};

struct by_id;
struct by_expiry;

using transaction_id_with_expiry_index = multi_index_container<
   transaction_id_with_expiry,
   indexed_by<
      hashed_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(transaction_id_with_expiry, transaction_id_type, trx_id)>,
      ordered_non_unique<tag<by_expiry>, BOOST_MULTI_INDEX_MEMBER(transaction_id_with_expiry, fc::time_point, expiry)>
   >
>;



enum class pending_block_mode {
   producing,
   speculating
};

class producer_plugin_impl : public std::enable_shared_from_this<producer_plugin_impl> {
   public:
      producer_plugin_impl(boost::asio::io_service& io)
      :_timer(io)
      ,_transaction_ack_channel(app().get_channel<compat::channels::transaction_ack>())
      {
      }

      optional<fc::time_point> calculate_next_block_time(const account_name& producer_name) const;
      void schedule_production_loop();
      void produce_block();
      bool maybe_produce_block();

      boost::program_options::variables_map _options;
      bool     _production_enabled                 = false;
      bool     _pause_production                   = false;
      uint32_t _required_producer_participation    = uint32_t(config::required_producer_participation);
      uint32_t _production_skip_flags              = 0; //eosio::chain::skip_nothing;

      std::map<chain::public_key_type, chain::private_key_type> _private_keys;
      std::set<chain::account_name>                             _producers;
      boost::asio::deadline_timer                               _timer;
      std::map<chain::account_name, uint32_t>                   _producer_watermarks;
      pending_block_mode                                        _pending_block_mode;
      transaction_id_with_expiry_index                          _persistent_transactions;

      int32_t                                                   _max_transaction_time_ms;
      fc::microseconds                                          _max_irreversible_block_age_us;
      fc::time_point                                            _irreversible_block_time;

      time_point _last_signed_block_time;
      time_point _start_time = fc::time_point::now();
      uint32_t   _last_signed_block_num = 0;

      producer_plugin* _self = nullptr;

      incoming::channels::block::channel_type::handle         _incoming_block_subscription;
      incoming::channels::transaction::channel_type::handle   _incoming_transaction_subscription;

      compat::channels::transaction_ack::channel_type&        _transaction_ack_channel;

      incoming::methods::block_sync::method_type::handle       _incoming_block_sync_provider;
      incoming::methods::transaction_sync::method_type::handle _incoming_transaction_sync_provider;

      transaction_id_with_expiry_index                         _blacklisted_transactions;

      fc::optional<scoped_connection>                          _accepted_block_connection;
      fc::optional<scoped_connection>                          _irreversible_block_connection;

      /*
       * HACK ALERT
       * Boost timers can be in a state where a handler has not yet executed but is not abortable.
       * As this method needs to mutate state handlers depend on for proper functioning to maintain
       * invariants for other code (namely accepting incoming transactions in a nearly full block)
       * the handlers capture a corelation ID at the time they are set.  When they are executed
       * they must check that correlation_id against the global ordinal.  If it does not match that
       * implies that this method has been called with the handler in the state where it should be
       * cancelled but wasn't able to be.
       */
      uint32_t _timer_corelation_id = 0;


      void on_block( const block_state_ptr& bsp ) {
         if( bsp->header.timestamp <= _last_signed_block_time ) return;
         if( bsp->header.timestamp <= _start_time ) return;
         if( bsp->block_num <= _last_signed_block_num ) return;

         const auto& active_producer_to_signing_key = bsp->active_schedule.producers;

         flat_set<account_name> active_producers;
         active_producers.reserve(bsp->active_schedule.producers.size());
         for (const auto& p: bsp->active_schedule.producers) {
            active_producers.insert(p.producer_name);
         }

         std::set_intersection( _producers.begin(), _producers.end(),
                                active_producers.begin(), active_producers.end(),
                                boost::make_function_output_iterator( [&]( const chain::account_name& producer )
         {
            if( producer != bsp->header.producer ) {
               auto itr = std::find_if( active_producer_to_signing_key.begin(), active_producer_to_signing_key.end(),
                                        [&](const producer_key& k){ return k.producer_name == producer; } );
               if( itr != active_producer_to_signing_key.end() ) {
                  auto private_key_itr = _private_keys.find( itr->block_signing_key );
                  if( private_key_itr != _private_keys.end() ) {
                     auto d = bsp->sig_digest();
                     auto sig = private_key_itr->second.sign( d );
                     _last_signed_block_time = bsp->header.timestamp;
                     _last_signed_block_num  = bsp->block_num;

   //                  ilog( "${n} confirmed", ("n",name(producer)) );
                     _self->confirmed_block( { bsp->id, d, producer, sig } );
                  }
               }
            }
         } ) );
      }

      void on_irreversible_block( const signed_block_ptr& lib ) {
         _irreversible_block_time = lib->timestamp.to_time_point();
      }

      template<typename Type, typename Channel, typename F>
      auto publish_results_of(const Type &data, Channel& channel, F f) {
         auto publish_success = fc::make_scoped_exit([&, this](){
            channel.publish(std::pair<fc::exception_ptr, Type>(nullptr, data));
         });

         try {
            auto trace = f();
            if (trace->except) {
               publish_success.cancel();
               channel.publish(std::pair<fc::exception_ptr, Type>(trace->except->dynamic_copy_exception(), data));
            }
            return trace;
         } catch (const fc::exception& e) {
            publish_success.cancel();
            channel.publish(std::pair<fc::exception_ptr, Type>(e.dynamic_copy_exception(), data));
            throw e;
         } catch( const std::exception& e ) {
            publish_success.cancel();
            auto fce = fc::exception(
               FC_LOG_MESSAGE( info, "Caught std::exception: ${what}", ("what",e.what())),
               fc::std_exception_code,
               BOOST_CORE_TYPEID(e).name(),
               e.what()
            );
            channel.publish(std::pair<fc::exception_ptr, Type>(fce.dynamic_copy_exception(),data));
            throw fce;
         } catch( ... ) {
            publish_success.cancel();
            auto fce = fc::unhandled_exception(
               FC_LOG_MESSAGE( info, "Caught unknown exception"),
               std::current_exception()
            );

            channel.publish(std::pair<fc::exception_ptr, Type>(fce.dynamic_copy_exception(), data));
            throw fce;
         }
      };

      void on_incoming_block(const signed_block_ptr& block) {
         fc_dlog(_log, "received incoming block ${id}", ("id", block->id()));

         FC_ASSERT( block->timestamp < (fc::time_point::now() + fc::seconds(1)), "received a block from the future, ignoring it" );


         chain::controller& chain = app().get_plugin<chain_plugin>().chain();

         // abort the pending block
         chain.abort_block();

         // exceptions throw out, make sure we restart our loop
         auto ensure = fc::make_scoped_exit([this](){
            schedule_production_loop();
         });

         // push the new block
         chain.push_block(block);

         if( chain.head_block_state()->header.timestamp.next().to_time_point() >= fc::time_point::now() )
            _production_enabled = true;


         if( fc::time_point::now() - block->timestamp < fc::seconds(5) || (block->block_num() % 1000 == 0) ) {
            ilog("Received block ${id}... #${n} @ ${t} signed by ${p} [trxs: ${count}, lib: ${lib}, confirmed: ${confs}]",
                 ("p",block->producer)("id",fc::variant(block->id()).as_string().substr(0,16))
                 ("n",block_header::num_from_id(block->id()))("t",block->timestamp)
                 ("count",block->transactions.size())("lib",chain.last_irreversible_block_num())("confs", block->confirmed) );
         }

      }

      transaction_trace_ptr on_incoming_transaction(const packed_transaction_ptr& trx, bool persist_until_expired = false) {
         fc_dlog(_log, "received incoming transaction ${id}", ("id", trx->id()));
         return publish_results_of(trx, _transaction_ack_channel, [&]() -> transaction_trace_ptr {
            while (true) {
               chain::controller& chain = app().get_plugin<chain_plugin>().chain();
               auto block_time = chain.pending_block_state()->header.timestamp.to_time_point();

               auto deadline = fc::time_point::now() + fc::milliseconds(_max_transaction_time_ms);
               bool deadline_is_subjective = false;
               if (_pending_block_mode == pending_block_mode::producing && block_time < deadline) {
                  deadline_is_subjective = true;
                  deadline = block_time;
               }

               auto trace = chain.push_transaction(std::make_shared<transaction_metadata>(*trx), deadline);

               if (trace->except) {
                  // edump((trace->except->to_detail_string()));
                  if (failure_is_subjective(*trace->except, deadline_is_subjective) ) {
                     // if we failed because the block was exhausted push the block out and try again if it succeeds
                     if (_pending_block_mode == pending_block_mode::producing ) {
                        fc_dlog(_log, "flushing block under production");

                        if (maybe_produce_block()) {
                           continue;
                        }
                     } else if (_pending_block_mode == pending_block_mode::speculating) {
                        fc_dlog(_log, "dropping block under speculation");

                        chain.abort_block();
                        schedule_production_loop();
                        continue;
                     }

                     // if we failed to produce a block that was not speculative (for some reason).  we are going to
                     // return the trace with an exception set to the caller. if they don't support any retry mechanics
                     // this may result in a lost transaction
                  } else {
                     trace->except->dynamic_rethrow_exception();
                  }
               } else if (persist_until_expired) {
                  // if this trx didnt fail/soft-fail and the persist flag is set, store its ID so that we can
                  // ensure its applied to all future speculative blocks as well.
                  _persistent_transactions.insert(transaction_id_with_expiry{trx->id(), trx->expiration()});
               }

               return trace;
            }
         });
      }

      fc::microseconds get_irreversible_block_age() {
         auto now = fc::time_point::now();
         if (now < _irreversible_block_time) {
            return fc::microseconds(0);
         } else {
            return now - _irreversible_block_time;
         }
      }

      bool production_disabled_by_policy() {
         return !_production_enabled || _pause_production || get_irreversible_block_age() >= _max_irreversible_block_age_us;
      }

      enum class start_block_result {
         succeeded,
         failed,
         exhausted
      };

      start_block_result start_block();
};

void new_chain_banner(const eosio::chain::controller& db)
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

   if( db.head_block_state()->header.timestamp.to_time_point() < (fc::time_point::now() - fc::milliseconds(200 * config::block_interval_ms)))
   {
      std::cerr << "Your genesis seems to have an old timestamp\n"
         "Please consider using the --genesis-timestamp option to give your genesis a recent timestamp\n"
         "\n"
         ;
   }
   return;
}

producer_plugin::producer_plugin()
   : my(new producer_plugin_impl(app().get_io_service())){
      my->_self = this;
   }

producer_plugin::~producer_plugin() {}

void producer_plugin::set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   auto default_priv_key = private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("nathan")));
   auto private_key_default = std::make_pair(default_priv_key.get_public_key(), default_priv_key );

   boost::program_options::options_description producer_options;

   producer_options.add_options()
         ("enable-stale-production,e", boost::program_options::bool_switch()->notifier([this](bool e){my->_production_enabled = e;}), "Enable block production, even if the chain is stale.")
         ("pause-on-startup,x", boost::program_options::bool_switch()->notifier([this](bool p){my->_pause_production = p;}), "Start this node in a state where production is paused")
         ("max-transaction-time", bpo::value<int32_t>()->default_value(30),
          "Limits the maximum time (in milliseconds) that is allowed a pushed transaction's code to execute before being considered invalid")
         ("max-irreversible-block-age", bpo::value<int32_t>()->default_value( 30 * 60 ),
          "Limits the maximum age (in seconds) of the DPOS Irreversible Block for a chain this node will produce blocks on")
         ("required-participation", boost::program_options::value<uint32_t>()
                                       ->default_value(uint32_t(config::required_producer_participation/config::percent_1))
                                       ->notifier([this](uint32_t e) {
                                          my->_required_producer_participation = std::min(e, 100u) * config::percent_1;
                                       }),
                                       "Percent of producers (0-100) that must be participating in order to produce blocks")
         ("producer-name,p", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "ID of producer controlled by this node (e.g. inita; may specify multiple times)")
         ("private-key", boost::program_options::value<vector<string>>()->composing()->multitoken()->default_value({fc::json::to_string(private_key_default)},
                                                                                                fc::json::to_string(private_key_default)),
          "Tuple of [public key, WIF private key] (may specify multiple times)")
         ;
   config_file_options.add(producer_options);
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

   my->_max_transaction_time_ms = options.at("max-transaction-time").as<int32_t>();

   my->_max_irreversible_block_age_us = fc::seconds(options.at("max-irreversible-block-age").as<int32_t>());

   my->_incoming_block_subscription = app().get_channel<incoming::channels::block>().subscribe([this](const signed_block_ptr& block){
      try {
         my->on_incoming_block(block);
      } FC_LOG_AND_DROP();
   });

   my->_incoming_transaction_subscription = app().get_channel<incoming::channels::transaction>().subscribe([this](const packed_transaction_ptr& trx){
      try {
         my->on_incoming_transaction(trx);
      } FC_LOG_AND_DROP();
   });

   my->_incoming_block_sync_provider = app().get_method<incoming::methods::block_sync>().register_provider([this](const signed_block_ptr& block){
      my->on_incoming_block(block);
   });

   my->_incoming_transaction_sync_provider = app().get_method<incoming::methods::transaction_sync>().register_provider([this](const packed_transaction_ptr& trx, bool persist_until_expired) -> transaction_trace_ptr {
      return my->on_incoming_transaction(trx, persist_until_expired);
   });

} FC_LOG_AND_RETHROW() }

void producer_plugin::plugin_startup()
{ try {
   if(fc::get_logger_map().find(logger_name) != fc::get_logger_map().end()) {
      _log = fc::get_logger_map()[logger_name];
   }

   ilog("producer plugin:  plugin_startup() begin");

   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   my->_accepted_block_connection.emplace(chain.accepted_block.connect( [this]( const auto& bsp ){ my->on_block( bsp ); } ));
   my->_irreversible_block_connection.emplace(chain.irreversible_block.connect( [this]( const auto& bsp ){ my->on_irreversible_block( bsp->block ); } ));

   const auto lib_num = chain.last_irreversible_block_num();
   const auto lib = chain.fetch_block_by_number(lib_num);
   if (lib) {
      my->on_irreversible_block(lib);
   } else {
      my->_irreversible_block_time = fc::time_point::maximum();
   }

   if (!my->_producers.empty()) {
      ilog("Launching block production for ${n} producers at ${time}.", ("n", my->_producers.size())("time",fc::time_point::now()));

      if (my->_production_enabled) {
         if (chain.head_block_num() == 0) {
            new_chain_banner(chain);
         }
         //_production_skip_flags |= eosio::chain::skip_undo_history_check;
      }
   }

   my->schedule_production_loop();

   ilog("producer plugin:  plugin_startup() end");
} FC_CAPTURE_AND_RETHROW() }

void producer_plugin::plugin_shutdown() {
   try {
      my->_timer.cancel();
   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
   }

   my->_accepted_block_connection.reset();
   my->_irreversible_block_connection.reset();
}

void producer_plugin::pause() {
   my->_pause_production = true;
}

void producer_plugin::resume() {
   my->_pause_production = false;
   // it is possible that we are only speculating because of this policy which we have now changed
   // re-evaluate that now
   //
   if (my->_pending_block_mode == pending_block_mode::speculating) {
      chain::controller& chain = app().get_plugin<chain_plugin>().chain();
      chain.abort_block();
      my->schedule_production_loop();
   }
}

bool producer_plugin::paused() const {
   return my->_pause_production;
}


optional<fc::time_point> producer_plugin_impl::calculate_next_block_time(const account_name& producer_name) const {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   const auto& pbs = chain.pending_block_state();
   const auto& active_schedule = pbs->active_schedule.producers;
   const auto& hbt = pbs->header.timestamp;

   // determine if this producer is in the active schedule and if so, where
   auto itr = std::find_if(active_schedule.begin(), active_schedule.end(), [&](const auto& asp){ return asp.producer_name == producer_name; });
   if (itr == active_schedule.end()) {
      // this producer is not in the active producer set
      return optional<fc::time_point>();
   }

   size_t producer_index = itr - active_schedule.begin();
   uint32_t minimum_offset = 1; // must at least be the "next" block

   // account for a watermark in the future which is disqualifying this producer for now
   // this is conservative assuming no blocks are dropped.  If blocks are dropped the watermark will
   // disqualify this producer for longer but it is assumed they will wake up, determine that they
   // are disqualified for longer due to skipped blocks and re-caculate their next block with better
   // information then
   auto current_watermark_itr = _producer_watermarks.find(producer_name);
   if (current_watermark_itr != _producer_watermarks.end()) {
      auto watermark = current_watermark_itr->second;
      if (watermark > pbs->block_num) {
         // if I have a watermark then I need to wait until after that watermark
         minimum_offset = watermark - pbs->block_num + 1;
      }
   }

   // this producers next opportuity to produce is the next time its slot arrives after or at the calculated minimum
   uint32_t minimum_slot = hbt.slot + minimum_offset;
   size_t minimum_slot_producer_index = (minimum_slot % (active_schedule.size() * config::producer_repetitions)) / config::producer_repetitions;
   if ( producer_index == minimum_slot_producer_index ) {
      // this is the producer for the minimum slot, go with that
      return block_timestamp_type(minimum_slot).to_time_point();
   } else {
      // calculate how many rounds are between the minimum producer and the producer in question
      size_t producer_distance = producer_index - minimum_slot_producer_index;
      // check for unsigned underflow
      if (producer_distance > producer_index) {
         producer_distance += active_schedule.size();
      }

      // align the minimum slot to the first of its set of reps
      uint32_t first_minimum_producer_slot = minimum_slot - (minimum_slot % config::producer_repetitions);

      // offset the aligned minimum to the *earliest* next set of slots for this producer
      uint32_t next_block_slot = first_minimum_producer_slot  + (producer_distance * config::producer_repetitions);
      return block_timestamp_type(next_block_slot).to_time_point();
   }
}

producer_plugin_impl::start_block_result producer_plugin_impl::start_block() {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   const auto& hbs = chain.head_block_state();

   //Schedule for the next second's tick regardless of chain state
   // If we would wait less than 50ms (1/10 of block_interval), wait for the whole block interval.
   fc::time_point now = fc::time_point::now();
   fc::time_point base = std::max<fc::time_point>(now, chain.head_block_time());
   int64_t min_time_to_next_block = (config::block_interval_us) - (base.time_since_epoch().count() % (config::block_interval_us) );
   fc::time_point block_time = base + fc::microseconds(min_time_to_next_block);


   if((block_time - now) < fc::microseconds(config::block_interval_us/10) ) {     // we must sleep for at least 50ms
//      ilog("Less than ${t}us to next block time, time_to_next_block_time ${bt}",
//           ("t", config::block_interval_us/10)("bt", block_time));
      block_time += fc::microseconds(config::block_interval_us);
   }

   _pending_block_mode = pending_block_mode::producing;

   // Not our turn
   const auto& scheduled_producer = hbs->get_scheduled_producer(block_time);
   auto currrent_watermark_itr = _producer_watermarks.find(scheduled_producer.producer_name);
   auto private_key_itr = _private_keys.find(scheduled_producer.block_signing_key);
   auto irreversible_block_age = get_irreversible_block_age();

   // If the next block production opportunity is in the present or future, we're synced.
   if( !_production_enabled ) {
      _pending_block_mode = pending_block_mode::speculating;
   } else if( _producers.find(scheduled_producer.producer_name) == _producers.end()) {
      _pending_block_mode = pending_block_mode::speculating;
   } else if (private_key_itr == _private_keys.end()) {
      elog("Not producing block because I don't have the private key for ${scheduled_key}", ("scheduled_key", scheduled_producer.block_signing_key));
      _pending_block_mode = pending_block_mode::speculating;
   } else if ( _pause_production ) {
      elog("Not producing block because production is explicitly paused");
      _pending_block_mode = pending_block_mode::speculating;
   } else if ( irreversible_block_age >= _max_irreversible_block_age_us ) {
      elog("Not producing block because the irreversible block is too old [age:${age}s, max:${max}s]", ("age", irreversible_block_age.count() / 1'000'000)( "max", _max_irreversible_block_age_us.count() / 1'000'000 ));
      _pending_block_mode = pending_block_mode::speculating;
   }

   if (_pending_block_mode == pending_block_mode::producing) {
      // determine if our watermark excludes us from producing at this point
      if (currrent_watermark_itr != _producer_watermarks.end()) {
         if (currrent_watermark_itr->second >= hbs->block_num + 1) {
            elog("Not producing block because \"${producer}\" signed a BFT confirmation OR block at a higher block number (${watermark}) than the current fork's head (${head_block_num})",
                ("producer", scheduled_producer.producer_name)
                ("watermark", currrent_watermark_itr->second)
                ("head_block_num", hbs->block_num));
            _pending_block_mode = pending_block_mode::speculating;
         }
      }
   }

   try {
      uint16_t blocks_to_confirm = 0;

      if (_pending_block_mode == pending_block_mode::producing) {
         // determine how many blocks this producer can confirm
         // 1) if it is not a producer from this node, assume no confirmations (we will discard this block anyway)
         // 2) if it is a producer on this node that has never produced, the conservative approach is to assume no
         //    confirmations to make sure we don't double sign after a crash TODO: make these watermarks durable?
         // 3) if it is a producer on this node where this node knows the last block it produced, safely set it -UNLESS-
         // 4) the producer on this node's last watermark is higher (meaning on a different fork)
         if (currrent_watermark_itr != _producer_watermarks.end()) {
            auto watermark = currrent_watermark_itr->second;
            if (watermark < hbs->block_num) {
               blocks_to_confirm = std::min<uint16_t>(std::numeric_limits<uint16_t>::max(), (uint16_t)(hbs->block_num - watermark));
            }
         }
      }

      chain.abort_block();
      chain.start_block(block_time, blocks_to_confirm);
   } FC_LOG_AND_DROP();

   const auto& pbs = chain.pending_block_state();
   if (pbs) {

      if (_pending_block_mode == pending_block_mode::producing && pbs->block_signing_key != scheduled_producer.block_signing_key) {
         elog("Block Signing Key is not expected value, reverting to speculative mode! [expected: \"${expected}\", actual: \"${actual\"", ("expected", scheduled_producer.block_signing_key)("actual", pbs->block_signing_key));
         _pending_block_mode = pending_block_mode::speculating;
      }

      // attempt to play persisted transactions first
      bool exhausted = false;
      auto unapplied_trxs = chain.get_unapplied_transactions();

      // remove all persisted transactions that have now expired
      auto& persisted_by_id = _persistent_transactions.get<by_id>();
      auto& persisted_by_expiry = _persistent_transactions.get<by_expiry>();
      while(!persisted_by_expiry.empty() && persisted_by_expiry.begin()->expiry <= pbs->header.timestamp.to_time_point()) {
         persisted_by_expiry.erase(persisted_by_expiry.begin());
      }

      for (auto itr = unapplied_trxs.begin(); itr != unapplied_trxs.end(); ++itr) {
         const auto& trx = *itr;
         if(persisted_by_id.find(trx->id) != persisted_by_id.end()) {
            // this is a persisted transaction, push it into the block (even if we are speculating) with
            // no deadline as it has already passed the subjective deadlines once and we want to represent
            // the state of the chain including this transaction
            try {
               chain.push_transaction(trx, fc::time_point::maximum());
            } FC_LOG_AND_DROP();

            // remove it from further consideration as it is applied
            *itr = nullptr;
         }
      }

      if (_pending_block_mode == pending_block_mode::producing) {
         for (const auto& trx : unapplied_trxs) {
            if (exhausted) {
               break;
            }

            if (!trx ) {
               // nulled in the loop above, skip it
               continue;
            }

            if (trx->packed_trx.expiration() > pbs->header.timestamp.to_time_point()) {
               // expired, drop it
               chain.drop_unapplied_transaction(trx);
               continue;
            }

            try {
               auto deadline = fc::time_point::now() + fc::milliseconds(_max_transaction_time_ms);
               bool deadline_is_subjective = false;
               if (_pending_block_mode == pending_block_mode::producing && block_time < deadline) {
                  deadline_is_subjective = true;
                  deadline = block_time;
               }

               auto trace = chain.push_transaction(trx, deadline);
               if (trace->except) {
                  if (failure_is_subjective(*trace->except, deadline_is_subjective)) {
                     exhausted = true;
                  } else {
                     // this failed our configured maximum transaction time, we don't want to replay it
                     chain.drop_unapplied_transaction(trx);
                  }
               }
            } FC_LOG_AND_DROP();
         }

         auto& blacklist_by_id = _blacklisted_transactions.get<by_id>();
         auto& blacklist_by_expiry = _blacklisted_transactions.get<by_expiry>();
         auto now = fc::time_point::now();
         while(!blacklist_by_expiry.empty() && blacklist_by_expiry.begin()->expiry <= now) {
            blacklist_by_expiry.erase(blacklist_by_expiry.begin());
         }

         auto scheduled_trxs = chain.get_scheduled_transactions();
         for (const auto& trx : scheduled_trxs) {
            if (exhausted) {
               break;
            }

            if (blacklist_by_id.find(trx) != blacklist_by_id.end()) {
               continue;
            }

            try {
               auto deadline = fc::time_point::now() + fc::milliseconds(_max_transaction_time_ms);
               bool deadline_is_subjective = false;
               if (_pending_block_mode == pending_block_mode::producing && block_time < deadline) {
                  deadline_is_subjective = true;
                  deadline = block_time;
               }

               auto trace = chain.push_scheduled_transaction(trx, deadline);
               if (trace->except) {
                  if (failure_is_subjective(*trace->except, deadline_is_subjective)) {
                     exhausted = true;
                  } else {
                     auto expiration = fc::time_point::now() + fc::seconds(chain.get_global_properties().configuration.deferred_trx_expiration_window);
                     // this failed our configured maximum transaction time, we don't want to replay it add it to a blacklist
                     _blacklisted_transactions.insert(transaction_id_with_expiry{trx, expiration});
                  }
               }
            } FC_LOG_AND_DROP();
         }

      }

      if (exhausted) {
         return start_block_result::exhausted;
      } else {
         return start_block_result::succeeded;
      }
   }

   return start_block_result::failed;
}

void producer_plugin_impl::schedule_production_loop() {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   _timer.cancel();
   std::weak_ptr<producer_plugin_impl> weak_this = shared_from_this();

   auto result = start_block();

   if (result == start_block_result::failed) {
      elog("Failed to start a pending block, will try again later");
      _timer.expires_from_now( boost::posix_time::microseconds( config::block_interval_us  / 10 ));

      // we failed to start a block, so try again later?
      _timer.async_wait([weak_this,cid=++_timer_corelation_id](const boost::system::error_code& ec) {
         auto self = weak_this.lock();
         if (self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id) {
            self->schedule_production_loop();
         }
      });
   } else if (_pending_block_mode == pending_block_mode::producing) {

      // we succeeded but block may be exhausted
      if (result == start_block_result::succeeded) {
         // ship this block off no later than its deadline
         static const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
         _timer.expires_at(epoch + boost::posix_time::microseconds(chain.pending_block_time().time_since_epoch().count()));
         fc_dlog(_log, "Scheduling Block Production on Normal Block #${num} for ${time}", ("num", chain.pending_block_state()->block_num)("time",chain.pending_block_time()));
      } else {
         // ship this block off immediately
         _timer.expires_from_now( boost::posix_time::microseconds( 0 ));
         fc_dlog(_log, "Scheduling Block Production on Exhausted Block #${num} immediately", ("num", chain.pending_block_state()->block_num));
      }

      _timer.async_wait([&chain,weak_this,cid=++_timer_corelation_id](const boost::system::error_code& ec) {
         auto self = weak_this.lock();
         if (self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id) {
            auto res = self->maybe_produce_block();
            fc_dlog(_log, "Producing Block #${num} returned: ${res}", ("num", chain.pending_block_state()->block_num)("res", res) );
         }
      });
   } else if (_pending_block_mode == pending_block_mode::speculating && !_producers.empty() && !production_disabled_by_policy()){
      // if we have any producers then we should at least set a timer for our next available slot
      optional<fc::time_point> wake_up_time;
      for (const auto&p: _producers) {
         auto next_producer_block_time = calculate_next_block_time(p);
         if (next_producer_block_time) {
            auto producer_wake_up_time = *next_producer_block_time - fc::microseconds(config::block_interval_us);
            if (wake_up_time) {
               // wake up with a full block interval to the deadline
               wake_up_time = std::min<fc::time_point>(*wake_up_time, producer_wake_up_time);
            } else {
               wake_up_time = producer_wake_up_time;
            }
         }
      }

      if (wake_up_time) {
         fc_dlog(_log, "Specualtive Block Created; Scheduling Speculative/Production Change at ${time}", ("time", wake_up_time));
         static const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
         _timer.expires_at(epoch + boost::posix_time::microseconds(wake_up_time->time_since_epoch().count()));
         _timer.async_wait([weak_this,cid=++_timer_corelation_id](const boost::system::error_code& ec) {
            auto self = weak_this.lock();
            if (self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id) {
               self->schedule_production_loop();
            }
         });
      } else {
         fc_dlog(_log, "Speculative Block Created; Not Scheduling Speculative/Production, no local producers had valid wake up times");
      }
   } else {
      fc_dlog(_log, "Speculative Block Created");
   }
}

bool producer_plugin_impl::maybe_produce_block() {
   auto reschedule = fc::make_scoped_exit([this]{
      schedule_production_loop();
   });

   try {
      produce_block();
      return true;
   } FC_LOG_AND_DROP();

   fc_dlog(_log, "Aborting block due to produce_block error");
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   chain.abort_block();
   return false;
}

void producer_plugin_impl::produce_block() {
   FC_ASSERT(_pending_block_mode == pending_block_mode::producing, "called produce_block while not actually producing");

   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   const auto& pbs = chain.pending_block_state();
   const auto& hbs = chain.head_block_state();
   FC_ASSERT(pbs, "pending_block_state does not exist but it should, another plugin may have corrupted it");
   auto private_key_itr = _private_keys.find( pbs->block_signing_key );

   FC_ASSERT(private_key_itr != _private_keys.end(), "Attempting to produce a block for which we don't have the private key");

   //idump( (fc::time_point::now() - chain.pending_block_time()) );
   chain.finalize_block();
   chain.sign_block( [&]( const digest_type& d ) { return private_key_itr->second.sign(d); } );
   chain.commit_block();
   auto hbt = chain.head_block_time();
   //idump((fc::time_point::now() - hbt));

   block_state_ptr new_bs = chain.head_block_state();
   // for newly installed producers we can set their watermarks to the block they became
   if (hbs->active_schedule.version != new_bs->active_schedule.version) {
      flat_set<account_name> new_producers;
      new_producers.reserve(new_bs->active_schedule.producers.size());
      for( const auto& p: new_bs->active_schedule.producers) {
         if (_producers.count(p.producer_name) > 0)
            new_producers.insert(p.producer_name);
      }

      for( const auto& p: hbs->active_schedule.producers) {
         new_producers.erase(p.producer_name);
      }

      for (const auto& new_producer: new_producers) {
         _producer_watermarks[new_producer] = chain.head_block_num();
      }
   }
   _producer_watermarks[new_bs->header.producer] = chain.head_block_num();

   ilog("Produced block ${id}... #${n} @ ${t} signed by ${p} [trxs: ${count}, lib: ${lib}, confirmed: ${confs}]",
        ("p",new_bs->header.producer)("id",fc::variant(new_bs->id).as_string().substr(0,16))
        ("n",new_bs->block_num)("t",new_bs->header.timestamp)
        ("count",new_bs->block->transactions.size())("lib",chain.last_irreversible_block_num())("confs", new_bs->header.confirmed));

}

} // namespace eosio
