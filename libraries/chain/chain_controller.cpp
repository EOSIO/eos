/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/chain/chain_controller.hpp>

#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contracts/contract_table_objects.hpp>
#include <eosio/chain/action_objects.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/producer_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/authority_checker.hpp>
#include <eosio/chain/contracts/chain_initializer.hpp>
#include <eosio/chain/scope_sequence_object.hpp>
#include <eosio/chain/merkle.hpp>

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wasm_interface.hpp>

#include <eosio/utilities/rand.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>
#include <fc/crypto/digest.hpp>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm_ext/is_sorted.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/range/algorithm/equal.hpp>

#include <fstream>
#include <functional>
#include <chrono>

namespace eosio { namespace chain {

bool chain_controller::is_start_of_round( block_num_type block_num )const  {
  return 0 == (block_num % blocks_per_round());
}

uint32_t chain_controller::blocks_per_round()const {
  return get_global_properties().active_producers.producers.size()*config::producer_repetitions;
}

chain_controller::chain_controller( const chain_controller::controller_config& cfg )
:_db( cfg.shared_memory_dir,
      (cfg.read_only ? database::read_only : database::read_write),
      cfg.shared_memory_size),
 _block_log(cfg.block_log_dir),
 _wasm_interface(cfg.wasm_runtime),
 _limits(cfg.limits),
 _resource_limits(_db)
{
   _initialize_indexes();
   _resource_limits.initialize_database();


   for (auto& f : cfg.applied_block_callbacks)
      applied_block.connect(f);
   for (auto& f : cfg.applied_irreversible_block_callbacks)
      applied_irreversible_block.connect(f);
   for (auto& f : cfg.on_pending_transaction_callbacks)
      on_pending_transaction.connect(f);

   contracts::chain_initializer starter(cfg.genesis);
   starter.register_types(*this, _db);

   // Behave as though we are applying a block during chain initialization (it's the genesis block!)
   with_applying_block([&] {
      _initialize_chain(starter);
   });

   _spinup_db();
   _spinup_fork_db();

   if (_block_log.read_head() && head_block_num() < _block_log.read_head()->block_num())
      replay();
} /// chain_controller::chain_controller


chain_controller::~chain_controller() {
   clear_pending();
   _db.flush();
}

bool chain_controller::is_known_block(const block_id_type& id)const
{
   return _fork_db.is_known_block(id) || _block_log.read_block_by_id(id);
}
/**
 * Only return true *if* the transaction has not expired or been invalidated. If this
 * method is called with a VERY old transaction we will return false, they should
 * query things by blocks if they are that old.
 */
bool chain_controller::is_known_transaction(const transaction_id_type& id)const
{
   const auto& trx_idx = _db.get_index<transaction_multi_index, by_trx_id>();
   return trx_idx.find( id ) != trx_idx.end();
}

block_id_type chain_controller::get_block_id_for_num(uint32_t block_num)const
{ try {
   if (const auto& block = fetch_block_by_number(block_num))
      return block->id();

   FC_THROW_EXCEPTION(unknown_block_exception, "Could not find block");
} FC_CAPTURE_AND_RETHROW((block_num)) }

optional<signed_block> chain_controller::fetch_block_by_id(const block_id_type& id)const
{
   auto b = _fork_db.fetch_block(id);
   if(b) return b->data;
   return _block_log.read_block_by_id(id);
}

optional<signed_block> chain_controller::fetch_block_by_number(uint32_t num)const
{
   if (const auto& block = _block_log.read_block_by_num(num))
      return *block;

   // Not in _block_log, so it must be since the last irreversible block. Grab it from _fork_db instead
   if (num <= head_block_num()) {
      auto block = _fork_db.head();
      while (block && block->num > num)
         block = block->prev.lock();
      if (block && block->num == num)
         return block->data;
   }

   return optional<signed_block>();
}

std::vector<block_id_type> chain_controller::get_block_ids_on_fork(block_id_type head_of_fork) const
{
  pair<fork_database::branch_type, fork_database::branch_type> branches = _fork_db.fetch_branch_from(head_block_id(), head_of_fork);
  if( !((branches.first.back()->previous_id() == branches.second.back()->previous_id())) )
  {
     edump( (head_of_fork)
            (head_block_id())
            (branches.first.size())
            (branches.second.size()) );
     assert(branches.first.back()->previous_id() == branches.second.back()->previous_id());
  }
  std::vector<block_id_type> result;
  for (const item_ptr& fork_block : branches.second)
    result.emplace_back(fork_block->id);
  result.emplace_back(branches.first.back()->previous_id());
  return result;
}


/**
 * Push block "may fail" in which case every partial change is unwound.  After
 * push block is successful the block is appended to the chain database on disk.
 *
 * @return true if we switched forks as a result of this push.
 */
void chain_controller::push_block(const signed_block& new_block, uint32_t skip)
{ try {
   with_skip_flags( skip, [&](){
      return without_pending_transactions( [&]() {
         return _db.with_write_lock( [&]() {
            return _push_block(new_block);
         } );
      });
   });
} FC_CAPTURE_AND_RETHROW((new_block)) }

bool chain_controller::_push_block(const signed_block& new_block)
{ try {
   uint32_t skip = _skip_flags;
   if (!(skip&skip_fork_db)) {
      /// TODO: if the block is greater than the head block and before the next maintenance interval
      // verify that the block signer is in the current set of active producers.

      shared_ptr<fork_item> new_head = _fork_db.push_block(new_block);
      //If the head block from the longest chain does not build off of the current head, we need to switch forks.
      if (new_head->data.previous != head_block_id()) {
         //If the newly pushed block is the same height as head, we get head back in new_head
         //Only switch forks if new_head is actually higher than head
         if (new_head->data.block_num() > head_block_num()) {
            wlog("Switching to fork: ${id}", ("id",new_head->data.id()));
            auto branches = _fork_db.fetch_branch_from(new_head->data.id(), head_block_id());

            // pop blocks until we hit the forked block
            while (head_block_id() != branches.second.back()->data.previous)
               pop_block();

            // push all blocks on the new fork
            for (auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr) {
                ilog("pushing blocks from fork ${n} ${id}", ("n",(*ritr)->data.block_num())("id",(*ritr)->data.id()));
                optional<fc::exception> except;
                try {
                   auto session = _db.start_undo_session(true);
                   _apply_block((*ritr)->data, skip);
                   session.push();
                }
                catch (const fc::exception& e) { except = e; }
                if (except) {
                   wlog("exception thrown while switching forks ${e}", ("e",except->to_detail_string()));
                   // remove the rest of branches.first from the fork_db, those blocks are invalid
                   while (ritr != branches.first.rend()) {
                      _fork_db.remove((*ritr)->data.id());
                      ++ritr;
                   }
                   _fork_db.set_head(branches.second.front());

                   // pop all blocks from the bad fork
                   while (head_block_id() != branches.second.back()->data.previous)
                      pop_block();

                   // restore all blocks from the good fork
                   for (auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr) {
                      auto session = _db.start_undo_session(true);
                      _apply_block((*ritr)->data, skip);
                      session.push();
                   }
                   throw *except;
                }
            }
            return true; //swithced fork
         }
         else return false; // didn't switch fork
      }
   }

   try {
      auto session = _db.start_undo_session(true);
      _apply_block(new_block, skip);
      session.push();
   } catch ( const fc::exception& e ) {
      elog("Failed to push new block:\n${e}", ("e", e.to_detail_string()));
      _fork_db.remove(new_block.id());
      throw;
   }

   return false;
} FC_CAPTURE_AND_RETHROW((new_block)) }

/**
 * Attempts to push the transaction into the pending queue
 *
 * When called to push a locally generated transaction, set the skip_block_size_check bit on the skip argument. This
 * will allow the transaction to be pushed even if it causes the pending block size to exceed the maximum block size.
 * Although the transaction will probably not propagate further now, as the peers are likely to have their pending
 * queues full as well, it will be kept in the queue to be propagated later when a new block flushes out the pending
 * queues.
 */
transaction_trace chain_controller::push_transaction(const packed_transaction& trx, uint32_t skip)
{ try {
   // If this is the first transaction pushed after applying a block, start a new undo session.
   // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
   if( !_pending_block ) {
      _start_pending_block();
   }

   return with_skip_flags(skip, [&]() {
      return _db.with_write_lock([&]() {
         return _push_transaction(trx);
      });
   });
} EOS_CAPTURE_AND_RETHROW( transaction_exception ) }

transaction_trace chain_controller::_push_transaction(const packed_transaction& packed_trx)
{ try {
   //edump((transaction_header(packed_trx.get_transaction())));
   auto start = fc::time_point::now();
   transaction_metadata   mtrx( packed_trx, get_chain_id(), head_block_time());
   //idump((transaction_header(mtrx.trx())));

   const auto delay = check_transaction_authorization(mtrx.trx(), packed_trx.signatures, packed_trx.context_free_data);
   auto setup_us = fc::time_point::now() - start;

   // enforce that the header is accurate as a commitment to net_usage
   uint32_t cfa_sig_net_usage = (uint32_t)(packed_trx.context_free_data.size() + fc::raw::pack_size(packed_trx.signatures));
   uint32_t net_usage_commitment = mtrx.trx().net_usage_words.value * 8U;
   uint32_t packed_size = (uint32_t)packed_trx.data.size();
   uint32_t net_usage = cfa_sig_net_usage + packed_size;
   EOS_ASSERT(net_usage <= net_usage_commitment,
                tx_resource_exhausted,
                "Packed Transaction and associated data does not fit into the space committed to by the transaction's header! [usage=${usage},commitment=${commit}]",
                ("usage", net_usage)("commit", net_usage_commitment));

   transaction_trace result(mtrx.id);
   result._setup_profiling_us = setup_us;

   if (!delay.sec_since_epoch()) {
      result = _push_transaction(std::move(mtrx));

   } else {
      auto delayed_transaction_processing = [&](transaction_metadata& meta) {
         result.status = transaction_trace::delayed;
         const auto trx = mtrx.trx();
         FC_ASSERT( !trx.actions.empty(), "transaction must have at least one action");

         FC_ASSERT( trx.expiration > (head_block_time() + fc::milliseconds(2*config::block_interval_ms)),
                                      "transaction is expired when created" );

         // add in the system account authorization
         action for_deferred = trx.actions[0];
         bool found = false;
         for (const auto& auth : for_deferred.authorization) {
            if (auth.actor == config::system_account_name &&
                auth.permission == config::active_name) {
               found = true;
               break;
            }
         }
         if (!found)
            for_deferred.authorization.push_back(permission_level{config::system_account_name, config::active_name});

         apply_context context(*this, _db, for_deferred, mtrx);

         time_point_sec execute_after = head_block_time();
         execute_after += time_point_sec(delay);
         //TODO: !!! WHO GETS BILLED TO STORE THE DELAYED TX?
         deferred_transaction dtrx(context.get_next_sender_id(), config::system_account_name, config::system_account_name, execute_after, trx);
         FC_ASSERT( dtrx.execute_after < dtrx.expiration, "transaction expires before it can execute" );

         result.deferred_transaction_requests.push_back(std::move(dtrx));

         _create_generated_transaction(result.deferred_transaction_requests[0].get<deferred_transaction>());

         return result;
      };

      result = wrap_transaction_processing( move(mtrx), delayed_transaction_processing);
   }

   // notify anyone listening to pending transactions
   on_pending_transaction(_pending_transaction_metas.back(), packed_trx);

   _pending_block->input_transactions.emplace_back(packed_trx);

   return result;

} FC_CAPTURE_AND_RETHROW( (transaction_header(packed_trx.get_transaction())) ) }

static void record_locks_for_data_access(transaction_trace& trace, flat_set<shard_lock>& read_locks, flat_set<shard_lock>& write_locks ) {
   for (const auto& at: trace.action_traces) {
      for (const auto& access: at.data_access) {
         if (access.type == data_access_info::read) {
            trace.read_locks.emplace(shard_lock{access.code, access.scope});
         } else {
            trace.write_locks.emplace(shard_lock{access.code, access.scope});
         }
      }
   }

   // remove read locks for write locks taken by other actions
   std::for_each(trace.write_locks.begin(), trace.write_locks.end(), [&]( const shard_lock& l){
      trace.read_locks.erase(l); read_locks.erase(l);
   });

   read_locks.insert(trace.read_locks.begin(), trace.read_locks.end());
   write_locks.insert(trace.write_locks.begin(), trace.write_locks.end());
}

transaction_trace chain_controller::_push_transaction( transaction_metadata&& data )
{ try {
   auto process_apply_transaction = [this](transaction_metadata& meta) {
      auto cyclenum = _pending_block->regions.back().cycles_summary.size() - 1;
      //wdump((transaction_header(meta.trx())));

      /// TODO: move _pending_cycle into db so that it can be undone if transation fails, for now we will apply
      /// the transaction first so that there is nothing to undo... this only works because things are currently
      /// single threaded
      // set cycle, shard, region etc
      meta.region_id = 0;
      meta.cycle_index = cyclenum;
      meta.shard_index = 0;
      return _apply_transaction( meta );
   };
 //  wdump((transaction_header(data.trx())));
   return wrap_transaction_processing( move(data), process_apply_transaction );
} FC_CAPTURE_AND_RETHROW( ) }


block_header chain_controller::head_block_header() const
{
   auto b = _fork_db.fetch_block(head_block_id());
   if( b ) return b->data;
   
   if (auto head_block = fetch_block_by_id(head_block_id()))
      return *head_block;
   return block_header();
}

void chain_controller::_start_pending_block( bool skip_deferred )
{
   FC_ASSERT( !_pending_block );
   _pending_block         = signed_block();
   _pending_block_trace   = block_trace(*_pending_block);
   _pending_block_session = _db.start_undo_session(true);
   _pending_block->regions.resize(1);
   _pending_block_trace->region_traces.resize(1);

   _start_pending_cycle();
   _apply_on_block_transaction();
   _finalize_pending_cycle();

   _start_pending_cycle();

   if ( !skip_deferred ) {
      _push_deferred_transactions( false );
      if (_pending_cycle_trace && _pending_cycle_trace->shard_traces.size() > 0 && _pending_cycle_trace->shard_traces.back().transaction_traces.size() > 0) {
         _finalize_pending_cycle();
         _start_pending_cycle();
      }
   }
}

transaction chain_controller::_get_on_block_transaction()
{
   action on_block_act;
   on_block_act.account = config::system_account_name;
   on_block_act.name = N(onblock);
   on_block_act.authorization = vector<permission_level>{{config::system_account_name, config::active_name}};
   on_block_act.data = fc::raw::pack(head_block_header());

   transaction trx;
   trx.actions.emplace_back(std::move(on_block_act));
   trx.set_reference_block(head_block_id());
   trx.expiration = head_block_time() + fc::seconds(1);
   trx.kcpu_usage = 2000; // 1 << 24;
   return trx;
}

void chain_controller::_apply_on_block_transaction()
{
   _pending_block_trace->implicit_transactions.emplace_back(_get_on_block_transaction());
   transaction_metadata mtrx(packed_transaction(_pending_block_trace->implicit_transactions.back()), get_chain_id(), head_block_time());
   _push_transaction(std::move(mtrx));
}

/**
 *  Wraps up all work for current shards, starts a new cycle, and
 *  executes any pending transactions
 */
void chain_controller::_start_pending_cycle() {
   // only add a new cycle if there are no cycles or if the previous cycle isn't empty
   if (_pending_block->regions.back().cycles_summary.empty() ||
       (!_pending_block->regions.back().cycles_summary.back().empty() &&
        !_pending_block->regions.back().cycles_summary.back().back().empty()))
      _pending_block->regions.back().cycles_summary.resize( _pending_block->regions[0].cycles_summary.size() + 1 );


   _pending_cycle_trace = cycle_trace();

   _pending_cycle_trace->shard_traces.resize(_pending_cycle_trace->shard_traces.size() + 1 );

   auto& bcycle = _pending_block->regions.back().cycles_summary.back();
   if(bcycle.empty() || !bcycle.back().empty())
      bcycle.resize( bcycle.size()+1 );
}

void chain_controller::_finalize_pending_cycle()
{
   // prune empty shard
   if (!_pending_block->regions.back().cycles_summary.empty() &&
       !_pending_block->regions.back().cycles_summary.back().empty() &&
       _pending_block->regions.back().cycles_summary.back().back().empty()) {
      _pending_block->regions.back().cycles_summary.back().resize( _pending_block->regions.back().cycles_summary.back().size() - 1 );
      _pending_cycle_trace->shard_traces.resize(_pending_cycle_trace->shard_traces.size() - 1 );
   }
   // prune empty cycle
   if (!_pending_block->regions.back().cycles_summary.empty() &&
       _pending_block->regions.back().cycles_summary.back().empty()) {
      _pending_block->regions.back().cycles_summary.resize( _pending_block->regions.back().cycles_summary.size() - 1 );
      _pending_cycle_trace.reset();
      return;
   }

   for( int idx = 0; idx < _pending_cycle_trace->shard_traces.size(); idx++ ) {
      auto& trace = _pending_cycle_trace->shard_traces.at(idx);
      auto& shard = _pending_block->regions.back().cycles_summary.back().at(idx);

      trace.finalize_shard();
      shard.read_locks.reserve(trace.read_locks.size());
      shard.read_locks.insert(shard.read_locks.end(), trace.read_locks.begin(), trace.read_locks.end());

      shard.write_locks.reserve(trace.write_locks.size());
      shard.write_locks.insert(shard.write_locks.end(), trace.write_locks.begin(), trace.write_locks.end());
   }

   _apply_cycle_trace(*_pending_cycle_trace);
   _pending_block_trace->region_traces.back().cycle_traces.emplace_back(std::move(*_pending_cycle_trace));
   _pending_cycle_trace.reset();
}

void chain_controller::_apply_cycle_trace( const cycle_trace& res )
{
   auto &generated_transaction_idx = _db.get_mutable_index<generated_transaction_multi_index>();
   const auto &generated_index = generated_transaction_idx.indices().get<by_sender_id>();

   for (const auto&st: res.shard_traces) {
      for (const auto &tr: st.transaction_traces) {
         for (const auto &req: tr.deferred_transaction_requests) {
            if ( req.contains<deferred_transaction>() ) {
               const auto& dt = req.get<deferred_transaction>();
               const auto itr = generated_index.lower_bound(boost::make_tuple(dt.sender, dt.sender_id));
               if ( itr != generated_index.end() && itr->sender == dt.sender && itr->sender_id == dt.sender_id ) {
                  _destroy_generated_transaction(*itr);
               }

               _create_generated_transaction(dt);
            } else if ( req.contains<deferred_reference>() ) {
               const auto& dr = req.get<deferred_reference>();
               const auto itr = generated_index.lower_bound(boost::make_tuple(dr.sender, dr.sender_id));
               if ( itr != generated_index.end() && itr->sender == dr.sender && itr->sender_id == dr.sender_id ) {
                  _destroy_generated_transaction(*itr);
               }
            }
         }
         ///TODO: hook this up as a signal handler in a de-coupled "logger" that may just silently drop them
         for (const auto &ar : tr.action_traces) {
            if (!ar.console.empty()) {
               auto prefix = fc::format_string(
                  "[(${a},${n})->${r}]",
                  fc::mutable_variant_object()
                     ("a", ar.act.account)
                     ("n", ar.act.name)
                     ("r", ar.receiver));
               ilog(prefix + ": CONSOLE OUTPUT BEGIN =====================");
               ilog(ar.console);
               ilog(prefix + ": CONSOLE OUTPUT END   =====================");
            }
         }
      }
   }
}

/**
 *  After applying all transactions successfully we can update
 *  the current block time, block number, producer stats, etc
 */
void chain_controller::_finalize_block( const block_trace& trace ) { try {
   const auto& b = trace.block;
   const producer_object& signing_producer = validate_block_header(_skip_flags, b);

   update_global_properties( b );
   update_global_dynamic_data( b );
   update_signing_producer(signing_producer, b);

   create_block_summary(b);
   clear_expired_transactions();

   update_last_irreversible_block();
   _resource_limits.process_account_limit_updates();

   // trigger an update of our elastic values for block limits
   _resource_limits.process_block_usage(b.block_num());

   applied_block( trace ); //emit
   if (_currently_replaying_blocks)
     applied_irreversible_block(b);

} FC_CAPTURE_AND_RETHROW( (trace.block) ) }

signed_block chain_controller::generate_block(
   block_timestamp_type when,
   account_name producer,
   const private_key_type& block_signing_private_key,
   uint32_t skip /* = 0 */
   )
{ try {
   return with_skip_flags( skip | created_block, [&](){
      return _db.with_write_lock( [&](){
         return _generate_block( when, producer, block_signing_private_key );
      });
   });
} FC_CAPTURE_AND_RETHROW( (when) ) }

signed_block chain_controller::_generate_block( block_timestamp_type when,
                                              account_name producer,
                                              const private_key_type& block_signing_key )
{ try {

   try {
      uint32_t skip     = _skip_flags;
      uint32_t slot_num = get_slot_at_time( when );
      FC_ASSERT( slot_num > 0 );
      account_name scheduled_producer = get_scheduled_producer( slot_num );
      FC_ASSERT( scheduled_producer == producer );

      const auto& producer_obj = get_producer(scheduled_producer);

      if( !_pending_block ) {
         _start_pending_block();
      }

      _finalize_pending_cycle();

      if( !(skip & skip_producer_signature) )
         FC_ASSERT( producer_obj.signing_key == block_signing_key.get_public_key(),
                    "producer key ${pk}, block key ${bk}", ("pk", producer_obj.signing_key)("bk", block_signing_key.get_public_key()) );

      _pending_block->timestamp   = when;
      _pending_block->producer    = producer_obj.owner;
      _pending_block->previous    = head_block_id();
      _pending_block->block_mroot = get_dynamic_global_properties().block_merkle_root.get_root();
      _pending_block->transaction_mroot = transaction_metadata::calculate_transaction_merkle_root( _pending_transaction_metas );
      _pending_block->action_mroot = _pending_block_trace->calculate_action_merkle_root();

      if( is_start_of_round( _pending_block->block_num() ) ) {
         auto latest_producer_schedule = _calculate_producer_schedule();
         if( latest_producer_schedule != _head_producer_schedule() )
            _pending_block->new_producers = latest_producer_schedule;
      }
      _pending_block->schedule_version = get_global_properties().active_producers.version;

      if( !(skip & skip_producer_signature) )
         _pending_block->sign( block_signing_key );

      _finalize_block( *_pending_block_trace );

      _pending_block_session->push();

      auto result = move( *_pending_block );

      clear_pending();

      if (!(skip&skip_fork_db)) {
         _fork_db.push_block(result);
      }
      return result;
   } catch ( ... ) {
      clear_pending();

      elog( "error while producing block" );
      _start_pending_block();
      throw;
   }

} FC_CAPTURE_AND_RETHROW( (producer) ) }

/**
 * Removes the most recent block from the database and undoes any changes it made.
 */
void chain_controller::pop_block()
{ try {
   _pending_block_session.reset();
   auto head_id = head_block_id();
   optional<signed_block> head_block = fetch_block_by_id( head_id );
   EOS_ASSERT( head_block.valid(), pop_empty_chain, "there are no blocks to pop" );

   _fork_db.pop_block();
   _db.undo();
} FC_CAPTURE_AND_RETHROW() }

void chain_controller::clear_pending()
{ try {
   _pending_block_trace.reset();
   _pending_block.reset();
   _pending_block_session.reset();
   _pending_transaction_metas.clear();
} FC_CAPTURE_AND_RETHROW() }

//////////////////// private methods ////////////////////

void chain_controller::_apply_block(const signed_block& next_block, uint32_t skip)
{
   auto block_num = next_block.block_num();
   if (_checkpoints.size() && _checkpoints.rbegin()->second != block_id_type()) {
      auto itr = _checkpoints.find(block_num);
      if (itr != _checkpoints.end())
         FC_ASSERT(next_block.id() == itr->second,
                   "Block did not match checkpoint", ("checkpoint",*itr)("block_id",next_block.id()));

      if (_checkpoints.rbegin()->first >= block_num)
         skip = ~0;// WE CAN SKIP ALMOST EVERYTHING
   }

   with_applying_block([&] {
      with_skip_flags(skip, [&] {
         __apply_block(next_block);
      });
   });
}

static void validate_shard_locks(const vector<shard_lock>& locks, const string& tag) {
   if (locks.size() < 2) {
      return;
   }

   for (auto cur = locks.begin() + 1; cur != locks.end(); ++cur) {
      auto prev = cur - 1;
      EOS_ASSERT(*prev != *cur, block_lock_exception, "${tag} lock \"${a}::${s}\" is not unique", ("tag",tag)("a",cur->account)("s",cur->scope));
      EOS_ASSERT(*prev < *cur,  block_lock_exception, "${tag} locks are not sorted", ("tag",tag));
   }
}

void chain_controller::__apply_block(const signed_block& next_block)
{ try {
   optional<fc::time_point> processing_deadline;
   if (!_currently_replaying_blocks && _limits.max_push_block_us.count() > 0) {
      processing_deadline = fc::time_point::now() + _limits.max_push_block_us;
   }

   uint32_t skip = _skip_flags;

   /*
   FC_ASSERT((skip & skip_merkle_check)
             || next_block.transaction_merkle_root == next_block.calculate_merkle_root(),
             "", ("next_block.transaction_merkle_root", next_block.transaction_merkle_root)
             ("calc",next_block.calculate_merkle_root())("next_block",next_block)("id",next_block.id()));
             */

   const producer_object& signing_producer = validate_block_header(skip, next_block);

   /// regions must be listed in order
   for( uint32_t i = 1; i < next_block.regions.size(); ++i )
      FC_ASSERT( next_block.regions[i-1].region < next_block.regions[i].region );

   block_trace next_block_trace(next_block);

   /// cache the input tranasction ids so that they can be looked up when executing the
   /// summary
   vector<transaction_metadata> input_metas;
   input_metas.reserve(next_block.input_transactions.size() + 1);
   {
      next_block_trace.implicit_transactions.emplace_back(_get_on_block_transaction());
      input_metas.emplace_back(packed_transaction(next_block_trace.implicit_transactions.back()), get_chain_id(), head_block_time());
   }
   map<transaction_id_type,size_t> trx_index;
   for( const auto& t : next_block.input_transactions ) {
      input_metas.emplace_back(t, chain_id_type(), next_block.timestamp);
      trx_index[input_metas.back().id] =  input_metas.size() - 1;
   }

   next_block_trace.region_traces.reserve(next_block.regions.size());

   for( uint32_t region_index = 0; region_index < next_block.regions.size(); ++region_index ) {
      const auto& r = next_block.regions[region_index];
      region_trace r_trace;
      r_trace.cycle_traces.reserve(r.cycles_summary.size());

      EOS_ASSERT(!r.cycles_summary.empty(), tx_empty_region,"region[${r_index}] has no cycles", ("r_index",region_index));
      for (uint32_t cycle_index = 0; cycle_index < r.cycles_summary.size(); cycle_index++) {
         const auto& cycle = r.cycles_summary.at(cycle_index);
         cycle_trace c_trace;
         c_trace.shard_traces.reserve(cycle.size());

         // validate that no read_scope is used as a write scope in this cycle and that no two shards
         // share write scopes
         set<shard_lock> read_locks;
         map<shard_lock, uint32_t> write_locks;

         EOS_ASSERT(!cycle.empty(), tx_empty_cycle,"region[${r_index}] cycle[${c_index}] has no shards", ("r_index",region_index)("c_index",cycle_index));
         for (uint32_t shard_index = 0; shard_index < cycle.size(); shard_index++) {
            const auto& shard = cycle.at(shard_index);
            EOS_ASSERT(!shard.empty(), tx_empty_shard,"region[${r_index}] cycle[${c_index}] shard[${s_index}] is empty",
                       ("r_index",region_index)("c_index",cycle_index)("s_index",shard_index));

            // Validate that the shards scopes are correct and available
            validate_shard_locks(shard.read_locks,  "read");
            validate_shard_locks(shard.write_locks, "write");

            for (const auto& s: shard.read_locks) {
               EOS_ASSERT(write_locks.count(s) == 0, block_concurrency_exception,
                  "shard ${i} requires read lock \"${a}::${s}\" which is locked for write by shard ${j}",
                  ("i", shard_index)("s", s)("j", write_locks[s]));
               read_locks.emplace(s);
            }

            for (const auto& s: shard.write_locks) {
               EOS_ASSERT(write_locks.count(s) == 0, block_concurrency_exception,
                  "shard ${i} requires write lock \"${a}::${s}\" which is locked for write by shard ${j}",
                  ("i", shard_index)("a", s.account)("s", s.scope)("j", write_locks[s]));
               EOS_ASSERT(read_locks.count(s) == 0, block_concurrency_exception,
                  "shard ${i} requires write lock \"${a}::${s}\" which is locked for read",
                  ("i", shard_index)("a", s.account)("s", s.scope));
               write_locks[s] = shard_index;
            }

            flat_set<shard_lock> used_read_locks;
            flat_set<shard_lock> used_write_locks;

            shard_trace s_trace;
            for (const auto& receipt : shard.transactions) {
                optional<transaction_metadata> _temp;
                auto make_metadata = [&]() -> transaction_metadata* {
                  auto itr = trx_index.find(receipt.id);
                  if( itr != trx_index.end() ) {
                     return &input_metas.at(itr->second);
                  } else {
                     const auto* gtrx = _db.find<generated_transaction_object,by_trx_id>(receipt.id);
                     if (gtrx != nullptr) {
                        auto trx = fc::raw::unpack<deferred_transaction>(gtrx->packed_trx.data(), gtrx->packed_trx.size());
                        FC_ASSERT( trx.execute_after <= head_block_time() , "deffered transaction executed prematurely" );
                        _temp.emplace(trx, gtrx->published, trx.sender, trx.sender_id, gtrx->packed_trx.data(), gtrx->packed_trx.size() );
                        _destroy_generated_transaction(*gtrx);
                        return &*_temp;
                     } else {
                        const auto& mtrx = input_metas[0];
                        FC_ASSERT(mtrx.id == receipt.id, "on-block transaction id mismatch");
                        return &input_metas[0];
                     }
                  }
               };

               auto *mtrx = make_metadata();
               mtrx->region_id = r.region;
               mtrx->cycle_index = cycle_index;
               mtrx->shard_index = shard_index;
               mtrx->allowed_read_locks.emplace(&shard.read_locks);
               mtrx->allowed_write_locks.emplace(&shard.write_locks);
               mtrx->processing_deadline = processing_deadline;

               s_trace.transaction_traces.emplace_back(_apply_transaction(*mtrx));
               record_locks_for_data_access(s_trace.transaction_traces.back(), used_read_locks, used_write_locks);

               EOS_ASSERT(receipt.status == s_trace.transaction_traces.back().status, tx_receipt_inconsistent_status,
                          "Received status of transaction from block (${rstatus}) does not match the applied transaction's status (${astatus})",
                          ("rstatus",receipt.status)("astatus",s_trace.transaction_traces.back().status));

               // validate_referenced_accounts(trx);
               // Check authorization, and allow irrelevant signatures.
               // If the block producer let it slide, we'll roll with it.
               // check_transaction_authorization(trx, true);
            } /// for each transaction id

            EOS_ASSERT( boost::equal( used_read_locks, shard.read_locks ),
               block_lock_exception, "Read locks for executing shard: ${s} do not match those listed in the block", ("s", shard_index));
            EOS_ASSERT( boost::equal( used_write_locks, shard.write_locks ),
               block_lock_exception, "Write locks for executing shard: ${s} do not match those listed in the block", ("s", shard_index));

            s_trace.finalize_shard();
            c_trace.shard_traces.emplace_back(move(s_trace));
         } /// for each shard

         _apply_cycle_trace(c_trace);
         r_trace.cycle_traces.emplace_back(move(c_trace));
      } /// for each cycle

      next_block_trace.region_traces.emplace_back(move(r_trace));
   } /// for each region

   FC_ASSERT(next_block.action_mroot == next_block_trace.calculate_action_merkle_root());
   FC_ASSERT( transaction_metadata::calculate_transaction_merkle_root(input_metas) == next_block.transaction_mroot, "merkle root does not match" );

   _finalize_block( next_block_trace );
} FC_CAPTURE_AND_RETHROW( (next_block.block_num()) )  }

flat_set<public_key_type> chain_controller::get_required_keys(const transaction& trx,
                                                              const flat_set<public_key_type>& candidate_keys)const
{
   auto checker = make_auth_checker( [&](const permission_level& p){ return get_permission(p).auth; },
                                     [](const permission_level& ) {},
                                     get_global_properties().configuration.max_authority_depth,
                                     candidate_keys);

   for (const auto& act : trx.actions ) {
      for (const auto& declared_auth : act.authorization) {
         if (!checker.satisfied(declared_auth)) {
            EOS_ASSERT(checker.satisfied(declared_auth), tx_missing_sigs,
                       "transaction declares authority '${auth}', but does not have signatures for it.",
                       ("auth", declared_auth));
         }
      }
   }

   return checker.used_keys();
}


class permission_visitor {
public:
   permission_visitor(const chain_controller& controller) : _chain_controller(controller) {}

