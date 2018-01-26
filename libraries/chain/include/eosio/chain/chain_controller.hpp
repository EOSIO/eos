/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/fork_database.hpp>
#include <eosio/chain/block_log.hpp>

#include <chainbase/chainbase.hpp>
#include <fc/scoped_exit.hpp>

#include <boost/signals2/signal.hpp>

#include <eosio/chain/protocol.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/contracts/genesis_state.hpp>
#include <eosio/chain/wasm_interface.hpp>

#include <fc/log/logger.hpp>

#include <map>

namespace eosio { namespace chain {
   using database = chainbase::database;
   using boost::signals2::signal;


   namespace contracts{ class chain_initializer; }

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
      skip_producer_schedule_check= 1 << 10, ///< used while reindexing
      skip_validate               = 1 << 11, ///< used prior to checkpoint, skips validate() call on transaction
      skip_scope_check            = 1 << 12, ///< used to skip checks for proper scope
      skip_output_check           = 1 << 13, ///< used to skip checks for outputs in block exactly matching those created from apply
      pushed_transaction          = 1 << 14, ///< used to indicate that the origination of the call was from a push_transaction, to determine time allotment
      created_block               = 1 << 15, ///< used to indicate that the origination of the call was for creating a block, to determine time allotment
      received_block              = 1 << 16,  ///< used to indicate that the origination of the call was for a received block, to determine time allotment
      genesis_setup               = 1 << 17, ///< used to indicate that the origination of the call was for a genesis transaction
      skip_missed_block_penalty   = 1 << 18, ///< used to indicate that missed blocks shouldn't count against producers (used in long unit tests)
   };


   /**
    *   @class database
    *   @brief tracks the blockchain state in an extensible manner
    */
   class chain_controller : public boost::noncopyable {
      public:
         struct controller_config {
            path                           block_log_dir       =  config::default_block_log_dir;
            path                           shared_memory_dir   =  config::default_shared_memory_dir;
            uint64_t                       shared_memory_size  =  config::default_shared_memory_size;
            bool                           read_only           =  false;
            std::vector<signal<void(const signed_block&)>::slot_type> applied_irreversible_block_callbacks;
            contracts::genesis_state_type  genesis;
         };

         chain_controller( const controller_config& cfg );
         ~chain_controller();


         void push_block( const signed_block& b, uint32_t skip = skip_nothing );
         transaction_trace push_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing );
         void push_deferred_transactions( bool flush = false );



      /**
          *  This signal is emitted after all operations and virtual operation for a
          *  block have been applied but before the get_applied_operations() are cleared.
          *
          *  You may not yield from this callback because the blockchain is holding
          *  the write lock and may be in an "inconstant state" until after it is
          *  released.
          */
         signal<void(const block_trace&)> applied_block;

         /**
          *  This signal is emitted after irreversible block is written to disk.
          *
          *  You may not yield from this callback because the blockchain is holding
          *  the write lock and may be in an "inconstant state" until after it is
          *  released.
          */
         signal<void(const signed_block&)> applied_irreversible_block;

         /**
          * This signal is emitted any time a new transaction is added to the pending
          * block state.
          */
         signal<void(const signed_transaction&)> on_pending_transaction;











         /**
          * @brief Check whether the controller is currently applying a block or not
          * @return True if the controller is now applying a block; false otherwise
          */
         bool is_applying_block()const { return _currently_applying_block; }


         chain_id_type get_chain_id()const { return chain_id_type(); } /// TODO: make this hash of constitution


         /**
          *  @return true if the block is in our fork DB or saved to disk as
          *  part of the official chain, otherwise return false
          */
         bool                           is_known_block( const block_id_type& id )const;
         bool                           is_known_transaction( const transaction_id_type& id )const;
         block_id_type                  get_block_id_for_num( uint32_t block_num )const;
         optional<signed_block>         fetch_block_by_id( const block_id_type& id )const;
         optional<signed_block>         fetch_block_by_number( uint32_t num )const;
         std::vector<block_id_type>     get_block_ids_on_fork(block_id_type head_of_fork)const;

         /**
          *  Usees the ABI for code::type to convert a JSON object (variant) into hex
         vector<char>       action_to_binary( name code, name type, const fc::variant& obj )const;
         fc::variant        action_from_binary( name code, name type, const vector<char>& bin )const;
          */


