/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <eos/producer_plugin/producer_plugin.hpp>
#include <eos/net_plugin/net_plugin.hpp>

#include <eos/chain/producer_object.hpp>

#include <eos/utilities/key_conversion.hpp>

#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>

using std::string;
using std::vector;

namespace eos {

class producer_plugin_impl {
public:
   producer_plugin_impl(boost::asio::io_service& io)
      : _timer(io) {}

   void schedule_production_loop();
   block_production_condition::block_production_condition_enum block_production_loop();
   block_production_condition::block_production_condition_enum maybe_produce_block(fc::mutable_variant_object& capture);

   boost::program_options::variables_map _options;
   bool _production_enabled = false;
   uint32_t _required_producer_participation = 33 * config::Percent1;
   uint32_t _production_skip_flags = eos::chain::chain_controller::skip_nothing;
   eos::chain::block_schedule::factory _production_scheduler = eos::chain::block_schedule::in_single_thread;

   std::map<chain::public_key_type, fc::ecc::private_key> _private_keys;
   std::set<types::AccountName> _producers;
   boost::asio::deadline_timer _timer;
};

void new_chain_banner(const eos::chain::chain_controller& db)
{
   std::cerr << "\n"
      "*******************************\n"
      "*                             *\n"
      "*   ------ NEW CHAIN ------   *\n"
      "*   -   Welcome to EOS!   -   *\n"
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
   auto default_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("nathan")));

   auto private_key_default = std::make_pair(chain::public_key_type(default_priv_key.get_public_key()),
                                             eos::utilities::key_to_wif(default_priv_key));

   boost::program_options::options_description producer_options;

   producer_options.add_options()
         ("enable-stale-production", boost::program_options::bool_switch()->notifier([this](bool e){my->_production_enabled = e;}), "Enable block production, even if the chain is stale.")
         ("required-participation", boost::program_options::bool_switch()->notifier([this](int e){my->_required_producer_participation = uint32_t(e*config::Percent1);}), "Percent of producers (0-99) that must be participating in order to produce blocks")
         ("producer-name,p", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          ("ID of producer controlled by this node (e.g. inita; may specify multiple times)"))
         ("private-key", boost::program_options::value<vector<string>>()->composing()->multitoken()->default_value({fc::json::to_string(private_key_default)},
                                                                                                fc::json::to_string(private_key_default)),
          "Tuple of [PublicKey, WIF private key] (may specify multiple times)")
         ;
   command_line_options.add(producer_options);
   config_file_options.add(producer_options);

   command_line_options.add_options()
         ("scheduler", boost::program_options::value<string>()->default_value("single-thread")->notifier([this](const string& v){
             if (v == "single-thread") {
                my->_production_scheduler = eos::chain::block_schedule::in_single_thread;
             } else if (v == "cycling-conflicts") {
                ilog("Using scheduler by_cycling_conflicts");
                my->_production_scheduler = eos::chain::block_schedule::by_cycling_conflicts;
             } else if (v == "threading-conflicts") {
                ilog("Using scheduler by_theading_conflicts");
                my->_production_scheduler = eos::chain::block_schedule::by_threading_conflicts;
             } else {
                FC_ASSERT(false, "Invalid scheduler specified ${s}", ("s", v));
             }
          }),
          "Specify scheduler producer should use. One of the following:\n"
          "  single-thread\n"
          "    \tA reference scheduler that puts all transactions in a single thread (FIFO).\n"
          "  cycling-conflicts\n"
          "    \tA greedy scheduler that attempts to cycle through threads to resolve scope contention before falling back on cycles.\n"
          "  threading-conflicts\n"
          "    \tA greedy scheduler that attempts to make short threads to resolve scope contention before falling back on cycles.")
         ;
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
   LOAD_VALUE_SET(options, "producer-name", my->_producers, types::AccountName)

   if( options.count("private-key") )
   {
      const std::vector<std::string> key_id_to_wif_pair_strings = options["private-key"].as<std::vector<std::string>>();
      for (const std::string& key_id_to_wif_pair_string : key_id_to_wif_pair_strings)
      {
         auto key_id_to_wif_pair = dejsonify<std::pair<chain::public_key_type, std::string>>(key_id_to_wif_pair_string);
         ilog("Public Key: ${public}", ("public", key_id_to_wif_pair.first));
         fc::optional<fc::ecc::private_key> private_key = eos::utilities::wif_to_key(key_id_to_wif_pair.second);
         FC_ASSERT(private_key, "Invalid WIF-format private key ${key_string}",
                   ("key_string", key_id_to_wif_pair.second));
         my->_private_keys[key_id_to_wif_pair.first] = *private_key;
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
         my->_production_skip_flags |= eos::chain::chain_controller::skip_undo_history_check;
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
   // If we would wait less than 50ms, wait for the whole second.
   fc::time_point now = fc::time_point::now();
   int64_t time_to_next_second = 1000000 - (now.time_since_epoch().count() % 1000000);
   if(time_to_next_second < 50000)      // we must sleep for at least 50ms
       time_to_next_second += 1000000;

   _timer.expires_from_now(boost::posix_time::microseconds(time_to_next_second));
   _timer.async_wait(boost::bind(&producer_plugin_impl::block_production_loop, this));
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
      auto pending   = db.pending().size();

      wlog("${p} generated block #${n} @ ${t} with ${count} trxs  ${pending} pending", ("p", producer)(capture)("pending",pending) );
      break;
   }
   case block_production_condition::not_synced:
      ilog("Not producing block because production is disabled until we receive a recent block (see: --enable-stale-production)");
      break;
   case block_production_condition::not_my_turn:
      //         ilog("Not producing block because it isn't my turn");
      break;
   case block_production_condition::not_time_yet:
      //         ilog("Not producing block because slot has not yet arrived");
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
      elog( "exception prodcing block" );
      break;
   }

   schedule_production_loop();
   return result;
}

