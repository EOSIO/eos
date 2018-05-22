#include <eosio/chain/controller.hpp>
#include <eosio/chain/block_context.hpp>
#include <eosio/chain/transaction_context.hpp>

#include <eosio/chain/block_log.hpp>
#include <eosio/chain/fork_database.hpp>

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/unconfirmed_block_object.hpp>

#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>

#include <chainbase/chainbase.hpp>
#include <fc/io/json.hpp>
#include <fc/scoped_exit.hpp>

#include <eosio/chain/eosio_contract.hpp>

namespace eosio { namespace chain {

using resource_limits::resource_limits_manager;


struct pending_state {
   pending_state( database::session&& s )
   :_db_session( move(s) ){}

   database::session                  _db_session;

   block_state_ptr                    _pending_block_state;

   vector<action_receipt>             _actions;

   block_context                      _block_ctx;

   void push() {
      _db_session.push();
   }
};

struct controller_impl {
   controller&                    self;
   chainbase::database            db;
   chainbase::database            unconfirmed_blocks; ///< a special database to persist blocks that have successfully been applied but are still reversible
   block_log                      blog;
   optional<pending_state>        pending;
   block_state_ptr                head;
   fork_database                  fork_db;
   wasm_interface                 wasmif;
   resource_limits_manager        resource_limits;
   authorization_manager          authorization;
   controller::config             conf;
   bool                           replaying = false;
   bool                           replaying_irreversible = false;

   typedef pair<scope_name,action_name>                   handler_key;
   map< account_name, map<handler_key, apply_handler> >   apply_handlers;

   /**
    *  Transactions that were undone by pop_block or abort_block, transactions
    *  are removed from this list if they are re-applied in other blocks. Producers
    *  can query this list when scheduling new transactions into blocks.
    */
   map<digest_type, transaction_metadata_ptr>     unapplied_transactions;

   void pop_block() {
      auto prev = fork_db.get_block( head->header.previous );
      FC_ASSERT( prev, "attempt to pop beyond last irreversible block" );

      if( const auto* b = unconfirmed_blocks.find<unconfirmed_block_object,by_num>(head->block_num) )
      {
         unconfirmed_blocks.remove( *b );
      }

      for( const auto& t : head->trxs )
         unapplied_transactions[t->signed_id] = t;
      head = prev;
      db.undo();

   }


   void set_apply_handler( account_name receiver, account_name contract, action_name action, apply_handler v ) {
      apply_handlers[receiver][make_pair(contract,action)] = v;
   }