   void operator()(const permission_level& perm_level) {
      const auto obj = _chain_controller.get_permission(perm_level);
      if (_max_delay < obj.delay)
         _max_delay = obj.delay;
   }

   const time_point& get_max_delay() const { return _max_delay; }
private:
   const chain_controller& _chain_controller;
   time_point _max_delay;
};

time_point chain_controller::check_authorization( const vector<action>& actions,
                                                  const vector<action>& context_free_actions,
                                                  const flat_set<public_key_type>& provided_keys,
                                                  bool allow_unused_signatures,
                                                  flat_set<account_name> provided_accounts )const

{
   auto checker = make_auth_checker( [&](const permission_level& p){ return get_permission(p).auth; },
                                     permission_visitor(*this),
                                     get_global_properties().configuration.max_authority_depth,
                                     provided_keys, provided_accounts );

   time_point max_delay;

   for( const auto& act : actions ) {
      for( const auto& declared_auth : act.authorization ) {

         // check a minimum permission if one is set, otherwise assume the contract code will validate
         auto min_permission_name = lookup_minimum_permission(declared_auth.actor, act.account, act.name);
         bool min_permission_required = true;
         if (!min_permission_name) {
            // for updateauth actions, need to determine the permission that is changing
            if (act.account == config::system_account_name && act.name == contracts::updateauth::get_name()) {
               auto update = act.data_as<contracts::updateauth>();
               const auto permission_to_change = _db.find<permission_object, by_owner>(boost::make_tuple(update.account, update.permission));
               if (permission_to_change != nullptr) {
                  // only determining delay
                  min_permission_required = false;
                  min_permission_name = update.permission;
               }
            }
         }
         if (min_permission_name) {
            const auto& min_permission = _db.get<permission_object, by_owner>(boost::make_tuple(declared_auth.actor, *min_permission_name));

            if ((_skip_flags & skip_authority_check) == false) {
               const auto& index = _db.get_index<permission_index>().indices();
               const optional<time_point> delay = get_permission(declared_auth).satisfies(min_permission, index);
               EOS_ASSERT(!min_permission_required || delay.valid(),
                          tx_irrelevant_auth,
                          "action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                          ("auth", declared_auth)("min", min_permission.name));
               if (max_delay < *delay)
                  max_delay = *delay;
            }
         }

         if (act.account == config::system_account_name) {
            // for link changes, we need to also determine the delay associated with an existing link that is being
            // moved or removed
            if (act.name == contracts::linkauth::get_name()) {
               auto link = act.data_as<contracts::linkauth>();
               if (declared_auth.actor == link.account) {
                  const auto linked_permission_name = lookup_linked_permission(link.account, link.code, link.type);
                  if( linked_permission_name.valid() ) {
                     const auto& linked_permission = _db.get<permission_object, by_owner>(boost::make_tuple(link.account, *linked_permission_name));
                     const auto& index = _db.get_index<permission_index>().indices();
                     const optional<time_point> delay = get_permission(declared_auth).satisfies(linked_permission, index);
                     if (delay.valid() && max_delay < *delay)
                        max_delay = *delay;
                  } // else it is only a new link, so don't need to delay
               }
            } else if (act.name == contracts::unlinkauth::get_name()) {
               auto unlink = act.data_as<contracts::unlinkauth>();
               if (declared_auth.actor == unlink.account) {
                  const auto unlinked_permission_name = lookup_linked_permission(unlink.account, unlink.code, unlink.type);
                  if (unlinked_permission_name.valid()) {
                     const auto& unlinked_permission = _db.get<permission_object, by_owner>(boost::make_tuple(unlink.account, *unlinked_permission_name));
                     const auto& index = _db.get_index<permission_index>().indices();
                     const optional<time_point> delay = get_permission(declared_auth).satisfies(unlinked_permission, index);
                     if (delay.valid() && max_delay < *delay)
                        max_delay = *delay;
                  }
               }
            }
         }

         if ((_skip_flags & skip_transaction_signatures) == false) {
            EOS_ASSERT(checker.satisfied(declared_auth), tx_missing_sigs,
                       "transaction declares authority '${auth}', but does not have signatures for it.",
                       ("auth", declared_auth));
         }
      }
   }

   for( const auto& act : context_free_actions ) {
      if (act.account == config::system_account_name && act.name == contracts::mindelay::get_name()) {
         const auto mindelay = act.data_as<contracts::mindelay>();
         const time_point delay = time_point_sec{mindelay.delay.convert_to<uint32_t>()};
         if (max_delay < delay)
            max_delay = delay;
      }
   }

   if (!allow_unused_signatures && (_skip_flags & skip_transaction_signatures) == false)
      EOS_ASSERT(checker.all_keys_used(), tx_irrelevant_sig,
                 "transaction bears irrelevant signatures from these keys: ${keys}",
                 ("keys", checker.unused_keys()));

   const auto checker_max_delay = checker.get_permission_visitor().get_max_delay();
   if (max_delay < checker_max_delay)
      max_delay = checker_max_delay;

   return max_delay;
}

bool chain_controller::check_authorization( account_name account, permission_name permission,
                                         flat_set<public_key_type> provided_keys,
                                         bool allow_unused_signatures)const
{
   auto checker = make_auth_checker( [&](const permission_level& p){ return get_permission(p).auth; },
                                  [](const permission_level& ) {},
                                  get_global_properties().configuration.max_authority_depth,
                                  provided_keys);

   auto satisfied = checker.satisfied({account, permission});

   if (satisfied && !allow_unused_signatures) {
      EOS_ASSERT(checker.all_keys_used(), tx_irrelevant_sig,
                 "irrelevant signatures from these keys: ${keys}",
                 ("keys", checker.unused_keys()));
   }

   return satisfied;
}

time_point chain_controller::check_transaction_authorization(const transaction& trx,
                                                             const vector<signature_type>& signatures,
                                                             const vector<bytes>& cfd,
                                                             bool allow_unused_signatures)const
{
   return check_authorization( trx.actions, trx.context_free_actions, trx.get_signature_keys( signatures, chain_id_type{}, cfd ), allow_unused_signatures );
}

optional<permission_name> chain_controller::lookup_minimum_permission(account_name authorizer_account,
                                                                    account_name scope,
                                                                    action_name act_name) const {
#warning TODO: this comment sounds like it is expecting a check ("may") somewhere else, but I have not found anything else
   // updateauth is a special case where any permission _may_ be suitable depending
   // on the contents of the action
   if (scope == config::system_account_name && act_name == contracts::updateauth::get_name()) {
      return optional<permission_name>();
   }

   try {
      optional<permission_name> linked_permission = lookup_linked_permission(authorizer_account, scope, act_name);
      if (!linked_permission )
         return config::active_name;

      if( *linked_permission == N(eosio.any) ) 
         return optional<permission_name>();

      return linked_permission;
   } FC_CAPTURE_AND_RETHROW((authorizer_account)(scope)(act_name))
}

optional<permission_name> chain_controller::lookup_linked_permission(account_name authorizer_account,
                                                                     account_name scope,
                                                                     action_name act_name) const {
   try {
      // First look up a specific link for this message act_name
      auto key = boost::make_tuple(authorizer_account, scope, act_name);
      auto link = _db.find<permission_link_object, by_action_name>(key);
      // If no specific link found, check for a contract-wide default
      if (link == nullptr) {
         get<2>(key) = "";
         link = _db.find<permission_link_object, by_action_name>(key);
      }

      // If no specific or default link found, use active permission
      if (link != nullptr) {
         return link->required_permission;
      } 
      return optional<permission_name>(); 

    //  return optional<permission_name>();
   } FC_CAPTURE_AND_RETHROW((authorizer_account)(scope)(act_name))
}

void chain_controller::validate_uniqueness( const transaction& trx )const {
   if( !should_check_for_duplicate_transactions() ) return;

   auto transaction = _db.find<transaction_object, by_trx_id>(trx.id());
   EOS_ASSERT(transaction == nullptr, tx_duplicate, "Transaction is not unique");
}

void chain_controller::record_transaction(const transaction& trx) 
{ 
   try {
       _db.create<transaction_object>([&](transaction_object& transaction) {
           transaction.trx_id = trx.id();
           transaction.expiration = trx.expiration;
       });
   } catch ( ... ) {
       EOS_ASSERT( false, transaction_exception,
                  "duplicate transaction ${id}", 
                  ("id", trx.id() ) );
   }
} 

static uint32_t calculate_transaction_cpu_usage( const transaction_trace& trace, const transaction_metadata& meta, const chain_config& chain_configuration ) {
   // calculate the sum of all actions retired
   uint32_t action_cpu_usage = 0;
   uint32_t context_free_actual_cpu_usage = 0;
   for (const auto &at: trace.action_traces) {
      if (at.context_free) {
         context_free_actual_cpu_usage += chain_configuration.base_per_action_cpu_usage + at.cpu_usage;
      } else {
         action_cpu_usage += chain_configuration.base_per_action_cpu_usage + at.cpu_usage;
         if (at.receiver == config::system_account_name &&
             at.act.account == config::system_account_name &&
             at.act.name == N(setcode)) {
            action_cpu_usage += chain_configuration.base_setcode_cpu_usage;
         }
      }
   }


   // charge a system controlled amount for signature verification/recovery
   uint32_t signature_cpu_usage = 0;
   if( meta.signing_keys ) {
      signature_cpu_usage = (uint32_t)meta.signing_keys->size() * chain_configuration.per_signature_cpu_usage;
   }

   // charge a system discounted amount for context free cpu usage
   //uint32_t context_free_cpu_commitment = (uint32_t)(meta.trx().kcpu_usage.value * 1024UL);
   /*
   EOS_ASSERT(context_free_actual_cpu_usage <= context_free_cpu_commitment,
              tx_resource_exhausted,
              "Transaction context free actions can not fit into the cpu usage committed to by the transaction's header! [usage=${usage},commitment=${commit}]",
              ("usage", context_free_actual_cpu_usage)("commit", context_free_cpu_commitment) );
              */

   uint32_t context_free_cpu_usage = (uint32_t)((uint64_t)context_free_actual_cpu_usage * chain_configuration.context_free_discount_cpu_usage_num / chain_configuration.context_free_discount_cpu_usage_den);

   auto actual_usage = chain_configuration.base_per_transaction_cpu_usage +
          action_cpu_usage +
          context_free_cpu_usage +
          signature_cpu_usage;

   

   if( meta.trx().kcpu_usage.value == 0 ) {
      return actual_usage;
   } else {
      EOS_ASSERT( actual_usage <= meta.trx().kcpu_usage.value*1000ll, tx_resource_exhausted, "transaction did not declare sufficient cpu usage: ${actual} > ${declared}", ("actual", actual_usage)("declared",meta.trx().kcpu_usage.value*1000ll) );
   }
   return meta.trx().kcpu_usage;
}

static uint32_t calculate_transaction_net_usage( const transaction_trace& trace, const transaction_metadata& meta, const chain_config& chain_configuration ) {
   // charge a system controlled per-lock overhead to account for shard bloat
   uint32_t lock_net_usage = uint32_t(trace.read_locks.size() + trace.write_locks.size()) * chain_configuration.per_lock_net_usage;
   uint32_t trx_wire_net_usage = meta.trx().net_usage_words.value * 8U;

   return chain_configuration.base_per_transaction_net_usage +
          trx_wire_net_usage +
          lock_net_usage;
}

void chain_controller::update_resource_usage( transaction_trace& trace, const transaction_metadata& meta ) {
   const auto& chain_configuration = get_global_properties().configuration;

   trace.cpu_usage = calculate_transaction_cpu_usage(trace, meta, chain_configuration);
   trace.net_usage = calculate_transaction_net_usage(trace, meta, chain_configuration);

   // enforce that the system controlled per tx limits are not violated
   EOS_ASSERT(trace.cpu_usage <= chain_configuration.max_transaction_cpu_usage,
              tx_resource_exhausted, "Transaction exceeds the maximum cpu usage [used: ${used}, max: ${max}]",
              ("used", trace.cpu_usage)("max", chain_configuration.max_transaction_cpu_usage));

   EOS_ASSERT(trace.net_usage <= chain_configuration.max_transaction_net_usage,
              tx_resource_exhausted, "Transaction exceeds the maximum net usage [used: ${used}, max: ${max}]",
              ("used", trace.net_usage)("max", chain_configuration.max_transaction_net_usage));

   // determine the accounts to bill
   set<std::pair<account_name, permission_name>> authorizations;
   for( const auto& act : meta.trx().actions )
      for( const auto& auth : act.authorization )
         authorizations.emplace( auth.actor, auth.permission );


   vector<account_name> bill_to_accounts;
   bill_to_accounts.reserve(authorizations.size());
   for( const auto& ap : authorizations ) {
      bill_to_accounts.push_back(ap.first);
   }

   // for account usage, the ordinal is based on possible blocks not actual blocks.  This means that as blocks are
   // skipped account usage will still decay.
   uint32_t ordinal = (uint32_t)(head_block_time().time_since_epoch().count() / fc::milliseconds(config::block_interval_ms).count());
   _resource_limits.add_transaction_usage(bill_to_accounts, trace.cpu_usage, trace.net_usage, ordinal);
}


void chain_controller::validate_tapos(const transaction& trx)const {
   if (!should_check_tapos()) return;

   const auto& tapos_block_summary = _db.get<block_summary_object>((uint16_t)trx.ref_block_num);

   //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
   EOS_ASSERT(trx.verify_reference_block(tapos_block_summary.block_id), invalid_ref_block_exception,
              "Transaction's reference block did not match. Is this transaction from a different fork?",
              ("tapos_summary", tapos_block_summary));
}

void chain_controller::validate_referenced_accounts( const transaction& trx )const
{ try {
   for( const auto& act : trx.actions ) {
      require_account(act.account);
      for (const auto& auth : act.authorization )
         require_account(auth.actor);
   }
} FC_CAPTURE_AND_RETHROW() }

void chain_controller::validate_expiration( const transaction& trx ) const
{ try {
   fc::time_point now = head_block_time();
   const auto& chain_configuration = get_global_properties().configuration;

   EOS_ASSERT( time_point(trx.expiration) <= now + fc::seconds(chain_configuration.max_transaction_lifetime),
               tx_exp_too_far_exception, "Transaction expiration is too far in the future, expiration is ${trx.expiration} but max expiration is ${max_til_exp}",
              ("trx.expiration",trx.expiration)("now",now)
              ("max_til_exp",chain_configuration.max_transaction_lifetime));
   EOS_ASSERT( now <= time_point(trx.expiration), expired_tx_exception, "Transaction is expired, now is ${now}, expiration is ${trx.exp}",
              ("now",now)("trx.exp",trx.expiration));
} FC_CAPTURE_AND_RETHROW((trx)) }


void chain_controller::require_scope( const scope_name& scope )const {
   switch( uint64_t(scope) ) {
      case config::eosio_all_scope:
      case config::eosio_auth_scope:
         return; /// built in scopes
      default:
         require_account(scope);
   }
}

void chain_controller::require_account(const account_name& name) const {
   auto account = _db.find<account_object, by_name>(name);
   FC_ASSERT(account != nullptr, "Account not found: ${name}", ("name", name));
}

const producer_object& chain_controller::validate_block_header(uint32_t skip, const signed_block& next_block)const {
   EOS_ASSERT(head_block_id() == next_block.previous, block_validate_exception, "",
              ("head_block_id",head_block_id())("next.prev",next_block.previous));
   EOS_ASSERT(head_block_time() < (fc::time_point)next_block.timestamp, block_validate_exception, "",
              ("head_block_time",head_block_time())("next",next_block.timestamp)("blocknum",next_block.block_num()));
   if (((fc::time_point)next_block.timestamp) > head_block_time() + fc::microseconds(config::block_interval_ms*1000)) {
      elog("head_block_time ${h}, next_block ${t}, block_interval ${bi}",
           ("h", head_block_time())("t", next_block.timestamp)("bi", config::block_interval_ms));
      elog("Did not produce block within block_interval ${bi}ms, took ${t}ms)",
           ("bi", config::block_interval_ms)("t", (time_point(next_block.timestamp) - head_block_time()).count() / 1000));
   }


   if( !is_start_of_round( next_block.block_num() ) )  {
      EOS_ASSERT(!next_block.new_producers, block_validate_exception,
                 "Producer changes may only occur at the end of a round.");
   }

   const producer_object& producer = get_producer(get_scheduled_producer(get_slot_at_time(next_block.timestamp)));

   if(!(skip&skip_producer_signature))
      EOS_ASSERT(next_block.validate_signee(producer.signing_key), block_validate_exception,
                 "Incorrect block producer key: expected ${e} but got ${a}",
                 ("e", producer.signing_key)("a", public_key_type(next_block.signee())));

   if(!(skip&skip_producer_schedule_check)) {
      EOS_ASSERT(next_block.producer == producer.owner, block_validate_exception,
                 "Producer produced block at wrong time",
                 ("block producer",next_block.producer)("scheduled producer",producer.owner));
   }

   EOS_ASSERT( next_block.schedule_version == get_global_properties().active_producers.version, block_validate_exception, "wrong producer schedule version specified" );

   return producer;
}

void chain_controller::create_block_summary(const signed_block& next_block) {
   auto sid = next_block.block_num() & 0xffff;
   _db.modify( _db.get<block_summary_object,by_id>(sid), [&](block_summary_object& p) {
         p.block_id = next_block.id();
   });
}

/**
 *  Takes the top config::producer_count producers by total vote excluding any producer whose
 *  block_signing_key is null.
 */
producer_schedule_type chain_controller::_calculate_producer_schedule()const {
   producer_schedule_type schedule = get_global_properties().new_active_producers;

   const auto& hps = _head_producer_schedule();
   schedule.version = hps.version;
   if( hps != schedule )
      ++schedule.version;
   return schedule;
}

/**
 *  Returns the most recent and/or pending producer schedule
 */
const shared_producer_schedule_type& chain_controller::_head_producer_schedule()const {
   const auto& gpo = get_global_properties();
   if( gpo.pending_active_producers.size() )
      return gpo.pending_active_producers.back().second;
   return gpo.active_producers;
}

void chain_controller::update_global_properties(const signed_block& b) { try {
   // If we're at the end of a round, update the BlockchainConfiguration, producer schedule
   // and "producers" special account authority
   if( is_start_of_round( b.block_num() ) ) {
      auto schedule = _calculate_producer_schedule();
      if( b.new_producers )
      {
          FC_ASSERT( schedule == *b.new_producers, "pending producer set different than expected" );
      }
      const auto& gpo = get_global_properties();

      if( _head_producer_schedule() != schedule ) {
         FC_ASSERT( b.new_producers, "pending producer set changed but block didn't indicate it" );
      }
      _db.modify( gpo, [&]( auto& props ) {
         if( props.pending_active_producers.size() && props.pending_active_producers.back().first == b.block_num() )
            props.pending_active_producers.back().second = schedule;
         else
         {
            props.pending_active_producers.emplace_back( props.pending_active_producers.get_allocator() );// props.pending_active_producers.size()+1, props.pending_active_producers.get_allocator() );
            auto& back = props.pending_active_producers.back();
            back.first = b.block_num();
            back.second = schedule;

         }
      });


      auto active_producers_authority = authority(config::producers_authority_threshold, {}, {});
      for(auto& name : gpo.active_producers.producers ) {
         active_producers_authority.accounts.push_back({{name.producer_name, config::active_name}, 1});
      }

      auto& po = _db.get<permission_object, by_owner>( boost::make_tuple(config::producers_account_name,
                                                                         config::active_name ) );
      _db.modify(po,[active_producers_authority] (permission_object& po) {
         po.auth = active_producers_authority;
      });
   }
} FC_CAPTURE_AND_RETHROW() }

void chain_controller::add_checkpoints( const flat_map<uint32_t,block_id_type>& checkpts ) {
   for (const auto& i : checkpts)
      _checkpoints[i.first] = i.second;
}

bool chain_controller::before_last_checkpoint()const {
   return (_checkpoints.size() > 0) && (_checkpoints.rbegin()->first >= head_block_num());
}

const global_property_object& chain_controller::get_global_properties()const {
   return _db.get<global_property_object>();
}

const dynamic_global_property_object&chain_controller::get_dynamic_global_properties() const {
   return _db.get<dynamic_global_property_object>();
}

time_point chain_controller::head_block_time()const {
   return get_dynamic_global_properties().time;
}

uint32_t chain_controller::head_block_num()const {
   return get_dynamic_global_properties().head_block_number;
}

block_id_type chain_controller::head_block_id()const {
   return get_dynamic_global_properties().head_block_id;
}

account_name chain_controller::head_block_producer() const {
   auto b = _fork_db.fetch_block(head_block_id());
   if( b ) return b->data.producer;

   if (auto head_block = fetch_block_by_id(head_block_id()))
      return head_block->producer;
   return {};
}

const producer_object& chain_controller::get_producer(const account_name& owner_name) const
{ try {
   return _db.get<producer_object, by_owner>(owner_name);
} FC_CAPTURE_AND_RETHROW( (owner_name) ) }

const permission_object&   chain_controller::get_permission( const permission_level& level )const
{ try {
   return _db.get<permission_object, by_owner>( boost::make_tuple(level.actor,level.permission) );
} EOS_RETHROW_EXCEPTIONS( chain::permission_query_exception, "Fail to retrieve permission: ${level}", ("level", level) ) }

uint32_t chain_controller::last_irreversible_block_num() const {
   return get_dynamic_global_properties().last_irreversible_block_num;
}

void chain_controller::_initialize_indexes() {
   _db.add_index<account_index>();
   _db.add_index<permission_index>();
   _db.add_index<permission_usage_index>();
   _db.add_index<permission_link_index>();
   _db.add_index<action_permission_index>();



   _db.add_index<contracts::table_id_multi_index>();
   _db.add_index<contracts::key_value_index>();
   _db.add_index<contracts::index64_index>();
   _db.add_index<contracts::index128_index>();
   _db.add_index<contracts::index256_index>();
   _db.add_index<contracts::index_double_index>();

   _db.add_index<global_property_multi_index>();
   _db.add_index<dynamic_global_property_multi_index>();
   _db.add_index<block_summary_multi_index>();
   _db.add_index<transaction_multi_index>();
   _db.add_index<generated_transaction_multi_index>();
   _db.add_index<producer_multi_index>();
   _db.add_index<scope_sequence_multi_index>();
}

void chain_controller::_initialize_chain(contracts::chain_initializer& starter)
{ try {
   if (!_db.find<global_property_object>()) {
      _db.with_write_lock([this, &starter] {
         auto initial_timestamp = starter.get_chain_start_time();
         FC_ASSERT(initial_timestamp != time_point(), "Must initialize genesis timestamp." );
         FC_ASSERT( block_timestamp_type(initial_timestamp) == initial_timestamp,
                    "Genesis timestamp must be divisible by config::block_interval_ms" );

         // Create global properties
         const auto& gp = _db.create<global_property_object>([&starter](global_property_object& p) {
            p.configuration = starter.get_chain_start_configuration();
            p.active_producers = starter.get_chain_start_producers();
            p.new_active_producers = starter.get_chain_start_producers();
         });

         _db.create<dynamic_global_property_object>([&](dynamic_global_property_object& p) {
            p.time = initial_timestamp;
            //p.recent_slots_filled = uint64_t(-1);
         });

         _resource_limits.initialize_chain();

         // Initialize block summary index
         for (int i = 0; i < 0x10000; i++)
            _db.create<block_summary_object>([&](block_summary_object&) {});

         starter.prepare_database(*this, _db);
      });
   }
} FC_CAPTURE_AND_RETHROW() }


void chain_controller::replay() {
   ilog("Replaying blockchain");
   auto start = fc::time_point::now();

   auto on_exit = fc::make_scoped_exit([&_currently_replaying_blocks = _currently_replaying_blocks](){
      _currently_replaying_blocks = false;
   });
   _currently_replaying_blocks = true;

   auto last_block = _block_log.read_head();
   if (!last_block) {
      elog("No blocks in block log; skipping replay");
      return;
   }

   const auto last_block_num = last_block->block_num();

   ilog("Replaying ${n} blocks...", ("n", last_block_num) );
   for (uint32_t i = 1; i <= last_block_num; ++i) {
      if (i % 5000 == 0)
         std::cerr << "   " << double(i*100)/last_block_num << "%   "<<i << " of " <<last_block_num<<"   \n";
      fc::optional<signed_block> block = _block_log.read_block_by_num(i);
      FC_ASSERT(block, "Could not find block #${n} in block_log!", ("n", i));
      _apply_block(*block, skip_producer_signature |
                          skip_transaction_signatures |
                          skip_transaction_dupe_check |
                          skip_tapos_check |
                          skip_producer_schedule_check |
                          skip_authority_check |
                          received_block);
   }
   auto end = fc::time_point::now();
   ilog("Done replaying ${n} blocks, elapsed time: ${t} sec",
        ("n", head_block_num())("t",double((end-start).count())/1000000.0));

   _db.set_revision(head_block_num());
}

void chain_controller::_spinup_db() {
   // Rewind the database to the last irreversible block
   _db.with_write_lock([&] {
      _db.undo_all();
      FC_ASSERT(_db.revision() == head_block_num(), "Chainbase revision does not match head block num",
                ("rev", _db.revision())("head_block", head_block_num()));

   });
}

void chain_controller::_spinup_fork_db()
{
   fc::optional<signed_block> last_block = _block_log.read_head();
   if(last_block.valid()) {
      _fork_db.start_block(*last_block);
      if (last_block->id() != head_block_id()) {
           FC_ASSERT(head_block_num() == 0, "last block ID does not match current chain state",
                     ("last_block->id", last_block->id())("head_block_num",head_block_num()));
      }
   }
}

/*
ProducerRound chain_controller::calculate_next_round(const signed_block& next_block) {
   auto schedule = _admin->get_next_round(_db);
   auto changes = get_global_properties().active_producers - schedule;
   EOS_ASSERT(boost::range::equal(next_block.producer_changes, changes), block_validate_exception,
              "Unexpected round changes in new block header",
              ("expected changes", changes)("block changes", next_block.producer_changes));

   fc::time_point tp = (fc::time_point)next_block.timestamp;
   utilities::rand::random rng(tp.sec_since_epoch());
   rng.shuffle(schedule);
   return schedule;
}*/

void chain_controller::update_global_dynamic_data(const signed_block& b) {
   const dynamic_global_property_object& _dgp = _db.get<dynamic_global_property_object>();

   const auto& bmroot = _dgp.block_merkle_root.get_root();
   FC_ASSERT( bmroot == b.block_mroot, "block merkle root does not match expected value" );

   uint32_t missed_blocks = head_block_num() == 0? 1 : get_slot_at_time((fc::time_point)b.timestamp);
   assert(missed_blocks != 0);
   missed_blocks--;

//   if (missed_blocks)
//      wlog("Blockchain continuing after gap of ${b} missed blocks", ("b", missed_blocks));

   if (!(_skip_flags & skip_missed_block_penalty)) {
      for (uint32_t i = 0; i < missed_blocks; ++i) {
         const auto &producer_missed = get_producer(get_scheduled_producer(i + 1));
         if (producer_missed.owner != b.producer) {
            /*
            const auto& producer_account = producer_missed.producer_account(*this);
            if( (fc::time_point::now() - b.timestamp) < fc::seconds(30) )
               wlog( "Producer ${name} missed block ${n} around ${t}", ("name",producer_account.name)("n",b.block_num())("t",b.timestamp) );
               */

            _db.modify(producer_missed, [&](producer_object &w) {
               w.total_missed++;
            });
         }
      }
   }

   const auto& props = get_global_properties();

   // dynamic global properties updating
   _db.modify( _dgp, [&]( dynamic_global_property_object& dgp ){
      dgp.head_block_number = b.block_num();
      dgp.head_block_id = b.id();
      dgp.time = b.timestamp;
      dgp.current_producer = b.producer;
      dgp.current_absolute_slot += missed_blocks+1;

      /*
      // If we've missed more blocks than the bitmap stores, skip calculations and simply reset the bitmap
      if (missed_blocks < sizeof(dgp.recent_slots_filled) * 8) {
         dgp.recent_slots_filled <<= 1;
         dgp.recent_slots_filled += 1;
         dgp.recent_slots_filled <<= missed_blocks;
      } else
         if(config::percent_100 * get_global_properties().active_producers.producers.size() / blocks_per_round() > config::required_producer_participation)
            dgp.recent_slots_filled = uint64_t(-1);
         else
            dgp.recent_slots_filled = 0;
      */
      dgp.block_merkle_root.append( head_block_id() );
   });

   _fork_db.set_max_size( _dgp.head_block_number - _dgp.last_irreversible_block_num + 1 );
}

void chain_controller::update_signing_producer(const producer_object& signing_producer, const signed_block& new_block)
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t new_block_aslot = dpo.current_absolute_slot + get_slot_at_time( (fc::time_point)new_block.timestamp );

   _db.modify( signing_producer, [&]( producer_object& _wit )
   {
      _wit.last_aslot = new_block_aslot;
      _wit.last_confirmed_block_num = new_block.block_num();
   } );
}

void chain_controller::update_permission_usage( const transaction_metadata& meta ) {
   // for any transaction not sent by code, update the affirmative last time a given permission was used
   if (!meta.sender) {
      for( const auto& act : meta.trx().actions ) {
         for( const auto& auth : act.authorization ) {
            const auto *puo = _db.find<permission_usage_object, by_account_permission>(boost::make_tuple(auth.actor, auth.permission));
            if (puo) {
               _db.modify(*puo, [this](permission_usage_object &pu) {
                  pu.last_used = head_block_time();
               });
            } else {
               _db.create<permission_usage_object>([this, &auth](permission_usage_object &pu){
                  pu.account = auth.actor;
                  pu.permission = auth.permission;
                  pu.last_used = head_block_time();
               });
            }
         }
      }
   }
}

void chain_controller::update_or_create_producers( const producer_schedule_type& producers ) {
   for ( auto prod : producers.producers ) {
      if ( _db.find<producer_object, by_owner>(prod.producer_name) == nullptr ) {
         _db.create<producer_object>( [&]( auto& pro ) {
            pro.owner       = prod.producer_name;
            pro.signing_key = prod.block_signing_key;
         });
      }
   }
}

void chain_controller::update_last_irreversible_block()
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   vector<const producer_object*> producer_objs;
   producer_objs.reserve(gpo.active_producers.producers.size());