         /**
          *  Calculate the percent of block production slots that were missed in the
          *  past 128 blocks, not including the current block.
          */
         uint32_t producer_participation_rate()const;

         void                                   add_checkpoints(const flat_map<uint32_t,block_id_type>& checkpts);
         const flat_map<uint32_t,block_id_type> get_checkpoints()const { return _checkpoints; }
         bool before_last_checkpoint()const;




         /**
          * Determine which public keys are needed to sign the given transaction.
          * @param trx transaction that requires signature
          * @param candidate_keys Set of public keys to examine for applicability
          * @return Subset of candidate_keys whose private keys should be used to sign transaction
          * @throws fc::exception if candidate_keys does not contain all required keys
          */
         flat_set<public_key_type> get_required_keys(const signed_transaction& trx, const flat_set<public_key_type>& candidate_keys)const;


         bool _push_block( const signed_block& b );

         signed_block generate_block(
            block_timestamp_type when,
            account_name producer,
            const private_key_type& block_signing_private_key,
            uint32_t skip = skip_nothing
            );
         signed_block _generate_block(
            block_timestamp_type when,
            account_name producer,
            const private_key_type& block_signing_private_key
            );


         template<typename Function>
         auto with_skip_flags( uint64_t flags, Function&& f )
         {
            auto on_exit   = fc::make_scoped_exit( [old_flags=_skip_flags,this](){ _skip_flags = old_flags; } );
            _skip_flags = flags;
            return f();
         }


