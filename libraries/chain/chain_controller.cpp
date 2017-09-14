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

#include <eos/chain/chain_controller.hpp>

#include <eos/chain/block_summary_object.hpp>
#include <eos/chain/global_property_object.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/action_objects.hpp>
#include <eos/chain/generated_transaction_object.hpp>
#include <eos/chain/transaction_object.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/permission_link_object.hpp>
#include <eos/chain/authority_checker.hpp>

#include <eos/chain/wasm_interface.hpp>

#include <eos/types/native.hpp>
#include <eos/types/generated.hpp>
#include <eos/types/AbiSerializer.hpp>

#include <eos/utilities/rand.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>
#include <fc/crypto/digest.hpp>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm_ext/is_sorted.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/map.hpp>

#include <fstream>
#include <functional>
#include <iostream>
#include <chrono>

namespace eos { namespace chain {

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

const SignedTransaction& chain_controller::get_recent_transaction(const transaction_id_type& trx_id) const
{
   auto& index = _db.get_index<transaction_multi_index, by_trx_id>();
   auto itr = index.find(trx_id);
   FC_ASSERT(itr != index.end());
   return itr->trx;
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

const GeneratedTransaction& chain_controller::get_generated_transaction( const generated_transaction_id_type& id ) const {
   auto& index = _db.get_index<generated_transaction_multi_index, generated_transaction_object::by_trx_id>();
   auto itr = index.find(id);
   FC_ASSERT(itr != index.end());
   return itr->trx;
}

/**
 * Push block "may fail" in which case every partial change is unwound.  After
 * push block is successful the block is appended to the chain database on disk.
 *
 * @return true if we switched forks as a result of this push.
 */
bool chain_controller::push_block(const signed_block& new_block, uint32_t skip)
{ try {
   return with_skip_flags( skip, [&](){ 
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
                   apply_block((*ritr)->data, skip);
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
                      apply_block((*ritr)->data, skip);
                      session.push();
                   }
                   throw *except;
                }
            }
            return true;
         }
         else return false;
      }
   }