   std::transform(gpo.active_producers.producers.begin(),
                  gpo.active_producers.producers.end(), std::back_inserter(producer_objs),
                  [this](const producer_key& pk) { return &get_producer(pk.producer_name); });

   static_assert(config::irreversible_threshold_percent > 0, "irreversible threshold must be nonzero");

   size_t offset = EOS_PERCENT(producer_objs.size(), config::percent_100- config::irreversible_threshold_percent);
   std::nth_element(producer_objs.begin(), producer_objs.begin() + offset, producer_objs.end(),
      [](const producer_object* a, const producer_object* b) {
         return a->last_confirmed_block_num < b->last_confirmed_block_num;
      });

   uint32_t new_last_irreversible_block_num = producer_objs[offset]->last_confirmed_block_num;
   // TODO: right now the code cannot handle the head block being irreversible for reasons that are purely
   // implementation details.  We can and should remove this special case once the rest of the logic is fixed.
   if (producer_objs.size() == 1) {
      new_last_irreversible_block_num -= 1;
   }


   if (new_last_irreversible_block_num > dpo.last_irreversible_block_num) {
      _db.modify(dpo, [&](dynamic_global_property_object& _dpo) {
         _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
      });
   }

   // Write newly irreversible blocks to disk. First, get the number of the last block on disk...
   auto old_last_irreversible_block = _block_log.head();
   unsigned last_block_on_disk = 0;
   // If this is null, there are no blocks on disk, so the zero is correct
   if (old_last_irreversible_block)
      last_block_on_disk = old_last_irreversible_block->block_num();

