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
#include <eos/chain/exceptions.hpp>

#include <eos/chain/block_summary_object.hpp>
#include <eos/chain/global_property_object.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/action_objects.hpp>
#include <eos/chain/transaction_object.hpp>
#include <eos/chain/producer_object.hpp>

#include <eos/chain/wasm_interface.hpp>

#include <eos/types/native.hpp>
#include <eos/types/generated.hpp>

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

//#include <Wren++.h>

namespace eos { namespace chain {


String apply_context::get( String key )const {
   /*
   const auto& obj = db.get<key_value_object,by_scope_key>( boost::make_tuple(scope, key) );
   return String(obj.value.begin(),obj.value.end());
   */
   return String();
}
void apply_context::set( String key, String value ) {
   /*
   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(scope, key) );
   if( obj ) {
      mutable_db.modify( *obj, [&]( auto& o ) {
         o.value.resize( value.size() );
        // memcpy( o.value.data(), value.data(), value.size() );
      });
   } else {
      mutable_db.create<key_value_object>( [&](auto& o) {
         o.scope = scope;
         o.key.insert( 0, key.data(), key.size() );
         o.value.insert( 0, value.data(), value.size() );
      });
   }
   */
}
void apply_context::remove( String key ) {
   /*
   const auto* obj = db.find<key_value_object,by_scope_key>( boost::make_tuple(scope, key) );
   if( obj ) {
      mutable_db.remove( *obj );
   }
   */
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
      apply_block(new_block, skip);
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
void chain_controller::push_transaction(const SignedTransaction& trx, uint32_t skip)
{ try {
   with_skip_flags(skip, [&]() {
      _db.with_write_lock([&]() {
         _push_transaction(trx);
      });
   });
} FC_CAPTURE_AND_RETHROW((trx)) }

void chain_controller::_push_transaction(const SignedTransaction& trx) {
   // If this is the first transaction pushed after applying a block, start a new undo session.
   // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
   if (!_pending_tx_session.valid())
      _pending_tx_session = _db.start_undo_session(true);

   auto temp_session = _db.start_undo_session(true);
   _apply_transaction(trx);
   _pending_transactions.push_back(trx);

   // notify_changed_objects();
   // The transaction applied successfully. Merge its changes into the pending block session.
   temp_session.squash();

   // notify anyone listening to pending transactions
   on_pending_transaction(trx);
}


signed_block chain_controller::generate_block(
   fc::time_point_sec when,
   const AccountName& producer,
   const fc::ecc::private_key& block_signing_private_key,
   uint32_t skip /* = 0 */
   )
{ try {
   return with_skip_flags( skip, [&](){
      auto b = _db.with_write_lock( [&](){
         return _generate_block( when, producer, block_signing_private_key );
      });
      push_block(b, skip);
      return b;
   });
} FC_CAPTURE_AND_RETHROW( (when) ) }

signed_block chain_controller::_generate_block(
   fc::time_point_sec when,
   const AccountName& producer,
   const fc::ecc::private_key& block_signing_private_key
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

   static const size_t max_block_header_size = fc::raw::pack_size( signed_block_header() ) + 4;
   auto maximum_block_size = get_global_properties().configuration.maxBlockSize;
   size_t total_block_size = max_block_header_size;

   signed_block pending_block;

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

   uint64_t postponed_tx_count = 0;
   // pop pending state (reset to head block state)
   for( const SignedTransaction& tx : _pending_transactions )
   {
      size_t new_total_size = total_block_size + fc::raw::pack_size( tx );

      // postpone transaction if it would make block too big
      if( new_total_size >= maximum_block_size )
      {
         postponed_tx_count++;
         continue;
      }

      try
      {
         auto temp_session = _db.start_undo_session(true);
         _apply_transaction(tx);
         temp_session.squash();

         total_block_size += fc::raw::pack_size(tx);
         if (pending_block.cycles.empty()) {
            pending_block.cycles.resize(1);
            pending_block.cycles.back().resize(1);
         }
         pending_block.cycles.back().back().user_input.emplace_back(tx);
#warning TODO: Populate generated blocks with generated transactions
      }
      catch ( const fc::exception& e )
      {
         // Do nothing, transaction will not be re-applied
         wlog( "Transaction was not processed while generating block due to ${e}", ("e", e) );
         wlog( "The transaction was ${t}", ("t", tx) );
      }
   }
   if( postponed_tx_count > 0 )
   {
      wlog( "Postponed ${n} transactions due to block size limit", ("n", postponed_tx_count) );
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
   with_skip_flags(skip, [&](){ _apply_block(next_block); });
}

void chain_controller::_apply_block(const signed_block& next_block)
{ try {
   uint32_t next_block_num = next_block.block_num();
   uint32_t skip = _skip_flags;

   FC_ASSERT((skip & skip_merkle_check) || next_block.transaction_merkle_root == next_block.calculate_merkle_root(),
             "", ("next_block.transaction_merkle_root", next_block.transaction_merkle_root)
             ("calc",next_block.calculate_merkle_root())("next_block",next_block)("id",next_block.id()));

   const producer_object& signing_producer = validate_block_header(skip, next_block);

   /* We do not need to push the undo state for each transaction
    * because they either all apply and are valid or the
    * entire block fails to apply.  We only need an "undo" state
    * for transactions when validating broadcast transactions or
    * when building a block.
    */
   for (const auto& cycle : next_block.cycles) {
      for (const auto& thread : cycle) {
         for(const auto& trx : thread.generated_input ) {
#warning TODO: Process generated transaction
         }
         for(const auto& trx : thread.user_input ) {
            apply_transaction(trx);
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

void chain_controller::apply_transaction(const SignedTransaction& trx, uint32_t skip)
{
   with_skip_flags( skip, [&]() { _apply_transaction(trx); });
}

void chain_controller::validate_transaction(const SignedTransaction& trx)const {
try {
   EOS_ASSERT(trx.messages.size() > 0, transaction_exception, "A transaction must have at least one message");

   validate_uniqueness(trx);
   validate_tapos(trx);
   validate_referenced_accounts(trx);
   validate_expiration(trx);

   for (const auto& tm : trx.messages) { /// TODO: this loop can be processed in parallel
      const Message* m = reinterpret_cast<const Message*>(&tm); //  m(tm);
      m->for_each_handler( [&]( const AccountName& a ) {

         #warning TODO: call validate handlers on all notified accounts, currently it only calls the recipient's validate

         message_validate_context mvc(_db,trx,*m,a);
         auto contract_handlers_itr = message_validate_handlers.find(a); /// namespace is the notifier
         if (contract_handlers_itr != message_validate_handlers.end()) {
            auto message_handler_itr = contract_handlers_itr->second.find({m->code, m->type});
            if (message_handler_itr != contract_handlers_itr->second.end()) {
               message_handler_itr->second(mvc);
               return;
            }
         }
         const auto& acnt = _db.get<account_object,by_name>( a );
         if( acnt.code.size() ) {
            wasm_interface::get().validate( mvc );
         }
       });
   }

} FC_CAPTURE_AND_RETHROW( (trx) ) }

void chain_controller::validate_uniqueness( const SignedTransaction& trx )const {
   if( !should_check_for_duplicate_transactions() ) return;

   auto transaction = _db.find<transaction_object, by_trx_id>(trx.id());
   EOS_ASSERT(transaction == nullptr, transaction_exception, "Transaction is not unique");
}

void chain_controller::validate_tapos(const SignedTransaction& trx)const {
   if (!should_check_tapos()) return;

   const auto& tapos_block_summary = _db.get<block_summary_object>((uint16_t)trx.refBlockNum);

   //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
   EOS_ASSERT(trx.verify_reference_block(tapos_block_summary.block_id), transaction_exception,
              "Transaction's reference block did not match. Is this transaction from a different fork?",
              ("tapos_summary", tapos_block_summary));
}

void chain_controller::validate_referenced_accounts(const SignedTransaction& trx)const {
   for(const auto& auth : trx.authorizations) {
      require_account(auth.account);
   }
   for(const auto& msg : trx.messages) {
      require_account(msg.code);
      const AccountName* previous_recipient = nullptr;
      for(const auto& current_recipient : msg.recipients) {
         require_account(current_recipient);
         if(previous_recipient) {
            EOS_ASSERT(*previous_recipient < current_recipient, message_validate_exception,
                       "Message recipient accounts out of order. Possibly a bug in the wallet?",
                       ("current", current_recipient.value)("previous", previous_recipient->value));
         }

         EOS_ASSERT(current_recipient != msg.code, message_validate_exception,
                    "Code account is listed among recipients. Possibly a bug in the wallet?");
         previous_recipient = &current_recipient;
      }
   }
}

void chain_controller::validate_expiration(const SignedTransaction& trx) const
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


void chain_controller::validate_message_precondition( precondition_validate_context& context )const 
{ try {
    const auto& m = context.msg;
    auto contract_handlers_itr = precondition_validate_handlers.find(context.code);
    if (contract_handlers_itr != precondition_validate_handlers.end()) {
       auto message_handler_itr = contract_handlers_itr->second.find({m.code, m.type});
       if (message_handler_itr != contract_handlers_itr->second.end()) {
          message_handler_itr->second(context);
          return;
       }
    }
    const auto& recipient = _db.get<account_object,by_name>(context.code);
    if (recipient.code.size()) {
       wasm_interface::get().precondition(context);
    }
} FC_CAPTURE_AND_RETHROW() }


/**
 *  When processing a message it needs to run:
 *
 *  code::precondition( message.code, message.apply )
 *  code::apply( message.code, message.apply )
 *
 *  Then for each recipient it needs to run 
 *
 *  ````
 *   recipient::precondition( message.code, message.apply )
 *   recipient::apply( message.code, message.apply )
 *  ```
 *
 *  The data that can be read is anything declared in trx.scope
 *  The data that can be written is anything stored in  * / [code | recipient] / *
 *
 *  The order of execution of precondition and apply can impact the validity of the
 *  entire message.
 */
void chain_controller::process_message( const Transaction& trx, const Message& message) {
   apply_context apply_ctx(_db, trx, message, message.code);

   /** TODO: pre condition validation and application can occur in parallel */
   /** TODO: verify that message is fully authorized
          (check that @ref SignedTransaction::authorizations are all present) */
   validate_message_precondition(apply_ctx);
   apply_message(apply_ctx);

   for (const auto& recipient : message.recipients) {
      try {
         apply_context recipient_ctx(_db, trx, message, recipient);
         validate_message_precondition(recipient_ctx);
         apply_message(recipient_ctx);
      } FC_CAPTURE_AND_RETHROW((recipient)(message))
   }
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
       wasm_interface::get().apply(context);
    }

} FC_CAPTURE_AND_RETHROW((context.msg)) }

void chain_controller::_apply_transaction(const SignedTransaction& trx)
{ try {
   validate_transaction(trx);

   auto getAuthority = [&db=_db](const types::AccountPermission& permission) {
      auto key = boost::make_tuple(permission.account, permission.permission);
      return db.get<permission_object, by_owner>(key).auth;
   };
#warning TODO: Use a real chain_id here (where is this stored? Do we still need it?)
   auto checker = MakeAuthorityChecker(std::move(getAuthority), trx.get_signature_keys(chain_id_type{}));

   for (const auto& requiredAuthority : trx.authorizations)
      EOS_ASSERT(checker.satisfied(requiredAuthority), tx_missing_auth, "Transaction is not authorized.");

   for (const auto& message : trx.messages) {
      process_message(trx, message);
   }

   //Insert transaction into unique transactions database.
   if (should_check_for_duplicate_transactions())
   {
      _db.create<transaction_object>([&](transaction_object& transaction) {
         transaction.trx_id = trx.id(); /// TODO: consider caching ID
         transaction.trx = trx;
      });
   }
} FC_CAPTURE_AND_RETHROW((trx)) }

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
   // If we're at the end of a round, update the BlockchainConfiguration and producer schedule
   if (b.block_num() % config::BlocksPerRound == 0) {
      auto schedule = calculate_next_round(b);
      auto config = _admin->get_blockchain_configuration(_db, schedule);

      const auto& gpo = get_global_properties();
      _db.modify(gpo, [schedule = std::move(schedule), config = std::move(config)] (global_property_object& gpo) {
         gpo.active_producers = std::move(schedule);
         gpo.configuration = std::move(config);
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
   _db.add_index<action_permission_index>();
   _db.add_index<key_value_index>();

   _db.add_index<global_property_multi_index>();
   _db.add_index<dynamic_global_property_multi_index>();
   _db.add_index<block_summary_multi_index>();
   _db.add_index<transaction_multi_index>();
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
         const Transaction trx; /// dummy tranaction required for scope validation
         std::for_each(messages.begin(), messages.end(), [&](const Message& m) { process_message(trx,m); });
      });
   }
} FC_CAPTURE_AND_RETHROW() }

chain_controller::chain_controller(database& database, fork_database& fork_db, block_log& blocklog,
                                   chain_initializer_interface& starter, unique_ptr<chain_administration_interface> admin)
   : _db(database), _fork_db(fork_db), _block_log(blocklog), _admin(std::move(admin)) {

   initialize_indexes();
   starter.register_types(*this, _db);
   initialize_chain(starter);
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

   ilog("Replaying blocks...");
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

void chain_controller::set_validate_handler( const AccountName& contract, const AccountName& scope, const ActionName& action, message_validate_handler v ) {
   message_validate_handlers[contract][std::make_pair(scope,action)] = v;
}
void chain_controller::set_precondition_validate_handler(  const AccountName& contract, const AccountName& scope, const ActionName& action, precondition_validate_handler v ) {
   precondition_validate_handlers[contract][std::make_pair(scope,action)] = v;
}
void chain_controller::set_apply_handler( const AccountName& contract, const AccountName& scope, const ActionName& action, apply_handler v ) {
   apply_handlers[contract][std::make_pair(scope,action)] = v;
}

chain_initializer_interface::~chain_initializer_interface() {}



} }