         /**
          *  This method will backup all tranasctions in the current pending block,
          *  undo the pending block, call f(), and then push the pending transactions
          *  on top of the new state.
          */
         template<typename Function>
         auto without_pending_transactions( Function&& f )
         {
            vector<signed_transaction> old_input;

            if( _pending_block )
               old_input = move(_pending_block->input_transactions);

            clear_pending();

            /** after applying f() push previously input transactions on top */
            auto on_exit = fc::make_scoped_exit( [&](){ 
               for( const auto& t : old_input ) {
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
         account_name get_scheduled_producer(uint32_t slot_num)const;

         /**
          * Get the time at which the given slot occurs.
          *
          * If slot_num == 0, return time_point_sec().
          *
          * If slot_num == N for N > 0, return the Nth next
          * block-interval-aligned time greater than head_block_time().
          */
          block_timestamp_type get_slot_time(uint32_t slot_num)const;

         /**
          * Get the last slot which occurs AT or BEFORE the given time.
          *
          * The return value is the greatest value N such that
          * get_slot_time( N ) <= when.
          *
          * If no such N exists, return 0.
          */
         uint32_t get_slot_at_time( block_timestamp_type when )const;

         const global_property_object&          get_global_properties()const;
         const dynamic_global_property_object&  get_dynamic_global_properties()const;
         const producer_object&                 get_producer(const account_name& ownername)const;
         const permission_object&               get_permission( const permission_level& level )const;

         time_point           head_block_time()const;
         uint32_t             head_block_num()const;
         block_id_type        head_block_id()const;
         account_name         head_block_producer()const;

         uint32_t last_irreversible_block_num() const;

         const chainbase::database& get_database() const { return _db; }
         chainbase::database&       get_mutable_database() { return _db; }

         wasm_cache& get_wasm_cache() {
            return _wasm_cache;
         }


         /**
          * @param provided_keys - the set of public keys which have authorized the transaction
          * @param provided_accounts - the set of accounts which have authorized the transaction (presumed to be owner)
          *
          * @return true if the provided keys and accounts are sufficient to authorize actions of the transaction
          */
         void check_authorization( const vector<action>& actions,
                                   flat_set<public_key_type> provided_keys,
                                   bool                      allow_unused_signatures = false,
                                   flat_set<account_name>    provided_accounts = flat_set<account_name>()
                                   )const;



      private:
         const apply_handler* find_apply_handler( account_name contract, scope_name scope, action_name act )const;


         friend class contracts::chain_initializer;
         friend class apply_context;

         bool should_check_scope()const                      { return !(_skip_flags&skip_scope_check);            }

         /**
          *  The controller can override any script endpoint with native code.
          */
         ///@{
         void _set_apply_handler( account_name contract, account_name scope, action_name action, apply_handler v );
         //@}


         transaction_trace _push_transaction( const signed_transaction& trx );
         transaction_trace _push_transaction( transaction_metadata& data );
         transaction_trace _apply_transaction( transaction_metadata& data );
         transaction_trace __apply_transaction( transaction_metadata& data );
         transaction_trace _apply_error( transaction_metadata& data );

         /// Reset the object graph in-memory
         void _initialize_indexes();
         void _initialize_chain(contracts::chain_initializer& starter);

         producer_schedule_type _calculate_producer_schedule()const;
         const producer_schedule_type& _head_producer_schedule()const;


         void replay();

         void _apply_block(const signed_block& next_block, uint32_t skip = skip_nothing);
         void __apply_block(const signed_block& next_block);

         template<typename Function>
         auto with_applying_block(Function&& f) -> decltype((*((Function*)nullptr))()) {
            auto on_exit = fc::make_scoped_exit([this](){
               _currently_applying_block = false;
            });
            _currently_applying_block = true;
            return f();
         }

         void check_transaction_authorization(const signed_transaction& trx, 
                                              bool allow_unused_signatures = false)const;


         void require_scope(const scope_name& name) const;
         void require_account(const account_name& name) const;

         /**
          * This method performs some consistency checks on a transaction.
          * @thow transaction_exception if the transaction is invalid
          */
         template<typename T>
         void validate_transaction(const T& trx) const {
         try {
            EOS_ASSERT(trx.messages.size() > 0, transaction_exception, "A transaction must have at least one message");

            validate_expiration(trx);
            validate_uniqueness(trx);
            validate_tapos(trx);

         } FC_CAPTURE_AND_RETHROW( (trx) ) }
         
         /// Validate transaction helpers @{
         void validate_uniqueness(const signed_transaction& trx)const;
         void validate_tapos(const transaction& trx)const;
         void validate_referenced_accounts(const transaction& trx)const;
         void validate_expiration(const transaction& trx) const;
         void record_transaction(const transaction& trx);
         /// @}

         /**
          * @brief Find the lowest authority level required for @ref authorizer_account to authorize a message of the
          * specified type
          * @param authorizer_account The account authorizing the message
          * @param code_account The account which publishes the contract that handles the message
          * @param type The type of message
          * @return
          */
         optional<permission_name> lookup_minimum_permission( account_name authorizer_account,
                                                             scope_name code_account,
                                                             action_name type) const;


         bool should_check_for_duplicate_transactions()const { return !(_skip_flags&skip_transaction_dupe_check); }
         bool should_check_tapos()const                      { return !(_skip_flags&skip_tapos_check);            }

         ///Steps involved in applying a new block
         ///@{
         const producer_object& validate_block_header(uint32_t skip, const signed_block& next_block)const;
         const producer_object& _validate_block_header(const signed_block& next_block)const;
         void create_block_summary(const signed_block& next_block);

         void update_global_properties(const signed_block& b);
         void update_global_dynamic_data(const signed_block& b);
         void update_usage( transaction_metadata&, uint32_t act_usage );
         void update_signing_producer(const producer_object& signing_producer, const signed_block& new_block);
         void update_last_irreversible_block();
         void clear_expired_transactions();
         /// @}

         void _spinup_db();
         void _spinup_fork_db();

         void _start_pending_block();
         void _start_pending_cycle();
         void _start_pending_shard();
         void _finalize_pending_cycle();
         void _apply_cycle_trace( const cycle_trace& trace );
         void _finalize_block( const block_trace& b );


      //        producer_schedule_type calculate_next_round( const signed_block& next_block );

         database                         _db;
         fork_database                    _fork_db;
         block_log                        _block_log;

         optional<database::session>      _pending_block_session;
         optional<signed_block>           _pending_block;
         optional<block_trace>            _pending_block_trace;
         uint32_t                         _pending_transaction_count = 0; 
         optional<cycle_trace>            _pending_cycle_trace;

         bool                             _currently_applying_block = false;
         bool                             _currently_replaying_blocks = false;
         uint64_t                         _skip_flags = 0;

         flat_map<uint32_t,block_id_type> _checkpoints;

         typedef pair<scope_name,action_name>                   handler_key;
         map< account_name, map<handler_key, apply_handler> >   _apply_handlers;

         wasm_cache                       _wasm_cache;
   };

} }
