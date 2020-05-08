#include <eosio/producer_plugin/producer_plugin_impl.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/chain_id_type.hpp>
#include <eosio/chain/generated_transaction_object.hpp>

// TODO: go through each type and funcion API call used and include its
//       respective header file
// TODO: organize the member vars and the function order of each class
// TODO: create the tests
// TODO: refactor in such a way to separate out the code even further
// TODO: make sure that I fully qualify the namespaces

// TODO: test what happens when I remove `namespace eosio {`

namespace eosio {

eosio::producer_plugin_impl::producer_plugin_impl(boost::asio::io_service &io)
: _timer(io)
, _transaction_ack_channel(appbase::app().get_channel<eosio::chain::plugin_interface::compat::channels::transaction_ack>())
{
}

void eosio::producer_plugin_impl::consider_new_watermark( eosio::chain::account_name producer, uint32_t block_num, eosio::chain::block_timestamp_type timestamp) {
   auto itr = _producer_watermarks.find( producer );
   if( itr != _producer_watermarks.end() ) {
      itr->second.first = std::max( itr->second.first, block_num );
      itr->second.second = std::max( itr->second.second, timestamp );
   } else if( _producers.count( producer ) > 0 ) {
      _producer_watermarks.emplace( producer, std::make_pair(block_num, timestamp) );
   }
}

std::optional<eosio::producer_plugin_impl::producer_watermark> eosio::producer_plugin_impl::get_watermark( eosio::chain::account_name producer ) const {
   auto itr = _producer_watermarks.find( producer );
   if( itr == _producer_watermarks.end() ) return {};
   return itr->second;
}

void eosio::producer_plugin_impl::on_block( const eosio::chain::block_state_ptr& bsp ) {
   auto before = _unapplied_transactions.size();
   _unapplied_transactions.clear_applied( bsp );
   fc_dlog( _log, "Removed applied transactions before: ${before}, after: ${after}",
            ("before", before)("after", _unapplied_transactions.size()) );
}

void eosio::producer_plugin_impl::on_block_header( const eosio::chain::block_state_ptr& bsp ) {
   consider_new_watermark( bsp->header.producer, bsp->block_num, bsp->block->timestamp );
}

void eosio::producer_plugin_impl::on_irreversible_block( const eosio::chain::signed_block_ptr& lib ) {
   _irreversible_block_time = lib->timestamp.to_time_point();
   const eosio::chain::controller& chain = chain_plug->chain();

   // promote any pending snapshots
   auto& snapshots_by_height = _pending_snapshot_index.get<by_height>();
   uint32_t lib_height = lib->block_num();

   while (!snapshots_by_height.empty() && snapshots_by_height.begin()->get_height() <= lib_height) {
      const auto& pending = snapshots_by_height.begin();
      auto next = pending->next;

      try {
         next(pending->finalize(chain));
      } CATCH_AND_CALL(next);

      snapshots_by_height.erase(snapshots_by_height.begin());
   }
}

template<typename Type, typename Channel, typename F>
auto producer_plugin_impl::publish_results_of(const Type &data, Channel& channel, F f) {
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
}

fc::microseconds producer_plugin_impl::get_irreversible_block_age() {
   auto now = fc::time_point::now();
   if (now < _irreversible_block_time) {
      return fc::microseconds(0);
   } else {
      return now - _irreversible_block_time;
   }
}

fc::time_point producer_plugin_impl::calculate_pending_block_time() const {
   const chain::controller& chain = chain_plug->chain();
   const fc::time_point now = fc::time_point::now();
   const fc::time_point base = std::max<fc::time_point>(now, chain.head_block_time());
   const int64_t min_time_to_next_block = (eosio::chain::config::block_interval_us) - (base.time_since_epoch().count() % (eosio::chain::config::block_interval_us) );
   fc::time_point block_time = base + fc::microseconds(min_time_to_next_block);
   return block_time;
}

producer_plugin_impl::start_block_result producer_plugin_impl::start_block() {
   chain::controller& chain = chain_plug->chain();

   if( !chain_plug->accept_transactions() )
      return start_block_result::waiting_for_block;

   const auto& hbs = chain.head_block_state();

   if( chain.get_terminate_at_block() > 0 && chain.get_terminate_at_block() < hbs->block_num ) {
      ilog("Reached configured maximum block ${num}; terminating", ("num", chain.get_terminate_at_block()));
      app().quit();
      return start_block_result::failed;
   }

   const fc::time_point now = fc::time_point::now();
   const fc::time_point block_time = calculate_pending_block_time();

   const eosio::pending_block_mode previous_pending_mode = _pending_block_mode;
   _pending_block_mode = eosio::pending_block_mode::producing;

   // Not our turn
   const auto& scheduled_producer = hbs->get_scheduled_producer(block_time);

   const auto current_watermark = get_watermark(scheduled_producer.producer_name);

   size_t num_relevant_signatures = 0;
   scheduled_producer.for_each_key([&](const public_key_type& key){
      const auto& iter = _signature_providers.find(key);
      if(iter != _signature_providers.end()) {
         num_relevant_signatures++;
      }
   });

   auto irreversible_block_age = get_irreversible_block_age();

   // If the next block production opportunity is in the present or future, we're synced.
   if( !_production_enabled ) {
      _pending_block_mode = eosio::pending_block_mode::speculating;
   } else if( _producers.find(scheduled_producer.producer_name) == _producers.end()) {
      _pending_block_mode = eosio::pending_block_mode::speculating;
   } else if (num_relevant_signatures == 0) {
      elog("Not producing block because I don't have any private keys relevant to authority: ${authority}", ("authority", scheduled_producer.authority));
      _pending_block_mode = eosio::pending_block_mode::speculating;
   } else if ( _pause_production ) {
      elog("Not producing block because production is explicitly paused");
      _pending_block_mode = eosio::pending_block_mode::speculating;
   } else if ( _max_irreversible_block_age_us.count() >= 0 && irreversible_block_age >= _max_irreversible_block_age_us ) {
      elog("Not producing block because the irreversible block is too old [age:${age}s, max:${max}s]", ("age", irreversible_block_age.count() / 1'000'000)( "max", _max_irreversible_block_age_us.count() / 1'000'000 ));
      _pending_block_mode = eosio::pending_block_mode::speculating;
   }

   if (_pending_block_mode == eosio::pending_block_mode::producing) {
      // determine if our watermark excludes us from producing at this point
      if (current_watermark) {
         const eosio::chain::block_timestamp_type block_timestamp{block_time};
         if (current_watermark->first > hbs->block_num) {
            elog("Not producing block because \"${producer}\" signed a block at a higher block number (${watermark}) than the current fork's head (${head_block_num})",
                 ("producer", scheduled_producer.producer_name)
                 ("watermark", current_watermark->first)
                 ("head_block_num", hbs->block_num));
            _pending_block_mode = eosio::pending_block_mode::speculating;
         } else if (current_watermark->second >= block_timestamp) {
            elog("Not producing block because \"${producer}\" signed a block at the next block time or later (${watermark}) than the pending block time (${block_timestamp})",
                 ("producer", scheduled_producer.producer_name)
                 ("watermark", current_watermark->second)
                 ("block_timestamp", block_timestamp));
            _pending_block_mode = eosio::pending_block_mode::speculating;
         }
      }
   }

   if (_pending_block_mode == eosio::pending_block_mode::speculating) {
      auto head_block_age = now - chain.head_block_time();
      if (head_block_age > fc::seconds(5))
         return start_block_result::waiting_for_block;
   }

   if (_pending_block_mode == eosio::pending_block_mode::producing) {
      const auto start_block_time = block_time - fc::microseconds( eosio::chain::config::block_interval_us );
      if( now < start_block_time ) {
         fc_dlog(_log, "Not producing block waiting for production window ${n} ${bt}", ("n", hbs->block_num + 1)("bt", block_time) );
         // start_block_time instead of block_time because schedule_delayed_production_loop calculates next block time from given time
         schedule_delayed_production_loop(weak_from_this(), calculate_producer_wake_up_time(start_block_time));
         return start_block_result::waiting_for_production;
      }
   } else if (previous_pending_mode == eosio::pending_block_mode::producing) {
      // just produced our last block of our round
      const auto start_block_time = block_time - fc::microseconds( eosio::chain::config::block_interval_us );
      fc_dlog(_log, "Not starting speculative block until ${bt}", ("bt", start_block_time) );
      schedule_delayed_production_loop( weak_from_this(), start_block_time);
      return start_block_result::waiting_for_production;
   }

   fc_dlog(_log, "Starting block #${n} at ${time} producer ${p}",
           ("n", hbs->block_num + 1)("time", now)("p", scheduled_producer.producer_name));

   try {
      uint16_t blocks_to_confirm = 0;

      if (_pending_block_mode == eosio::pending_block_mode::producing) {
         // determine how many blocks this producer can confirm
         // 1) if it is not a producer from this node, assume no confirmations (we will discard this block anyway)
         // 2) if it is a producer on this node that has never produced, the conservative approach is to assume no
         //    confirmations to make sure we don't double sign after a crash TODO: make these watermarks durable?
         // 3) if it is a producer on this node where this node knows the last block it produced, safely set it -UNLESS-
         // 4) the producer on this node's last watermark is higher (meaning on a different fork)
         if (current_watermark) {
            auto watermark_bn = current_watermark->first;
            if (watermark_bn < hbs->block_num) {
               blocks_to_confirm = (uint16_t)(std::min<uint32_t>(std::numeric_limits<uint16_t>::max(), (uint32_t)(hbs->block_num - watermark_bn)));
            }
         }

         // can not confirm irreversible blocks
         blocks_to_confirm = (uint16_t)(std::min<uint32_t>(blocks_to_confirm, (uint32_t)(hbs->block_num - hbs->dpos_irreversible_blocknum)));
      }

      _unapplied_transactions.add_aborted( chain.abort_block() );

      auto features_to_activate = chain.get_preactivated_protocol_features();
      if( _pending_block_mode == eosio::pending_block_mode::producing && _protocol_features_to_activate.size() > 0 ) {
         bool drop_features_to_activate = false;
         try {
            chain.validate_protocol_features( _protocol_features_to_activate );
         } catch( const fc::exception& e ) {
            wlog( "protocol features to activate are no longer all valid: ${details}",
                  ("details",e.to_detail_string()) );
            drop_features_to_activate = true;
         }

         if( drop_features_to_activate ) {
            _protocol_features_to_activate.clear();
         } else {
            auto protocol_features_to_activate = _protocol_features_to_activate; // do a copy as pending_block might be aborted
            if( features_to_activate.size() > 0 ) {
               protocol_features_to_activate.reserve( protocol_features_to_activate.size()
                                                         + features_to_activate.size() );
               std::set<eosio::chain::digest_type> set_of_features_to_activate( protocol_features_to_activate.begin(),
                                                                  protocol_features_to_activate.end() );
               for( const auto& f : features_to_activate ) {
                  auto res = set_of_features_to_activate.insert( f );
                  if( res.second ) {
                     protocol_features_to_activate.push_back( f );
                  }
               }
               features_to_activate.clear();
            }
            std::swap( features_to_activate, protocol_features_to_activate );
            _protocol_features_signaled = true;
            ilog( "signaling activation of the following protocol features in block ${num}: ${features_to_activate}",
                  ("num", hbs->block_num + 1)("features_to_activate", features_to_activate) );
         }
      }

      chain.start_block( block_time, blocks_to_confirm, features_to_activate );
   } LOG_AND_DROP();

   if( chain.is_building_block() ) {
      const auto& pending_block_signing_authority = chain.pending_block_signing_authority();
      const fc::time_point preprocess_deadline = calculate_block_deadline(block_time);

      if (_pending_block_mode == eosio::pending_block_mode::producing && pending_block_signing_authority != scheduled_producer.authority) {
         elog("Unexpected block signing authority, reverting to speculative mode! [expected: \"${expected}\", actual: \"${actual\"", ("expected", scheduled_producer.authority)("actual", pending_block_signing_authority));
         _pending_block_mode = eosio::pending_block_mode::speculating;
      }

      try {
         if( !remove_expired_trxs( preprocess_deadline ) )
            return start_block_result::exhausted;
         if( !remove_expired_blacklisted_trxs( preprocess_deadline ) )
            return start_block_result::exhausted;

         // limit execution of pending incoming to once per block
         size_t pending_incoming_process_limit = _unapplied_transactions.incoming_size();

         if( !process_unapplied_trxs( preprocess_deadline ) )
            return start_block_result::exhausted;

         if (_pending_block_mode == eosio::pending_block_mode::producing) {
            auto scheduled_trx_deadline = preprocess_deadline;
            if (_max_scheduled_transaction_time_per_block_ms >= 0) {
               scheduled_trx_deadline = std::min<fc::time_point>(
                     scheduled_trx_deadline,
                     fc::time_point::now() + fc::milliseconds(_max_scheduled_transaction_time_per_block_ms)
               );
            }
            // may exhaust scheduled_trx_deadline but not preprocess_deadline, exhausted preprocess_deadline checked below
            process_scheduled_and_incoming_trxs( scheduled_trx_deadline, pending_incoming_process_limit );
         }

         if( app().is_quiting() ) // db guard exception above in LOG_AND_DROP could have called app().quit()
            return start_block_result::failed;
         if (preprocess_deadline <= fc::time_point::now() || block_is_exhausted()) {
            return start_block_result::exhausted;
         } else {
            if( !process_incoming_trxs( preprocess_deadline, pending_incoming_process_limit ) )
               return start_block_result::exhausted;
            return start_block_result::succeeded;
         }

      } catch ( const eosio::chain::guard_exception& e ) {
         chain_plugin::handle_guard_exception(e);
         return start_block_result::failed;
      } catch ( std::bad_alloc& ) {
         chain_plugin::handle_bad_alloc();
      } catch ( boost::interprocess::bad_alloc& ) {
         chain_plugin::handle_db_exhaustion();
      }

   }

   return start_block_result::failed;
}

// Example:
// --> Start block A (block time x.500) at time x.000
// -> start_block()
// --> deadline, produce block x.500 at time x.400 (assuming 80% cpu block effort)
// -> Idle
// --> Start block B (block time y.000) at time x.500
void producer_plugin_impl::schedule_production_loop() {
   _timer.cancel();

   auto result = start_block();

   if (result == start_block_result::failed) {
      elog("Failed to start a pending block, will try again later");
      _timer.expires_from_now( boost::posix_time::microseconds( eosio::chain::config::block_interval_us  / 10 ));

      // we failed to start a block, so try again later?
      _timer.async_wait( app().get_priority_queue().wrap( priority::high,
          [weak_this = weak_from_this(), cid = ++_timer_corelation_id]( const boost::system::error_code& ec ) {
             auto self = weak_this.lock();
             if( self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id ) {
                self->schedule_production_loop();
             }
          } ) );
   } else if (result == start_block_result::waiting_for_block){
      if (!_producers.empty() && !production_disabled_by_policy()) {
         fc_dlog(_log, "Waiting till another block is received and scheduling Speculative/Production Change");
         schedule_delayed_production_loop(weak_from_this(), calculate_producer_wake_up_time(calculate_pending_block_time()));
      } else {
         fc_dlog(_log, "Waiting till another block is received");
         // nothing to do until more blocks arrive
      }

   } else if (result == start_block_result::waiting_for_production) {
      // scheduled in start_block()

   } else if (_pending_block_mode == eosio::pending_block_mode::producing) {
      schedule_maybe_produce_block( result == start_block_result::exhausted );

   } else if (_pending_block_mode == eosio::pending_block_mode::speculating && !_producers.empty() && !production_disabled_by_policy()){
      chain::controller& chain = chain_plug->chain();
      fc_dlog(_log, "Speculative Block Created; Scheduling Speculative/Production Change");
      EOS_ASSERT( chain.is_building_block(), eosio::chain::missing_pending_block_state, "speculating without pending_block_state" );
      schedule_delayed_production_loop(weak_from_this(), calculate_producer_wake_up_time(chain.pending_block_time()));
   } else {
      fc_dlog(_log, "Speculative Block Created");
   }
}

bool producer_plugin_impl::on_incoming_block(const eosio::chain::signed_block_ptr& block, const std::optional<eosio::chain::block_id_type>& block_id) {
   auto& chain = chain_plug->chain();
   if ( _pending_block_mode == eosio::pending_block_mode::producing ) {
      fc_wlog( _log, "dropped incoming block #${num} id: ${id}",
               ("num", block->block_num())("id", block_id ? (*block_id).str() : "UNKNOWN") );
      return false;
   }

   const auto& id = block_id ? *block_id : block->id();
   auto blk_num = block->block_num();

   fc_dlog(_log, "received incoming block ${n} ${id}", ("n", blk_num)("id", id));

   EOS_ASSERT( block->timestamp < (fc::time_point::now() + fc::seconds( 7 )), eosio::chain::block_from_the_future,
               "received a block from the future, ignoring it: ${id}", ("id", id) );

   /* de-dupe here... no point in aborting block if we already know the block */
   auto existing = chain.fetch_block_by_id( id );
   if( existing ) { return false; }

   // start processing of block
   auto bsf = chain.create_block_state_future( block );

   // abort the pending block
   _unapplied_transactions.add_aborted( chain.abort_block() );

   // exceptions throw out, make sure we restart our loop
   auto ensure = fc::make_scoped_exit([this](){
      schedule_production_loop();
   });

   // push the new block
   try {
      chain.push_block( bsf, [this]( const eosio::chain::branch_type& forked_branch ) {
         _unapplied_transactions.add_forked( forked_branch );
      }, [this]( const transaction_id_type& id ) {
         return _unapplied_transactions.get_trx( id );
      } );
   } catch ( const eosio::chain::guard_exception& e ) {
      chain_plugin::handle_guard_exception(e);
      return false;
   } catch( const fc::exception& e ) {
      elog((e.to_detail_string()));
      app().get_channel<chain::plugin_interface::channels::rejected_block>().publish( priority::medium, block );
      throw;
   } catch ( const std::bad_alloc& ) {
      chain_plugin::handle_bad_alloc();
   } catch ( boost::interprocess::bad_alloc& ) {
      chain_plugin::handle_db_exhaustion();
   }

   const auto& hbs = chain.head_block_state();
   if( hbs->header.timestamp.next().to_time_point() >= fc::time_point::now() ) {
      _production_enabled = true;
   }

   if( fc::time_point::now() - block->timestamp < fc::minutes(5) || (blk_num % 1000 == 0) ) {
      ilog("Received block ${id}... #${n} @ ${t} signed by ${p} [trxs: ${count}, lib: ${lib}, conf: ${confs}, latency: ${latency} ms]",
           ("p",block->producer)("id",id.str().substr(8,16))("n",blk_num)("t",block->timestamp)
           ("count",block->transactions.size())("lib",chain.last_irreversible_block_num())
           ("confs", block->confirmed)("latency", (fc::time_point::now() - block->timestamp).count()/1000 ) );
      if( chain.get_read_mode() != eosio::chain::db_read_mode::IRREVERSIBLE && hbs->id != id && hbs->block != nullptr ) { // not applied to head
         ilog("Block not applied to head ${id}... #${n} @ ${t} signed by ${p} [trxs: ${count}, dpos: ${dpos}, conf: ${confs}, latency: ${latency} ms]",
              ("p",hbs->block->producer)("id",hbs->id.str().substr(8,16))("n",hbs->block_num)("t",hbs->block->timestamp)
              ("count",hbs->block->transactions.size())("dpos", hbs->dpos_irreversible_blocknum)
              ("confs", hbs->block->confirmed)("latency", (fc::time_point::now() - hbs->block->timestamp).count()/1000 ) );
      }
   }

   return true;
}

void producer_plugin_impl::on_incoming_transaction_async(const eosio::chain::packed_transaction_ptr& trx, bool persist_until_expired, eosio::producer_plugin::next_function<eosio::chain::transaction_trace_ptr> next) {
   chain::controller& chain = chain_plug->chain();
   const auto max_trx_time_ms = _max_transaction_time_ms.load();
   fc::microseconds max_trx_cpu_usage = max_trx_time_ms < 0 ? fc::microseconds::maximum() : fc::milliseconds( max_trx_time_ms );

   auto future = eosio::chain::transaction_metadata::start_recover_keys( trx, _thread_pool->get_executor(),
          chain.get_chain_id(), fc::microseconds( max_trx_cpu_usage ), chain.configured_subjective_signature_length_limit() );
   boost::asio::post( _thread_pool->get_executor(), [self = this, future{std::move(future)}, persist_until_expired, next{std::move(next)}]() mutable {
      if( future.valid() ) {
         future.wait();
         app().post( priority::low, [self, future{std::move(future)}, persist_until_expired, next{std::move( next )}]() mutable {
            try {
               if( !self->process_incoming_transaction_async( future.get(), persist_until_expired, std::move( next ) ) ) {
                  if( self->_pending_block_mode == eosio::pending_block_mode::producing ) {
                     self->schedule_maybe_produce_block( true );
                  }
               }
            } CATCH_AND_CALL(next);
         } );
      }
   });
}

bool producer_plugin_impl::process_incoming_transaction_async(const eosio::chain::transaction_metadata_ptr& trx, bool persist_until_expired, eosio::producer_plugin::next_function<eosio::chain::transaction_trace_ptr> next) {
   bool exhausted = false;
   chain::controller& chain = chain_plug->chain();

   auto send_response = [this, &trx, &chain, &next](const fc::static_variant<fc::exception_ptr, eosio::chain::transaction_trace_ptr>& response) {
      next(response);
      if (response.contains<fc::exception_ptr>()) {
         _transaction_ack_channel.publish(priority::low, std::pair<fc::exception_ptr, eosio::chain::transaction_metadata_ptr>(response.get<fc::exception_ptr>(), trx));
         if (_pending_block_mode == eosio::pending_block_mode::producing) {
            fc_dlog(_trx_trace_log, "[TRX_TRACE] Block ${block_num} for producer ${prod} is REJECTING tx: ${txid} : ${why} ",
                  ("block_num", chain.head_block_num() + 1)
                  ("prod", chain.pending_block_producer())
                  ("txid", trx->id())
                  ("why",response.get<fc::exception_ptr>()->what()));
         } else {
            fc_dlog(_trx_trace_log, "[TRX_TRACE] Speculative execution is REJECTING tx: ${txid} : ${why} ",
                    ("txid", trx->id())
                    ("why",response.get<fc::exception_ptr>()->what()));
         }
      } else {
         _transaction_ack_channel.publish(priority::low, std::pair<fc::exception_ptr, eosio::chain::transaction_metadata_ptr>(nullptr, trx));
         if (_pending_block_mode == eosio::pending_block_mode::producing) {
            fc_dlog(_trx_trace_log, "[TRX_TRACE] Block ${block_num} for producer ${prod} is ACCEPTING tx: ${txid}",
                    ("block_num", chain.head_block_num() + 1)
                    ("prod", chain.pending_block_producer())
                    ("txid", trx->id()));
         } else {
            fc_dlog(_trx_trace_log, "[TRX_TRACE] Speculative execution is ACCEPTING tx: ${txid}",
                    ("txid", trx->id()));
         }
      }
   };

   try {
      const auto& id = trx->id();

      fc::time_point bt = chain.is_building_block() ? chain.pending_block_time() : chain.head_block_time();
      if( fc::time_point( trx->packed_trx()->expiration()) < bt ) {
         send_response( std::static_pointer_cast<fc::exception>(
                           std::make_shared<eosio::chain::expired_tx_exception>(
                     FC_LOG_MESSAGE( error, "expired transaction ${id}, expiration ${e}, block time ${bt}",
                                     ("id", id)("e", trx->packed_trx()->expiration())( "bt", bt )))));
         return true;
      }

      if( chain.is_known_unexpired_transaction( id )) {
         send_response( std::static_pointer_cast<fc::exception>( std::make_shared<eosio::chain::tx_duplicate>(
               FC_LOG_MESSAGE( error, "duplicate transaction ${id}", ("id", id)))) );
         return true;
      }

      if( !chain.is_building_block()) {
         _unapplied_transactions.add_incoming( trx, persist_until_expired, next );
         return true;
      }

      auto deadline = fc::time_point::now() + fc::milliseconds( _max_transaction_time_ms );
      bool deadline_is_subjective = false;
      const auto block_deadline = calculate_block_deadline( chain.pending_block_time() );
      if( _max_transaction_time_ms < 0 ||
          (_pending_block_mode == eosio::pending_block_mode::producing && block_deadline < deadline)) {
         deadline_is_subjective = true;
         deadline = block_deadline;
      }

      auto trace = chain.push_transaction( trx, deadline, trx->billed_cpu_time_us, false );
      if( trace->except ) {
         if( exception_is_exhausted( *trace->except, deadline_is_subjective )) {
            _unapplied_transactions.add_incoming( trx, persist_until_expired, next );
            if( _pending_block_mode == eosio::pending_block_mode::producing ) {
               fc_dlog( _trx_trace_log, "[TRX_TRACE] Block ${block_num} for producer ${prod} COULD NOT FIT, tx: ${txid} RETRYING ",
                        ("block_num", chain.head_block_num() + 1)
                        ("prod", chain.pending_block_producer())
                        ("txid", trx->id()));
            } else {
               fc_dlog( _trx_trace_log, "[TRX_TRACE] Speculative execution COULD NOT FIT tx: ${txid} RETRYING",
                        ("txid", trx->id()));
            }
            if( !exhausted )
               exhausted = block_is_exhausted();
         } else {
            auto e_ptr = trace->except->dynamic_copy_exception();
            send_response( e_ptr );
         }
      } else {
         if( persist_until_expired ) {
            // if this trx didnt fail/soft-fail and the persist flag is set, store its ID so that we can
            // ensure its applied to all future speculative blocks as well.
            _unapplied_transactions.add_persisted( trx );
         }
         send_response( trace );
      }

   } catch ( const eosio::chain::guard_exception& e ) {
      chain_plugin::handle_guard_exception(e);
   } catch ( boost::interprocess::bad_alloc& ) {
      chain_plugin::handle_db_exhaustion();
   } catch ( std::bad_alloc& ) {
      chain_plugin::handle_bad_alloc();
   } CATCH_AND_CALL(send_response);

   return !exhausted;
}

bool producer_plugin_impl::production_disabled_by_policy() {
   return !_production_enabled || _pause_production || (_max_irreversible_block_age_us.count() >= 0 && get_irreversible_block_age() >= _max_irreversible_block_age_us);
}

std::optional<fc::time_point> producer_plugin_impl::calculate_next_block_time(const eosio::chain::account_name& producer_name, const eosio::chain::block_timestamp_type& current_block_time) const {
   chain::controller& chain = chain_plug->chain();
   const auto& hbs = chain.head_block_state();
   const auto& active_schedule = hbs->active_schedule.producers;

   // determine if this producer is in the active schedule and if so, where
   auto itr = std::find_if(active_schedule.begin(), active_schedule.end(), [&](const auto& asp){ return asp.producer_name == producer_name; });
   if (itr == active_schedule.end()) {
      // this producer is not in the active producer set
      return std::optional<fc::time_point>();
   }

   size_t producer_index = itr - active_schedule.begin();
   uint32_t minimum_offset = 1; // must at least be the "next" block

   // account for a watermark in the future which is disqualifying this producer for now
   // this is conservative assuming no blocks are dropped.  If blocks are dropped the watermark will
   // disqualify this producer for longer but it is assumed they will wake up, determine that they
   // are disqualified for longer due to skipped blocks and re-caculate their next block with better
   // information then
   auto current_watermark = get_watermark(producer_name);
   if (current_watermark) {
      const auto watermark = *current_watermark;
      auto block_num = chain.head_block_state()->block_num;
      if (chain.is_building_block()) {
         ++block_num;
      }
      if (watermark.first > block_num) {
         // if I have a watermark block number then I need to wait until after that watermark
         minimum_offset = watermark.first - block_num + 1;
      }
      if (watermark.second > current_block_time) {
          // if I have a watermark block timestamp then I need to wait until after that watermark timestamp
          minimum_offset = std::max(minimum_offset, watermark.second.slot - current_block_time.slot + 1);
      }
   }

   // this producers next opportuity to produce is the next time its slot arrives after or at the calculated minimum
   uint32_t minimum_slot = current_block_time.slot + minimum_offset;
   size_t minimum_slot_producer_index = (minimum_slot % (active_schedule.size() * eosio::chain::config::producer_repetitions)) / eosio::chain::config::producer_repetitions;
   if ( producer_index == minimum_slot_producer_index ) {
      // this is the producer for the minimum slot, go with that
      return eosio::chain::block_timestamp_type(minimum_slot).to_time_point();
   } else {
      // calculate how many rounds are between the minimum producer and the producer in question
      size_t producer_distance = producer_index - minimum_slot_producer_index;
      // check for unsigned underflow
      if (producer_distance > producer_index) {
         producer_distance += active_schedule.size();
      }

      // align the minimum slot to the first of its set of reps
      uint32_t first_minimum_producer_slot = minimum_slot - (minimum_slot % eosio::chain::config::producer_repetitions);

      // offset the aligned minimum to the *earliest* next set of slots for this producer
      uint32_t next_block_slot = first_minimum_producer_slot  + (producer_distance * eosio::chain::config::producer_repetitions);
      return eosio::chain::block_timestamp_type(next_block_slot).to_time_point();
   }
}



fc::time_point producer_plugin_impl::calculate_block_deadline( const fc::time_point& block_time ) const {
   bool last_block = ((eosio::chain::block_timestamp_type(block_time).slot % eosio::chain::config::producer_repetitions) == eosio::chain::config::producer_repetitions - 1);
   return block_time + fc::microseconds(last_block ? _last_block_time_offset_us : _produce_time_offset_us);
}

bool producer_plugin_impl::remove_expired_trxs( const fc::time_point& deadline ) {
   chain::controller& chain = chain_plug->chain();
   auto pending_block_time = chain.pending_block_time();

   // remove all expired transactions
   size_t num_expired_persistent = 0;
   size_t num_expired_other = 0;
   size_t orig_count = _unapplied_transactions.size();
   bool exhausted = !_unapplied_transactions.clear_expired( pending_block_time, deadline,
                  [&num_expired_persistent, &num_expired_other, pbm = _pending_block_mode,
                   &chain, has_producers = !_producers.empty()]( const eosio::chain::transaction_id_type& txid, eosio::chain::trx_enum_type trx_type ) {
            if( trx_type == eosio::chain::trx_enum_type::persisted ) {
               if( pbm == eosio::pending_block_mode::producing ) {
                  fc_dlog( _trx_trace_log,
                           "[TRX_TRACE] Block ${block_num} for producer ${prod} is EXPIRING PERSISTED tx: ${txid}",
                           ("block_num", chain.head_block_num() + 1)("prod", chain.pending_block_producer())("txid", txid));
               } else {
                  fc_dlog( _trx_trace_log, "[TRX_TRACE] Speculative execution is EXPIRING PERSISTED tx: ${txid}", ("txid", txid));
               }
               ++num_expired_persistent;
            } else {
               if (has_producers) {
                  fc_dlog(_trx_trace_log,
                        "[TRX_TRACE] Node with producers configured is dropping an EXPIRED transaction that was PREVIOUSLY ACCEPTED : ${txid}",
                        ("txid", txid));
               }
               ++num_expired_other;
            }
   });

   if( exhausted ) {
      fc_wlog( _log, "Unable to process all expired transactions in unapplied queue before deadline, "
                     "Persistent expired ${persistent_expired}, Other expired ${other_expired}",
               ("persistent_expired", num_expired_persistent)("other_expired", num_expired_other) );
   } else {
      fc_dlog( _log, "Processed ${m} expired transactions of the ${n} transactions in the unapplied queue, "
                     "Persistent expired ${persistent_expired}, Other expired ${other_expired}",
               ("m", num_expired_persistent+num_expired_other)("n", orig_count)
               ("persistent_expired", num_expired_persistent)("other_expired", num_expired_other) );
   }

   return !exhausted;
}

bool producer_plugin_impl::remove_expired_blacklisted_trxs( const fc::time_point& deadline ) {
   bool exhausted = false;
   auto& blacklist_by_expiry = _blacklisted_transactions.get<by_expiry>();
   if(!blacklist_by_expiry.empty()) {
      const chain::controller& chain = chain_plug->chain();
      const auto lib_time = chain.last_irreversible_block_time();

      int num_expired = 0;
      int orig_count = _blacklisted_transactions.size();

      while (!blacklist_by_expiry.empty() && blacklist_by_expiry.begin()->expiry <= lib_time) {
         if (deadline <= fc::time_point::now()) {
            exhausted = true;
            break;
         }
         blacklist_by_expiry.erase(blacklist_by_expiry.begin());
         num_expired++;
      }

      fc_dlog(_log, "Processed ${n} blacklisted transactions, Expired ${expired}",
              ("n", orig_count)("expired", num_expired));
   }
   return !exhausted;
}

bool producer_plugin_impl::process_unapplied_trxs( const fc::time_point& deadline ) {
   bool exhausted = false;
   if( !_unapplied_transactions.empty() ) {
      chain::controller& chain = chain_plug->chain();
      int num_applied = 0, num_failed = 0, num_processed = 0;
      auto unapplied_trxs_size = _unapplied_transactions.size();
      // unapplied and persisted do not have a next method to call
      auto itr     = (_pending_block_mode == eosio::pending_block_mode::producing) ?
                     _unapplied_transactions.unapplied_begin() : _unapplied_transactions.persisted_begin();
      auto end_itr = (_pending_block_mode == eosio::pending_block_mode::producing) ?
                     _unapplied_transactions.unapplied_end()   : _unapplied_transactions.persisted_end();
      while( itr != end_itr ) {
         if( deadline <= fc::time_point::now() ) {
            exhausted = true;
            break;
         }

         const eosio::chain::transaction_metadata_ptr trx = itr->trx_meta;
         ++num_processed;
         try {
            auto trx_deadline = fc::time_point::now() + fc::milliseconds( _max_transaction_time_ms );
            bool deadline_is_subjective = false;
            if( _max_transaction_time_ms < 0 ||
                (_pending_block_mode == eosio::pending_block_mode::producing && deadline < trx_deadline) ) {
               deadline_is_subjective = true;
               trx_deadline = deadline;
            }

            auto trace = chain.push_transaction( trx, trx_deadline, trx->billed_cpu_time_us, false );
            if( trace->except ) {
               if( exception_is_exhausted( *trace->except, deadline_is_subjective ) ) {
                  if( block_is_exhausted() ) {
                     exhausted = true;
                     // don't erase, subjective failure so try again next time
                     break;
                  }
                  // don't erase, subjective failure so try again next time
               } else {
                  // this failed our configured maximum transaction time, we don't want to replay it
                  ++num_failed;
                  if( itr->next ) itr->next( trace );
                  itr = _unapplied_transactions.erase( itr );
                  continue;
               }
            } else {
               ++num_applied;
               if( itr->trx_type != eosio::chain::trx_enum_type::persisted ) {
                  if( itr->next ) itr->next( trace );
                  itr = _unapplied_transactions.erase( itr );
                  continue;
               }
            }
         } LOG_AND_DROP();
         ++itr;
      }

      fc_dlog( _log, "Processed ${m} of ${n} previously applied transactions, Applied ${applied}, Failed/Dropped ${failed}",
               ("m", num_processed)( "n", unapplied_trxs_size )("applied", num_applied)("failed", num_failed) );
   }
   return !exhausted;
}

void producer_plugin_impl::process_scheduled_and_incoming_trxs( const fc::time_point& deadline, size_t& pending_incoming_process_limit ) {
   // scheduled transactions
   int num_applied = 0;
   int num_failed = 0;
   int num_processed = 0;
   bool exhausted = false;
   double incoming_trx_weight = 0.0;

   auto& blacklist_by_id = _blacklisted_transactions.get<by_id>();
   eosio::chain::controller& chain = chain_plug->chain();
   fc::time_point pending_block_time = chain.pending_block_time();
   auto itr = _unapplied_transactions.incoming_begin();
   auto end = _unapplied_transactions.incoming_end();
   const auto& sch_idx = chain.db().get_index<eosio::chain::generated_transaction_multi_index, eosio::chain::by_delay>();
   const auto scheduled_trxs_size = sch_idx.size();
   auto sch_itr = sch_idx.begin();
   while( sch_itr != sch_idx.end() ) {
      if( sch_itr->delay_until > pending_block_time) break;    // not scheduled yet
      if( exhausted || deadline <= fc::time_point::now() ) {
         exhausted = true;
         break;
      }
      if( sch_itr->published >= pending_block_time ) {
         ++sch_itr;
         continue; // do not allow schedule and execute in same block
      }

      if (blacklist_by_id.find(sch_itr->trx_id) != blacklist_by_id.end()) {
         ++sch_itr;
         continue;
      }

      const transaction_id_type trx_id = sch_itr->trx_id; // make copy since reference could be invalidated
      const auto sch_expiration = sch_itr->expiration;
      auto sch_itr_next = sch_itr; // save off next since sch_itr may be invalidated by loop
      ++sch_itr_next;
      const auto next_delay_until = sch_itr_next != sch_idx.end() ? sch_itr_next->delay_until : sch_itr->delay_until;
      const auto next_id = sch_itr_next != sch_idx.end() ? sch_itr_next->id : sch_itr->id;

      num_processed++;

      // configurable ratio of incoming txns vs deferred txns
      while (incoming_trx_weight >= 1.0 && pending_incoming_process_limit && itr != end ) {
         if (deadline <= fc::time_point::now()) {
            exhausted = true;
            break;
         }

         --pending_incoming_process_limit;
         incoming_trx_weight -= 1.0;

         auto trx_meta = itr->trx_meta;
         auto next = itr->next;
         bool persist_until_expired = itr->trx_type == eosio::chain::trx_enum_type::incoming_persisted;
         itr = _unapplied_transactions.erase( itr );
         if( !process_incoming_transaction_async( trx_meta, persist_until_expired, next ) ) {
            exhausted = true;
            break;
         }
      }

      if (exhausted || deadline <= fc::time_point::now()) {
         exhausted = true;
         break;
      }

      try {
         auto trx_deadline = fc::time_point::now() + fc::milliseconds(_max_transaction_time_ms);
         bool deadline_is_subjective = false;
         if (_max_transaction_time_ms < 0 || (_pending_block_mode == eosio::pending_block_mode::producing && deadline < trx_deadline)) {
            deadline_is_subjective = true;
            trx_deadline = deadline;
         }

         auto trace = chain.push_scheduled_transaction(trx_id, trx_deadline, 0, false);
         if (trace->except) {
            if (exception_is_exhausted(*trace->except, deadline_is_subjective)) {
               if( block_is_exhausted() ) {
                  exhausted = true;
                  break;
               }
               // do not blacklist
            } else {
               // this failed our configured maximum transaction time, we don't want to replay it add it to a blacklist
               _blacklisted_transactions.insert(transaction_id_with_expiry{trx_id, sch_expiration});
               num_failed++;
            }
         } else {
            num_applied++;
         }
      } LOG_AND_DROP();

      incoming_trx_weight += _incoming_defer_ratio;
      if (!pending_incoming_process_limit) incoming_trx_weight = 0.0;

      if( sch_itr_next == sch_idx.end() ) break;
      sch_itr = sch_idx.lower_bound( boost::make_tuple( next_delay_until, next_id ) );
   }

   if( scheduled_trxs_size > 0 ) {
      fc_dlog( _log,
               "Processed ${m} of ${n} scheduled transactions, Applied ${applied}, Failed/Dropped ${failed}",
               ( "m", num_processed )( "n", scheduled_trxs_size )( "applied", num_applied )( "failed", num_failed ) );
   }
}

bool producer_plugin_impl::process_incoming_trxs( const fc::time_point& deadline, size_t& pending_incoming_process_limit )
{
   bool exhausted = false;
   if( pending_incoming_process_limit ) {
      size_t processed = 0;
      fc_dlog( _log, "Processing ${n} pending transactions", ("n", pending_incoming_process_limit) );
      auto itr = _unapplied_transactions.incoming_begin();
      auto end = _unapplied_transactions.incoming_end();
      while( pending_incoming_process_limit && itr != end ) {
         if (deadline <= fc::time_point::now()) {
            exhausted = true;
            break;
         }
         --pending_incoming_process_limit;
         auto trx_meta = itr->trx_meta;
         auto next = itr->next;
         bool persist_until_expired = itr->trx_type == eosio::chain::trx_enum_type::incoming_persisted;
         itr = _unapplied_transactions.erase( itr );
         ++processed;
         if( !process_incoming_transaction_async( trx_meta, persist_until_expired, next ) ) {
            exhausted = true;
            break;
         }
      }
      fc_dlog( _log, "Processed ${n} pending transactions, ${p} left", ("n", processed)("p", _unapplied_transactions.incoming_size()) );
   }
   return !exhausted;
}

bool producer_plugin_impl::block_is_exhausted() const {
   const chain::controller& chain = chain_plug->chain();
   const auto& rl = chain.get_resource_limits_manager();

   const uint64_t cpu_limit = rl.get_block_cpu_limit();
   if( cpu_limit < _max_block_cpu_usage_threshold_us ) return true;
   const uint64_t net_limit = rl.get_block_net_limit();
   if( net_limit < _max_block_net_usage_threshold_bytes ) return true;
   return false;
}

void producer_plugin_impl::schedule_maybe_produce_block( bool exhausted ) {
   chain::controller& chain = chain_plug->chain();

   // we succeeded but block may be exhausted
   static const boost::posix_time::ptime epoch( boost::gregorian::date( 1970, 1, 1 ) );
   auto deadline = calculate_block_deadline( chain.pending_block_time() );

   if( !exhausted && deadline > fc::time_point::now() ) {
      // ship this block off no later than its deadline
      EOS_ASSERT( chain.is_building_block(), eosio::chain::missing_pending_block_state,
                  "producing without pending_block_state, start_block succeeded" );
      _timer.expires_at( epoch + boost::posix_time::microseconds( deadline.time_since_epoch().count() ) );
      fc_dlog( _log, "Scheduling Block Production on Normal Block #${num} for ${time}",
               ("num", chain.head_block_num() + 1)( "time", deadline ) );
   } else {
      EOS_ASSERT( chain.is_building_block(), eosio::chain::missing_pending_block_state, "producing without pending_block_state" );
      _timer.expires_from_now( boost::posix_time::microseconds( 0 ) );
      fc_dlog( _log, "Scheduling Block Production on ${desc} Block #${num} immediately",
               ("num", chain.head_block_num() + 1)("desc", block_is_exhausted() ? "Exhausted" : "Deadline exceeded") );
   }

   _timer.async_wait( app().get_priority_queue().wrap( priority::high,
         [&chain, weak_this = weak_from_this(), cid=++_timer_corelation_id](const boost::system::error_code& ec) {
            auto self = weak_this.lock();
            if( self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id ) {
               // pending_block_state expected, but can't assert inside async_wait
               auto block_num = chain.is_building_block() ? chain.head_block_num() + 1 : 0;
               fc_dlog( _log, "Produce block timer for ${num} running at ${time}", ("num", block_num)("time", fc::time_point::now()) );
               auto res = self->maybe_produce_block();
               fc_dlog( _log, "Producing Block #${num} returned: ${res}", ("num", block_num)( "res", res ) );
            }
         } ) );
}

fc::optional<fc::time_point> producer_plugin_impl::calculate_producer_wake_up_time( const eosio::chain::block_timestamp_type& ref_block_time ) const {
   // if we have any producers then we should at least set a timer for our next available slot
   optional<fc::time_point> wake_up_time;
   for (const auto& p : _producers) {
      auto next_producer_block_time = calculate_next_block_time(p, ref_block_time);
      if (next_producer_block_time) {
         auto producer_wake_up_time = *next_producer_block_time - fc::microseconds(eosio::chain::config::block_interval_us);
         if (wake_up_time) {
            // wake up with a full block interval to the deadline
            if( producer_wake_up_time < *wake_up_time ) {
               wake_up_time = producer_wake_up_time;
            }
         } else {
            wake_up_time = producer_wake_up_time;
         }
      }
   }
   if( !wake_up_time ) {
      fc_dlog(_log, "Not Scheduling Speculative/Production, no local producers had valid wake up times");
   }

   return wake_up_time;
}

void producer_plugin_impl::schedule_delayed_production_loop(const std::weak_ptr<producer_plugin_impl>& weak_this, optional<fc::time_point> wake_up_time) {
   if (wake_up_time) {
      fc_dlog(_log, "Scheduling Speculative/Production Change at ${time}", ("time", wake_up_time));
      static const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
      _timer.expires_at(epoch + boost::posix_time::microseconds(wake_up_time->time_since_epoch().count()));
      _timer.async_wait( app().get_priority_queue().wrap( priority::high,
         [weak_this,cid=++_timer_corelation_id](const boost::system::error_code& ec) {
            auto self = weak_this.lock();
            if( self && ec != boost::asio::error::operation_aborted && cid == self->_timer_corelation_id ) {
               self->schedule_production_loop();
            }
         } ) );
   }
}

bool producer_plugin_impl::maybe_produce_block() {
   auto reschedule = fc::make_scoped_exit([this]{
      schedule_production_loop();
   });

   try {
      produce_block();
      return true;
   } LOG_AND_DROP();

   fc_dlog(_log, "Aborting block due to produce_block error");
   chain::controller& chain = chain_plug->chain();
   _unapplied_transactions.add_aborted( chain.abort_block() );
   return false;
}


void producer_plugin_impl::produce_block() {
   //ilog("produce_block ${t}", ("t", fc::time_point::now())); // for testing _produce_time_offset_us
   EOS_ASSERT(_pending_block_mode == eosio::pending_block_mode::producing, eosio::chain::producer_exception, "called produce_block while not actually producing");
   chain::controller& chain = chain_plug->chain();
   EOS_ASSERT(chain.is_building_block(), eosio::chain::missing_pending_block_state, "pending_block_state does not exist but it should, another plugin may have corrupted it");

   const auto& auth = chain.pending_block_signing_authority();
   std::vector<std::reference_wrapper<const signature_provider_type>> relevant_providers;

   relevant_providers.reserve(_signature_providers.size());

   eosio::chain::producer_authority::for_each_key(auth, [&](const public_key_type& key){
      const auto& iter = _signature_providers.find(key);
      if (iter != _signature_providers.end()) {
         relevant_providers.emplace_back(iter->second);
      }
   });

   EOS_ASSERT(relevant_providers.size() > 0, eosio::chain::producer_priv_key_not_found, "Attempting to produce a block for which we don't have any relevant private keys");

   if (_protocol_features_signaled) {
      _protocol_features_to_activate.clear(); // clear _protocol_features_to_activate as it is already set in pending_block
      _protocol_features_signaled = false;
   }

   //idump( (fc::time_point::now() - chain.pending_block_time()) );
   chain.finalize_block( [&]( const eosio::chain::digest_type& d ) {
      auto debug_logger = maybe_make_debug_time_logger();
      std::vector<eosio::chain::signature_type> sigs;
      sigs.reserve(relevant_providers.size());

      // sign with all relevant public keys
      for (const auto& p : relevant_providers) {
         sigs.emplace_back(p.get()(d));
      }
      return sigs;
   } );

   chain.commit_block();

   eosio::chain::block_state_ptr new_bs = chain.head_block_state();

   ilog("Produced block ${id}... #${n} @ ${t} signed by ${p} [trxs: ${count}, lib: ${lib}, confirmed: ${confs}]",
        ("p",new_bs->header.producer)("id",new_bs->id.str().substr(8,16))
        ("n",new_bs->block_num)("t",new_bs->header.timestamp)
        ("count",new_bs->block->transactions.size())("lib",chain.last_irreversible_block_num())("confs", new_bs->header.confirmed));

}

}