   try {
      auto session = _db.start_undo_session(true);
      auto exec_start = std::chrono::high_resolution_clock::now();
      apply_block(new_block, skip);
      if( (fc::time_point::now() - new_block.timestamp) < fc::seconds(60) )
      {
         auto exec_stop = std::chrono::high_resolution_clock::now();
         auto exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_stop - exec_start);      
         size_t trxcount = 0;
         for (const auto& cycle : new_block.cycles)
            for (const auto& thread : cycle)
               trxcount += thread.user_input.size();
         ilog( "${producer} #${num} @${time}  | ${trxcount} trx, ${pending} pending, exectime_ms=${extm}", 
            ("producer", new_block.producer) 
            ("time", new_block.timestamp)
            ("num", new_block.block_num())
            ("trxcount", trxcount)
            ("pending", _pending_transactions.size())
            ("extm", exec_ms.count())
         );
      }
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
ProcessedTransaction chain_controller::push_transaction(const SignedTransaction& trx, uint32_t skip)
{ try {
   return with_skip_flags(skip, [&]() {
      return _db.with_write_lock([&]() {
         return _push_transaction(trx);
      });
   });
} FC_CAPTURE_AND_RETHROW((trx)) }

ProcessedTransaction chain_controller::_push_transaction(const SignedTransaction& trx) {
   // If this is the first transaction pushed after applying a block, start a new undo session.
   // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
   if (!_pending_tx_session.valid())
      _pending_tx_session = _db.start_undo_session(true);

   auto temp_session = _db.start_undo_session(true);
   validate_referenced_accounts(trx);
   check_transaction_authorization(trx);
   auto pt = apply_transaction(trx);
   _pending_transactions.push_back(trx);

   // notify_changed_objects();
   // The transaction applied successfully. Merge its changes into the pending block session.
   temp_session.squash();

   // notify anyone listening to pending transactions
   on_pending_transaction(trx); /// TODO move this to apply... ??? why... 

   return pt;
}

signed_block chain_controller::generate_block(
   fc::time_point_sec when,
   const AccountName& producer,
   const fc::ecc::private_key& block_signing_private_key,
   block_schedule::factory scheduler, /* = block_schedule::by_threading_conflits */
   uint32_t skip /* = 0 */
   )
{ try {
   return with_skip_flags( skip, [&](){
      auto b = _db.with_write_lock( [&](){
         return _generate_block( when, producer, block_signing_private_key, scheduler );
      });
      push_block(b, skip);
      return b;
   });
} FC_CAPTURE_AND_RETHROW( (when) ) }

signed_block chain_controller::_generate_block(
   fc::time_point_sec when,
   const AccountName& producer,
   const fc::ecc::private_key& block_signing_private_key,
   block_schedule::factory scheduler
   )
{
   try {
   uint32_t skip = _skip_flags;
   uint32_t slot_num = get_slot_at_time( when );
   FC_ASSERT( slot_num > 0 );
   AccountName scheduled_producer = get_scheduled_producer( slot_num );
   FC_ASSERT( scheduled_producer == producer );

   const auto& producer_obj = get_producer(scheduled_producer);

   if( !(skip & skip_producer_signature) )
      FC_ASSERT( producer_obj.signing_key == block_signing_private_key.get_public_key() );

  
   //
   // The following code throws away existing pending_tx_session and
   // rebuilds it by re-applying pending transactions.
   //
   // This rebuild is necessary because pending transactions' validity
   // and semantics may have changed since they were received, because
   // time-based semantics are evaluated based on the current block
   // time.  These changes can only be reflected in the database when
   // the value of the "when" variable is known, which means we need to
   // re-apply pending transactions in this method.
   //
   _pending_tx_session.reset();
   _pending_tx_session = _db.start_undo_session(true);

   const auto& generated = _db.get_index<generated_transaction_multi_index, generated_transaction_object::by_status>().equal_range(generated_transaction_object::PENDING);

   vector<pending_transaction> pending;
   std::set<transaction_id_type> invalid_pending;
   pending.reserve(std::distance(generated.first, generated.second) + _pending_transactions.size());
   for (auto iter = generated.first; iter != generated.second; ++iter) {
      const auto& gt = *iter;
      pending.emplace_back(std::reference_wrapper<const GeneratedTransaction> {gt.trx});
   }
   
   for(const auto& st: _pending_transactions) {
      pending.emplace_back(std::reference_wrapper<const SignedTransaction> {st});
   }

   auto schedule = scheduler(pending, get_global_properties());

   signed_block pending_block;
   pending_block.cycles.reserve(schedule.cycles.size());

   size_t invalid_transaction_count = 0;
   size_t valid_transaction_count = 0;

   for (const auto &c : schedule.cycles) {
     cycle block_cycle;
     block_cycle.reserve(c.size());

     for (const auto &t : c) {
       thread block_thread;
       block_thread.user_input.reserve(t.transactions.size());
       block_thread.generated_input.reserve(t.transactions.size());
       for (const auto &trx : t.transactions) {
          try
          {
             auto temp_session = _db.start_undo_session(true);
             if (trx.contains<std::reference_wrapper<const SignedTransaction>>()) {
                const auto& t = trx.get<std::reference_wrapper<const SignedTransaction>>().get();
                validate_referenced_accounts(t);
                check_transaction_authorization(t);
                auto processed = apply_transaction(t);
                block_thread.user_input.emplace_back(processed);
             } else if (trx.contains<std::reference_wrapper<const GeneratedTransaction>>()) {
                const auto& t = trx.get<std::reference_wrapper<const GeneratedTransaction>>().get();
                auto processed = apply_transaction(t);
                block_thread.generated_input.emplace_back(processed);
             } else {
                FC_THROW_EXCEPTION(tx_scheduling_exception, "Unknown transaction type in block_schedule");
             }
             
             temp_session.squash();
             valid_transaction_count++;
          }
          catch ( const fc::exception& e )
          {
             // Do nothing, transaction will not be re-applied
             elog( "Transaction was not processed while generating block due to ${e}", ("e", e) );
             if (trx.contains<std::reference_wrapper<const SignedTransaction>>()) {
                const auto& t = trx.get<std::reference_wrapper<const SignedTransaction>>().get();
                wlog( "The transaction was ${t}", ("t", t ) );
                invalid_pending.emplace(t.id());
             } else if (trx.contains<std::reference_wrapper<const GeneratedTransaction>>()) {
                wlog( "The transaction was ${t}", ("t", trx.get<std::reference_wrapper<const GeneratedTransaction>>().get()) );
             } 
             invalid_transaction_count++;
          }
       }

       if (!(block_thread.generated_input.empty() && block_thread.user_input.empty())) {
          block_thread.generated_input.shrink_to_fit();
          block_thread.user_input.shrink_to_fit();
          block_cycle.emplace_back(std::move(block_thread));
       }
     }

     if (!block_cycle.empty()) {
        pending_block.cycles.emplace_back(std::move(block_cycle));
     }
   }
   
   size_t postponed_tx_count = pending.size() - valid_transaction_count - invalid_transaction_count;
   if( postponed_tx_count > 0 )
   {
      wlog( "Postponed ${n} transactions due to block size limit", ("n", postponed_tx_count) );
   }

   if( invalid_transaction_count > 0 )
   {
      wlog( "Postponed ${n} transactions errors when processing", ("n", invalid_transaction_count) );

      // remove pending transactions determined to be bad during scheduling
      if (invalid_pending.size() > 0) {
         for (auto itr = _pending_transactions.begin(); itr != _pending_transactions.end(); ) {
            auto& tx = *itr;
            if (invalid_pending.find(tx.id()) != invalid_pending.end()) {
               itr = _pending_transactions.erase(itr);
            } else {
               ++itr;
            }
         }
      }
   }

   _pending_tx_session.reset();

   // We have temporarily broken the invariant that
   // _pending_tx_session is the result of applying _pending_tx, as
   // _pending_transactions now consists of the set of postponed transactions.
   // However, the push_block() call below will re-create the
   // _pending_tx_session.

   pending_block.previous = head_block_id();
   pending_block.timestamp = when;
   pending_block.transaction_merkle_root = pending_block.calculate_merkle_root();

   pending_block.producer = producer_obj.owner;

   // If this block is last in a round, calculate the schedule for the new round
   if (pending_block.block_num() % config::BlocksPerRound == 0) {
      auto new_schedule = _admin->get_next_round(_db);
      pending_block.producer_changes = get_global_properties().active_producers - new_schedule;
   }

   if( !(skip & skip_producer_signature) )
      pending_block.sign( block_signing_private_key );

   // TODO:  Move this to _push_block() so session is restored.
   /*
   if( !(skip & skip_block_size_check) )
   {
      FC_ASSERT( fc::raw::pack_size(pending_block) <= get_global_properties().parameters.maximum_block_size );
   }
   */

   // push_block( pending_block, skip );

   return pending_block;
} FC_CAPTURE_AND_RETHROW( (producer) ) }

/**
 * Removes the most recent block from the database and undoes any changes it made.
 */
void chain_controller::pop_block()
{ try {
   _pending_tx_session.reset();
   auto head_id = head_block_id();
   optional<signed_block> head_block = fetch_block_by_id( head_id );
   EOS_ASSERT( head_block.valid(), pop_empty_chain, "there are no blocks to pop" );

   _fork_db.pop_block();
   _db.undo();
} FC_CAPTURE_AND_RETHROW() }

void chain_controller::clear_pending()
{ try {
   _pending_transactions.clear();
   _pending_tx_session.reset();
} FC_CAPTURE_AND_RETHROW() }

//////////////////// private methods ////////////////////

void chain_controller::apply_block(const signed_block& next_block, uint32_t skip)
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
         _apply_block(next_block);
      });
   });
}

struct path_cons_list {
   typedef static_variant<int, const char *> head_type;
   typedef const path_cons_list* tail_ref_type;
   typedef optional<tail_ref_type> tail_type;
   
   path_cons_list(int index, const path_cons_list &rest) 
      : head(head_type(index))
      , tail(tail_ref_type(&rest))
   { }

   path_cons_list(const char *path, const path_cons_list &rest) 
      : head(head_type(path))
      , tail(tail_ref_type(&rest))
   { }

   path_cons_list(const char *path) 
      : head(head_type(path))
      , tail()
   { }

   //path_cons_list( path_cons_list && ) = delete;
   
   const head_type head;
   tail_type tail;

   path_cons_list operator() (int index) const {
      return path_cons_list(index, *this);
   }

   path_cons_list operator() (const char *path) const {
      return path_cons_list(path, *this);
   }