   if (last_block_on_disk < new_last_irreversible_block_num) {
      for (auto block_to_write = last_block_on_disk + 1;
           block_to_write <= new_last_irreversible_block_num;
           ++block_to_write) {
         auto block = fetch_block_by_number(block_to_write);
         FC_ASSERT( block, "unable to find last irreversible block to write" );
         _block_log.append(*block);
         applied_irreversible_block(*block);
      }
   }

   if( new_last_irreversible_block_num > last_block_on_disk ) {
      /// TODO: use upper / lower bound to find
      optional<producer_schedule_type> new_producer_schedule;
      for( const auto& item : gpo.pending_active_producers ) {
         if( item.first < new_last_irreversible_block_num ) {
            new_producer_schedule = item.second;
         }
      }
      if( new_producer_schedule ) {
         update_or_create_producers( *new_producer_schedule );
         _db.modify( gpo, [&]( auto& props ){
              boost::range::remove_erase_if(props.pending_active_producers,
                                            [new_last_irreversible_block_num](const typename decltype(props.pending_active_producers)::value_type& v) -> bool {
                                               return v.first < new_last_irreversible_block_num;
                                            });
              props.active_producers = *new_producer_schedule;
         });
      }
   }

   // Trim fork_database and undo histories
   _fork_db.set_max_size(head_block_num() - new_last_irreversible_block_num + 1);
   _db.commit(new_last_irreversible_block_num);
}

void chain_controller::clear_expired_transactions()
{ try {
   //Look for expired transactions in the deduplication list, and remove them.
   //transactions must have expired by at least two forking windows in order to be removed.
   auto& transaction_idx = _db.get_mutable_index<transaction_multi_index>();
   const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
   while( (!dedupe_index.empty()) && (head_block_time() > fc::time_point(dedupe_index.rbegin()->expiration) ) )
      transaction_idx.remove(*dedupe_index.rbegin());

   //Look for expired transactions in the pending generated list, and remove them.
   //transactions must have expired by at least two forking windows in order to be removed.
   auto& generated_transaction_idx = _db.get_mutable_index<generated_transaction_multi_index>();
   const auto& generated_index = generated_transaction_idx.indices().get<by_expiration>();
   while( (!generated_index.empty()) && (head_block_time() > generated_index.rbegin()->expiration) ) {
      _destroy_generated_transaction(*generated_index.rbegin());
   }

} FC_CAPTURE_AND_RETHROW() }

using boost::container::flat_set;

account_name chain_controller::get_scheduled_producer(uint32_t slot_num)const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t current_aslot = dpo.current_absolute_slot + slot_num;
   const auto& gpo = _db.get<global_property_object>();
   auto number_of_active_producers = gpo.active_producers.producers.size();
   auto index = current_aslot % (number_of_active_producers * config::producer_repetitions);
   index /= config::producer_repetitions;
   FC_ASSERT( gpo.active_producers.producers.size() > 0, "no producers defined" );

