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

#include <eos/chain/database.hpp>
#include <eos/chain/exceptions.hpp>

#include <eos/chain/block_summary_object.hpp>
#include <eos/chain/chain_property_object.hpp>
#include <eos/chain/global_property_object.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/transaction_object.hpp>
#include <eos/chain/producer_object.hpp>

#include <eos/chain/sys_contract.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>
#include <fc/crypto/digest.hpp>
#include <fc/io/fstream.hpp>

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <functional>
#include <iostream>

namespace eos { namespace chain {

bool database::is_known_block(const block_id_type& id)const
{
   return _fork_db.is_known_block(id) || _block_log.read_block_by_id(id);
}
/**
 * Only return true *if* the transaction has not expired or been invalidated. If this
 * method is called with a VERY old transaction we will return false, they should
 * query things by blocks if they are that old.
 */
bool database::is_known_transaction(const transaction_id_type& id)const
{
   const auto& trx_idx = get_index<transaction_multi_index, by_trx_id>();
   return trx_idx.find( id ) != trx_idx.end();
}

block_id_type database::get_block_id_for_num(uint32_t block_num)const
{ try {
   if (const auto& block = fetch_block_by_number(block_num))
      return block->id();

   FC_THROW_EXCEPTION(unknown_block_exception, "Could not find block");
} FC_CAPTURE_AND_RETHROW((block_num)) }

optional<signed_block> database::fetch_block_by_id(const block_id_type& id)const
{
   auto b = _fork_db.fetch_block(id);
   if(b) return b->data;
   return _block_log.read_block_by_id(id);
}

optional<signed_block> database::fetch_block_by_number(uint32_t num)const
{
   if (const auto& block = _block_log.read_block_by_num(num))
      return *block;

   // Not in _block_id_to_block, so it must be since the last irreversible block. Grab it from _fork_db instead
   if (num <= head_block_num()) {
      auto block = _fork_db.head();
      while (block && block->num > num)
         block = block->prev.lock();
      if (block && block->num == num)
         return block->data;
   }

   return optional<signed_block>();
}

const signed_transaction& database::get_recent_transaction(const transaction_id_type& trx_id) const
{
   auto& index = get_index<transaction_multi_index, by_trx_id>();
   auto itr = index.find(trx_id);
   FC_ASSERT(itr != index.end());
   return itr->trx;
}

std::vector<block_id_type> database::get_block_ids_on_fork(block_id_type head_of_fork) const
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
bool database::push_block(const signed_block& new_block, uint32_t skip)
{ try {
   /// TODO: the simulated network code will cause push_block to be called
   /// recursively which in turn will cause the write lock to hang
   if( new_block.block_num() == head_block_num() && new_block.id() == head_block_id() ) 
      return false;

   return with_skip_flags( skip, [&](){ 
      return without_pending_transactions( [&]() {
         return with_write_lock( [&]() { 
            return _push_block(new_block);
         } );
      });
   });
} FC_CAPTURE_AND_RETHROW((new_block)) }

bool database::_push_block(const signed_block& new_block)
{ try {
   uint32_t skip = get_node_properties().skip_flags;
   if (!(skip&skip_fork_db)) {
      /// TODO: if the block is greater than the head block and before the next maitenance interval
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
                   auto session = start_undo_session(true);
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
                      auto session = start_undo_session(true);
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
      auto session = start_undo_session(true);
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
void database::push_transaction(const signed_transaction& trx, uint32_t skip)
{ try {
   with_skip_flags(skip, [&]() {
      with_producing([&](){
         with_write_lock([&]() {
            _push_transaction(trx);
         });
      });
   });
} FC_CAPTURE_AND_RETHROW((trx)) }

void database::_push_transaction(const signed_transaction& trx) {

   // If this is the first transaction pushed after applying a block, start a new undo session.
   // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
   if( !_pending_tx_session.valid() )
      _pending_tx_session = start_undo_session( true );

   auto temp_session = start_undo_session(true);
   _apply_transaction(trx);
   _pending_transactions.push_back(trx);

   // notify_changed_objects();
   // The transaction applied successfully. Merge its changes into the pending block session.
   temp_session.squash();

   // notify anyone listening to pending transactions
   on_pending_transaction(trx);
}


signed_block database::generate_block(
   fc::time_point_sec when,
   producer_id_type producer_id,
   const fc::ecc::private_key& block_signing_private_key,
   uint32_t skip /* = 0 */
   )
{ try {
   return with_producing( [&]() {
      return with_skip_flags( skip, [&](){
         auto b = with_write_lock( [&](){
           return _generate_block( when, producer_id, block_signing_private_key );
         });
         push_block(b);
         return b;
      });
    });
} FC_CAPTURE_AND_RETHROW( (when) ) }

signed_block database::_generate_block(
   fc::time_point_sec when,
   producer_id_type producer_id,
   const fc::ecc::private_key& block_signing_private_key
   )
{
   try {
   uint32_t skip = get_node_properties().skip_flags;
   uint32_t slot_num = get_slot_at_time( when );
   FC_ASSERT( slot_num > 0 );
   producer_id_type scheduled_producer = get_scheduled_producer( slot_num );
   FC_ASSERT( scheduled_producer == producer_id );

   const auto& producer_obj = get(scheduled_producer);

   if( !(skip & skip_producer_signature) )
      FC_ASSERT( producer_obj.signing_key == block_signing_private_key.get_public_key() );

   static const size_t max_block_header_size = fc::raw::pack_size( signed_block_header() ) + 4;
   auto maximum_block_size = get_global_properties().parameters.maximum_block_size;
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
   _pending_tx_session = start_undo_session(true);

   uint64_t postponed_tx_count = 0;
   // pop pending state (reset to head block state)
   for( const signed_transaction& tx : _pending_transactions )
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
         auto temp_session = start_undo_session(true);
         _apply_transaction(tx);
         temp_session.squash();

         total_block_size += fc::raw::pack_size(tx);
//         pending_block.transactions.push_back(tx);
#warning TODO: Populate generated blocks with transactions
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

   pending_block.producer = static_cast<uint16_t>(producer_id._id); //pa.name.c_str(); //producer_id;

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
} FC_CAPTURE_AND_RETHROW( (producer_id) ) }

/**
 * Removes the most recent block from the database and undoes any changes it made.
 */
void database::pop_block()
{ try {
   _pending_tx_session.reset();
   auto head_id = head_block_id();
   optional<signed_block> head_block = fetch_block_by_id( head_id );
   EOS_ASSERT( head_block.valid(), pop_empty_chain, "there are no blocks to pop" );

   _fork_db.pop_block();
   undo();
} FC_CAPTURE_AND_RETHROW() }

void database::clear_pending()
{ try {
   _pending_transactions.clear();
   _pending_tx_session.reset();
} FC_CAPTURE_AND_RETHROW() }

//////////////////// private methods ////////////////////

void database::apply_block(const signed_block& next_block, uint32_t skip)
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


void database::_apply_block(const signed_block& next_block)
{ try {
   uint32_t next_block_num = next_block.block_num();
   uint32_t skip = get_node_properties().skip_flags;

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

   update_global_dynamic_data(next_block);
   update_signing_producer(signing_producer, next_block);
   update_last_irreversible_block();

   create_block_summary(next_block);
   clear_expired_transactions();

   // TODO:  figure out if we could collapse this function into
   // update_global_dynamic_data() as perhaps these methods only need
   // to be called for header validation?
   update_producer_schedule();
   if( !_node_property_object.debug_updates.empty() )
      apply_debug_updates();

   // notify observers that the block has been applied
   // TODO: do this outside the write lock...? 
   applied_block( next_block ); //emit

} FC_CAPTURE_AND_RETHROW( (next_block.block_num()) )  }

void database::apply_transaction(const signed_transaction& trx, uint32_t skip)
{
   with_skip_flags( skip, [&]() { _apply_transaction(trx); });
}

void database::validate_tapos( const signed_transaction& trx )const {
try {
   if( !check_tapos() ) return;

   const chain_parameters& chain_parameters    = get_global_properties().parameters;
   const auto&             tapos_block_summary = get<block_summary_object>(trx.ref_block_num);

   //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
   FC_ASSERT(trx.ref_block_prefix == tapos_block_summary.block_id._hash[1]);

   fc::time_point_sec now = head_block_time();

   FC_ASSERT(trx.expiration <= now + chain_parameters.maximum_time_until_expiration, "",
            ("trx.expiration",trx.expiration)("now",now)("max_til_exp",chain_parameters.maximum_time_until_expiration));
   FC_ASSERT(now <= trx.expiration, "", ("now",now)("trx.exp",trx.expiration));
} FC_CAPTURE_AND_RETHROW( (trx) ) }

void database::validate_uniqueness( const signed_transaction& trx )const {
   if( !check_for_duplicate_transactions() ) return;

   auto trx_id = trx.id();
   auto& trx_idx = get_index<transaction_multi_index>();
   FC_ASSERT( trx_idx.indices().get<by_trx_id>().find(trx_id) == trx_idx.indices().get<by_trx_id>().end());
}

void database::validate_referenced_accounts( const signed_transaction& trx )const {
   for( const auto& auth : trx.provided_authorizations ) {
      get_account( auth.authorizing_account );
   }
   for( const auto& msg  : trx.messages ) {
      get_account( msg.sender );
      get_account( msg.recipient );
      const account_name* prev = nullptr;
      for( const auto& acnt : msg.notify ) {
        get_account( acnt );
        if( prev )
           FC_ASSERT( acnt < *prev );
        FC_ASSERT( acnt != msg.sender );
        FC_ASSERT( acnt != msg.recipient );
        prev = &acnt;
      }
   }
}
void database::validate_transaction( const signed_transaction& trx )const {
try {
  FC_ASSERT( trx.messages.size() > 0, "A transaction must have at least one message" );

  validate_uniqueness( trx );
  validate_tapos( trx );
  validate_referenced_accounts( trx );

  for( const auto& m : trx.messages ) { /// TODO: this loop can be processed in parallel
    message_validate_context mvc( trx, m );
    auto contract_handlers_itr = message_validate_handlers.find( m.recipient );
    if( contract_handlers_itr != message_validate_handlers.end() ) {
       auto message_handelr_itr = contract_handlers_itr->second.find( m.recipient + '/' + m.type );
       if( message_handelr_itr != contract_handlers_itr->second.end() ) {
          message_handelr_itr->second(mvc);
          continue;
       }
    }

    /// TODO: dispatch to script if not handled above
  }
} FC_CAPTURE_AND_RETHROW( (trx) ) }

void database::validate_message_precondition( precondition_validate_context& context )const {
    const auto& m = context.msg;
    auto contract_handlers_itr = precondition_validate_handlers.find( context.receiver );
    if( contract_handlers_itr != precondition_validate_handlers.end() ) {
       auto message_handelr_itr = contract_handlers_itr->second.find( m.recipient + "/" + m.type );
       if( message_handelr_itr != contract_handlers_itr->second.end() ) {
          message_handelr_itr->second(context);
          return;
       }
    }
    /// TODO: dispatch to script if not handled above
}

void database::apply_message( apply_context& context ) {
    const auto& m = context.msg;
    auto contract_handlers_itr = apply_handlers.find( context.receiver );
    if( contract_handlers_itr != apply_handlers.end() ) {
       auto message_handelr_itr = contract_handlers_itr->second.find( m.recipient + "/" + m.type );
       if( message_handelr_itr != contract_handlers_itr->second.end() ) {
          message_handelr_itr->second(context);
          return;
       }
    }
    /// TODO: dispatch to script if not handled above
}


void database::_apply_transaction(const signed_transaction& trx)
{ try {
   validate_transaction( trx );

   for( const auto& m : trx.messages ) {
      apply_context ac( *this, trx, m, m.recipient );

      /** TODO: pre condition validation and application can occur in parallel */
      validate_message_precondition( ac );
      apply_message( ac );

      for( const auto& n : m.notify ) {
         apply_context c( *this, trx, m, n );
         validate_message_precondition( c );
         apply_message( c );
      }

   }

   //Insert transaction into unique transactions database.
   if( check_for_duplicate_transactions() )
   {
      create<transaction_object>([&](transaction_object& transaction) {
         transaction.trx_id = trx.id(); /// TODO: consider caching ID
         transaction.trx = trx;
      });
   }
} FC_CAPTURE_AND_RETHROW((trx)) }

const producer_object& database::validate_block_header(uint32_t skip, const signed_block& next_block)const {
   FC_ASSERT(head_block_id() == next_block.previous, "",
             ("head_block_id",head_block_id())("next.prev",next_block.previous));
   FC_ASSERT(head_block_time() < next_block.timestamp, "",
             ("head_block_time",head_block_time())("next",next_block.timestamp)("blocknum",next_block.block_num()));
   const producer_object& producer = get(get_scheduled_producer(get_slot_at_time(next_block.timestamp)));

   if(!(skip&skip_producer_signature))
      FC_ASSERT(next_block.validate_signee(producer.signing_key),
                "Incorrect block producer key: expected ${e} but got ${a}",
                ("e", producer.signing_key)("a", public_key_type(next_block.signee())));

   if(!(skip&skip_producer_schedule_check)) {
      FC_ASSERT(next_block.producer == producer.id, "Producer produced block at wrong time",
                ("block producer",next_block.producer)("scheduled producer",producer.id));
   }

   return producer;
}

void database::create_block_summary(const signed_block& next_block) {
   auto sid = next_block.block_num() & 0xffff;
   modify( get<block_summary_object,by_id>(sid), [&](block_summary_object& p) {
         p.block_id = next_block.id();
   });
}

void database::add_checkpoints( const flat_map<uint32_t,block_id_type>& checkpts ) {
   for (const auto& i : checkpts)
      _checkpoints[i.first] = i.second;
}

bool database::before_last_checkpoint()const {
   return (_checkpoints.size() > 0) && (_checkpoints.rbegin()->first >= head_block_num());
}

/**
 *  This method dumps the state of the blockchain in a semi-human readable form for the
 *  purpose of tracking down funds and mismatches in currency allocation
 */
void database::debug_dump() {
}

void debug_apply_update(database& db, const fc::variant_object& vo) {
}

void database::apply_debug_updates() {
}

void database::debug_update(const fc::variant_object& update) {
}

const global_property_object& database::get_global_properties()const {
   return get<global_property_object>();
}

const dynamic_global_property_object&database::get_dynamic_global_properties() const {
   return get<dynamic_global_property_object>();
}

time_point_sec database::head_block_time()const {
   return get_dynamic_global_properties().time;
}

uint32_t database::head_block_num()const {
   return get_dynamic_global_properties().head_block_number;
}

block_id_type database::head_block_id()const {
   return get_dynamic_global_properties().head_block_id;
}

producer_id_type database::head_block_producer() const {
   if (auto head_block = fetch_block_by_id(head_block_id()))
      return head_block->producer;
   return {};
}

const chain_id_type& database::get_chain_id()const {
   return get<chain_property_object>().chain_id;
}

const node_property_object& database::get_node_properties()const {
   return _node_property_object;
}

node_property_object& database::node_properties() {
   return _node_property_object;
}

uint32_t database::last_irreversible_block_num() const {
   return get_dynamic_global_properties().last_irreversible_block_num;
}

void database::initialize_indexes() {
   add_index<account_index>();
   add_index<permission_index>();
   add_index<action_code_index>();
   add_index<action_permission_index>();

   add_index<global_property_multi_index>();
   add_index<dynamic_global_property_multi_index>();
   add_index<block_summary_multi_index>();
   add_index<transaction_multi_index>();
   add_index<producer_multi_index>();
   add_index<chain_property_multi_index>();
}

void database::init_genesis(const genesis_state_type& genesis_state)
{ try {
   set_validate_handler( "sys", "sys/Transfer", [&]( message_validate_context& context ) {
       idump((context.msg));
       auto transfer = context.msg.as<Transfer>();
       FC_ASSERT( context.msg.has_notify( transfer.to ), "Must notify recipient of transfer" );
   });

   set_precondition_validate_handler( "sys", "sys/Transfer", [&]( precondition_validate_context& context ) {
       idump((context.msg)(context.receiver));
       auto transfer = context.msg.as<Transfer>();
       const auto& from = get_account( transfer.from );
       FC_ASSERT( from.balance > transfer.amount, "Insufficient Funds", 
                  ("from.balance",from.balance)("transfer.amount",transfer.amount) );
   });

   set_apply_handler( "sys", "sys/Transfer", [&]( apply_context& context ) {
       idump((context.msg)(context.receiver));
       auto transfer = context.msg.as<Transfer>();
       const auto& from = get_account( transfer.from );
       const auto& to   = get_account( transfer.to   );
       modify( from, [&]( account_object& a ) {
          a.balance -= transfer.amount;
       });
       modify( to, [&]( account_object& a ) {
          a.balance += transfer.amount;
       });
   });

   FC_ASSERT( genesis_state.initial_timestamp != time_point_sec(), "Must initialize genesis timestamp." );
   FC_ASSERT( genesis_state.initial_timestamp.sec_since_epoch() % config::BlockIntervalSeconds == 0,
              "Genesis timestamp must be divisible by config::BlockIntervalSeconds." );

   struct auth_inhibitor {
      auth_inhibitor(database& db) : db(db), old_flags(db.node_properties().skip_flags)
      { db.node_properties().skip_flags |= skip_authority_check; }
      ~auth_inhibitor()
      { db.node_properties().skip_flags = old_flags; }
   private:
      database& db;
      uint32_t old_flags;
   } inhibitor(*this);


   /// create the system contract
   create<account_object>([&](account_object& a) {
      a.name = "sys";
   });

   // Create initial accounts
   for (const auto& acct : genesis_state.initial_accounts) {
      create<account_object>([&acct](account_object& a) {
         a.name = acct.name.c_str();
         a.balance = acct.balance;
         idump((acct.name)(a.balance));
//         a.active_key = acct.active_key;
//         a.owner_key = acct.owner_key;
      });
   }
   // Create initial producers
   std::vector<producer_id_type> initial_producers;
   for (const auto& producer : genesis_state.initial_producers) {
      const auto& owner = get<account_object, by_name>(producer.owner_name);
      auto id = create<producer_object>([&](producer_object& w) {
         w.signing_key = producer.block_signing_key;
         w.owner = owner.id;
      }).id;
      initial_producers.push_back(id);
   }

   // Initialize block summary index
#warning TODO: Figure out how to do this

   chain_id_type chain_id = genesis_state.compute_chain_id();

   // Create global properties
   create<global_property_object>([&](global_property_object& p) {
       p.parameters = genesis_state.initial_parameters;
       std::copy(initial_producers.begin(), initial_producers.end(), p.active_producers.begin());
   });
   create<dynamic_global_property_object>([&](dynamic_global_property_object& p) {
      p.time = genesis_state.initial_timestamp;
      p.recent_slots_filled = uint64_t(-1);
   });

   FC_ASSERT( (genesis_state.immutable_parameters.min_producer_count & 1) == 1, "min_producer_count must be odd" );

   create<chain_property_object>([&](chain_property_object& p)
   {
      p.chain_id = chain_id;
      p.immutable_parameters = genesis_state.immutable_parameters;
   } );

   for( int i = 0; i < 0x10000; i++ )
      create< block_summary_object >( [&]( block_summary_object& ) {});

} FC_CAPTURE_AND_RETHROW() }

database::database()
{}

database::~database()
{
   close();
//   clear_pending();
}

void database::replay(fc::path data_dir, uint64_t shared_file_size, const genesis_state_type& initial_allocation)
{ try {
   ilog("Replaying blockchain");
   wipe(data_dir, false);
   open(data_dir, shared_file_size, [&initial_allocation]{return initial_allocation;});

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

   set_revision(head_block_num());
} FC_CAPTURE_AND_RETHROW((data_dir)) }

void database::wipe(const fc::path& data_dir, bool include_blocks)
{
   ilog("Wiping database", ("include_blocks", include_blocks));
   close();
   chainbase::database::wipe(data_dir);
   if(include_blocks) {
      fc::remove_all(data_dir / "blocks.log");
      fc::remove_all(data_dir / "blocks.log.index");
   }
}

void database::open(const fc::path& data_dir, uint64_t shared_file_size,
                    std::function<genesis_state_type()> genesis_loader)
{ try {
      chainbase::database::open(data_dir, read_write, shared_file_size);

      initialize_indexes();

      _block_log.open(data_dir / "blocks.log");

      if (!find<global_property_object>()) {
         with_write_lock([&] {
            init_genesis(genesis_loader());
         });
      }

      // Rewind the database to the last irreversible block
      with_write_lock([&] {
         undo_all();

         FC_ASSERT(revision() == head_block_num(), "Chainbase revision does not match head block num",
                   ("rev", revision())("head_block", head_block_num()));
      });

      fc::optional<signed_block> last_block = _block_log.read_head();
      if(last_block.valid()) {
         _fork_db.start_block(*last_block);
         idump((last_block->id())(last_block->block_num()));
         idump((head_block_id())(head_block_num()));
         if (last_block->id() != head_block_id()) {
              FC_ASSERT(head_block_num() == 0, "last block ID does not match current chain state",
                        ("last_block->id", last_block->id())("head_block_num",head_block_num()));
         }
      }
} FC_CAPTURE_LOG_AND_RETHROW((data_dir)) }

void database::close()
{
   clear_pending();

   chainbase::database::flush();
   chainbase::database::close();

   if( _block_log.is_open() )
      _block_log.close();

   _fork_db.reset();
}

void database::update_global_dynamic_data(const signed_block& b) {
   const dynamic_global_property_object& _dgp = get<dynamic_global_property_object>();

   uint32_t missed_blocks = head_block_num() == 0? 1 : get_slot_at_time(b.timestamp);
   assert(missed_blocks != 0);
   missed_blocks--;

//   if (missed_blocks)
//      wlog("Blockchain continuing after gap of ${b} missed blocks", ("b", missed_blocks));

   for(uint32_t i = 0; i < missed_blocks; ++i) {
      const auto& producer_missed = get(get_scheduled_producer(i+1));
      if(producer_missed.id != b.producer) {
         /*
         const auto& producer_account = producer_missed.producer_account(*this);
         if( (fc::time_point::now() - b.timestamp) < fc::seconds(30) )
            wlog( "Producer ${name} missed block ${n} around ${t}", ("name",producer_account.name)("n",b.block_num())("t",b.timestamp) );
            */

         modify( producer_missed, [&]( producer_object& w ) {
           w.total_missed++;
         });
      }
   }

   // dynamic global properties updating
   modify( _dgp, [&]( dynamic_global_property_object& dgp ){
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

void database::update_signing_producer(const producer_object& signing_producer, const signed_block& new_block)
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t new_block_aslot = dpo.current_absolute_slot + get_slot_at_time( new_block.timestamp );

   modify( signing_producer, [&]( producer_object& _wit )
   {
      _wit.last_aslot = new_block_aslot;
      _wit.last_confirmed_block_num = new_block.block_num();
   } );
}

void database::update_last_irreversible_block()
{
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   vector<const producer_object*> producer_objs;
   producer_objs.reserve(gpo.active_producers.size());
   std::transform(gpo.active_producers.begin(), gpo.active_producers.end(), std::back_inserter(producer_objs),
                  [this](producer_id_type id) { return &get(id); });

   static_assert(config::IrreversibleThresholdPercent > 0, "irreversible threshold must be nonzero");

   size_t offset = EOS_PERCENT(producer_objs.size(), config::Percent100 - config::IrreversibleThresholdPercent);
   std::nth_element(producer_objs.begin(), producer_objs.begin() + offset, producer_objs.end(),
      [](const producer_object* a, const producer_object* b) {
         return a->last_confirmed_block_num < b->last_confirmed_block_num;
      });

   uint32_t new_last_irreversible_block_num = producer_objs[offset]->last_confirmed_block_num;

   if (new_last_irreversible_block_num > dpo.last_irreversible_block_num) {
      modify(dpo, [&](dynamic_global_property_object& _dpo) {
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
   commit(new_last_irreversible_block_num);
}

void database::clear_expired_transactions()
{ try {
   //Look for expired transactions in the deduplication list, and remove them.
   //Transactions must have expired by at least two forking windows in order to be removed.
   auto& transaction_idx = get_mutable_index<transaction_multi_index>();
   const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
   while( (!dedupe_index.empty()) && (head_block_time() > dedupe_index.rbegin()->trx.expiration) )
      transaction_idx.remove(*dedupe_index.rbegin());
} FC_CAPTURE_AND_RETHROW() }

using boost::container::flat_set;

producer_id_type database::get_scheduled_producer(uint32_t slot_num)const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   uint64_t current_aslot = dpo.current_absolute_slot + slot_num;
   const auto& gpo = get<global_property_object>();
   return gpo.active_producers[current_aslot % gpo.active_producers.size()];
}

fc::time_point_sec database::get_slot_time(uint32_t slot_num)const
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

uint32_t database::get_slot_at_time(fc::time_point_sec when)const
{
   fc::time_point_sec first_slot_time = get_slot_time( 1 );
   if( when < first_slot_time )
      return 0;
   return (when - first_slot_time).to_seconds() / block_interval() + 1;
}

uint32_t database::producer_participation_rate()const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   return uint64_t(config::Percent100) * __builtin_popcountll(dpo.recent_slots_filled) / 64;
}

void database::update_producer_schedule()
{
}
void database::set_validate_handler( const account_name& contract, const message_type& action, message_validate_handler v ) {
   message_validate_handlers[contract][action] = v;
}
void database::set_precondition_validate_handler(  const account_name& contract, const message_type& action, precondition_validate_handler v ) {
   precondition_validate_handlers[contract][action] = v;
}
void database::set_apply_handler( const account_name& contract, const message_type& action, apply_handler v ) {
   apply_handlers[contract][action] = v;
}

const account_object&   database::get_account( const account_name& name )const {
try {
    return get<account_object,by_name>(name);
} FC_CAPTURE_AND_RETHROW( (name) ) }

} }