   void add_to_stream( std::stringstream& ss ) const {
      if (tail) {
         (*tail)->add_to_stream(ss);
      }

      if (head.contains<int>()) {
         ss << "[" << head.get<int>() << "]";
      } else {
         ss << head.get<const char *>();
      }
   }
};

string resolve_path_string(const path_cons_list& path) {
   std::stringstream ss;
   path.add_to_stream(ss);
   return ss.str();
}

template<typename T>
void check_output(const T& expected, const T& actual, const path_cons_list& path) {
   try {
      EOS_ASSERT((expected == actual), block_tx_output_exception, 
         "expected: ${expected}, actual: ${actual}", ("expected", expected)("actual", actual));
   } FC_RETHROW_EXCEPTIONS(warn, "at: ${path}", ("path", resolve_path_string(path)));
}

template<typename T>
void check_output(const vector<T>& expected, const vector<T>& actual, const path_cons_list& path) {
   check_output(expected.size(), actual.size(), path(".size()"));
   for(int idx=0; idx < expected.size(); idx++) {
      const auto &expected_element = expected.at(idx);
      const auto &actual_element = actual.at(idx);
      check_output(expected_element, actual_element, path(idx));
   }
}

template<typename T>
void check_output(const fc::optional<T>& expected, const fc::optional<T>& actual, const path_cons_list& path) {
   check_output(expected.valid(), actual.valid(), path(".valid()"));
   if (expected.valid()) {
      check_output(*expected, *actual, path);
   }
}

template<>
void check_output(const types::Bytes& expected, const types::Bytes& actual, const path_cons_list& path) {
  check_output(expected.size(), actual.size(), path(".size()"));
  
  auto cmp_result = std::memcmp(expected.data(), actual.data(), expected.size());
  check_output(cmp_result, 0, path("@memcmp()"));
}

template<>
void check_output(const types::AccountPermission& expected, const types::AccountPermission& actual, const path_cons_list& path) {
   check_output(expected.account, actual.account, path(".account"));
   check_output(expected.permission, actual.permission, path(".permission"));
}

template<>
void check_output(const types::Message& expected, const types::Message& actual, const path_cons_list& path) {
   check_output(expected.code, actual.code, path(".code"));
   check_output(expected.type, actual.type, path(".type"));
   check_output(expected.authorization, actual.authorization, path(".authorization"));
   check_output(expected.data, actual.data, path(".data"));
}

template<>
void check_output(const MessageOutput& expected, const MessageOutput& actual, const path_cons_list& path);

template<>
void check_output(const NotifyOutput& expected, const NotifyOutput& actual, const path_cons_list& path) {
   check_output(expected.name, actual.name, path(".name"));
   check_output(expected.output, actual.output, path(".output"));
}

template<>
void check_output(const Transaction& expected, const Transaction& actual, const path_cons_list& path) {
   check_output(expected.refBlockNum, actual.refBlockNum, path(".refBlockNum"));
   check_output(expected.refBlockPrefix, actual.refBlockPrefix, path(".refBlockPrefix"));
   check_output(expected.expiration, actual.expiration, path(".expiration"));
   check_output(expected.scope, actual.scope, path(".scope"));
   check_output(expected.messages, actual.messages, path(".messages"));
}


template<>
void check_output(const InlineTransaction& expected, const InlineTransaction& actual, const path_cons_list& path) {
   check_output<Transaction>(expected, actual, path);
   check_output(expected.output, actual.output, path(".output"));
}

template<>
void check_output(const GeneratedTransaction& expected, const GeneratedTransaction& actual, const path_cons_list& path) {
   check_output(expected.id, actual.id, path(".id"));
   check_output<Transaction>(expected, actual, path);
}

template<>
void check_output(const MessageOutput& expected, const MessageOutput& actual, const path_cons_list& path) {
   check_output(expected.notify, actual.notify, path(".notify"));
   check_output(expected.inline_transaction, actual.inline_transaction, path(".inline_transaction"));
   check_output(expected.deferred_transactions, actual.deferred_transactions, path(".deferred_transactions"));
}

template<typename T>
void chain_controller::check_transaction_output(const T& expected, const T& actual, const path_cons_list& path)const {
   if (!(_skip_flags & skip_output_check)) {
      check_output(expected.output, actual.output, path(".output"));
   }
}

void chain_controller::_apply_block(const signed_block& next_block)
{ try {
   uint32_t next_block_num = next_block.block_num();
   uint32_t skip = _skip_flags;

   FC_ASSERT((skip & skip_merkle_check) || next_block.transaction_merkle_root == next_block.calculate_merkle_root(),
             "", ("next_block.transaction_merkle_root", next_block.transaction_merkle_root)
             ("calc",next_block.calculate_merkle_root())("next_block",next_block)("id",next_block.id()));

   const producer_object& signing_producer = validate_block_header(skip, next_block);
   
   for (const auto& cycle : next_block.cycles)
      for (const auto& thread : cycle)
         for (const auto& trx : thread.user_input) {
            validate_referenced_accounts(trx);
            // Check authorization, and allow irrelevant signatures.
            // If the block producer let it slide, we'll roll with it.
            check_transaction_authorization(trx, true);
         }

   /* We do not need to push the undo state for each transaction
    * because they either all apply and are valid or the
    * entire block fails to apply.  We only need an "undo" state
    * for transactions when validating broadcast transactions or
    * when building a block.
    */
   auto root_path = path_cons_list("next_block.cycles");
   for (int c_idx = 0; c_idx < next_block.cycles.size(); c_idx++) {
      const auto& cycle = next_block.cycles.at(c_idx);
      auto c_path = path_cons_list(c_idx, root_path);

      for (int t_idx = 0; t_idx < cycle.size(); t_idx++) {
         const auto& thread = cycle.at(t_idx);
         auto t_path = path_cons_list(t_idx, c_path);

         auto gen_path = path_cons_list(".generated_input", t_path);
         for(int p_idx = 0; p_idx < thread.generated_input.size(); p_idx++ ) {
            const auto& ptrx = thread.generated_input.at(p_idx);
            const auto& trx = get_generated_transaction(ptrx.id);
            auto processed = apply_transaction(trx);
            check_transaction_output(ptrx, processed, gen_path(p_idx));
         }

         auto user_path = path_cons_list(".user_input", t_path);
         for(int p_idx = 0; p_idx < thread.user_input.size(); p_idx++ ) {
            const auto& ptrx = thread.user_input.at(p_idx);
            const SignedTransaction& trx = ptrx;
            auto processed = apply_transaction(trx);
            check_transaction_output(ptrx, processed, user_path(p_idx));
         }
      }
   }

   update_global_properties(next_block);
   update_global_dynamic_data(next_block);
   update_signing_producer(signing_producer, next_block);
   update_last_irreversible_block();

   create_block_summary(next_block);
   clear_expired_transactions();

   // notify observers that the block has been applied
   // TODO: do this outside the write lock...? 
   applied_block( next_block ); //emit

} FC_CAPTURE_AND_RETHROW( (next_block.block_num()) )  }