   return gpo.active_producers.producers[index].producer_name;
}

block_timestamp_type chain_controller::get_slot_time(uint32_t slot_num)const
{
   if( slot_num == 0)
      return block_timestamp_type();

   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   if( head_block_num() == 0 )
   {
      // n.b. first block is at genesis_time plus one block interval
      auto genesis_time = block_timestamp_type(dpo.time);
      genesis_time.slot += slot_num;
      return (fc::time_point)genesis_time;
   }

   auto head_block_abs_slot = block_timestamp_type(head_block_time());
   head_block_abs_slot.slot += slot_num;
   return head_block_abs_slot;
}

uint32_t chain_controller::get_slot_at_time( block_timestamp_type when )const
{
   auto first_slot_time = get_slot_time(1);
   if( when < first_slot_time )
      return 0;
   return block_timestamp_type(when).slot - first_slot_time.slot + 1;
}

uint32_t chain_controller::producer_participation_rate()const
{
   //const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   //return uint64_t(config::percent_100) * __builtin_popcountll(dpo.recent_slots_filled) / 64;
   return static_cast<uint32_t>(config::percent_100); // Ignore participation rate for now until we construct a better metric.
}

void chain_controller::_set_apply_handler( account_name contract, scope_name scope, action_name action, apply_handler v ) {
   _apply_handlers[contract][make_pair(scope,action)] = v;
}