   controller_impl( const controller::config& cfg, controller& s  )
   :self(s),
    db( cfg.shared_memory_dir,
        cfg.read_only ? database::read_only : database::read_write,
        cfg.shared_memory_size ),
    unconfirmed_blocks( cfg.block_log_dir/"unconfirmed",
        cfg.read_only ? database::read_only : database::read_write,
        cfg.unconfirmed_cache_size ),
    blog( cfg.block_log_dir ),
    fork_db( cfg.shared_memory_dir ),
    wasmif( cfg.wasm_runtime ),
    resource_limits( db ),
    authorization( s, db ),
    conf( cfg )
   {

#define SET_APP_HANDLER( receiver, contract, action) \
   set_apply_handler( #receiver, #contract, #action, &BOOST_PP_CAT(apply_, BOOST_PP_CAT(contract, BOOST_PP_CAT(_,action) ) ) )

   SET_APP_HANDLER( eosio, eosio, newaccount );
   SET_APP_HANDLER( eosio, eosio, setcode );
   SET_APP_HANDLER( eosio, eosio, setabi );
   SET_APP_HANDLER( eosio, eosio, updateauth );
   SET_APP_HANDLER( eosio, eosio, deleteauth );
   SET_APP_HANDLER( eosio, eosio, linkauth );
   SET_APP_HANDLER( eosio, eosio, unlinkauth );
/*
   SET_APP_HANDLER( eosio, eosio, postrecovery );
   SET_APP_HANDLER( eosio, eosio, passrecovery );
   SET_APP_HANDLER( eosio, eosio, vetorecovery );
*/

   SET_APP_HANDLER( eosio, eosio, canceldelay );

   fork_db.irreversible.connect( [&]( auto b ) {
                                 on_irreversible(b);
                                 });

   }

   /**
    *  Plugins / observers listening to signals emited (such as accepted_transaction) might trigger
    *  errors and throw exceptions. Unless those exceptions are caught it could impact consensus and/or
    *  cause a node to fork.
    *
    *  If it is ever desirable to let a signal handler bubble an exception out of this method
    *  a full audit of its uses needs to be undertaken.
    *
    */
   template<typename Signal, typename Arg>
   void emit( const Signal& s, Arg&& a ) {
      try {
        s(std::forward<Arg>(a));
      } catch (boost::interprocess::bad_alloc& e) {
         wlog( "bad alloc" );
         throw e;
      } catch ( fc::exception& e ) {
         wlog( "${details}", ("details", e.to_detail_string()) );
      } catch ( ... ) {
         wlog( "signal handler threw exception" );
      }
   }

   void on_irreversible( const block_state_ptr& s ) {
      if( !blog.head() )
         blog.read_head();

      const auto& log_head = blog.head();
      FC_ASSERT( log_head );
      auto lh_block_num = log_head->block_num();

      if( s->block_num <= lh_block_num ) {
//         edump((s->block_num)("double call to on_irr"));
//         edump((s->block_num)(s->block->previous)(log_head->id()));
         return;
      }

      FC_ASSERT( s->block_num - 1  == lh_block_num, "unlinkable block", ("s->block_num",s->block_num)("lh_block_num", lh_block_num) );
      FC_ASSERT( s->block->previous == log_head->id(), "irreversible doesn't link to block log head" );
      blog.append(s->block);

      emit( self.irreversible_block, s );
      db.commit( s->block_num );

      const auto& ubi = unconfirmed_blocks.get_index<unconfirmed_block_index,by_num>();
      auto objitr = ubi.begin();
      while( objitr != ubi.end() && objitr->blocknum <= s->block_num ) {
         unconfirmed_blocks.remove( *objitr );
         objitr = ubi.begin();
      }
   }

   void init() {

      /**
      *  The fork database needs an initial block_state to be set before
      *  it can accept any new blocks. This initial block state can be found
      *  in the database (whose head block state should be irreversible) or
      *  it would be the genesis state.
      */
      if( !head ) {
         initialize_fork_db(); // set head to genesis state

         auto end = blog.read_head();
         if( end && end->block_num() > 1 ) {
            replaying = true;
            replaying_irreversible = true;
            ilog( "existing block log, attempting to replay ${n} blocks", ("n",end->block_num()) );

            auto start = fc::time_point::now();
            while( auto next = blog.read_block_by_num( head->block_num + 1 ) ) {
               self.push_block( next, true );
               if( next->block_num() % 100 == 0 ) {
                  std::cerr << std::setw(10) << next->block_num() << " of " << end->block_num() <<"\r";
               }
            }
            replaying_irreversible = false;

            int unconf = 0;
            while( auto obj = unconfirmed_blocks.find<unconfirmed_block_object,by_num>(head->block_num+1) ) {
               ++unconf;
               self.push_block( obj->get_block(), true );
            }

            std::cerr<< "\n";
            ilog( "${n} unconfirmed blocks replayed", ("n",unconf) );
            auto end = fc::time_point::now();
            ilog( "replayed blocks in ${n} seconds, ${spb} spb",
                  ("n", head->block_num)("spb", ((end-start).count()/1000000.0)/head->block_num)  );
            std::cerr<< "\n";
            replaying = false;

         } else if( !end ) {
            blog.append( head->block );
         }
      }

      const auto& ubi = unconfirmed_blocks.get_index<unconfirmed_block_index,by_num>();
      auto objitr = ubi.rbegin();
      if( objitr != ubi.rend() ) {
         FC_ASSERT( objitr->blocknum == head->block_num,
                    "unconfirmed block database is inconsistent with fork database, replay blockchain",
                    ("head",head->block_num)("unconfimed", objitr->blocknum)         );
      } else {
         auto end = blog.read_head();
         FC_ASSERT( end && end->block_num() == head->block_num,
                    "fork database exists but unconfirmed block database does not, replay blockchain",
                    ("blog_head",end->block_num())("head",head->block_num)  );
      }

      FC_ASSERT( db.revision() >= head->block_num, "fork database is inconsistent with shared memory",
                 ("db",db.revision())("head",head->block_num) );

      if( db.revision() > head->block_num ) {
         wlog( "warning: database revision (${db}) is greater than head block number (${head}), "
               "attempting to undo pending changes",
               ("db",db.revision())("head",head->block_num) );
      }
      while( db.revision() > head->block_num ) {
         db.undo();
      }

   }

   ~controller_impl() {
      pending.reset();
      fork_db.close();

      edump((db.revision())(head->block_num)(blog.read_head()->block_num()));

      db.flush();
      unconfirmed_blocks.flush();
   }

   void add_indices() {
      unconfirmed_blocks.add_index<unconfirmed_block_index>();

      db.add_index<account_index>();
      db.add_index<account_sequence_index>();

      db.add_index<table_id_multi_index>();
      db.add_index<key_value_index>();
      db.add_index<index64_index>();
      db.add_index<index128_index>();
      db.add_index<index256_index>();
      db.add_index<index_double_index>();
      db.add_index<index_long_double_index>();

      db.add_index<global_property_multi_index>();
      db.add_index<dynamic_global_property_multi_index>();
      db.add_index<block_summary_multi_index>();
      db.add_index<transaction_multi_index>();
      db.add_index<generated_transaction_multi_index>();

      authorization.add_indices();
      resource_limits.add_indices();
   }

   void clear_all_undo() {
      // Rewind the database to the last irreversible block
      db.with_write_lock([&] {
         db.undo_all();
         /*
         FC_ASSERT(db.revision() == self.head_block_num(),
                   "Chainbase revision does not match head block num",
                   ("rev", db.revision())("head_block", self.head_block_num()));
                   */
      });
   }

   /**
    *  Sets fork database head to the genesis state.
    */
   void initialize_fork_db() {
      wlog( " Initializing new blockchain with genesis state                  " );
      producer_schedule_type initial_schedule{ 0, {{N(eosio), conf.genesis.initial_key}} };

      block_header_state genheader;
      genheader.active_schedule       = initial_schedule;
      genheader.pending_schedule      = initial_schedule;
      genheader.pending_schedule_hash = fc::sha256::hash(initial_schedule);
      genheader.header.timestamp      = conf.genesis.initial_timestamp;
      genheader.header.action_mroot   = conf.genesis.compute_chain_id();
      genheader.id                    = genheader.header.id();
      genheader.block_num             = genheader.header.block_num();

      head = std::make_shared<block_state>( genheader );
      head->block = std::make_shared<signed_block>(genheader.header);
      fork_db.set( head );
      db.set_revision( head->block_num );

      initialize_database();
   }

   void create_native_account( account_name name, const authority& owner, const authority& active, bool is_privileged = false ) {
      db.create<account_object>([&](auto& a) {
         a.name = name;
         a.creation_date = conf.genesis.initial_timestamp;
         a.privileged = is_privileged;

         if( name == config::system_account_name ) {
            a.set_abi(eosio_contract_abi(abi_def()));
         }
      });
      db.create<account_sequence_object>([&](auto & a) {
        a.name = name;
      });

      const auto& owner_permission  = authorization.create_permission(name, config::owner_name, 0,
                                                                      owner, conf.genesis.initial_timestamp );
      const auto& active_permission = authorization.create_permission(name, config::active_name, owner_permission.id,
                                                                      active, conf.genesis.initial_timestamp );

      resource_limits.initialize_account(name);

      int64_t ram_delta = config::overhead_per_account_ram_bytes;
      ram_delta += 2*config::billable_size_v<permission_object>;
      ram_delta += owner_permission.auth.get_billable_size();
      ram_delta += active_permission.auth.get_billable_size();

      resource_limits.add_pending_ram_usage(name, ram_delta);
      resource_limits.verify_account_ram_usage(name);
   }

   void initialize_database() {
      // Initialize block summary index
      for (int i = 0; i < 0x10000; i++)
         db.create<block_summary_object>([&](block_summary_object&) {});

      const auto& tapos_block_summary = db.get<block_summary_object>(1);
      db.modify( tapos_block_summary, [&]( auto& bs ) {
        bs.block_id = head->id;
      });

      db.create<global_property_object>([&](auto& gpo ){
        gpo.configuration = conf.genesis.initial_configuration;
      });
      db.create<dynamic_global_property_object>([](auto&){});

      authorization.initialize_database();
      resource_limits.initialize_database();

      authority system_auth(conf.genesis.initial_key);
      create_native_account( config::system_account_name, system_auth, system_auth, true );

      auto empty_authority = authority(1, {}, {});
      auto active_producers_authority = authority(1, {}, {});
      active_producers_authority.accounts.push_back({{config::system_account_name, config::active_name}, 1});

      create_native_account( config::null_account_name, empty_authority, empty_authority );
      create_native_account( config::producers_account_name, empty_authority, active_producers_authority );
      const auto& active_permission       = authorization.get_permission({config::producers_account_name, config::active_name});
      const auto& majority_permission     = authorization.create_permission( config::producers_account_name,
                                                                             config::majority_producers_permission_name,
                                                                             active_permission.id,
                                                                             active_producers_authority,
                                                                             conf.genesis.initial_timestamp );
      const auto& minority_permission     = authorization.create_permission( config::producers_account_name,
                                                                             config::minority_producers_permission_name,
                                                                             majority_permission.id,
                                                                             active_producers_authority,
                                                                             conf.genesis.initial_timestamp );
   }



   void commit_block( bool add_to_fork_db ) {
      if( add_to_fork_db ) {
         pending->_pending_block_state->validated = true;
         auto new_bsp = fork_db.add( pending->_pending_block_state );
         emit( self.accepted_block_header, pending->_pending_block_state );
         head = fork_db.head();
         FC_ASSERT( new_bsp == head, "committed block did not become the new head in fork database" );

      }

  //    ilog((fc::json::to_pretty_string(*pending->_pending_block_state->block)));
      emit( self.accepted_block, pending->_pending_block_state );

      if( !replaying ) {
         unconfirmed_blocks.create<unconfirmed_block_object>( [&]( auto& ubo ) {
            ubo.blocknum = pending->_pending_block_state->block_num;
            ubo.set_block( pending->_pending_block_state->block );
         });
      }

      pending->push();
      pending.reset();

   }

   // The returned scoped_exit should not exceed the lifetime of the pending which existed when make_block_restore_point was called.
   fc::scoped_exit<std::function<void()>> make_block_restore_point() {
      auto orig_block_transactions_size = pending->_pending_block_state->block->transactions.size();
      auto orig_state_transactions_size = pending->_pending_block_state->trxs.size();
      auto orig_state_actions_size      = pending->_actions.size();

      std::function<void()> callback = [this,
                                        orig_block_transactions_size,
                                        orig_state_transactions_size,
                                        orig_state_actions_size]()
      {
         pending->_pending_block_state->block->transactions.resize(orig_block_transactions_size);
         pending->_pending_block_state->trxs.resize(orig_state_transactions_size);
         pending->_actions.resize(orig_state_actions_size);
      };

      return fc::make_scoped_exit( std::move(callback) );
   }

   transaction_trace_ptr apply_onerror( const generated_transaction_object& gto,
                                        fc::time_point deadline,
                                        fc::time_point start,
                                        uint32_t billed_cpu_time_us) {
      signed_transaction etrx;
      // Deliver onerror action containing the failed deferred transaction directly back to the sender.
      etrx.actions.emplace_back( vector<permission_level>{},
                                 onerror( gto.sender_id, gto.packed_trx.data(), gto.packed_trx.size() ) );
      etrx.expiration = self.pending_block_time() + fc::microseconds(999'999); // Round up to avoid appearing expired
      etrx.set_reference_block( self.head_block_id() );

      transaction_context trx_context( self, etrx, etrx.id(), start );
      trx_context.deadline = deadline;
      trx_context.billed_cpu_time_us = billed_cpu_time_us;
      transaction_trace_ptr trace = trx_context.trace;
      try {
         trx_context.init_for_implicit_trx();
         trx_context.published = gto.published;
         trx_context.trace->action_traces.emplace_back();
         trx_context.dispatch_action( trx_context.trace->action_traces.back(), etrx.actions.back(), gto.sender );
         trx_context.finalize(); // Automatically rounds up network and CPU usage in trace and bills payers if successful

         auto restore = make_block_restore_point();
         trace->receipt = push_receipt( gto.trx_id, transaction_receipt::soft_fail,
                                        trx_context.billed_cpu_time_us, trace->net_usage );
         fc::move_append( pending->_actions, move(trx_context.executed) );

         emit( self.applied_transaction, trace );

         trx_context.squash();
         restore.cancel();
         return trace;
      } catch( const fc::exception& e ) {
         trace->except = e;
         trace->except_ptr = std::current_exception();
      }
      return trace;
   }

   void remove_scheduled_transaction( const generated_transaction_object& gto ) {
      resource_limits.add_pending_ram_usage(
         gto.payer,
         -(config::billable_size_v<generated_transaction_object> + gto.packed_trx.size())
      );
      // No need to verify_account_ram_usage since we are only reducing memory

      db.remove( gto );
   }

   bool failure_is_subjective( const fc::exception& e ) {
      auto code = e.code();
      return (code == block_net_usage_exceeded::code_value) ||
             (code == block_cpu_usage_exceeded::code_value) ||
             (code == deadline_exception::code_value)       ||
             (code == leeway_deadline_exception::code_value);
   }

   transaction_trace_ptr push_scheduled_transaction( const transaction_id_type& trxid, fc::time_point deadline, uint32_t billed_cpu_time_us ) {
      const auto& idx = db.get_index<generated_transaction_multi_index,by_trx_id>();
      auto itr = idx.find( trxid );
      FC_ASSERT( itr != idx.end(), "unknown transaction" );
      return push_scheduled_transaction( *itr, deadline, billed_cpu_time_us );
   }

   transaction_trace_ptr push_scheduled_transaction( const generated_transaction_object& gto, fc::time_point deadline, uint32_t billed_cpu_time_us )
   { try {
      auto undo_session = db.start_undo_session(true);
      fc::datastream<const char*> ds( gto.packed_trx.data(), gto.packed_trx.size() );

      auto remove_retained_state = fc::make_scoped_exit([&, this](){
         remove_scheduled_transaction(gto);
      });

      FC_ASSERT( gto.delay_until <= self.pending_block_time(), "this transaction isn't ready",
                 ("gto.delay_until",gto.delay_until)("pbt",self.pending_block_time())          );
      if( gto.expiration < self.pending_block_time() ) {
         auto trace = std::make_shared<transaction_trace>();
         trace->id = gto.trx_id;
         trace->scheduled = false;
         trace->receipt = push_receipt( gto.trx_id, transaction_receipt::expired, billed_cpu_time_us, 0 ); // expire the transaction
         undo_session.squash();
         return trace;
      }

      signed_transaction dtrx;
      fc::raw::unpack(ds,static_cast<transaction&>(dtrx) );

      transaction_context trx_context( self, dtrx, gto.trx_id );
      trx_context.deadline = deadline;
      trx_context.billed_cpu_time_us = billed_cpu_time_us;
      transaction_trace_ptr trace = trx_context.trace;
      flat_set<account_name>  bill_to_accounts;
      try {
         trx_context.init_for_deferred_trx( gto.published );
         bill_to_accounts = trx_context.bill_to_accounts;
         trx_context.exec();
         trx_context.finalize(); // Automatically rounds up network and CPU usage in trace and bills payers if successful

         auto restore = make_block_restore_point();

         trace->receipt = push_receipt( gto.trx_id,
                                        transaction_receipt::executed,
                                        trx_context.billed_cpu_time_us,
                                        trace->net_usage );

         fc::move_append( pending->_actions, move(trx_context.executed) );

         emit( self.applied_transaction, trace );

         trx_context.squash();
         undo_session.squash();
         restore.cancel();
         return trace;
      } catch( const fc::exception& e ) {
         trace->except = e;
         trace->except_ptr = std::current_exception();
         trace->elapsed = fc::time_point::now() - trx_context.start;
      }
      trx_context.undo_session.undo();

      // Only soft or hard failure logic below:

      if( gto.sender != account_name() && !failure_is_subjective(*trace->except)) {
         // Attempt error handling for the generated transaction.
         edump((trace->except->to_detail_string()));
         auto error_trace = apply_onerror( gto, deadline, trx_context.start, trx_context.billed_cpu_time_us );
         error_trace->failed_dtrx_trace = trace;
         trace = error_trace;
         if( !trace->except_ptr ) {
            undo_session.squash();
            return trace;
         }
      }

      // Only hard failure OR subjective failure logic below:

      trace->elapsed = fc::time_point::now() - trx_context.start;

      resource_limits.add_transaction_usage( bill_to_accounts, trx_context.billed_cpu_time_us, 0,
                                             block_timestamp_type(self.pending_block_time()).slot ); // Should never fail

      if (failure_is_subjective(*trace->except)) {
         // this is a subjective failure, don't remove the retained state so it can be
         // retried at a later time and don't include any artifact of the transaction in the pending block
         remove_retained_state.cancel();
      } else {
         trace->receipt = push_receipt(gto.trx_id, transaction_receipt::hard_fail, trx_context.billed_cpu_time_us, 0);
         emit( self.applied_transaction, trace );
         undo_session.squash();
      }

      return trace;
   } FC_CAPTURE_AND_RETHROW() } /// push_scheduled_transaction


   /**
    *  Adds the transaction receipt to the pending block and returns it.
    */
   template<typename T>
   const transaction_receipt& push_receipt( const T& trx, transaction_receipt_header::status_enum status,
                                            uint64_t cpu_usage_us, uint64_t net_usage ) {
      uint64_t net_usage_words = net_usage / 8;
      FC_ASSERT( net_usage_words*8 == net_usage, "net_usage is not divisible by 8" );
      pending->_pending_block_state->block->transactions.emplace_back( trx );
      transaction_receipt& r = pending->_pending_block_state->block->transactions.back();
      r.cpu_usage_us         = cpu_usage_us;
      r.net_usage_words      = net_usage_words;
      r.status               = status;
      return r;
   }

   /**
    *  This is the entry point for new transactions to the block state. It will check authorization and
    *  determine whether to execute it now or to delay it. Lastly it inserts a transaction receipt into
    *  the pending block.
    */
   transaction_trace_ptr push_transaction( const transaction_metadata_ptr& trx,
                                           fc::time_point deadline,
                                           bool implicit,
                                           uint32_t billed_cpu_time_us  )
   {
      FC_ASSERT(deadline != fc::time_point(), "deadline cannot be uninitialized");

      transaction_trace_ptr trace;
      try {
         transaction_context trx_context(self, trx->trx, trx->id);
         trx_context.deadline = deadline;
         trx_context.billed_cpu_time_us = billed_cpu_time_us;
         trace = trx_context.trace;
         try {
            if (implicit) {
               trx_context.init_for_implicit_trx();
            } else {
               trx_context.init_for_input_trx( trx->packed_trx.get_unprunable_size(),
                                               trx->packed_trx.get_prunable_size(),
                                               trx->trx.signatures.size() );
            }

            trx_context.delay = fc::seconds(trx->trx.delay_sec);

            if( !self.skip_auth_check() && !implicit ) {
               authorization.check_authorization(
                       trx->trx.actions,
                       trx->recover_keys(),
                       {},
                       trx_context.delay,
                       [](){}
                       /*std::bind(&transaction_context::add_cpu_usage_and_check_time, &trx_context,
                                 std::placeholders::_1)*/,
                       false
               );
            }

            trx_context.exec();
            trx_context.finalize(); // Automatically rounds up network and CPU usage in trace and bills payers if successful

            auto restore = make_block_restore_point();

            if (!implicit) {
               transaction_receipt::status_enum s = (trx_context.delay == fc::seconds(0))
                                                    ? transaction_receipt::executed
                                                    : transaction_receipt::delayed;
               trace->receipt = push_receipt(trx->packed_trx, s, trx_context.billed_cpu_time_us, trace->net_usage);
               pending->_pending_block_state->trxs.emplace_back(trx);
            } else {
               transaction_receipt_header r;
               r.status = transaction_receipt::executed;
               r.cpu_usage_us = trx_context.billed_cpu_time_us;
               r.net_usage_words = trace->net_usage / 8;
               trace->receipt = r;
            }

            fc::move_append(pending->_actions, move(trx_context.executed));

            // call the accept signal but only once for this transaction
            if (!trx->accepted) {
               emit( self.accepted_transaction, trx);
               trx->accepted = true;
            }

            emit(self.applied_transaction, trace);

            trx_context.squash();
            restore.cancel();

            if (!implicit) {
               unapplied_transactions.erase( trx->signed_id );
            }
            return trace;
         } catch (const fc::exception& e) {
            trace->except = e;
            trace->except_ptr = std::current_exception();
         }

         if (!failure_is_subjective(*trace->except)) {
            unapplied_transactions.erase( trx->signed_id );
         }

         return trace;
      } FC_CAPTURE_AND_RETHROW((trace))
   } /// push_transaction


   void start_block( block_timestamp_type when, uint16_t confirm_block_count ) {
      FC_ASSERT( !pending );

      FC_ASSERT( db.revision() == head->block_num, "",
                ("db.revision()", db.revision())("controller_head_block", head->block_num)("fork_db_head_block", fork_db.head()->block_num) );

      auto guard_pending = fc::make_scoped_exit([this](){
         pending.reset();
      });

      pending = db.start_undo_session(true);

      pending->_pending_block_state = std::make_shared<block_state>( *head, when ); // promotes pending schedule (if any) to active
      pending->_pending_block_state->in_current_chain = true;

      pending->_pending_block_state->set_confirmed(confirm_block_count);

      auto was_pending_promoted = pending->_pending_block_state->maybe_promote_pending();

      const auto& gpo = db.get<global_property_object>();
      if( gpo.proposed_schedule_block_num.valid() && // if there is a proposed schedule that was proposed in a block ...
          ( *gpo.proposed_schedule_block_num <= pending->_pending_block_state->dpos_irreversible_blocknum ) && // ... that has now become irreversible ...
          pending->_pending_block_state->pending_schedule.producers.size() == 0 && // ... and there is room for a new pending schedule ...
          !was_pending_promoted // ... and not just because it was promoted to active at the start of this block, then:
        )
      {
         // Promote proposed schedule to pending schedule.
         if( !replaying ) {
            ilog( "promoting proposed schedule (set in block ${proposed_num}) to pending; current block: ${n} lib: ${lib} schedule: ${schedule} ",
                  ("proposed_num", *gpo.proposed_schedule_block_num)("n", pending->_pending_block_state->block_num)
                  ("lib", pending->_pending_block_state->dpos_irreversible_blocknum)
                  ("schedule", static_cast<producer_schedule_type>(gpo.proposed_schedule) ) );
         }
         pending->_pending_block_state->set_new_producers( gpo.proposed_schedule );
         db.modify( gpo, [&]( auto& gp ) {
            gp.proposed_schedule_block_num = optional<block_num_type>();
            gp.proposed_schedule.clear();
         });
      }

      try {
         auto onbtrx = std::make_shared<transaction_metadata>( get_on_block_transaction() );
         push_transaction( onbtrx, fc::time_point::maximum(), true, self.get_global_properties().configuration.min_transaction_cpu_usage );
      } catch ( ... ) {
         ilog( "on block transaction failed, but shouldn't impact block generation, system contract needs update" );
      }

      clear_expired_input_transactions();
      update_producers_authority();
      guard_pending.cancel();
   } // start_block



   void sign_block( const std::function<signature_type( const digest_type& )>& signer_callback, bool trust  ) {
      auto p = pending->_pending_block_state;

      try {
         p->sign( signer_callback, false); //trust );
      } catch ( ... ) {
         edump(( fc::json::to_pretty_string( p->header ) ) );
         throw;
      }

      static_cast<signed_block_header&>(*p->block) = p->header;
   } /// sign_block

   void apply_block( const signed_block_ptr& b, bool trust ) { try {
      try {
         FC_ASSERT( b->block_extensions.size() == 0, "no supported extensions" );
         start_block( b->timestamp, b->confirmed );

         for( const auto& receipt : b->transactions ) {
            if( receipt.trx.contains<packed_transaction>() ) {
               auto& pt = receipt.trx.get<packed_transaction>();
               auto mtrx = std::make_shared<transaction_metadata>(pt);
               push_transaction( mtrx, fc::time_point::maximum(), false, receipt.cpu_usage_us );
            }
            else if( receipt.trx.contains<transaction_id_type>() ) {
               push_scheduled_transaction( receipt.trx.get<transaction_id_type>(), fc::time_point::maximum(), receipt.cpu_usage_us );
            }
         }

         finalize_block();
         sign_block( [&]( const auto& ){ return b->producer_signature; }, trust );

         // this is implied by the signature passing
         //FC_ASSERT( b->id() == pending->_pending_block_state->block->id(),
         //           "applying block didn't produce expected block id" );

         commit_block(false);
         return;
      } catch ( const fc::exception& e ) {
         edump((e.to_detail_string()));
         abort_block();
         throw;
      }
   } FC_CAPTURE_AND_RETHROW() } /// apply_block


   void push_block( const signed_block_ptr& b, bool trust ) {
    //  idump((fc::json::to_pretty_string(*b)));
      FC_ASSERT(!pending, "it is not valid to push a block when there is a pending block");
      try {
         FC_ASSERT( b );
         auto new_header_state = fork_db.add( b, trust );
         emit( self.accepted_block_header, new_header_state );
         maybe_switch_forks( trust );
      } FC_LOG_AND_RETHROW( )
   }

   void push_confirmation( const header_confirmation& c ) {
      FC_ASSERT(!pending, "it is not valid to push a confirmation when there is a pending block");
      fork_db.add( c );
      emit( self.accepted_confirmation, c );
      maybe_switch_forks();
   }

   void maybe_switch_forks( bool trust = false ) {
      auto new_head = fork_db.head();

      if( new_head->header.previous == head->id ) {
         try {
            apply_block( new_head->block, trust );
            fork_db.mark_in_current_chain( new_head, true );
            fork_db.set_validity( new_head, true );
            head = new_head;
         } catch ( const fc::exception& e ) {
            fork_db.set_validity( new_head, false ); // Removes new_head from fork_db index, so no need to mark it as not in the current chain.
            throw;
         }
      } else if( new_head->id != head->id ) {
         ilog("switching forks from ${current_head_id} (block number ${current_head_num}) to ${new_head_id} (block number ${new_head_num})",
              ("current_head_id", head->id)("current_head_num", head->block_num)("new_head_id", new_head->id)("new_head_num", new_head->block_num) );
         auto branches = fork_db.fetch_branch_from( new_head->id, head->id );

         for( auto itr = branches.second.begin(); itr != branches.second.end(); ++itr ) {
            fork_db.mark_in_current_chain( *itr , false );
            pop_block();
         }
         FC_ASSERT( self.head_block_id() == branches.second.back()->header.previous,
                    "loss of sync between fork_db and chainbase during fork switch" ); // _should_ never fail

         for( auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr) {
            optional<fc::exception> except;
            try {
               apply_block( (*ritr)->block, false /*don't trust*/  );
               head = *ritr;
               fork_db.mark_in_current_chain( *ritr, true );
            }
            catch (const fc::exception& e) { except = e; }
            if (except) {
               elog("exception thrown while switching forks ${e}", ("e",except->to_detail_string()));

               while (ritr != branches.first.rend() ) {
                  fork_db.set_validity( *ritr, false );
                  ++ritr;
               }

               // pop all blocks from the bad fork
               for( auto itr = (ritr + 1).base(); itr != branches.second.end(); ++itr ) {
                  fork_db.mark_in_current_chain( *itr , false );
                  pop_block();
               }
               FC_ASSERT( self.head_block_id() == branches.second.back()->header.previous,
                          "loss of sync between fork_db and chainbase during fork switch reversal" ); // _should_ never fail

               // re-apply good blocks
               for( auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr ) {
                  apply_block( (*ritr)->block, true /* we previously validated these blocks*/ );
                  head = *ritr;
                  fork_db.mark_in_current_chain( *ritr, true );
               }
               throw *except;
            } // end if exception
         } /// end for each block in branch
         ilog("successfully switched fork to new head ${new_head_id}", ("new_head_id", new_head->id));
      }
   } /// push_block

   void abort_block() {
      if( pending ) {
         for( const auto& t : pending->_pending_block_state->trxs )
            unapplied_transactions[t->signed_id] = t;
         pending.reset();
      }
   }


   bool should_enforce_runtime_limits()const {
      return false;
   }

   void set_action_merkle() {
      vector<digest_type> action_digests;
      action_digests.reserve( pending->_actions.size() );
      for( const auto& a : pending->_actions )
         action_digests.emplace_back( a.digest() );

      pending->_pending_block_state->header.action_mroot = merkle( move(action_digests) );
   }

   void set_trx_merkle() {
      vector<digest_type> trx_digests;
      const auto& trxs = pending->_pending_block_state->block->transactions;
      trx_digests.reserve( trxs.size() );
      for( const auto& a : trxs )
         trx_digests.emplace_back( a.digest() );

      pending->_pending_block_state->header.transaction_mroot = merkle( move(trx_digests) );
   }


   void finalize_block()
   {
      FC_ASSERT(pending, "it is not valid to finalize when there is no pending block");
      try {


      /*
      ilog( "finalize block ${n} (${id}) at ${t} by ${p} (${signing_key}); schedule_version: ${v} lib: ${lib} #dtrxs: ${ndtrxs} ${np}",
            ("n",pending->_pending_block_state->block_num)
            ("id",pending->_pending_block_state->header.id())
            ("t",pending->_pending_block_state->header.timestamp)
            ("p",pending->_pending_block_state->header.producer)
            ("signing_key", pending->_pending_block_state->block_signing_key)
            ("v",pending->_pending_block_state->header.schedule_version)
            ("lib",pending->_pending_block_state->dpos_irreversible_blocknum)
            ("ndtrxs",db.get_index<generated_transaction_multi_index,by_trx_id>().size())
            ("np",pending->_pending_block_state->header.new_producers)
            );
      */

      // Update resource limits:
      resource_limits.process_account_limit_updates();
      const auto& chain_config = self.get_global_properties().configuration;
      uint32_t max_virtual_mult = 1000;
      uint64_t CPU_TARGET = EOS_PERCENT(chain_config.max_block_cpu_usage, chain_config.target_block_cpu_usage_pct);
      resource_limits.set_block_parameters(
         { CPU_TARGET, chain_config.max_block_cpu_usage, config::block_cpu_usage_average_window_ms / config::block_interval_ms, max_virtual_mult, {99, 100}, {1000, 999}},
         {EOS_PERCENT(chain_config.max_block_net_usage, chain_config.target_block_net_usage_pct), chain_config.max_block_net_usage, config::block_size_average_window_ms / config::block_interval_ms, max_virtual_mult, {99, 100}, {1000, 999}}
      );
      resource_limits.process_block_usage(pending->_pending_block_state->block_num);

      set_action_merkle();
      set_trx_merkle();

      auto p = pending->_pending_block_state;
      p->id = p->header.id();

      create_block_summary(p->id);

   } FC_CAPTURE_AND_RETHROW() }

   void update_producers_authority() {
      const auto& producers = pending->_pending_block_state->active_schedule.producers;

      auto update_permission = [&]( auto& permission, auto threshold ) {
         auto auth = authority( threshold, {}, {});
         for( auto& p : producers ) {
            auth.accounts.push_back({{p.producer_name, config::active_name}, 1});
         }

         if( static_cast<authority>(permission.auth) != auth ) { // TODO: use a more efficient way to check that authority has not changed
            db.modify(permission, [&]( auto& po ) {
               po.auth = auth;
            });
         }
      };

      uint32_t num_producers = producers.size();
      auto calculate_threshold = [=]( uint32_t numerator, uint32_t denominator ) {
         return ( (num_producers * numerator) / denominator ) + 1;
      };

      update_permission( authorization.get_permission({config::producers_account_name,
                                                       config::active_name}),
                         calculate_threshold( 2, 3 ) /* more than two-thirds */                      );

      update_permission( authorization.get_permission({config::producers_account_name,
                                                       config::majority_producers_permission_name}),
                         calculate_threshold( 1, 2 ) /* more than one-half */                        );

      update_permission( authorization.get_permission({config::producers_account_name,
                                                       config::minority_producers_permission_name}),
                         calculate_threshold( 1, 3 ) /* more than one-third */                       );

      //TODO: Add tests
   }

   void create_block_summary(const block_id_type& id) {
      auto block_num = block_header::num_from_id(id);
      auto sid = block_num & 0xffff;
      db.modify( db.get<block_summary_object,by_id>(sid), [&](block_summary_object& bso ) {
          bso.block_id = id;
      });
   }


   void clear_expired_input_transactions() {
      //Look for expired transactions in the deduplication list, and remove them.
      auto& transaction_idx = db.get_mutable_index<transaction_multi_index>();
      const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
      auto now = self.pending_block_time();
      while( (!dedupe_index.empty()) && ( now > fc::time_point(dedupe_index.begin()->expiration) ) ) {
         transaction_idx.remove(*dedupe_index.begin());
      }
   }

   /*
   bool should_check_tapos()const { return true; }

   void validate_tapos( const transaction& trx )const {
      if( !should_check_tapos() ) return;

      const auto& tapos_block_summary = db.get<block_summary_object>((uint16_t)trx.ref_block_num);

      //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
      EOS_ASSERT(trx.verify_reference_block(tapos_block_summary.block_id), invalid_ref_block_exception,
                 "Transaction's reference block did not match. Is this transaction from a different fork?",
                 ("tapos_summary", tapos_block_summary));
   }
   */


   /**
    *  At the start of each block we notify the system contract with a transaction that passes in
    *  the block header of the prior block (which is currently our head block)
    */
   signed_transaction get_on_block_transaction()
   {
      action on_block_act;
      on_block_act.account = config::system_account_name;
      on_block_act.name = N(onblock);
      on_block_act.authorization = vector<permission_level>{{config::system_account_name, config::active_name}};
      on_block_act.data = fc::raw::pack(self.head_block_header());

      signed_transaction trx;
      trx.actions.emplace_back(std::move(on_block_act));
      trx.set_reference_block(self.head_block_id());
      trx.expiration = self.pending_block_time() + fc::microseconds(999'999); // Round up to nearest second to avoid appearing expired
      return trx;
   }

}; /// controller_impl

const resource_limits_manager&   controller::get_resource_limits_manager()const
{
   return my->resource_limits;
}
resource_limits_manager&         controller::get_mutable_resource_limits_manager()
{
   return my->resource_limits;
}

const authorization_manager&   controller::get_authorization_manager()const
{
   return my->authorization;
}
authorization_manager&         controller::get_mutable_authorization_manager()
{
   return my->authorization;
}

controller::controller( const controller::config& cfg )
:my( new controller_impl( cfg, *this ) )
{
}

controller::~controller() {
   my->abort_block();
}


void controller::startup() {

   // ilog( "${c}", ("c",fc::json::to_pretty_string(cfg)) );
   my->add_indices();

   my->head = my->fork_db.head();
   if( !my->head ) {
      elog( "No head block in fork db, perhaps we need to replay" );
   }
   my->init();
}

chainbase::database& controller::db()const { return my->db; }

fork_database& controller::fork_db()const { return my->fork_db; }


void controller::start_block( block_timestamp_type when, uint16_t confirm_block_count ) {
   my->start_block(when, confirm_block_count);
}

void controller::finalize_block() {
   my->finalize_block();
}

void controller::sign_block( const std::function<signature_type( const digest_type& )>& signer_callback ) {
   my->sign_block( signer_callback, false /* don't trust */);
}

void controller::commit_block() {
   my->commit_block(true);
}

void controller::abort_block() {
   my->abort_block();
}

void controller::push_block( const signed_block_ptr& b, bool trust ) {
   my->push_block( b, trust );
}

void controller::push_confirmation( const header_confirmation& c ) {
   my->push_confirmation( c );
}

transaction_trace_ptr controller::push_transaction( const transaction_metadata_ptr& trx, fc::time_point deadline, uint32_t billed_cpu_time_us ) {
   return my->push_transaction(trx, deadline, false, billed_cpu_time_us);
}

transaction_trace_ptr controller::push_scheduled_transaction( const transaction_id_type& trxid, fc::time_point deadline, uint32_t billed_cpu_time_us )
{
   return my->push_scheduled_transaction( trxid, deadline, billed_cpu_time_us );
}

uint32_t controller::head_block_num()const {
   return my->head->block_num;
}
time_point controller::head_block_time()const {
   return my->head->header.timestamp;
}
block_id_type controller::head_block_id()const {
   return my->head->id;
}
account_name  controller::head_block_producer()const {
   return my->head->header.producer;
}
const block_header& controller::head_block_header()const {
   return my->head->header;
}
block_state_ptr controller::head_block_state()const {
   return my->head;
}

block_state_ptr controller::pending_block_state()const {
   if( my->pending ) return my->pending->_pending_block_state;
   return block_state_ptr();
}
time_point controller::pending_block_time()const {
   FC_ASSERT( my->pending, "no pending block" );
   return my->pending->_pending_block_state->header.timestamp;
}

uint32_t controller::last_irreversible_block_num() const {
   return std::max(my->head->bft_irreversible_blocknum, my->head->dpos_irreversible_blocknum);
}

block_id_type controller::last_irreversible_block_id() const {
   auto lib_num = last_irreversible_block_num();
   const auto& tapos_block_summary = db().get<block_summary_object>((uint16_t)lib_num);

   if( block_header::num_from_id(tapos_block_summary.block_id) == lib_num )
      return tapos_block_summary.block_id;

   return fetch_block_by_number(lib_num)->id();

}

const dynamic_global_property_object& controller::get_dynamic_global_properties()const {
  return my->db.get<dynamic_global_property_object>();
}
const global_property_object& controller::get_global_properties()const {
  return my->db.get<global_property_object>();
}

signed_block_ptr controller::fetch_block_by_id( block_id_type id )const {
   auto state = my->fork_db.get_block(id);
   if( state ) return state->block;
   auto bptr = fetch_block_by_number( block_header::num_from_id(id) );
   if( bptr && bptr->id() == id ) return bptr;
   return signed_block_ptr();
}

signed_block_ptr controller::fetch_block_by_number( uint32_t block_num )const  { try {
   auto blk_state = my->fork_db.get_block_in_current_chain_by_num( block_num );
   if( blk_state ) {
      return blk_state->block;
   }

   return my->blog.read_block_by_num(block_num);
} FC_CAPTURE_AND_RETHROW( (block_num) ) }

block_id_type controller::get_block_id_for_num( uint32_t block_num )const { try {
   auto blk_state = my->fork_db.get_block_in_current_chain_by_num( block_num );
   if( blk_state ) {
      return blk_state->id;
   }

   auto signed_blk = my->blog.read_block_by_num(block_num);

   EOS_ASSERT( BOOST_LIKELY( signed_blk != nullptr ), unknown_block_exception,
               "Could not find block: ${block}", ("block", block_num) );

   return signed_blk->id();
} FC_CAPTURE_AND_RETHROW( (block_num) ) }

void controller::pop_block() {
   my->pop_block();
}

bool controller::set_proposed_producers( vector<producer_key> producers ) {
   const auto& gpo = get_global_properties();
   auto cur_block_num = head_block_num() + 1;

   if( gpo.proposed_schedule_block_num.valid() ) {
      if( *gpo.proposed_schedule_block_num != cur_block_num )
         return false; // there is already a proposed schedule set in a previous block, wait for it to become pending

      if( std::equal( producers.begin(), producers.end(),
                      gpo.proposed_schedule.producers.begin(), gpo.proposed_schedule.producers.end() ) )
         return false; // the proposed producer schedule does not change
   }

   producer_schedule_type sch;

   decltype(sch.producers.cend()) end;
   decltype(end)                  begin;

   if( my->pending->_pending_block_state->pending_schedule.producers.size() == 0 ) {
      const auto& active_sch = my->pending->_pending_block_state->active_schedule;
      begin = active_sch.producers.begin();
      end   = active_sch.producers.end();
      sch.version = active_sch.version + 1;
   } else {
      const auto& pending_sch = my->pending->_pending_block_state->pending_schedule;
      begin = pending_sch.producers.begin();
      end   = pending_sch.producers.end();
      sch.version = pending_sch.version + 1;
   }

   if( std::equal( producers.begin(), producers.end(), begin, end ) )
      return false; // the producer schedule would not change

   sch.producers = std::move(producers);

   my->db.modify( gpo, [&]( auto& gp ) {
      gp.proposed_schedule_block_num = cur_block_num;
      gp.proposed_schedule = std::move(sch);
   });
   return true;
}

const producer_schedule_type&    controller::active_producers()const {
   if ( !(my->pending) )
      return  my->head->active_schedule;
   return my->pending->_pending_block_state->active_schedule;
}

const producer_schedule_type&    controller::pending_producers()const {
   if ( !(my->pending) )
      return  my->head->pending_schedule;
   return my->pending->_pending_block_state->pending_schedule;
}

optional<producer_schedule_type> controller::proposed_producers()const {
   const auto& gpo = get_global_properties();
   if( !gpo.proposed_schedule_block_num.valid() )
      return optional<producer_schedule_type>();

   return gpo.proposed_schedule;
}

bool controller::skip_auth_check()const {
   return my->replaying_irreversible && !my->conf.force_all_checks;
}

const apply_handler* controller::find_apply_handler( account_name receiver, account_name scope, action_name act ) const
{
   auto native_handler_scope = my->apply_handlers.find( receiver );
   if( native_handler_scope != my->apply_handlers.end() ) {
      auto handler = native_handler_scope->second.find( make_pair( scope, act ) );
      if( handler != native_handler_scope->second.end() )
         return &handler->second;
   }
   return nullptr;
}
wasm_interface& controller::get_wasm_interface() {
   return my->wasmif;
}

const account_object& controller::get_account( account_name name )const
{ try {
   return my->db.get<account_object, by_name>(name);
} FC_CAPTURE_AND_RETHROW( (name) ) }

vector<transaction_metadata_ptr> controller::get_unapplied_transactions() const {
   vector<transaction_metadata_ptr> result;
   result.reserve(my->unapplied_transactions.size());
   for ( const auto& entry: my->unapplied_transactions ) {
      result.emplace_back(entry.second);
   }
   return result;
}

void controller::drop_unapplied_transaction(const transaction_metadata_ptr& trx) {
   my->unapplied_transactions.erase(trx->signed_id);
}

vector<transaction_id_type> controller::get_scheduled_transactions() const {
   const auto& idx = db().get_index<generated_transaction_multi_index,by_delay>();

   vector<transaction_id_type> result;

   static const size_t max_reserve = 64;
   result.reserve(std::min(idx.size(), max_reserve));

   auto itr = idx.begin();
   while( itr != idx.end() && itr->delay_until <= pending_block_time() ) {
      result.emplace_back(itr->trx_id);
      ++itr;
   }
   return result;
}

void controller::validate_referenced_accounts( const transaction& trx )const {
   for( const auto& a : trx.context_free_actions ) {
      auto* code = my->db.find<account_object, by_name>(a.account);
      EOS_ASSERT( code != nullptr, transaction_exception,
                  "action's code account '${account}' does not exist", ("account", a.account) );
      EOS_ASSERT( a.authorization.size() == 0, transaction_exception,
                  "context-free actions cannot have authorizations" );
   }
   bool one_auth = false;
   for( const auto& a : trx.actions ) {
      auto* code = my->db.find<account_object, by_name>(a.account);
      EOS_ASSERT( code != nullptr, transaction_exception,
                  "action's code account '${account}' does not exist", ("account", a.account) );
      for( const auto& auth : a.authorization ) {
         one_auth = true;
         auto* actor = my->db.find<account_object, by_name>(auth.actor);
         EOS_ASSERT( actor  != nullptr, transaction_exception,
                     "action's authorizing actor '${account}' does not exist", ("account", auth.actor) );
         EOS_ASSERT( my->authorization.find_permission(auth) != nullptr, transaction_exception,
                     "action's authorizations include a non-existent permission: {permission}",
                     ("permission", auth) );
      }
   }
   EOS_ASSERT( one_auth, tx_no_auths, "transaction must have at least one authorization" );
}

void controller::validate_expiration( const transaction& trx )const { try {
   const auto& chain_configuration = get_global_properties().configuration;

   EOS_ASSERT( time_point(trx.expiration) >= pending_block_time(),
               expired_tx_exception,
               "transaction has expired, "
               "expiration is ${trx.expiration} and pending block time is ${pending_block_time}",
               ("trx.expiration",trx.expiration)("pending_block_time",pending_block_time()));
   EOS_ASSERT( time_point(trx.expiration) <= pending_block_time() + fc::seconds(chain_configuration.max_transaction_lifetime),
               tx_exp_too_far_exception,
               "Transaction expiration is too far in the future relative to the reference time of ${reference_time}, "
               "expiration is ${trx.expiration} and the maximum transaction lifetime is ${max_til_exp} seconds",
               ("trx.expiration",trx.expiration)("reference_time",pending_block_time())
               ("max_til_exp",chain_configuration.max_transaction_lifetime) );
} FC_CAPTURE_AND_RETHROW((trx)) }

void controller::validate_tapos( const transaction& trx )const { try {
   const auto& tapos_block_summary = db().get<block_summary_object>((uint16_t)trx.ref_block_num);

   //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
   EOS_ASSERT(trx.verify_reference_block(tapos_block_summary.block_id), invalid_ref_block_exception,
              "Transaction's reference block did not match. Is this transaction from a different fork?",
              ("tapos_summary", tapos_block_summary));
} FC_CAPTURE_AND_RETHROW() }


} } /// eosio::chain