namespace {

  auto make_get_permission(const chainbase::database& db) {
     return [&db](const types::AccountPermission& permission) {
        auto key = boost::make_tuple(permission.account, permission.permission);
        return db.get<permission_object, by_owner>(key);
     };
  }

  auto make_authority_checker(const chainbase::database& db, const flat_set<public_key_type>& signingKeys) {
     auto getPermission = make_get_permission(db);
     auto getAuthority = [getPermission](const types::AccountPermission& permission) {
        return getPermission(permission).auth;
     };
     auto depthLimit = db.get<global_property_object>().configuration.authDepthLimit;
     return MakeAuthorityChecker(std::move(getAuthority), depthLimit, signingKeys);
  }

}

flat_set<public_key_type> chain_controller::get_required_keys(const SignedTransaction& trx, const flat_set<public_key_type>& candidateKeys)const {
   auto checker = make_authority_checker(_db, candidateKeys);

   for (const auto& message : trx.messages) {
      for (const auto& declaredAuthority : message.authorization) {
         if (!checker.satisfied(declaredAuthority)) {
            EOS_ASSERT(checker.satisfied(declaredAuthority), tx_missing_sigs,
                       "Transaction declares authority '${auth}', but does not have signatures for it.",
                       ("auth", declaredAuthority));
         }
      }
   }

   return checker.used_keys();
}

void chain_controller::check_transaction_authorization(const SignedTransaction& trx, bool allow_unused_signatures)const {
   if ((_skip_flags & skip_transaction_signatures) && (_skip_flags & skip_authority_check)) {
      //ilog("Skipping auth and sigs checks");
      return;
   }

   auto getPermission = make_get_permission(_db);
#warning TODO: Use a real chain_id here (where is this stored? Do we still need it?)
   auto checker = make_authority_checker(_db, trx.get_signature_keys(chain_id_type{}));

   for (const auto& message : trx.messages)
      for (const auto& declaredAuthority : message.authorization) {
         const auto& minimumPermission = lookup_minimum_permission(declaredAuthority.account,
                                                                   message.code, message.type);
         if ((_skip_flags & skip_authority_check) == false) {
            const auto& index = _db.get_index<permission_index>().indices();
            EOS_ASSERT(getPermission(declaredAuthority).satisfies(minimumPermission, index), tx_irrelevant_auth,
                       "Message declares irrelevant authority '${auth}'; minimum authority is ${min}",
                       ("auth", declaredAuthority)("min", minimumPermission.name));
         }
         if ((_skip_flags & skip_transaction_signatures) == false) {
            EOS_ASSERT(checker.satisfied(declaredAuthority), tx_missing_sigs,
                       "Transaction declares authority '${auth}', but does not have signatures for it.",
                       ("auth", declaredAuthority));
         }
      }

   if (!allow_unused_signatures && (_skip_flags & skip_transaction_signatures) == false)
      EOS_ASSERT(checker.all_keys_used(), tx_irrelevant_sig,
                 "Transaction bears irrelevant signatures from these keys: ${keys}", ("keys", checker.unused_keys()));
}

void chain_controller::validate_scope( const Transaction& trx )const {
   EOS_ASSERT(trx.scope.size() + trx.readscope.size() > 0, transaction_exception, "No scope specified by transaction" );
   for( uint32_t i = 1; i < trx.scope.size(); ++i )
      EOS_ASSERT( trx.scope[i-1] < trx.scope[i], transaction_exception, "Scopes must be sorted and unique" );
   for( uint32_t i = 1; i < trx.readscope.size(); ++i )
      EOS_ASSERT( trx.readscope[i-1] < trx.readscope[i], transaction_exception, "Scopes must be sorted and unique" );

   vector<types::AccountName> intersection;
   std::set_intersection( trx.scope.begin(), trx.scope.end(),
                          trx.readscope.begin(), trx.readscope.end(),
                          std::back_inserter(intersection) );
   FC_ASSERT( intersection.size() == 0, "a transaction may not redeclare scope in readscope" );
}

const permission_object& chain_controller::lookup_minimum_permission(types::AccountName authorizer_account,
                                                                    types::AccountName code_account,
                                                                    types::FuncName type) const {
   try {
      // First look up a specific link for this message type
      auto key = boost::make_tuple(authorizer_account, code_account, type);
      auto link = _db.find<permission_link_object, by_message_type>(key);
      // If no specific link found, check for a contract-wide default
      if (link == nullptr) {
         get<2>(key) = "";
         link = _db.find<permission_link_object, by_message_type>(key);
      }

      // If no specific or default link found, use active permission
      auto permissionKey = boost::make_tuple<AccountName, PermissionName>(authorizer_account, "active");
      if (link != nullptr)
         get<1>(permissionKey) = link->required_permission;
      return _db.get<permission_object, by_owner>(permissionKey);
   } FC_CAPTURE_AND_RETHROW((authorizer_account)(code_account)(type))
}

void chain_controller::validate_uniqueness( const SignedTransaction& trx )const {
   if( !should_check_for_duplicate_transactions() ) return;

   auto transaction = _db.find<transaction_object, by_trx_id>(trx.id());
   EOS_ASSERT(transaction == nullptr, tx_duplicate, "Transaction is not unique");
}

void chain_controller::validate_uniqueness( const GeneratedTransaction& trx )const {
   if( !should_check_for_duplicate_transactions() ) return;
}

void chain_controller::record_transaction(const SignedTransaction& trx) {
   //Insert transaction into unique transactions database.
    _db.create<transaction_object>([&](transaction_object& transaction) {
        transaction.trx_id = trx.id(); /// TODO: consider caching ID
        transaction.trx = trx;
    });
}