static void log_handled_exceptions(const transaction& trx) {
   try {
      throw;
   } catch (const checktime_exceeded&) {
      throw;
   } FC_CAPTURE_AND_LOG((trx));
}

transaction_trace chain_controller::__apply_transaction( transaction_metadata& meta ) 
{ try {
   transaction_trace result(meta.id);

   validate_transaction( meta.trx() );

   for (const auto &act : meta.trx().context_free_actions) {
      FC_ASSERT( act.authorization.size() == 0, "context free actions cannot require authorization" );
      apply_context context(*this, _db, act, meta);
      context.context_free = true;
      context.exec();
      fc::move_append(result.action_traces, std::move(context.results.applied_actions));
      FC_ASSERT( result.deferred_transaction_requests.size() == 0 );
   }

   for (const auto &act : meta.trx().actions) {
      apply_context context(*this, _db, act, meta);
      context.exec();
      context.used_context_free_api |= act.authorization.size();

      FC_ASSERT( context.used_context_free_api, "action did not reference database state, it should be moved to context_free_actions", ("act",act) );
      fc::move_append(result.action_traces, std::move(context.results.applied_actions));
      fc::move_append(result.deferred_transaction_requests, std::move(context.results.deferred_transaction_requests));
   }

   update_resource_usage(result, meta);

   update_permission_usage(meta);
   record_transaction(meta.trx());
   return result;
} FC_CAPTURE_AND_RETHROW() }

