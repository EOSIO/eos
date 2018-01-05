/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/global_property_object.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/fork_database.hpp>
#include <eos/chain/block_log.hpp>

#include <chainbase/chainbase.hpp>
#include <fc/scoped_exit.hpp>

#include <boost/signals2/signal.hpp>

#include <eos/chain/block_schedule.hpp>
#include <eos/chain/protocol.hpp>
#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/chain_initializer_interface.hpp>
#include <eos/chain/chain_administration_interface.hpp>
#include <eos/chain/exceptions.hpp>

#include <fc/log/logger.hpp>

#include <map>

namespace eosio { namespace chain {
   using database = chainbase::database;
   using boost::signals2::signal;
   using applied_irreverisable_block_func = fc::optional<signal<void(const signed_block&)>::slot_type>;
   struct path_cons_list;

   /**
    *   @class database
    *   @brief tracks the blockchain state in an extensible manner
    */
   class chain_controller {
      public:
         struct txn_msg_limits;

         chain_controller(database& database, fork_database& fork_db, block_log& blocklog,
                          chain_initializer_interface& starter, unique_ptr<chain_administration_interface> admin,
                          uint32_t block_interval_seconds,
                          uint32_t txn_execution_time, uint32_t rcvd_block_txn_execution_time,
                          uint32_t create_block_txn_execution_time,
                          const txn_msg_limits& rate_limit,
                          const applied_irreverisable_block_func& applied_func = {});
         chain_controller(const chain_controller&) = delete;
         chain_controller(chain_controller&&) = delete;
         chain_controller& operator=(const chain_controller&) = delete;
         chain_controller& operator=(chain_controller&&) = delete;
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

         /**
          *  The controller can override any script endpoint with native code.
          */
         ///@{
         void set_apply_handler( const account_name& contract, const account_name& scope, const action_name& action, apply_handler v );
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
            skip_producer_schedule_check= 1 << 10, ///< used while reindexing
            skip_validate               = 1 << 11, ///< used prior to checkpoint, skips validate() call on transaction
            skip_scope_check            = 1 << 12, ///< used to skip checks for proper scope
            skip_output_check           = 1 << 13, ///< used to skip checks for outputs in block exactly matching those created from apply
            pushed_transaction          = 1 << 14, ///< used to indicate that the origination of the call was from a push_transaction, to determine time allotment
            created_block               = 1 << 15, ///< used to indicate that the origination of the call was for creating a block, to determine time allotment
            received_block              = 1 << 16, ///< used to indicate that the origination of the call was for a received block, to determine time allotment
            irreversible                = 1 << 17  ///< indicates the block was received while catching up and is already considered irreversible.
         };

         /**
          *  @return true if the block is in our fork DB or saved to disk as
          *  part of the official chain, otherwise return false
          */
         bool                        is_known_block( const block_id_type& id )const;
         bool                        is_known_transaction( const transaction_id_type& id )const;
         block_id_type               get_block_id_for_num( uint32_t block_num )const;
         optional<signed_block>      fetch_block_by_id( const block_id_type& id )const;
         optional<signed_block>      fetch_block_by_number( uint32_t num )const;
         std::vector<block_id_type>  get_block_ids_on_fork(block_id_type head_of_fork)const;
         const generated_transaction& get_generated_transaction( const generated_transaction_id_type& id ) const;


         /**
          *  This method will convert a variant to a SignedTransaction using a contract's ABI to
          *  interpret the message types.
          */
         processed_transaction transaction_from_variant( const fc::variant& v )const;

         /**
          * This method will convert a signed transaction into a human-friendly variant that can be
          * converted to JSON.
          */
         fc::variant       transaction_to_variant( const processed_transaction& trx )const;

         /**
          *  Usees the ABI for code::type to convert a JSON object (variant) into hex
          */
         vector<char>       message_to_binary( name code, name type, const fc::variant& obj )const;
         fc::variant        message_from_binary( name code, name type, const vector<char>& bin )const;


         /**
          *  Calculate the percent of block production slots that were missed in the
          *  past 128 blocks, not including the current block.
          */
         uint32_t producer_participation_rate()const;

         void                                   add_checkpoints(const flat_map<uint32_t,block_id_type>& checkpts);
         const flat_map<uint32_t,block_id_type> get_checkpoints()const { return _checkpoints; }
         bool before_last_checkpoint()const;