void chain_controller::record_transaction(const GeneratedTransaction& trx) {
   _db.modify( _db.get<generated_transaction_object,generated_transaction_object::by_trx_id>(trx.id), [&](generated_transaction_object& transaction) {
      transaction.status = generated_transaction_object::PROCESSED;
   });
}     



void chain_controller::validate_tapos(const Transaction& trx)const {
   if (!should_check_tapos()) return;

   const auto& tapos_block_summary = _db.get<block_summary_object>((uint16_t)trx.refBlockNum);

   //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
   EOS_ASSERT(transaction_verify_reference_block(trx, tapos_block_summary.block_id), transaction_exception,
              "Transaction's reference block did not match. Is this transaction from a different fork?",
              ("tapos_summary", tapos_block_summary));
}

void chain_controller::validate_referenced_accounts(const Transaction& trx)const {
   for (const auto& scope : trx.scope)
      require_account(scope);
   for (const auto& msg : trx.messages) {
      require_account(msg.code);
      for (const auto& auth : msg.authorization)
         require_account(auth.account);
   }
}

void chain_controller::validate_expiration(const Transaction& trx) const
{ try {
   fc::time_point_sec now = head_block_time();
   const BlockchainConfiguration& chain_configuration = get_global_properties().configuration;

   EOS_ASSERT(trx.expiration <= now + int32_t(chain_configuration.maxTrxLifetime),
              transaction_exception, "Transaction expiration is too far in the future",
              ("trx.expiration",trx.expiration)("now",now)
              ("max_til_exp",chain_configuration.maxTrxLifetime));
   EOS_ASSERT(now <= trx.expiration, transaction_exception, "Transaction is expired",
              ("now",now)("trx.exp",trx.expiration));
} FC_CAPTURE_AND_RETHROW((trx)) }


void chain_controller::process_message(const Transaction& trx, AccountName code,
                                       const Message& message, MessageOutput& output, apply_context* parent_context) {
   apply_context apply_ctx(*this, _db, trx, message, code);
   apply_message(apply_ctx);

   output.notify.reserve( apply_ctx.notified.size() );

   for( uint32_t i = 0; i < apply_ctx.notified.size(); ++i ) {
      try {
         auto notify_code = apply_ctx.notified[i];
         output.notify.push_back( {notify_code} );
         process_message(trx, notify_code, message, output.notify.back().output, &apply_ctx);
      } FC_CAPTURE_AND_RETHROW((apply_ctx.notified[i]))
   }

   // combine inline messages and process
   if (apply_ctx.inline_messages.size() > 0) {
      output.inline_transaction = InlineTransaction(trx);
      (*output.inline_transaction).messages = std::move(apply_ctx.inline_messages);
   }

   for( auto& asynctrx : apply_ctx.deferred_transactions ) {
      digest_type::encoder enc;
      fc::raw::pack( enc, trx );
      fc::raw::pack( enc, asynctrx );
      auto id = enc.result();
      auto gtrx = GeneratedTransaction(id, asynctrx);

      _db.create<generated_transaction_object>([&](generated_transaction_object& transaction) {
         transaction.trx = gtrx;
         transaction.status = generated_transaction_object::PENDING;
      });

      output.deferred_transactions.emplace_back( gtrx );
   }

   // propagate used_authorizations up the context chain
   if (parent_context != nullptr)
      for (int i = 0; i < apply_ctx.used_authorizations.size(); ++i)
         if (apply_ctx.used_authorizations[i])
            parent_context->used_authorizations[i] = true;

   // process_message recurses for each notified account, but we only want to run this check at the top level
   if (parent_context == nullptr && (_skip_flags & skip_authority_check) == false)
      EOS_ASSERT(apply_ctx.all_authorizations_used(), tx_irrelevant_auth,
                 "Message declared authorities it did not need: ${unused}",
                 ("unused", apply_ctx.unused_authorizations())("message", message));
}

void chain_controller::apply_message(apply_context& context)
{ try {
    /// context.code => the execution namespace
    /// message.code / message.type => Event
    const auto& m = context.msg;
    auto contract_handlers_itr = apply_handlers.find(context.code);
    if (contract_handlers_itr != apply_handlers.end()) {
       auto message_handler_itr = contract_handlers_itr->second.find({m.code, m.type});
       if (message_handler_itr != contract_handlers_itr->second.end()) {
          message_handler_itr->second(context);
          return;
       }
    }
    const auto& recipient = _db.get<account_object,by_name>(context.code);
    if (recipient.code.size()) {
       //idump((context.code)(context.msg.type));
       wasm_interface::get().apply(context);
    }

} FC_CAPTURE_AND_RETHROW((context.msg)) }

template<typename T>
typename T::Processed chain_controller::apply_transaction(const T& trx)
{ try {
   validate_transaction(trx);
   record_transaction(trx);
   return process_transaction( trx, 0, fc::time_point::now());

} FC_CAPTURE_AND_RETHROW((trx)) }

/**
 *  @pre the transaction is assumed valid and all signatures / duplicate checks have bee performed
 */
template<typename T>
typename T::Processed chain_controller::process_transaction( const T& trx, int depth, const fc::time_point& start_time ) 
{ try {
   const BlockchainConfiguration& chain_configuration = get_global_properties().configuration;
   EOS_ASSERT((fc::time_point::now() - start_time).count() < chain_configuration.maxTrxRuntime, checktime_exceeded,
      "Transaction exceeded maximum total transaction time of ${limit}ms", ("limit", chain_configuration.maxTrxRuntime));

   EOS_ASSERT(depth < chain_configuration.inlineDepthLimit, tx_resource_exhausted,
      "Transaction exceeded maximum inline recursion depth of ${limit}", ("limit", chain_configuration.inlineDepthLimit));

   typename T::Processed ptrx( trx );
   ptrx.output.resize( trx.messages.size() );

   for( uint32_t i = 0; i < trx.messages.size(); ++i ) {
      auto& output = ptrx.output[i];
      process_message(trx, trx.messages[i].code, trx.messages[i], output);
      if (output.inline_transaction.valid() ) {
         const Transaction& trx = *output.inline_transaction;
         output.inline_transaction = process_transaction(PendingInlineTransaction(trx), depth + 1, start_time);
      }
   }

   return ptrx;
} FC_CAPTURE_AND_RETHROW( (trx) ) }

