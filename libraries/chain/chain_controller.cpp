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

#include <fstream>
#include <functional>
#include <iostream>
#include <chrono>

namespace eosio { namespace chain {

bool chain_controller::is_start_of_round( block_num_type block_num )const  {
  return 0 == (block_num % blocks_per_round());
}

uint32_t chain_controller::blocks_per_round()const {
  return get_global_properties().active_producers.producers.size()*config::producer_repititions;
}

chain_controller::chain_controller( const chain_controller::controller_config& cfg )
:_db( cfg.shared_memory_dir, 
      (cfg.read_only ? database::read_only : database::read_write), 
      cfg.shared_memory_size), 
 _block_log(cfg.block_log_dir),
 _limits(cfg.limits)
{
   _initialize_indexes();

   for (auto& f : cfg.applied_irreversible_block_callbacks)
      applied_irreversible_block.connect(f);

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
   return with_skip_flags(skip, [&]() {
      return _db.with_write_lock([&]() {
         return _push_transaction(trx);
      });
   });
} FC_CAPTURE_AND_RETHROW((trx)) }

transaction_trace chain_controller::_push_transaction(const packed_transaction& trx) {
   transaction_metadata   mtrx( trx, get_chain_id(), head_block_time());
   check_transaction_authorization(mtrx.trx(), trx.signatures);

   auto result = _push_transaction(std::move(mtrx));

   // notify anyone listening to pending transactions
   on_pending_transaction(trx);

   _pending_block->input_transactions.emplace_back(trx);

   return result;

}

static void record_locks_for_data_access(const vector<action_trace>& action_traces, vector<shard_lock>& read_locks, vector<shard_lock>& write_locks ) {
   for (const auto& at: action_traces) {
      for (const auto& access: at.data_access) {
         if (access.type == data_access_info::read) {
            read_locks.emplace_back(shard_lock{access.code, access.scope});
         } else {
            write_locks.emplace_back(shard_lock{access.code, access.scope});
         }
      }
   }
}

transaction_trace chain_controller::_push_transaction( transaction_metadata&& data )
{
   if (_limits.max_push_transaction_us.count() > 0) {
      data.processing_deadline = fc::time_point::now() + _limits.max_push_transaction_us;
   }

   const transaction& trx = data.trx();
   // If this is the first transaction pushed after applying a block, start a new undo session.
   // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
   if( !_pending_block ) {
      _start_pending_block();
   }

   auto temp_session = _db.start_undo_session(true);

   // for now apply the transaction serially but schedule it according to those invariants
   validate_referenced_accounts(trx);

   auto cyclenum = _pending_block->regions.back().cycles_summary.size() - 1;

   /// TODO: move _pending_cycle into db so that it can be undone if transation fails, for now we will apply
   /// the transaction first so that there is nothing to undo... this only works because things are currently
   /// single threaded
   // set cycle, shard, region etc
   data.region_id = 0;
   data.cycle_index = cyclenum;
   data.shard_index = 0;
   auto result = _apply_transaction( data );

   auto& bcycle = _pending_block->regions.back().cycles_summary.back();
   auto& bshard = bcycle.front();

   record_locks_for_data_access(result.action_traces, bshard.read_locks, bshard.write_locks);

   fc::deduplicate(bshard.read_locks);
   fc::deduplicate(bshard.write_locks);

   bshard.transactions.emplace_back( result );

   _pending_cycle_trace->shard_traces.at(0).append(result);

   // The transaction applied successfully. Merge its changes into the pending block session.
   temp_session.squash();

   _pending_transaction_metas.emplace_back(std::forward<transaction_metadata>(data));

   return result;
}

void chain_controller::_start_pending_block()
{
   FC_ASSERT( !_pending_block );
   _pending_block         = signed_block();
   _pending_block_trace   = block_trace(*_pending_block);
   _pending_block_session = _db.start_undo_session(true);
   _pending_block->regions.resize(1);
   _pending_block_trace->region_traces.resize(1);
   _start_pending_cycle();
}

/**
 *  Wraps up all work for current shards, starts a new cycle, and
 *  executes any pending transactions
 */
void chain_controller::_start_pending_cycle() {
   _pending_block->regions.back().cycles_summary.resize( _pending_block->regions[0].cycles_summary.size() + 1 );
   _pending_cycle_trace = cycle_trace();
   _start_pending_shard();

   /// TODO: check for deferred transactions and schedule them
} // _start_pending_cycle

void chain_controller::_start_pending_shard()
{
   auto& bcycle = _pending_block->regions.back().cycles_summary.back();
   bcycle.resize( bcycle.size()+1 );

   _pending_cycle_trace->shard_traces.resize(_pending_cycle_trace->shard_traces.size() + 1 );
}

void chain_controller::_finalize_pending_cycle()
{
   for( auto& shard : _pending_cycle_trace->shard_traces ) {
      shard.calculate_root();
   }

   _apply_cycle_trace(*_pending_cycle_trace);
   _pending_block_trace->region_traces.back().cycle_traces.emplace_back(std::move(*_pending_cycle_trace));
   _pending_cycle_trace.reset();
}

void chain_controller::_apply_cycle_trace( const cycle_trace& res )
{
   for (const auto&st: res.shard_traces) {
      for (const auto &tr: st.transaction_traces) {
         for (const auto &dt: tr.deferred_transactions) {
            _db.create<generated_transaction_object>([&](generated_transaction_object &obj) {
               obj.trx_id = dt.id();
               obj.sender = dt.sender;
               obj.sender_id = dt.sender_id;
               obj.expiration = dt.expiration;
               obj.delay_until = dt.execute_after;
               obj.published = head_block_time();
               obj.packed_trx.resize(fc::raw::pack_size(dt));
               fc::datastream<char *> ds(obj.packed_trx.data(), obj.packed_trx.size());
               fc::raw::pack(ds, dt);
            });
         }

         if (tr.canceled_deferred.size() > 0 ) {
            auto &generated_transaction_idx = _db.get_mutable_index<generated_transaction_multi_index>();
            const auto &generated_index = generated_transaction_idx.indices().get<by_sender_id>();
            for (const auto &dr: tr.canceled_deferred) {
               while(!generated_index.empty()) {
                  const auto& itr = generated_index.lower_bound(boost::make_tuple(dr.sender, dr.sender_id));
                  if (itr == generated_index.end() || itr->sender != dr.sender || itr->sender_id != dr.sender_id ) {
                     break;
                  }

                  generated_transaction_idx.remove(*itr);
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
               std::cerr << prefix << ": CONSOLE OUTPUT BEGIN =====================" << std::endl;
               std::cerr << ar.console;
               std::cerr << prefix << ": CONSOLE OUTPUT END   =====================" << std::endl;
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


   /// cache the input tranasction ids so that they can be looked up when executing the
   /// summary
   vector<transaction_metadata> input_metas;
   input_metas.reserve(next_block.input_transactions.size());
   map<transaction_id_type,size_t> trx_index;
   for( const auto& t : next_block.input_transactions ) {
      input_metas.emplace_back(t, chain_id_type(), next_block.timestamp);
      trx_index[input_metas.back().id] =  input_metas.size() - 1;
   }

   block_trace next_block_trace(next_block);
   next_block_trace.region_traces.reserve(next_block.regions.size());

   for( const auto& r : next_block.regions ) {
      region_trace r_trace;
      r_trace.cycle_traces.reserve(r.cycles_summary.size());

      for (uint32_t cycle_index = 0; cycle_index < r.cycles_summary.size(); cycle_index++) {
         const auto& cycle = r.cycles_summary.at(cycle_index);
         cycle_trace c_trace;
         c_trace.shard_traces.reserve(cycle.size());

         // validate that no read_scope is used as a write scope in this cycle and that no two shards
         // share write scopes
         set<shard_lock> read_locks;
         map<shard_lock, uint32_t> write_locks;

         for (uint32_t shard_index = 0; shard_index < cycle.size(); shard_index++) {
            const auto& shard = cycle.at(shard_index);

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

            vector<shard_lock> used_read_locks;
            vector<shard_lock> used_write_locks;

            shard_trace s_trace;
            for (const auto& receipt : shard.transactions) {
                optional<transaction_metadata> _temp;
                auto make_metadata = [&]() -> transaction_metadata* {
                  auto itr = trx_index.find(receipt.id);
                  if( itr != trx_index.end() ) {
                     return &input_metas.at(itr->second);
                  } else {
                     const auto& gtrx = _db.get<generated_transaction_object,by_trx_id>(receipt.id);
                     auto trx = fc::raw::unpack<deferred_transaction>(gtrx.packed_trx.data(), gtrx.packed_trx.size());
                     _temp.emplace(trx, gtrx.published, trx.sender, trx.sender_id, gtrx.packed_trx.data(), gtrx.packed_trx.size() );
                     return &*_temp;
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
               record_locks_for_data_access(s_trace.transaction_traces.back().action_traces, used_read_locks, used_write_locks);

               FC_ASSERT(receipt.status == s_trace.transaction_traces.back().status);

               // validate_referenced_accounts(trx);
               // Check authorization, and allow irrelevant signatures.
               // If the block producer let it slide, we'll roll with it.
               // check_transaction_authorization(trx, true);
            } /// for each transaction id

            // Validate that the producer didn't list extra locks to bloat the size of the block
            // TODO: this check can be removed when blocks are irreversible
            fc::deduplicate(used_read_locks);
            fc::deduplicate(used_write_locks);

            EOS_ASSERT(std::equal(used_read_locks.cbegin(), used_read_locks.cend(), shard.read_locks.begin()),
               block_lock_exception, "Read locks for executing shard: ${s} do not match those listed in the block", ("s", shard_index));
            EOS_ASSERT(std::equal(used_write_locks.cbegin(), used_write_locks.cend(), shard.write_locks.begin()),
               block_lock_exception, "Write locks for executing shard: ${s} do not match those listed in the block", ("s", shard_index));

            s_trace.calculate_root();
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

void chain_controller::check_authorization( const vector<action>& actions,
                                            flat_set<public_key_type> provided_keys,
                                            bool allow_unused_signatures,
                                            flat_set<account_name>    provided_accounts  )const
{
   auto checker = make_auth_checker( [&](const permission_level& p){ return get_permission(p).auth; },
                                     get_global_properties().configuration.max_authority_depth,
                                     provided_keys, provided_accounts );


   for( const auto& act : actions ) {
      for( const auto& declared_auth : act.authorization ) {

         // check a minimum permission if one is set, otherwise assume the contract code will validate
         auto min_permission_name = lookup_minimum_permission(declared_auth.actor, act.account, act.name);
         if (min_permission_name) {
            const auto& min_permission = _db.get<permission_object, by_owner>(boost::make_tuple(declared_auth.actor, *min_permission_name));


            if ((_skip_flags & skip_authority_check) == false) {
               const auto &index = _db.get_index<permission_index>().indices();
               EOS_ASSERT(get_permission(declared_auth).satisfies(min_permission, index),
                          tx_irrelevant_auth,
                          "action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                          ("auth", declared_auth)("min", min_permission.name));
            }
         }
         if ((_skip_flags & skip_transaction_signatures) == false) {
            EOS_ASSERT(checker.satisfied(declared_auth), tx_missing_sigs,
                       "transaction declares authority '${auth}', but does not have signatures for it.",
                       ("auth", declared_auth));
         }
      }
   }

   if (!allow_unused_signatures && (_skip_flags & skip_transaction_signatures) == false)
      EOS_ASSERT(checker.all_keys_used(), tx_irrelevant_sig,
                 "transaction bears irrelevant signatures from these keys: ${keys}", 
                 ("keys", checker.unused_keys()));
}

void chain_controller::check_transaction_authorization(const transaction& trx,
                                                       const vector<signature_type>& signatures,
                                                       bool allow_unused_signatures)const 
{
   check_authorization( trx.actions, trx.get_signature_keys( signatures, chain_id_type{} ), allow_unused_signatures );
}

optional<permission_name> chain_controller::lookup_minimum_permission(account_name authorizer_account,
                                                                    account_name scope,
                                                                    action_name act_name) const {
   // updateauth is a special case where any permission _may_ be suitable depending
   // on the contents of the action
   if (scope == config::system_account_name && act_name == N(updateauth)) {
      return optional<permission_name>();
   }

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
      if (link != nullptr)
         return link->required_permission;
      else
         return N(active);
   } FC_CAPTURE_AND_RETHROW((authorizer_account)(scope)(act_name))
}

void chain_controller::validate_uniqueness( const transaction& trx )const {
   if( !should_check_for_duplicate_transactions() ) return;

   auto transaction = _db.find<transaction_object, by_trx_id>(trx.id());
   EOS_ASSERT(transaction == nullptr, tx_duplicate, "transaction is not unique");
}

void chain_controller::record_transaction(const transaction& trx) {
   //Insert transaction into unique transactions database.
    _db.create<transaction_object>([&](transaction_object& transaction) {
        transaction.trx_id = trx.id(); 
        transaction.expiration = trx.expiration;
    });
}


void chain_controller::validate_tapos(const transaction& trx)const {
   if (!should_check_tapos()) return;

   const auto& tapos_block_summary = _db.get<block_summary_object>((uint16_t)trx.ref_block_num);

   //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
   EOS_ASSERT(trx.verify_reference_block(tapos_block_summary.block_id), transaction_exception,
              "transaction's reference block did not match. Is this transaction from a different fork?",
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
              transaction_exception, "transaction expiration is too far in the future",
              ("trx.expiration",trx.expiration)("now",now)
              ("max_til_exp",chain_configuration.max_transaction_lifetime));
   EOS_ASSERT( now <= time_point(trx.expiration), transaction_exception, "transaction is expired",
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
} FC_CAPTURE_AND_RETHROW( (level) ) }

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


   _db.add_index<contracts::keystr_value_index>();
   _db.add_index<contracts::key128x128_value_index>();
   _db.add_index<contracts::key64x64_value_index>();
   _db.add_index<contracts::key64x64x64_value_index>();

   _db.add_index<global_property_multi_index>();
   _db.add_index<dynamic_global_property_multi_index>();
   _db.add_index<block_summary_multi_index>();
   _db.add_index<transaction_multi_index>();
   _db.add_index<generated_transaction_multi_index>();
   _db.add_index<producer_multi_index>();
   _db.add_index<scope_sequence_multi_index>();
   _db.add_index<bandwidth_usage_index>();
   _db.add_index<compute_usage_index>();
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
            wdump((starter.get_chain_start_producers()));
         });

         _db.create<dynamic_global_property_object>([&](dynamic_global_property_object& p) {
            p.time = initial_timestamp;
            p.recent_slots_filled = uint64_t(-1);
            p.virtual_net_bandwidth = gp.configuration.max_block_size * (config::blocksize_average_window_ms / config::block_interval_ms );
            p.virtual_act_bandwidth = gp.configuration.max_block_acts * (config::blocksize_average_window_ms / config::block_interval_ms );
         });

         // Initialize block summary index
         for (int i = 0; i < 0x10000; i++)
            _db.create<block_summary_object>([&](block_summary_object&) {});

         starter.prepare_database(*this, _db);

         ilog( "done initializing chain" );
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
      dgp.average_block_size.add_usage( fc::raw::pack_size(b), b.timestamp );

      dgp.update_virtual_net_bandwidth( props.configuration );
      dgp.update_virtual_act_bandwidth( props.configuration );


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
   int last_block_on_disk = 0;
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
   /*
   auto& transaction_idx = _db.get_mutable_index<transaction_multi_index>();
   const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
   while( (!dedupe_index.empty()) && (head_block_time() > dedupe_index.rbegin()->expiration) )
      transaction_idx.remove(*dedupe_index.rbegin());
      */
   //Look for expired transactions in the pending generated list, and remove them.
   //transactions must have expired by at least two forking windows in order to be removed.
   auto& generated_transaction_idx = _db.get_mutable_index<generated_transaction_multi_index>();
   const auto& generated_index = generated_transaction_idx.indices().get<by_expiration>();
   while( (!generated_index.empty()) && (head_block_time() > generated_index.rbegin()->expiration) )
      generated_transaction_idx.remove(*generated_index.rbegin());

} FC_CAPTURE_AND_RETHROW() }

using boost::container::flat_set;

account_name chain_controller::get_scheduled_producer(uint32_t slot_num)const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t current_aslot = dpo.current_absolute_slot + slot_num;
   const auto& gpo = _db.get<global_property_object>();
   auto number_of_active_producers = gpo.active_producers.producers.size();
   auto index = current_aslot % (number_of_active_producers);
   index /= config::producer_repititions;

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
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   return uint64_t(config::percent_100) * __builtin_popcountll(dpo.recent_slots_filled) / 64;
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

transaction_trace chain_controller::__apply_transaction( transaction_metadata& meta ) {
   transaction_trace result(meta.id);
   for (const auto &act : meta.trx().actions) {
      apply_context context(*this, _db, act, meta);
      context.exec();
      fc::move_append(result.action_traces, std::move(context.results.applied_actions));
      fc::move_append(result.deferred_transactions, std::move(context.results.generated_transactions));
      fc::move_append(result.canceled_deferred, std::move(context.results.canceled_deferred));
   }

   uint32_t act_usage = result.action_traces.size();

   for (auto &at: result.action_traces) {
      at.region_id = meta.region_id;
      at.cycle_index = meta.cycle_index;
      if (at.receiver == config::system_account_name &&
          at.act.account == config::system_account_name &&
          at.act.name == N(setcode)) {
         act_usage += config::setcode_act_usage;
      }
   }

   update_usage(meta, act_usage);
   record_transaction(meta.trx());
   return result;
}

transaction_trace chain_controller::_apply_transaction( transaction_metadata& meta ) {
   try {
      auto temp_session = _db.start_undo_session(true);
      auto result = __apply_transaction(meta);
      temp_session.squash();
      return result;
   } catch (...) {
      // if there is no sender, there is no error handling possible, rethrow
      if (!meta.sender) {
         throw;
      }
      // log exceptions we can handle with the error handle, throws otherwise
      log_handled_exceptions(meta.trx());

      return _apply_error( meta );
   }
}

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
      fc::move_append(result.deferred_transactions, std::move(context.results.generated_transactions));

      uint32_t act_usage = result.action_traces.size();

      for (auto &at: result.action_traces) {
         at.region_id = meta.region_id;
         at.cycle_index = meta.cycle_index;
      }

      update_usage(meta, act_usage);
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

void chain_controller::push_deferred_transactions( bool flush )
{
   if (flush && _pending_cycle_trace && _pending_cycle_trace->shard_traces.size() > 0) {
      // TODO: when we go multithreaded this will need a better way to see if there are flushable
      // deferred transactions in the shards
      auto maybe_start_new_cycle = [&]() {
         for (const auto &st: _pending_cycle_trace->shard_traces) {
            for (const auto &tr: st.transaction_traces) {
               for (const auto &dt: tr.deferred_transactions) {
                  if (fc::time_point(dt.execute_after) <= head_block_time()) {
                     // force a new cycle and break out
                     _finalize_pending_cycle();
                     _start_pending_cycle();
                     return;
                  }
               }
            }
         }
      };

      maybe_start_new_cycle();
   }

   auto& generated_transaction_idx = _db.get_mutable_index<generated_transaction_multi_index>();
   auto& generated_index = generated_transaction_idx.indices().get<by_delay>();
   vector<const generated_transaction_object*> candidates;

   for( auto itr = generated_index.rbegin(); itr != generated_index.rend() && (head_block_time() >= itr->delay_until); ++itr) {
      const auto &gtrx = *itr;
      candidates.emplace_back(&gtrx);
   }

   for (const auto* trx_p: candidates) {
      if (!is_known_transaction(trx_p->trx_id)) {
         try {
            auto trx = fc::raw::unpack<deferred_transaction>(trx_p->packed_trx.data(), trx_p->packed_trx.size());
            transaction_metadata mtrx (trx, trx_p->published, trx.sender, trx.sender_id, trx_p->packed_trx.data(), trx_p->packed_trx.size());
            _push_transaction(std::move(mtrx));
            generated_transaction_idx.remove(*trx_p);
         } FC_CAPTURE_AND_LOG((trx_p->trx_id)(trx_p->sender));
      } else {
         generated_transaction_idx.remove(*trx_p);
      }
   }
}


/**
 *  @param act_usage The number of "actions" delivered directly or indirectly by applying meta.trx
 */
void chain_controller::update_usage( transaction_metadata& meta, uint32_t act_usage )
{
   set<std::pair<account_name, permission_name>> authorizing_accounts;

   for( const auto& act : meta.trx().actions )
      for( const auto& auth : act.authorization )
         authorizing_accounts.emplace( auth.actor, auth.permission );

   auto trx_size = meta.bandwidth_usage + config::fixed_bandwidth_overhead_per_transaction;

   const auto& dgpo = get_dynamic_global_properties();

   if( meta.signing_keys ) {
      act_usage += meta.signing_keys->size();
   }

   auto head_time = head_block_time();
   for( const auto& authaccnt : authorizing_accounts ) {

      const auto& buo = _db.get<bandwidth_usage_object,by_owner>( authaccnt.first );
      _db.modify( buo, [&]( auto& bu ){
          bu.bytes.add_usage( trx_size, head_time );
          bu.acts.add_usage( act_usage, head_time );
      });

      uint128_t  used_ubytes        = buo.bytes.value;
      uint128_t  used_uacts         = buo.acts.value;
      uint128_t  virtual_max_ubytes = dgpo.virtual_net_bandwidth * config::rate_limiting_precision;
      uint128_t  virtual_max_uacts  = dgpo.virtual_act_bandwidth * config::rate_limiting_precision;
      
      if( !(_skip_flags & genesis_setup) ) {
         #warning TODO: restore bandwidth checks
         /* setting of bandwidth currently not implemented
         FC_ASSERT( (used_ubytes * dgpo.total_net_weight) <=  (buo.net_weight * virtual_max_ubytes), "authorizing account '${n}' has insufficient net bandwidth for this transaction",
                    ("n",name(authaccnt.first))
                    ("used_bytes",double(used_ubytes)/1000000.)
                    ("user_net_weight",buo.net_weight)
                    ("virtual_max_bytes", double(virtual_max_ubytes)/1000000. )
                    ("total_net_weight", dgpo.total_net_weight)
                    );
         FC_ASSERT( (used_uacts * dgpo.total_cpu_weight)  <=  (buo.cpu_weight* virtual_max_uacts),  "authorizing account '${n}' has insufficient compute bandwidth for this transaction",
                    ("n",name(authaccnt.first))
                    ("used_acts",double(used_uacts)/1000000.)
                    ("user_cpu_weight",buo.cpu_weight)
                    ("virtual_max_uacts", double(virtual_max_uacts)/1000000. )
                    ("total_cpu_tokens", dgpo.total_cpu_weight)
                  );
         */
      }

      // for any transaction not sent by code, update the affirmative last time a given permission was used
      if (!meta.sender) {
         const auto *puo = _db.find<permission_usage_object, by_account_permission>(boost::make_tuple(authaccnt.first, authaccnt.second));
         if (puo) {
            _db.modify(*puo, [this](permission_usage_object &pu) {
               pu.last_used = head_block_time();
            });
         } else {
            _db.create<permission_usage_object>([this, &authaccnt](permission_usage_object &pu){
               pu.account = authaccnt.first;
               pu.permission = authaccnt.second;
               pu.last_used = head_block_time();
            });
         }
      }
   }

   _db.modify( dgpo, [&]( auto& props ) {
      props.average_block_acts.add_usage( act_usage, head_time );
   });

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

} } /// eosio::chain