transaction_trace chain_controller::_apply_transaction( transaction_metadata& meta ) { try {
   auto execute = [this](transaction_metadata& meta) -> transaction_trace {
      try {
         auto temp_session = _db.start_undo_session(true);
         auto result =  __apply_transaction(meta);
         temp_session.squash();
         return result;
      } catch (...) {
         // if there is no sender, there is no error handling possible, rethrow
         if (!meta.sender) {
            throw;
         }

         // log exceptions we can handle with the error handle, throws otherwise
         log_handled_exceptions(meta.trx());

         return _apply_error(meta);
      }
   };

   auto start = fc::time_point::now();
   auto result = execute(meta);
   result._profiling_us = fc::time_point::now() - start;
   return result;
} FC_CAPTURE_AND_RETHROW( (transaction_header(meta.trx())) ) }

transaction_trace chain_controller::_apply_error( transaction_metadata& meta ) {
   transaction_trace result(meta.id);
   result.status = transaction_trace::soft_fail;

   transaction etrx;
   etrx.actions.emplace_back(vector<permission_level>{{meta.sender_id,config::active_name}},
                             contracts::onerror(meta.raw_data, meta.raw_data + meta.raw_size) );

   try {
      auto temp_session = _db.start_undo_session(true);

      apply_context context(*this, _db, etrx.actions.front(), meta);
      context.exec();
      fc::move_append(result.action_traces, std::move(context.results.applied_actions));
      fc::move_append(result.deferred_transaction_requests, std::move(context.results.deferred_transaction_requests));

      uint32_t act_usage = result.action_traces.size();

      for (auto &at: result.action_traces) {
         at.region_id = meta.region_id;
         at.cycle_index = meta.cycle_index;
      }

      update_resource_usage(result, meta);
      record_transaction(meta.trx());

      temp_session.squash();
      return result;

   } catch (...) {
      // log exceptions we can handle with the error handle, throws otherwise
      log_handled_exceptions(etrx);

      // fall through to marking this tx as hard-failing
   }

   // if we have an objective error, on an error handler, we return hard fail for the trx
   result.status = transaction_trace::hard_fail;
   return result;
}