void chain_controller::require_account(const types::AccountName& name) const {
   auto account = _db.find<account_object, by_name>(name);
   FC_ASSERT(account != nullptr, "Account not found: ${name}", ("name", name));
}

const producer_object& chain_controller::validate_block_header(uint32_t skip, const signed_block& next_block)const {
   EOS_ASSERT(head_block_id() == next_block.previous, block_validate_exception, "",
              ("head_block_id",head_block_id())("next.prev",next_block.previous));
   EOS_ASSERT(head_block_time() < next_block.timestamp, block_validate_exception, "",
              ("head_block_time",head_block_time())("next",next_block.timestamp)("blocknum",next_block.block_num()));
   if (next_block.block_num() % config::BlocksPerRound != 0) {
      EOS_ASSERT(next_block.producer_changes.empty(), block_validate_exception,
                 "Producer changes may only occur at the end of a round.");
   } else {
      using boost::is_sorted;
      using namespace boost::adaptors;
      EOS_ASSERT(is_sorted(keys(next_block.producer_changes)) && is_sorted(values(next_block.producer_changes)),
                 block_validate_exception, "Producer changes are not sorted correctly",
                 ("changes", next_block.producer_changes));
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

void chain_controller::update_global_properties(const signed_block& b) {
   // If we're at the end of a round, update the BlockchainConfiguration, producer schedule
   // and "producers" special account authority
   if (b.block_num() % config::BlocksPerRound == 0) {
      auto schedule = calculate_next_round(b);
      auto config = _admin->get_blockchain_configuration(_db, schedule);

      const auto& gpo = get_global_properties();
      _db.modify(gpo, [schedule = std::move(schedule), config = std::move(config)] (global_property_object& gpo) {
         gpo.active_producers = std::move(schedule);
         gpo.configuration = std::move(config);
      });

      auto active_producers_authority = types::Authority(config::ProducersAuthorityThreshold, {}, {});
      for(auto& name : gpo.active_producers) {
         active_producers_authority.accounts.push_back({{name, config::ActiveName}, 1});
      }

      auto& po = _db.get<permission_object, by_owner>( boost::make_tuple(config::ProducersAccountName, config::ActiveName) );
      _db.modify(po,[active_producers_authority] (permission_object& po) {
         po.auth = active_producers_authority;
      });
   }
}

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

time_point_sec chain_controller::head_block_time()const {
   return get_dynamic_global_properties().time;
}

uint32_t chain_controller::head_block_num()const {
   return get_dynamic_global_properties().head_block_number;
}

block_id_type chain_controller::head_block_id()const {
   return get_dynamic_global_properties().head_block_id;
}

types::AccountName chain_controller::head_block_producer() const {
   auto b = _fork_db.fetch_block(head_block_id());
   if( b ) return b->data.producer;

   if (auto head_block = fetch_block_by_id(head_block_id()))
      return head_block->producer;
   return {};
}

const producer_object& chain_controller::get_producer(const types::AccountName& ownerName) const {
   return _db.get<producer_object, by_owner>(ownerName);
}

uint32_t chain_controller::last_irreversible_block_num() const {
   return get_dynamic_global_properties().last_irreversible_block_num;
}

void chain_controller::initialize_indexes() {
   _db.add_index<account_index>();
   _db.add_index<permission_index>();
   _db.add_index<permission_link_index>();
   _db.add_index<action_permission_index>();
   _db.add_index<key_value_index>();
   _db.add_index<key128x128_value_index>();
   _db.add_index<key64x64x64_value_index>();

   _db.add_index<global_property_multi_index>();
   _db.add_index<dynamic_global_property_multi_index>();
   _db.add_index<block_summary_multi_index>();
   _db.add_index<transaction_multi_index>();
   _db.add_index<generated_transaction_multi_index>();
   _db.add_index<producer_multi_index>();
}

void chain_controller::initialize_chain(chain_initializer_interface& starter)
{ try {
   if (!_db.find<global_property_object>()) {
      _db.with_write_lock([this, &starter] {
         auto initial_timestamp = starter.get_chain_start_time();
         FC_ASSERT(initial_timestamp != time_point_sec(), "Must initialize genesis timestamp." );
         FC_ASSERT(initial_timestamp.sec_since_epoch() % config::BlockIntervalSeconds == 0,
                    "Genesis timestamp must be divisible by config::BlockIntervalSeconds." );

         // Create global properties
         _db.create<global_property_object>([&starter](global_property_object& p) {
            p.configuration = starter.get_chain_start_configuration();
            p.active_producers = starter.get_chain_start_producers();
         });
         _db.create<dynamic_global_property_object>([&](dynamic_global_property_object& p) {
            p.time = initial_timestamp;
            p.recent_slots_filled = uint64_t(-1);
         });

         // Initialize block summary index
         for (int i = 0; i < 0x10000; i++)
            _db.create<block_summary_object>([&](block_summary_object&) {});

         auto messages = starter.prepare_database(*this, _db);
         std::for_each(messages.begin(), messages.end(), [&](const Message& m) { 
            MessageOutput output;
            ProcessedTransaction trx; /// dummy tranaction required for scope validation
            std::sort(trx.scope.begin(), trx.scope.end() );
            with_skip_flags(skip_scope_check | skip_transaction_signatures | skip_authority_check, [&](){
               process_message(trx,m.code,m,output); 
            });
         });
      });
   }
} FC_CAPTURE_AND_RETHROW() }

chain_controller::chain_controller(database& database, fork_database& fork_db, block_log& blocklog,
                                   chain_initializer_interface& starter, unique_ptr<chain_administration_interface> admin)
   : _db(database), _fork_db(fork_db), _block_log(blocklog), _admin(std::move(admin)) {

   initialize_indexes();
   starter.register_types(*this, _db);

   // Behave as though we are applying a block during chain initialization (it's the genesis block!)
   with_applying_block([&] {
      initialize_chain(starter);
   });

   spinup_db();
   spinup_fork_db();

   if (_block_log.read_head() && head_block_num() < _block_log.read_head()->block_num())
      replay();
}

chain_controller::~chain_controller() {
   clear_pending();
   _db.flush();
   _fork_db.reset();
}

void chain_controller::replay() {
   ilog("Replaying blockchain");
   auto start = fc::time_point::now();
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
      apply_block(*block, skip_producer_signature |
                          skip_transaction_signatures |
                          skip_transaction_dupe_check |
                          skip_tapos_check |
                          skip_producer_schedule_check |
                          skip_authority_check);
   }
   auto end = fc::time_point::now();
   ilog("Done replaying ${n} blocks, elapsed time: ${t} sec",
        ("n", head_block_num())("t",double((end-start).count())/1000000.0));

   _db.set_revision(head_block_num());
}

void chain_controller::spinup_db() {
   // Rewind the database to the last irreversible block
   _db.with_write_lock([&] {
      _db.undo_all();

      FC_ASSERT(_db.revision() == head_block_num(), "Chainbase revision does not match head block num",
                ("rev", _db.revision())("head_block", head_block_num()));
   });
}

void chain_controller::spinup_fork_db()
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

ProducerRound chain_controller::calculate_next_round(const signed_block& next_block) {
   auto schedule = _admin->get_next_round(_db);
   auto changes = get_global_properties().active_producers - schedule;
   EOS_ASSERT(boost::range::equal(next_block.producer_changes, changes), block_validate_exception,
              "Unexpected round changes in new block header",
              ("expected changes", changes)("block changes", next_block.producer_changes));

   utilities::rand::random rng(next_block.timestamp.sec_since_epoch());
   rng.shuffle(schedule);
   return schedule;
}

void chain_controller::update_global_dynamic_data(const signed_block& b) {
   const dynamic_global_property_object& _dgp = _db.get<dynamic_global_property_object>();

   uint32_t missed_blocks = head_block_num() == 0? 1 : get_slot_at_time(b.timestamp);
   assert(missed_blocks != 0);
   missed_blocks--;

//   if (missed_blocks)
//      wlog("Blockchain continuing after gap of ${b} missed blocks", ("b", missed_blocks));

   for(uint32_t i = 0; i < missed_blocks; ++i) {
      const auto& producer_missed = get_producer(get_scheduled_producer(i+1));
      if(producer_missed.owner != b.producer) {
         /*
         const auto& producer_account = producer_missed.producer_account(*this);
         if( (fc::time_point::now() - b.timestamp) < fc::seconds(30) )
            wlog( "Producer ${name} missed block ${n} around ${t}", ("name",producer_account.name)("n",b.block_num())("t",b.timestamp) );
            */

         _db.modify( producer_missed, [&]( producer_object& w ) {
           w.total_missed++;
         });
      }
   }

   // dynamic global properties updating
   _db.modify( _dgp, [&]( dynamic_global_property_object& dgp ){
      dgp.head_block_number = b.block_num();
      dgp.head_block_id = b.id();
      dgp.time = b.timestamp;
      dgp.current_producer = b.producer;
      dgp.current_absolute_slot += missed_blocks+1;

      // If we've missed more blocks than the bitmap stores, skip calculations and simply reset the bitmap
      if (missed_blocks < sizeof(dgp.recent_slots_filled) * 8) {
         dgp.recent_slots_filled <<= 1;
         dgp.recent_slots_filled += 1;
         dgp.recent_slots_filled <<= missed_blocks;
      } else
         dgp.recent_slots_filled = 0;
   });

   _fork_db.set_max_size( _dgp.head_block_number - _dgp.last_irreversible_block_num + 1 );
}

void chain_controller::update_signing_producer(const producer_object& signing_producer, const signed_block& new_block)
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t new_block_aslot = dpo.current_absolute_slot + get_slot_at_time( new_block.timestamp );

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
   producer_objs.reserve(gpo.active_producers.size());
   std::transform(gpo.active_producers.begin(), gpo.active_producers.end(), std::back_inserter(producer_objs),
                  [this](const AccountName& owner) { return &get_producer(owner); });

   static_assert(config::IrreversibleThresholdPercent > 0, "irreversible threshold must be nonzero");

   size_t offset = EOS_PERCENT(producer_objs.size(), config::Percent100 - config::IrreversibleThresholdPercent);
   std::nth_element(producer_objs.begin(), producer_objs.begin() + offset, producer_objs.end(),
      [](const producer_object* a, const producer_object* b) {
         return a->last_confirmed_block_num < b->last_confirmed_block_num;
      });

   uint32_t new_last_irreversible_block_num = producer_objs[offset]->last_confirmed_block_num;

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

   if (last_block_on_disk < new_last_irreversible_block_num)
      for (auto block_to_write = last_block_on_disk + 1;
           block_to_write <= new_last_irreversible_block_num;
           ++block_to_write) {
         auto block = fetch_block_by_number(block_to_write);
         assert(block);
         _block_log.append(*block);
      }

   // Trim fork_database and undo histories
   _fork_db.set_max_size(head_block_num() - new_last_irreversible_block_num + 1);
   _db.commit(new_last_irreversible_block_num);
}

