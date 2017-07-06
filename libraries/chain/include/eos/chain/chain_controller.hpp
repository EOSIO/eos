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
#pragma once
#include <eos/chain/global_property_object.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/fork_database.hpp>
#include <eos/chain/block_log.hpp>

#include <chainbase/chainbase.hpp>
#include <fc/scoped_exit.hpp>

#include <boost/signals2/signal.hpp>

#include <eos/chain/protocol.hpp>
#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/chain_initializer_interface.hpp>
#include <eos/chain/chain_administration_interface.hpp>

#include <fc/log/logger.hpp>

#include <map>

namespace eos { namespace chain {
   using database = chainbase::database;
   using boost::signals2::signal;

   /**
    *   @class database
    *   @brief tracks the blockchain state in an extensible manner
    */
   class chain_controller {
      public:
         chain_controller(database& database, fork_database& fork_db, block_log& blocklog,
                          chain_initializer_interface& starter, unique_ptr<chain_administration_interface> admin);
         chain_controller(chain_controller&&) = default;
         ~chain_controller();

         /**
          *  This signal is emitted after all operations and virtual operation for a
          *  block have been applied but before the get_applied_operations() are cleared.
          *
          *  You may not yield from this callback because the blockchain is holding
          *  the write lock and may be in an "inconstant state" until after it is
          *  released.
          */
         signal<void(const signed_block&)> applied_block;

         /**
          * This signal is emitted any time a new transaction is added to the pending
          * block state.
          */
         signal<void(const SignedTransaction&)> on_pending_transaction;


         /**
          *  The controller can override any script endpoint with native code.
          */
         ///@{
         void set_validate_handler( const AccountName& contract, const AccountName& scope, const ActionName& action, message_validate_handler v );
         void set_precondition_validate_handler(  const AccountName& contract, const AccountName& scope, const ActionName& action, precondition_validate_handler v );
         void set_apply_handler( const AccountName& contract, const AccountName& scope, const ActionName& action, apply_handler v );
         //@}

         enum validation_steps
         {
            skip_nothing                = 0,
            skip_producer_signature     = 1 << 0,  ///< used while reindexing
            skip_transaction_signatures = 1 << 1,  ///< used by non-producer nodes
            skip_transaction_dupe_check = 1 << 2,  ///< used while reindexing
            skip_fork_db                = 1 << 3,  ///< used while reindexing
            skip_block_size_check       = 1 << 4,  ///< used when applying locally generated transactions
            skip_tapos_check            = 1 << 5,  ///< used while reindexing -- note this skips expiration check as well
            skip_authority_check        = 1 << 6,  ///< used while reindexing -- disables any checking of authority on transactions
            skip_merkle_check           = 1 << 7,  ///< used while reindexing
            skip_assert_evaluation      = 1 << 8,  ///< used while reindexing
            skip_undo_history_check     = 1 << 9,  ///< used while reindexing
            skip_producer_schedule_check= 1 << 10,  ///< used while reindexing
            skip_validate               = 1 << 11 ///< used prior to checkpoint, skips validate() call on transaction
         };

         /**
          *  @return true if the block is in our fork DB or saved to disk as
          *  part of the official chain, otherwise return false
          */
         bool                       is_known_block( const block_id_type& id )const;
         bool                       is_known_transaction( const transaction_id_type& id )const;
         block_id_type              get_block_id_for_num( uint32_t block_num )const;
         optional<signed_block>     fetch_block_by_id( const block_id_type& id )const;
         optional<signed_block>     fetch_block_by_number( uint32_t num )const;
         const SignedTransaction&   get_recent_transaction( const transaction_id_type& trx_id )const;
         std::vector<block_id_type> get_block_ids_on_fork(block_id_type head_of_fork) const;

         /**
          *  Calculate the percent of block production slots that were missed in the
          *  past 128 blocks, not including the current block.
          */
         uint32_t producer_participation_rate()const;

         void                                   add_checkpoints(const flat_map<uint32_t,block_id_type>& checkpts);
         const flat_map<uint32_t,block_id_type> get_checkpoints()const { return _checkpoints; }
         bool before_last_checkpoint()const;

         bool push_block( const signed_block& b, uint32_t skip = skip_nothing );
         void push_transaction( const SignedTransaction& trx, uint32_t skip = skip_nothing );
         bool _push_block( const signed_block& b );
         void _push_transaction( const SignedTransaction& trx );

         signed_block generate_block(
            fc::time_point_sec when,
            const AccountName& producer,
            const fc::ecc::private_key& block_signing_private_key,
            uint32_t skip
            );
         signed_block _generate_block(
            fc::time_point_sec when,
            const AccountName& producer,
            const fc::ecc::private_key& block_signing_private_key
            );


         template<typename Function>
         auto with_skip_flags( uint64_t flags, Function&& f ) -> decltype((*((Function*)nullptr))()) 
         {
            auto old_flags = _skip_flags;
            auto on_exit   = fc::make_scoped_exit( [&](){ _skip_flags = old_flags; } );
            _skip_flags = flags;
            return f();
         }