block_production_condition::block_production_condition_enum producer_plugin_impl::maybe_produce_block(fc::mutable_variant_object& capture) {
   chain::chain_controller& chain = app().get_plugin<chain_plugin>().chain();
   fc::time_point now_fine = fc::time_point::now();
   fc::time_point_sec now = now_fine + fc::microseconds(500000);

   if (app().get_plugin<chain_plugin>().is_skipping_transaction_signatures()) {
      _production_skip_flags |= chain_controller::skip_transaction_signatures;
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

   eos::types::AccountName scheduled_producer = chain.get_scheduled_producer( slot );
   // we must control the producer scheduled to produce the next block.
   if( _producers.find( scheduled_producer ) == _producers.end() )
   {
      capture("scheduled_producer", scheduled_producer);
      return block_production_condition::not_my_turn;
   }

   fc::time_point_sec scheduled_time = chain.get_slot_time( slot );
   eos::chain::public_key_type scheduled_key = chain.get_producer(scheduled_producer).signing_key;
   auto private_key_itr = _private_keys.find( scheduled_key );

   if( private_key_itr == _private_keys.end() )
   {
      capture("scheduled_key", scheduled_key);
      return block_production_condition::no_private_key;
   }

   uint32_t prate = chain.producer_participation_rate();
   if( prate < _required_producer_participation )
   {
      capture("pct", uint32_t(100*uint64_t(prate) / config::Percent1));
      return block_production_condition::low_participation;
   }

   if( llabs((scheduled_time - now).count()) > fc::milliseconds( 500 ).count() )
   {
      capture("scheduled_time", scheduled_time)("now", now);
      return block_production_condition::lag;
   }

   auto block = chain.generate_block(
      scheduled_time,
      scheduled_producer,
      private_key_itr->second,
      _production_scheduler,
      _production_skip_flags
      );

   uint32_t count = 0;
   for( const auto& cycle : block.cycles ) {
      for( const auto& thread : cycle ) {
         count += thread.generated_input.size();
         count += thread.user_input.size();
      }
   }

   capture("n", block.block_num())("t", block.timestamp)("c", now)("count",count);

   app().get_plugin<net_plugin>().broadcast_block(block);
   return block_production_condition::produced;
}

} // namespace eos