void chain_controller::clear_expired_transactions()
{ try {
   //Look for expired transactions in the deduplication list, and remove them.
   //Transactions must have expired by at least two forking windows in order to be removed.
   auto& transaction_idx = _db.get_mutable_index<transaction_multi_index>();
   const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
   while( (!dedupe_index.empty()) && (head_block_time() > dedupe_index.rbegin()->trx.expiration) )
      transaction_idx.remove(*dedupe_index.rbegin());

   //Look for expired transactions in the pending generated list, and remove them.
   //Transactions must have expired by at least two forking windows in order to be removed.
   auto& generated_transaction_idx = _db.get_mutable_index<generated_transaction_multi_index>();
   const auto& generated_index = generated_transaction_idx.indices().get<generated_transaction_object::by_expiration>();
   while( (!generated_index.empty()) && (head_block_time() > generated_index.rbegin()->trx.expiration) )
      generated_transaction_idx.remove(*generated_index.rbegin());
} FC_CAPTURE_AND_RETHROW() }

using boost::container::flat_set;

types::AccountName chain_controller::get_scheduled_producer(uint32_t slot_num)const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t current_aslot = dpo.current_absolute_slot + slot_num;
   const auto& gpo = _db.get<global_property_object>();
   return gpo.active_producers[current_aslot % gpo.active_producers.size()];
}

fc::time_point_sec chain_controller::get_slot_time(uint32_t slot_num)const
{
   if( slot_num == 0 )
      return fc::time_point_sec();

   auto interval = block_interval();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   if( head_block_num() == 0 )
   {
      // n.b. first block is at genesis_time plus one block interval
      fc::time_point_sec genesis_time = dpo.time;
      return genesis_time + slot_num * interval;
   }

   int64_t head_block_abs_slot = head_block_time().sec_since_epoch() / interval;
   fc::time_point_sec head_slot_time(head_block_abs_slot * interval);

   return head_slot_time + (slot_num * interval);
}