void chain_controller::_destroy_generated_transaction( const generated_transaction_object& gto ) {
   auto& generated_transaction_idx = _db.get_mutable_index<generated_transaction_multi_index>();
   _resource_limits.add_account_ram_usage(gto.payer, -(sizeof(generated_transaction_object) + gto.packed_trx.size() + config::overhead_per_row_ram_bytes));
   generated_transaction_idx.remove(gto);

}

void chain_controller::_create_generated_transaction( const deferred_transaction& dto ) {
   size_t trx_size = fc::raw::pack_size(dto);
   _resource_limits.add_account_ram_usage(
      dto.payer,
      (sizeof(generated_transaction_object) + (int64_t)trx_size + config::overhead_per_row_ram_bytes),
      "Generated Transaction ${id} from ${s}", _V("id", dto.sender_id)("s",dto.sender)
   );

   _db.create<generated_transaction_object>([&](generated_transaction_object &obj) {
      obj.trx_id = dto.id();
      obj.sender = dto.sender;
      obj.sender_id = dto.sender_id;
      obj.payer = dto.payer;
      obj.expiration = dto.expiration;
      obj.delay_until = dto.execute_after;
      obj.published = head_block_time();
      obj.packed_trx.resize(trx_size);
      fc::datastream<char *> ds(obj.packed_trx.data(), obj.packed_trx.size());
      fc::raw::pack(ds, dto);
   });
}

vector<transaction_trace> chain_controller::push_deferred_transactions( bool flush, uint32_t skip )
{ try {
   if( !_pending_block ) {
      _start_pending_block( true );
   }

   return with_skip_flags(skip, [&]() {
      return _db.with_write_lock([&]() {
         return _push_deferred_transactions( flush );
      });
   });
} FC_CAPTURE_AND_RETHROW() }

vector<transaction_trace> chain_controller::_push_deferred_transactions( bool flush )
{
   FC_ASSERT( _pending_block, " block not started" );

   if (flush && _pending_cycle_trace && _pending_cycle_trace->shard_traces.size() > 0) {
      // TODO: when we go multithreaded this will need a better way to see if there are flushable
      // deferred transactions in the shards
      auto maybe_start_new_cycle = [&]() {
         for (const auto &st: _pending_cycle_trace->shard_traces) {
            for (const auto &tr: st.transaction_traces) {
               for (const auto &req: tr.deferred_transaction_requests) {
                  if ( req.contains<deferred_transaction>() ) {
                     const auto& dt = req.get<deferred_transaction>();
                     if ( fc::time_point(dt.execute_after) <= head_block_time() ) {
                        // force a new cycle and break out
                        _finalize_pending_cycle();
                        _start_pending_cycle();
                        return;
                     }
                  }
               }
            }
         }
      };

      maybe_start_new_cycle();
   }

   const auto& generated_transaction_idx = _db.get_index<generated_transaction_multi_index>();
   auto& generated_index = generated_transaction_idx.indices().get<by_delay>();
   vector<const generated_transaction_object*> candidates;

   for( auto itr = generated_index.begin(); itr != generated_index.end() && (head_block_time() >= itr->delay_until); ++itr) {
      const auto &gtrx = *itr;
      candidates.emplace_back(&gtrx);
   }

   auto deferred_transactions_deadline = fc::time_point::now() + fc::microseconds(config::deffered_transactions_max_time_per_block_us);
   vector<transaction_trace> res;
   for (const auto* trx_p: candidates) {
      if (!is_known_transaction(trx_p->trx_id)) {
         try {
            auto trx = fc::raw::unpack<deferred_transaction>(trx_p->packed_trx.data(), trx_p->packed_trx.size());
            transaction_metadata mtrx (trx, trx_p->published, trx.sender, trx.sender_id, trx_p->packed_trx.data(), trx_p->packed_trx.size(), deferred_transactions_deadline);
            res.push_back( _push_transaction(std::move(mtrx)) );
         } FC_CAPTURE_AND_LOG((trx_p->trx_id)(trx_p->sender));
      }

      _destroy_generated_transaction(*trx_p);

      if ( deferred_transactions_deadline <= fc::time_point::now() ) {
         break;
      }
   }
   return res;
}

const apply_handler* chain_controller::find_apply_handler( account_name receiver, account_name scope, action_name act ) const
{
   auto native_handler_scope = _apply_handlers.find( receiver );
   if( native_handler_scope != _apply_handlers.end() ) {
      auto handler = native_handler_scope->second.find( make_pair( scope, act ) );
      if( handler != native_handler_scope->second.end() )
         return &handler->second;
   }
   return nullptr;
}

template<typename TransactionProcessing>
transaction_trace chain_controller::wrap_transaction_processing( transaction_metadata&& data, TransactionProcessing trx_processing )
{ try {
   FC_ASSERT( _pending_block, " block not started" );

   if (_limits.max_push_transaction_us.count() > 0) {
      auto newval = fc::time_point::now() + _limits.max_push_transaction_us;
      if ( !data.processing_deadline || newval < *data.processing_deadline ) {
         data.processing_deadline = newval;
      }
   }

   const transaction& trx = data.trx();

   //wdump((transaction_header(trx)));

   auto temp_session = _db.start_undo_session(true);

   // for now apply the transaction serially but schedule it according to those invariants
   validate_referenced_accounts(trx);

   auto result = trx_processing(data);

   auto& bcycle = _pending_block->regions.back().cycles_summary.back();
   auto& bshard = bcycle.front();
   auto& bshard_trace = _pending_cycle_trace->shard_traces.at(0);

   record_locks_for_data_access(result, bshard_trace.read_locks, bshard_trace.write_locks);

   bshard.transactions.emplace_back( result );
   
   bshard_trace.append(result);

   // The transaction applied successfully. Merge its changes into the pending block session.
   temp_session.squash();

   //wdump((transaction_header(data.trx())));
   _pending_transaction_metas.emplace_back( move(data) );

   return result;
} FC_CAPTURE_AND_RETHROW( (transaction_header(data.trx())) ) }

} } /// eosio::chain