         template<typename Function>
         auto without_pending_transactions( Function&& f ) -> decltype((*((Function*)nullptr))()) 
         {
            auto old_pending = std::move( _pending_transactions );
            _pending_tx_session.reset();
            auto on_exit = fc::make_scoped_exit( [&](){ 
               for( const auto& t : old_pending ) {
                  try {
                     if (!is_known_transaction(t.id()))
                        push_transaction( t );
                  } catch ( ... ){}
               }
            });
            return f();
         }

         void pop_block();
         void clear_pending();

         /**
          * @brief Get the producer scheduled for block production in a slot.
          *
          * slot_num always corresponds to a time in the future.
          *
          * If slot_num == 1, returns the next scheduled producer.
          * If slot_num == 2, returns the next scheduled producer after
          * 1 block gap.
          *
          * Use the get_slot_time() and get_slot_at_time() functions
          * to convert between slot_num and timestamp.
          *
          * Passing slot_num == 0 returns EOS_NULL_PRODUCER
          */
         AccountName get_scheduled_producer(uint32_t slot_num)const;

         /**
          * Get the time at which the given slot occurs.
          *
          * If slot_num == 0, return time_point_sec().
          *
          * If slot_num == N for N > 0, return the Nth next
          * block-interval-aligned time greater than head_block_time().
          */
         fc::time_point_sec get_slot_time(uint32_t slot_num)const;

         /**
          * Get the last slot which occurs AT or BEFORE the given time.
          *
          * The return value is the greatest value N such that
          * get_slot_time( N ) <= when.
          *
          * If no such N exists, return 0.
          */
         uint32_t get_slot_at_time(fc::time_point_sec when)const;

         const global_property_object&          get_global_properties()const;
         const dynamic_global_property_object&  get_dynamic_global_properties()const;
         const producer_object&                 get_producer(const AccountName& ownerName)const;

         time_point_sec   head_block_time()const;
         uint32_t         head_block_num()const;
         block_id_type    head_block_id()const;
         AccountName      head_block_producer()const;

         uint32_t block_interval()const { return config::BlockIntervalSeconds; }

         uint32_t last_irreversible_block_num() const;

   protected:
         const chainbase::database& get_database() const { return _db; }
         
   private:

         /// Reset the object graph in-memory
         void initialize_indexes();
         void initialize_chain(chain_initializer_interface& starter);

         void replay();

         void apply_block(const signed_block& next_block, uint32_t skip = skip_nothing);
         void apply_transaction(const SignedTransaction& trx, uint32_t skip = skip_nothing);
         void _apply_block(const signed_block& next_block);
         void _apply_transaction(const SignedTransaction& trx);

         void require_account(const AccountName& name) const;

         /**
          * This method validates transactions without adding it to the pending state.
          * @return true if the transaction would validate
          */
         void validate_transaction(const SignedTransaction& trx)const;
         /// Validate transaction helpers @{
         void validate_uniqueness(const SignedTransaction& trx)const;
         void validate_tapos(const SignedTransaction& trx)const;
         void validate_referenced_accounts(const SignedTransaction& trx)const;
         void validate_expiration(const SignedTransaction& trx) const;
         /// @}

         void validate_message_precondition(precondition_validate_context& c)const;
         void process_message(const Transaction& trx, const Message& message);
         void apply_message(apply_context& c);

         bool should_check_for_duplicate_transactions()const { return !(_skip_flags&skip_transaction_dupe_check); }
         bool should_check_tapos()const                      { return !(_skip_flags&skip_tapos_check);            }

         ///Steps involved in applying a new block
         ///@{
         const producer_object& validate_block_header(uint32_t skip, const signed_block& next_block)const;
         const producer_object& _validate_block_header(const signed_block& next_block)const;
         void create_block_summary(const signed_block& next_block);

         void update_global_properties(const signed_block& b);
         void update_global_dynamic_data(const signed_block& b);
         void update_signing_producer(const producer_object& signing_producer, const signed_block& new_block);
         void update_last_irreversible_block();
         void clear_expired_transactions();
         /// @}

         void spinup_db();
         void spinup_fork_db();

         ProducerRound calculate_next_round(const signed_block& next_block);

         database&                        _db;
         fork_database&                   _fork_db;
         block_log&                       _block_log;

         unique_ptr<chain_administration_interface> _admin;

         optional<database::session>      _pending_tx_session;
         deque<SignedTransaction>         _pending_transactions;

         bool                             _pushing  = false;
         uint64_t                         _skip_flags = 0;

         flat_map<uint32_t,block_id_type> _checkpoints;

         typedef pair<AccountName,types::Name> handler_key;

         map< AccountName, map<handler_key, message_validate_handler> >        message_validate_handlers;
         map< AccountName, map<handler_key, precondition_validate_handler> >   precondition_validate_handlers;
         map< AccountName, map<handler_key, apply_handler> >                   apply_handlers;
   };

} }