         bool push_block( const signed_block& b, uint32_t skip = skip_nothing );


         processed_transaction push_transaction( const signed_transaction& trx, uint32_t skip = skip_nothing );
         processed_transaction _push_transaction( const signed_transaction& trx );

         /**
          * Determine which public keys are needed to sign the given transaction.
          * @param trx Transaction that requires signature
          * @param candidateKeys Set of public keys to examine for applicability
          * @return Subset of candidateKeys whose private keys should be used to sign transaction
          * @throws fc::exception if candidateKeys does not contain all required keys
          */
         flat_set<public_key_type> get_required_keys(const signed_transaction& trx, const flat_set<public_key_type>& candidateKeys)const;


         bool _push_block( const signed_block& b );

         signed_block generate_block(
            fc::time_point_sec when,
            const account_name& producer,
            const fc::ecc::private_key& block_signing_private_key,
            block_schedule::factory scheduler = block_schedule::in_single_thread,
            uint32_t skip = skip_nothing
            );
         signed_block _generate_block(
            fc::time_point_sec when,
            const account_name& producer,
            const fc::ecc::private_key& block_signing_private_key,
            block_schedule::factory scheduler
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
         account_name get_scheduled_producer(uint32_t slot_num)const;

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
         const producer_object&                 get_producer(const account_name& ownerName)const;

         time_point_sec   head_block_time()const;
         uint32_t         head_block_num()const;
         block_id_type    head_block_id()const;
         account_name     head_block_producer()const;

         uint32_t block_interval()const { return _block_interval_seconds; }

         uint32_t last_irreversible_block_num() const;

 //  protected:
         const chainbase::database& get_database() const { return _db; }
         chainbase::database& get_mutable_database() { return _db; }

         bool should_check_scope()const                      { return !(_skip_flags&skip_scope_check);                     }

         // returns true to indicate that the caller should interpret the current block or transaction
         // as one that is being produced, or otherwise validated. This is never true if the block is
         // received as part of catching up with the rest of the chain, and is known to be irreversible.
         bool is_producing()const                            { return (_skip_flags & (received_block | pushed_transaction)) && !(_skip_flags & irreversible); }

         const deque<signed_transaction>&  pending()const { return _pending_transactions; }

         /**
          * Enum to indicate what type of rate limiting is being performed.
          */
         enum rate_limit_type
         {
            authorization_account,
            code_account
         };

         /**
          * Determine what the current message rate is.
          * @param now                       The current block time seconds
          * @param last_update_sec           The block time at the last update of the message rate
          * @param rate_limit_time_frame_sec The time frame, in seconds, that the rate limit is over
          * @param rate_limit                The rate that is not allowed to be exceeded
          * @param previous_rate             The rate at the last_update_sec
          * @param type                      The type of the rate limit
          * @param name                      The account name associated with this rate (for logging errors)
          * @return the calculated rate at this time
          * @throws tx_msgs_auth_exceeded if current message rate exceeds the passed in rate_limit, and type is authorization_account
          * @throws tx_msgs_code_exceeded if current message rate exceeds the passed in rate_limit, and type is code_account
          */
         static uint32_t _transaction_message_rate(const fc::time_point_sec& now, const fc::time_point_sec& last_update_sec, const fc::time_point_sec& rate_limit_time_frame_sec,
                                                   uint32_t rate_limit, uint32_t previous_rate, rate_limit_type type, const account_name& name);

         struct txn_msg_limits {
            fc::time_point_sec per_auth_account_txn_msg_rate_time_frame_sec = fc::time_point_sec(config::default_per_auth_account_rate_time_frame_seconds);
            uint32_t per_auth_account_txn_msg_rate = config::default_per_auth_account_rate;
            fc::time_point_sec per_code_account_txn_msg_rate_time_frame_sec = fc::time_point_sec(config::default_per_code_account_rate_time_frame_seconds);
            uint32_t per_code_account_txn_msg_rate = config::default_per_code_account_rate;
            uint32_t pending_txn_depth_limit = config::default_pending_txn_depth_limit;
            fc::microseconds gen_block_time_limit = config::default_gen_block_time_limit;
         };

   private:

         /// Reset the object graph in-memory
         void initialize_indexes();
         void initialize_chain(chain_initializer_interface& starter);

         void replay();

         void apply_block(const signed_block& next_block, uint32_t skip = skip_nothing);
         void _apply_block(const signed_block& next_block);

         template<typename Function>
         auto with_applying_block(Function&& f) -> decltype((*((Function*)nullptr))()) {
            auto on_exit = fc::make_scoped_exit([this](){
               _currently_applying_block = false;
            });
            _currently_applying_block = true;
            return f();
         }

         void check_transaction_authorization(const signed_transaction& trx, bool allow_unused_signatures = false)const;

         template<typename T>
         void check_transaction_output(const T& expected, const T& actual, const path_cons_list& path)const;

         template<typename T>
         typename T::processed apply_transaction(const T& trx);

         template<typename T>
         typename T::processed process_transaction(const T& trx, int depth, const fc::time_point& start_time);

         void require_account(const account_name& name) const;

         /**
          * This method performs some consistency checks on a transaction.
          * @thow transaction_exception if the transaction is invalid
          */
         template<typename T>
         void validate_transaction(const T& trx) const {
         try {
            EOS_ASSERT(trx.messages.size() > 0, transaction_exception, "A transaction must have at least one message");

            validate_scope(trx);
            validate_expiration(trx);
            validate_uniqueness(trx);
            validate_tapos(trx);

         } FC_CAPTURE_AND_RETHROW( (trx) ) }

         /// Validate transaction helpers @{
         void validate_uniqueness(const signed_transaction& trx)const;
         void validate_uniqueness(const generated_transaction& trx)const;
         void validate_tapos(const transaction& trx)const;
         void validate_referenced_accounts(const transaction& trx)const;
         void validate_expiration(const transaction& trx) const;
         void validate_scope(const transaction& trx) const;

         void record_transaction(const signed_transaction& trx);
         void record_transaction(const generated_transaction& trx);
         /// @}

         /**
          * @brief Find the lowest authority level required for @ref authorizer_account to authorize a message of the
          * specified type
          * @param authorizer_account The account authorizing the message
          * @param code_account The account which publishes the contract that handles the message
          * @param type The type of message
          * @return
          */
         const permission_object& lookup_minimum_permission(types::account_name authorizer_account,
                                                            types::account_name code_account,
                                                            types::func_name type) const;

         /**
          * Calculate all rates associated with the given message and enforce rate limiting.
          * @param message  The message to calculate
          * @throws tx_msgs_auth_exceeded if any of the calculated message rates exceed the configured authorization account rate limit
          * @throws tx_msgs_code_exceeded if the calculated message rate exceed the configured code account rate limit
          */
         void rate_limit_message(const message& message);

         void process_message(const transaction& trx, account_name code, const message& message,
                              message_output& output, apply_context* parent_context = nullptr,
                              int depth = 0, const fc::time_point& start_time = fc::time_point::now());
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

         producer_round calculate_next_round(const signed_block& next_block);

         database&                        _db;
         fork_database&                   _fork_db;
         block_log&                       _block_log;

         unique_ptr<chain_administration_interface> _admin;

         optional<database::session>      _pending_tx_session;
         deque<signed_transaction>         _pending_transactions;

         bool                             _currently_applying_block = false;
         bool                             _currently_replaying_blocks = false;
         uint64_t                         _skip_flags = 0;

         const uint32_t                   _block_interval_seconds;
         const uint32_t                   _txn_execution_time;
         const uint32_t                   _rcvd_block_txn_execution_time;
         const uint32_t                   _create_block_txn_execution_time;
         const fc::time_point_sec         _per_auth_account_txn_msg_rate_limit_time_frame_sec;
         const uint32_t                   _per_auth_account_txn_msg_rate_limit;
         const fc::time_point_sec         _per_code_account_txn_msg_rate_limit_time_frame_sec;
         const uint32_t                   _per_code_account_txn_msg_rate_limit;
         const uint32_t                   _pending_txn_depth_limit;
         const fc::microseconds           _gen_block_time_limit;

         flat_map<uint32_t,block_id_type> _checkpoints;

         typedef pair<account_name,types::name> handler_key;

         map< account_name, map<handler_key, apply_handler> >                   apply_handlers;
   };

} }