uint32_t chain_controller::get_slot_at_time(fc::time_point_sec when)const
{
   fc::time_point_sec first_slot_time = get_slot_time( 1 );
   if( when < first_slot_time )
      return 0;
   return (when - first_slot_time).to_seconds() / block_interval() + 1;
}

uint32_t chain_controller::producer_participation_rate()const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   return uint64_t(config::Percent100) * __builtin_popcountll(dpo.recent_slots_filled) / 64;
}

void chain_controller::set_apply_handler( const AccountName& contract, const AccountName& scope, const ActionName& action, apply_handler v ) {
   apply_handlers[contract][std::make_pair(scope,action)] = v;
}

chain_initializer_interface::~chain_initializer_interface() {}


ProcessedTransaction chain_controller::transaction_from_variant( const fc::variant& v )const {
   const variant_object& vo = v.get_object();
#define GET_FIELD( VO, FIELD, RESULT ) \
   if( VO.contains(#FIELD) ) fc::from_variant( VO[#FIELD], RESULT.FIELD )

   ProcessedTransaction result;
   GET_FIELD( vo, refBlockNum, result );
   GET_FIELD( vo, refBlockPrefix, result );
   GET_FIELD( vo, expiration, result );
   GET_FIELD( vo, scope, result );
   GET_FIELD( vo, signatures, result );

   if( vo.contains( "messages" ) ) {
      const vector<variant>& msgs = vo["messages"].get_array();
      result.messages.resize( msgs.size() );
      for( uint32_t i = 0; i <  msgs.size(); ++i ) {
         const auto& vo = msgs[i].get_object();
         GET_FIELD( vo, code, result.messages[i] );
         GET_FIELD( vo, type, result.messages[i] );
         GET_FIELD( vo, authorization, result.messages[i] );

         if( vo.contains( "data" ) ) {
            const auto& data = vo["data"];
            if( data.is_string() ) {
               GET_FIELD( vo, data, result.messages[i] );
            } else if ( data.is_object() ) {
               result.messages[i].data = message_to_binary( result.messages[i].code, result.messages[i].type, data ); 
               /*
               const auto& code_account = _db.get<account_object,by_name>( result.messages[i].code );
               if( code_account.abi.size() > 4 ) { /// 4 == packsize of empty Abi
                  fc::datastream<const char*> ds( code_account.abi.data(), code_account.abi.size() );
                  eos::types::Abi abi;
                  fc::raw::unpack( ds, abi );
                  types::AbiSerializer abis( abi );
                  result.messages[i].data = abis.variantToBinary( abis.getActionType( result.messages[i].type ), data );
               }
               */
            }
         }
      }
   }
   if( vo.contains( "output" ) ) {
      const vector<variant>& outputs = vo["output"].get_array();
   }
   return result;
#undef GET_FIELD
}

vector<char> chain_controller::message_to_binary( Name code, Name type, const fc::variant& obj )const 
{ try {
   const auto& code_account = _db.get<account_object,by_name>( code );
   if( code_account.abi.size() > 4 ) { /// 4 == packsize of empty Abi
      fc::datastream<const char*> ds( code_account.abi.data(), code_account.abi.size() );
      eos::types::Abi abi;
      fc::raw::unpack( ds, abi );
      types::AbiSerializer abis( abi );
      return abis.variantToBinary( abis.getActionType( type ), obj );
   }
   return vector<char>();
} FC_CAPTURE_AND_RETHROW( (code)(type)(obj) ) }
fc::variant chain_controller::message_from_binary( Name code, Name type, const vector<char>& data )const {
   const auto& code_account = _db.get<account_object,by_name>( code );
   if( code_account.abi.size() > 4 ) { /// 4 == packsize of empty Abi
      fc::datastream<const char*> ds( code_account.abi.data(), code_account.abi.size() );
      eos::types::Abi abi;
      fc::raw::unpack( ds, abi );
      types::AbiSerializer abis( abi );
      return abis.binaryToVariant( abis.getActionType( type ), data );
   }
   return fc::variant();
}

fc::variant  chain_controller::transaction_to_variant( const ProcessedTransaction& trx )const {
#define SET_FIELD( MVO, OBJ, FIELD ) MVO(#FIELD, OBJ.FIELD)

    fc::mutable_variant_object trx_mvo;
    SET_FIELD( trx_mvo, trx, refBlockNum );
    SET_FIELD( trx_mvo, trx, refBlockPrefix );
    SET_FIELD( trx_mvo, trx, expiration );
    SET_FIELD( trx_mvo, trx, scope );
    SET_FIELD( trx_mvo, trx, signatures );

    vector<fc::mutable_variant_object> msgs( trx.messages.size() );
    vector<fc::variant> msgsv(msgs.size());

    for( uint32_t i = 0; i < trx.messages.size(); ++i ) {
       auto& msg_mvo = msgs[i];
       auto& msg     = trx.messages[i];
       SET_FIELD( msg_mvo, msg, code );
       SET_FIELD( msg_mvo, msg, type );
       SET_FIELD( msg_mvo, msg, authorization );

       const auto& code_account = _db.get<account_object,by_name>( msg.code );
       if( code_account.abi.size() > 4 ) { /// 4 == packsize of empty Abi
          try {
             msg_mvo( "data", message_from_binary( msg.code, msg.type, msg.data ) ); 
             msg_mvo( "hex_data", msg.data );
          } catch ( ... ) {
            SET_FIELD( msg_mvo, msg, data );
          }
       }
       else {
         SET_FIELD( msg_mvo, msg, data );
       }
       msgsv[i] = std::move( msgs[i] );
    }
    trx_mvo( "messages", std::move(msgsv) );

    /* TODO: recursively process generated transactions 
    vector<fc::mutable_variant_object> outs( trx.messages.size() );
    for( uint32_t i = 0; i < trx.output.size(); ++i ) {
       auto& out_mvo = outs[i];
       auto& out = trx.outputs[i];
    }
    */
    trx_mvo( "output", fc::variant( trx.output ) );

    return fc::variant( std::move( trx_mvo ) );
#undef SET_FIELD
}


} }
