#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>

#include <eosio/chain/block_log.hpp>
#include <eosio/chain/fork_database.hpp>
#include <eosio/chain/exceptions.hpp>

#include <eosio/chain/eosio_contract.hpp>

#include <eosio/chain/protocol_feature_manager.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <eosio/chain/platform_timer.hpp>

#include <chainbase/chainbase.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/scoped_exit.hpp>
#include <fc/variant_object.hpp>
#include <b1/chain_kv/chain_kv.hpp>

#include <new>

#if defined(EOSIO_EOS_VM_RUNTIME_ENABLED) || defined(EOSIO_EOS_VM_JIT_RUNTIME_ENABLED)
#include <eosio/vm/allocator.hpp>
#endif

namespace eosio { namespace chain {

using resource_limits::resource_limits_manager;

struct building_block {
   building_block( const block_header_state& prev,
                   block_timestamp_type when,
                   uint16_t num_prev_blocks_to_confirm,
                   const vector<digest_type>& new_protocol_feature_activations )
   :_pending_block_header_state( prev.next( when, num_prev_blocks_to_confirm ) )
   ,_new_protocol_feature_activations( new_protocol_feature_activations )
   ,_trx_mroot_or_receipt_digests( digests_t{} )
   {}

   pending_block_header_state                 _pending_block_header_state;
   std::optional<producer_authority_schedule> _new_pending_producer_schedule;
   vector<digest_type>                        _new_protocol_feature_activations;
   size_t                                     _num_new_protocol_features_that_have_activated = 0;
   deque<transaction_metadata_ptr>            _pending_trx_metas;
   deque<transaction_receipt>                 _pending_trx_receipts; // boost deque in 1.71 with 1024 elements performs better
   std::variant<checksum256_type, digests_t>  _trx_mroot_or_receipt_digests;
   digests_t                                  _action_receipt_digests;
};

struct assembled_block {
   block_id_type                     _id;
   pending_block_header_state        _pending_block_header_state;
   deque<transaction_metadata_ptr>   _trx_metas;
   signed_block_ptr                  _unsigned_block;

   // if the _unsigned_block pre-dates block-signing authorities this may be present.
   std::optional<producer_authority_schedule> _new_producer_authority_cache;
};

struct completed_block {
   block_state_ptr                   _block_state;
};

using block_stage_type = std::variant<building_block, assembled_block, completed_block>;

struct pending_state {
   pending_state( combined_session&& s, const block_header_state& prev,
                  block_timestamp_type when,
                  uint16_t num_prev_blocks_to_confirm,
                  const vector<digest_type>& new_protocol_feature_activations )
   :_db_session( move(s) )
   ,_block_stage( building_block( prev, when, num_prev_blocks_to_confirm, new_protocol_feature_activations ) )
   {}

   combined_session                   _db_session;
   block_stage_type                   _block_stage;
   controller::block_status           _block_status = controller::block_status::incomplete;
   std::optional<block_id_type>       _producer_block_id;

   /** @pre _block_stage cannot hold completed_block alternative */
   const pending_block_header_state& get_pending_block_header_state()const {
      if( std::holds_alternative<building_block>(_block_stage) )
         return std::get<building_block>(_block_stage)._pending_block_header_state;

      return std::get<assembled_block>(_block_stage)._pending_block_header_state;     
   }

   const deque<transaction_receipt>& get_trx_receipts()const {
      if( std::holds_alternative<building_block>(_block_stage) )
         return std::get<building_block>(_block_stage)._pending_trx_receipts;

      if( std::holds_alternative<assembled_block>(_block_stage) )
         return std::get<assembled_block>(_block_stage)._unsigned_block->transactions;

      return std::get<completed_block>(_block_stage)._block_state->block->transactions;
    }

   deque<transaction_metadata_ptr> extract_trx_metas() {
      if( std::holds_alternative<building_block>(_block_stage) )
         return std::move( std::get<building_block>(_block_stage)._pending_trx_metas );

      if( std::holds_alternative<assembled_block>(_block_stage) )
         return std::move( std::get<assembled_block>(_block_stage)._trx_metas );

      return std::get<completed_block>(_block_stage)._block_state->extract_trxs_metas();
   }

   bool is_protocol_feature_activated( const digest_type& feature_digest )const {
      if( std::holds_alternative<building_block>(_block_stage) ) {
        auto& bb = std::get<building_block>(_block_stage);
         const auto& activated_features = bb._pending_block_header_state.prev_activated_protocol_features->protocol_features;

         if( activated_features.find( feature_digest ) != activated_features.end() ) return true;

         if( bb._num_new_protocol_features_that_have_activated == 0 ) return false;

         auto end = bb._new_protocol_feature_activations.begin() + bb._num_new_protocol_features_that_have_activated;
         return (std::find( bb._new_protocol_feature_activations.begin(), end, feature_digest ) != end);
      }

      if( std::holds_alternative<assembled_block>(_block_stage) ) {
         // Calling is_protocol_feature_activated during the assembled_block stage is not efficient.
         // We should avoid doing it.
         // In fact for now it isn't even implemented.
         EOS_THROW( misc_exception,
                    "checking if protocol feature is activated in the assembled_block stage is not yet supported" );
         // TODO: implement this
      }

      const auto& activated_features = std::get<completed_block>(_block_stage)._block_state->activated_protocol_features->protocol_features;
      return (activated_features.find( feature_digest ) != activated_features.end());
   }

   void push() {
      _db_session.push();
   }
};

struct controller_impl {

   // LLVM sets the new handler, we need to reset this to throw a bad_alloc exception so we can possibly exit cleanly
   // and not just abort.
   struct reset_new_handler {
      reset_new_handler() { std::set_new_handler([](){ throw std::bad_alloc(); }); }
   };

   reset_new_handler                   rnh; // placed here to allow for this to be set before constructing the other fields
   controller&                         self;
   std::function<void()>               shutdown;
   chainbase::database                 db;
   chainbase::database                 reversible_blocks; ///< a special database to persist blocks that have successfully been applied but are still reversible
   combined_database                   kv_db;
   block_log                           blog;
   std::optional<pending_state>        pending;
   block_state_ptr                     head;
   fork_database                       fork_db;
   wasm_interface                      wasmif;
   resource_limits_manager             resource_limits;
   authorization_manager               authorization;
   protocol_feature_manager            protocol_features;
   controller::config                  conf;
   const chain_id_type                 chain_id; // read by thread_pool threads, value will not be changed
   std::optional<fc::time_point>       replay_head_time;
   db_read_mode                        read_mode = db_read_mode::SPECULATIVE;
   bool                                in_trx_requiring_checks = false; ///< if true, checks that are normally skipped on replay (e.g. auth checks) cannot be skipped
   std::optional<fc::microseconds>     subjective_cpu_leeway;
   bool                                trusted_producer_light_validation = false;
   uint32_t                            snapshot_head_block = 0;
   named_thread_pool                   thread_pool;
   platform_timer                      timer;
   fc::logger*                         deep_mind_logger = nullptr;
#if defined(EOSIO_EOS_VM_RUNTIME_ENABLED) || defined(EOSIO_EOS_VM_JIT_RUNTIME_ENABLED)
   vm::wasm_allocator               wasm_alloc;
#endif

   typedef pair<scope_name,action_name>                   handler_key;
   map< account_name, map<handler_key, apply_handler> >   apply_handlers;
   unordered_map< builtin_protocol_feature_t, std::function<void(controller_impl&)>, enum_hash<builtin_protocol_feature_t> > protocol_feature_activation_handlers;

   void pop_block() {
      auto prev = fork_db.get_block( head->header.previous );

      if( !prev ) {
         EOS_ASSERT( fork_db.root()->id == head->header.previous, block_validate_exception, "attempt to pop beyond last irreversible block" );
         prev = fork_db.root();
      }

      if( const auto* b = reversible_blocks.find<reversible_block_object,by_num>(head->block_num) )
      {
         reversible_blocks.remove( *b );
      }

      if ( read_mode == db_read_mode::SPECULATIVE ) {
         EOS_ASSERT( head->block, block_validate_exception, "attempting to pop a block that was sparsely loaded from a snapshot");
      }

      head = prev;

      kv_db.undo();

      protocol_features.popped_blocks_to( prev->block_num );
   }

   template<builtin_protocol_feature_t F>
   void on_activation();

   template<builtin_protocol_feature_t F>
   inline void set_activation_handler() {
      auto res = protocol_feature_activation_handlers.emplace( F, &controller_impl::on_activation<F> );
      EOS_ASSERT( res.second, misc_exception, "attempting to set activation handler twice" );
   }

   inline void trigger_activation_handler( builtin_protocol_feature_t f ) {
      auto itr = protocol_feature_activation_handlers.find( f );
      if( itr == protocol_feature_activation_handlers.end() ) return;
      (itr->second)( *this );
   }

   void set_apply_handler( account_name receiver, account_name contract, action_name action, apply_handler v ) {
      apply_handlers[receiver][make_pair(contract,action)] = v;
   }

   controller_impl( const controller::config& cfg, controller& s, protocol_feature_set&& pfs, const chain_id_type& chain_id )
   :rnh(),
    self(s),
    db( cfg.state_dir,
        cfg.read_only ? database::read_only : database::read_write,
        cfg.state_size, false, cfg.db_map_mode ),
    reversible_blocks( cfg.blog.log_dir/config::reversible_blocks_dir_name,
        cfg.read_only ? database::read_only : database::read_write,
        cfg.reversible_cache_size, false, cfg.db_map_mode ),
    kv_db(cfg.backing_store == backing_store_type::CHAINBASE
          ? combined_database(db, cfg.persistent_storage_mbytes_batch)
          : combined_database(db, cfg)), 
    blog( cfg.blog ),
    fork_db( cfg.state_dir ),
    wasmif( cfg.wasm_runtime, cfg.eosvmoc_tierup, db, cfg.state_dir, cfg.eosvmoc_config ),
    resource_limits( db, [&s]() { return s.get_deep_mind_logger(); }),
    authorization( s, db ),
    protocol_features( std::move(pfs), [&s]() { return s.get_deep_mind_logger(); } ),
    conf( cfg ),
    chain_id( chain_id ),
    read_mode( cfg.read_mode ),
    thread_pool( "chain", cfg.thread_pool_size )
   {
      fork_db.open( [this]( block_timestamp_type timestamp,
                            const flat_set<digest_type>& cur_features,
                            const vector<digest_type>& new_features )
                           { check_protocol_features( timestamp, cur_features, new_features ); }
      );

      set_activation_handler<builtin_protocol_feature_t::preactivate_feature>();
      set_activation_handler<builtin_protocol_feature_t::replace_deferred>();
      set_activation_handler<builtin_protocol_feature_t::get_sender>();
      set_activation_handler<builtin_protocol_feature_t::webauthn_key>();
      set_activation_handler<builtin_protocol_feature_t::wtmsig_block_signatures>();
      set_activation_handler<builtin_protocol_feature_t::action_return_value>();
      set_activation_handler<builtin_protocol_feature_t::kv_database>();
      set_activation_handler<builtin_protocol_feature_t::configurable_wasm_limits>();
      set_activation_handler<builtin_protocol_feature_t::blockchain_parameters>();

      self.irreversible_block.connect([this](const block_state_ptr& bsp) {
         wasmif.current_lib(bsp->block_num);
      });

#define SET_APP_HANDLER( receiver, contract, action) \
   set_apply_handler( account_name(#receiver), account_name(#contract), action_name(#action), \
                      &BOOST_PP_CAT(apply_, BOOST_PP_CAT(contract, BOOST_PP_CAT(_,action) ) ) )

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
         s( std::forward<Arg>( a ));
      } catch (std::bad_alloc& e) {
         wlog( "std::bad_alloc: ${w}", ("w", e.what()) );
         throw e;
      } catch (boost::interprocess::bad_alloc& e) {
         wlog( "boost::interprocess::bad alloc: ${w}", ("w", e.what()) );
         throw e;
      } catch ( controller_emit_signal_exception& e ) {
         wlog( "controller_emit_signal_exception: ${details}", ("details", e.to_detail_string()) );
         throw e;
      } catch ( fc::exception& e ) {
         wlog( "fc::exception: ${details}", ("details", e.to_detail_string()) );
      } catch ( std::exception& e ) {
         wlog( "std::exception: ${details}", ("details", e.what()) );
      } catch ( ... ) {
         wlog( "signal handler threw exception" );
      }
   }

   void log_irreversible() {
      EOS_ASSERT( fork_db.root(), fork_database_exception, "fork database not properly initialized" );

      const auto& log_head = blog.head();

      auto lib_num = log_head ? log_head->block_num() : (blog.first_block_num() - 1);

      auto root_id = fork_db.root()->id;

      if( log_head ) {
         // todo: move this check to startup so id does not have to be calculated
         EOS_ASSERT( root_id == log_head->calculate_id(), fork_database_exception, "fork database root does not match block log head" );
      } else {
         EOS_ASSERT( fork_db.root()->block_num == lib_num, fork_database_exception,
                     "empty block log expects the first appended block to build off a block that is not the fork database root. root block number: ${block_num}, lib: ${lib_num}", ("block_num", fork_db.root()->block_num) ("lib_num", lib_num) );
      }

      auto fork_head = (read_mode == db_read_mode::IRREVERSIBLE) ? fork_db.pending_head() : fork_db.head();

      if( fork_head->dpos_irreversible_blocknum <= lib_num )
         return;

      auto branch = fork_db.fetch_branch( fork_head->id, fork_head->dpos_irreversible_blocknum );
      try {
         const auto& rbi = reversible_blocks.get_index<reversible_block_index,by_num>();

         std::vector<std::future<std::tuple<signed_block_ptr, std::vector<char>>>> v;
         v.reserve( branch.size() );
         for( auto bitr = branch.rbegin(); bitr != branch.rend(); ++bitr ) {
            v.emplace_back( blog.create_append_future( thread_pool.get_executor(), (*bitr)->block,
                                                       packed_transaction::cf_compression_type::none ) );
         }
         auto it = v.begin();

         for( auto bitr = branch.rbegin(); bitr != branch.rend(); ++bitr ) {
            if( read_mode == db_read_mode::IRREVERSIBLE ) {
               apply_block( *bitr, controller::block_status::complete, trx_meta_cache_lookup{} );
               head = (*bitr);
               fork_db.mark_valid( head );
            }

            emit( self.irreversible_block, *bitr );

            // blog.append could fail due to failures like running out of space.
            // Do it before commit so that in case it throws, DB can be rolled back.
            blog.append( std::move( *it ) );
            ++it;

            kv_db.commit( (*bitr)->block_num );
            root_id = (*bitr)->id;

            auto rbitr = rbi.begin();
            while( rbitr != rbi.end() && rbitr->blocknum <= (*bitr)->block_num ) {
               reversible_blocks.remove( *rbitr );
               rbitr = rbi.begin();
            }
         }
      } catch( std::exception& ) {
         if( root_id != fork_db.root()->id ) {
            fork_db.advance_root( root_id );
         }
         throw;
      }

      if( root_id != fork_db.root()->id ) {
         branch.emplace_back(fork_db.root());
         fork_db.advance_root( root_id );
      }

      // delete branch in thread pool
      boost::asio::post( thread_pool.get_executor(), [branch{std::move(branch)}]() {} );
   }

   /**
    *  Sets fork database head to the genesis state.
    */
   void initialize_blockchain_state(const genesis_state& genesis) {
      wlog( "Initializing new blockchain with genesis state" );
      producer_authority_schedule initial_schedule = { 0, { producer_authority{config::system_account_name, block_signing_authority_v0{ 1, {{genesis.initial_key, 1}} } } } };
      legacy::producer_schedule_type initial_legacy_schedule{ 0, {{config::system_account_name, genesis.initial_key}} };

      block_header_state genheader;
      genheader.active_schedule                = initial_schedule;
      genheader.pending_schedule.schedule      = initial_schedule;
      // NOTE: if wtmsig block signatures are enabled at genesis time this should be the hash of a producer authority schedule
      genheader.pending_schedule.schedule_hash = fc::sha256::hash(initial_legacy_schedule);
      genheader.header.timestamp               = genesis.initial_timestamp;
      genheader.header.action_mroot            = genesis.compute_chain_id();
      genheader.id                             = genheader.header.calculate_id();
      genheader.block_num                      = genheader.header.block_num();

      head = std::make_shared<block_state>();
      static_cast<block_header_state&>(*head) = genheader;
      head->activated_protocol_features = std::make_shared<protocol_feature_activation_set>();
      head->block = std::make_shared<signed_block>(genheader.header);
      kv_db.set_revision( head->block_num );
      initialize_database(genesis);
   }

   void replay(std::function<bool()> check_shutdown) {
      auto blog_head = blog.head();
      auto blog_head_time = blog_head->timestamp.to_time_point();
      replay_head_time = blog_head_time;
      auto start_block_num = head->block_num + 1;
      auto start = fc::time_point::now();

      std::exception_ptr except_ptr;

      if( start_block_num <= blog_head->block_num() ) {
         ilog( "existing block log, attempting to replay from ${s} to ${n} blocks",
               ("s", start_block_num)("n", blog_head->block_num()) );
         try {
            while( std::unique_ptr<signed_block> next = blog.read_signed_block_by_num( head->block_num + 1 ) ) {
               auto block_num = next->block_num();
               replay_push_block( std::move(next), controller::block_status::irreversible );
               if( check_shutdown() ) break;
               if( block_num % 500 == 0 ) {
                  ilog( "${n} of ${head}", ("n", block_num)("head", blog_head->block_num()) );
               }
            }
         } catch(  const database_guard_exception& e ) {
            except_ptr = std::current_exception();
         }
         ilog( "${n} irreversible blocks replayed", ("n", 1 + head->block_num - start_block_num) );

         auto pending_head = fork_db.pending_head();
         if( pending_head->block_num < head->block_num || head->block_num < fork_db.root()->block_num ) {
            ilog( "resetting fork database with new last irreversible block as the new root: ${id}",
                  ("id", head->id) );
            fork_db.reset( *head );
         } else if( head->block_num != fork_db.root()->block_num ) {
            auto new_root = fork_db.search_on_branch( pending_head->id, head->block_num );
            EOS_ASSERT( new_root, fork_database_exception, "unexpected error: could not find new LIB in fork database" );
            ilog( "advancing fork database root to new last irreversible block within existing fork database: ${id}",
                  ("id", new_root->id) );
            fork_db.mark_valid( new_root );
            fork_db.advance_root( new_root->id );
         }

         // if the irreverible log is played without undo sessions enabled, we need to sync the
         // revision ordinal to the appropriate expected value here.
         if( self.skip_db_sessions( controller::block_status::irreversible ) )
            kv_db.set_revision( head->block_num );
      } else {
         ilog( "no irreversible blocks need to be replayed" );
      }

      if( !except_ptr && !check_shutdown() ) {
         int rev = 0;
         while( auto obj = reversible_blocks.find<reversible_block_object,by_num>(head->block_num+1) ) {
            if( check_shutdown() ) break;
            ++rev;
            replay_push_block( obj->get_block(), controller::block_status::validated );
         }
         ilog( "${n} reversible blocks replayed", ("n",rev) );
      }

      auto end = fc::time_point::now();
      ilog( "replayed ${n} blocks in ${duration} seconds, ${mspb} ms/block",
            ("n", head->block_num + 1 - start_block_num)("duration", (end-start).count()/1000000)
            ("mspb", ((end-start).count()/1000.0)/(head->block_num-start_block_num)) );
      replay_head_time.reset();

      if( except_ptr ) {
         std::rethrow_exception( except_ptr );
      }
   }

   void startup(std::function<void()> shutdown, std::function<bool()> check_shutdown, const snapshot_reader_ptr& snapshot) {
      EOS_ASSERT( snapshot, snapshot_exception, "No snapshot reader provided" );
      EOS_ASSERT( db.revision() == kv_db.revision(), database_revision_mismatch_exception,
                  "chainbase is at revision ${a}, but chain-kv is at revision ${b}", ("a", db.revision())("b", kv_db.revision()) );
      this->shutdown = shutdown;
      ilog( "Starting initialization from snapshot, this may take a significant amount of time" );
      try {
         snapshot->validate();
         if( blog.head() ) {
            kv_db.read_from_snapshot( snapshot, blog.first_block_num(), blog.head()->block_num(),
                                      authorization, resource_limits,
                                      fork_db, head, snapshot_head_block, chain_id );
         } else {
            kv_db.read_from_snapshot( snapshot, 0, std::numeric_limits<uint32_t>::max(),
                                      authorization, resource_limits,
                                      fork_db, head, snapshot_head_block, chain_id );
            const uint32_t lib_num = head->block_num;
            EOS_ASSERT( lib_num > 0, snapshot_exception,
                        "Snapshot indicates controller head at block number 0, but that is not allowed. "
                        "Snapshot is invalid." );
            blog.reset( chain_id, lib_num + 1 );
         }
         const auto hash = calculate_integrity_hash();
         ilog( "database initialized with hash: ${hash}", ("hash", hash) );

         init(check_shutdown, true);
      } catch (boost::interprocess::bad_alloc& e) {
         elog( "db storage not configured to have enough storage for the provided snapshot, please increase and retry snapshot" );
         throw e;
      }

      ilog( "Finished initialization from snapshot" );
   }

   void startup(std::function<void()> shutdown, std::function<bool()> check_shutdown, const genesis_state& genesis) {
      EOS_ASSERT( db.revision() < 1, database_exception, "This version of controller::startup only works with a fresh state database." );
      EOS_ASSERT( db.revision() == kv_db.revision(), database_revision_mismatch_exception,
                  "chainbase is at revision ${a}, but chain-kv is at revision ${b}", ("a", db.revision())("b", kv_db.revision()) );
      const auto& genesis_chain_id = genesis.compute_chain_id();
      EOS_ASSERT( genesis_chain_id == chain_id, chain_id_type_exception,
                  "genesis state provided to startup corresponds to a chain ID (${genesis_chain_id}) that does not match the chain ID that controller was constructed with (${controller_chain_id})",
                  ("genesis_chain_id", genesis_chain_id)("controller_chain_id", chain_id)
      );

      this->shutdown = shutdown;
      if( fork_db.head() ) {
         if( read_mode == db_read_mode::IRREVERSIBLE && fork_db.head()->id != fork_db.root()->id ) {
            fork_db.rollback_head_to_root();
         }
         wlog( "No existing chain state. Initializing fresh blockchain state." );
      } else {
         wlog( "No existing chain state or fork database. Initializing fresh blockchain state and resetting fork database.");
      }
      initialize_blockchain_state(genesis); // sets head to genesis state

      if( !fork_db.head() ) {
         fork_db.reset( *head );
      }

      if( blog.head() ) {
         EOS_ASSERT( blog.first_block_num() == 1, block_log_exception,
                     "block log does not start with genesis block"
         );
      } else {
         blog.reset( genesis, head->block, packed_transaction::cf_compression_type::none );
      }
      init(check_shutdown, true);
   }

   void startup(std::function<void()> shutdown, std::function<bool()> check_shutdown) {
      EOS_ASSERT( db.revision() >= 1, database_exception, "This version of controller::startup does not work with a fresh state database." );
      EOS_ASSERT( db.revision() == kv_db.revision(), database_revision_mismatch_exception,
                  "chainbase is at revision ${a}, but chain-kv is at revision ${b}", ("a", db.revision())("b", kv_db.revision()) );
      EOS_ASSERT( fork_db.head(), fork_database_exception, "No existing fork database despite existing chain state. Replay required." );

      this->shutdown = shutdown;
      uint32_t lib_num = fork_db.root()->block_num;
      auto first_block_num = blog.first_block_num();
      if( blog.head() ) {
         EOS_ASSERT( first_block_num <= lib_num && lib_num <= blog.head()->block_num(),
                     block_log_exception,
                     "block log (ranging from ${block_log_first_num} to ${block_log_last_num}) does not contain the last irreversible block (${fork_db_lib})",
                     ("block_log_first_num", first_block_num)
                     ("block_log_last_num", blog.head()->block_num())
                     ("fork_db_lib", lib_num)
         );
         lib_num = blog.head()->block_num();
      } else {
         if( first_block_num != (lib_num + 1) ) {
            blog.reset( chain_id, lib_num + 1 );
         }
      }

      if( read_mode == db_read_mode::IRREVERSIBLE && fork_db.head()->id != fork_db.root()->id ) {
         fork_db.rollback_head_to_root();
      }
      head = fork_db.head();

      init(check_shutdown, false);
   }


   static auto validate_db_version( const chainbase::database& db ) {
      // check database version
      const auto& header_idx = db.get_index<database_header_multi_index>().indices().get<by_id>();

      EOS_ASSERT(header_idx.begin() != header_idx.end(), bad_database_version_exception,
                 "state database version pre-dates versioning, please restore from a compatible snapshot or replay!");

      auto header_itr = header_idx.begin();
      header_itr->validate();

      return header_itr;
   }

   void init(std::function<bool()> check_shutdown, bool clean_startup) {
      uint32_t lib_num = (blog.head() ? blog.head()->block_num() : fork_db.root()->block_num);

      auto header_itr = validate_db_version( db );

      {
         const auto& state_chain_id = db.get<global_property_object>().chain_id;
         EOS_ASSERT( state_chain_id == chain_id, chain_id_type_exception,
                     "chain ID in state (${state_chain_id}) does not match the chain ID that controller was constructed with (${controller_chain_id})",
                     ("state_chain_id", state_chain_id)("controller_chain_id", chain_id)
         );
      }

      // upgrade to the latest compatible version
      if (header_itr->version != database_header_object::current_version) {
         db.modify(*header_itr, [](auto& header) {
            header.version = database_header_object::current_version;
         });
      }

      kv_db.check_backing_store_setting( clean_startup );

      // At this point head != nullptr && fork_db.head() != nullptr && fork_db.root() != nullptr.
      // Furthermore, fork_db.root()->block_num <= lib_num.
      // Also, even though blog.head() may still be nullptr, blog.first_block_num() is guaranteed to be lib_num + 1.

      EOS_ASSERT( db.revision() >= head->block_num, fork_database_exception,
                  "fork database head (${head}) is inconsistent with state (${db})",
                  ("db",db.revision())("head",head->block_num) );

      if( db.revision() > head->block_num ) {
         wlog( "database revision (${db}) is greater than head block number (${head}), "
               "attempting to undo pending changes",
               ("db",db.revision())("head",head->block_num) );
      }
      while( db.revision() > head->block_num ) {
         kv_db.undo();
      }

      protocol_features.init( db );

      const auto& rbi = reversible_blocks.get_index<reversible_block_index,by_num>();
      auto last_block_num = lib_num;

      if( read_mode == db_read_mode::IRREVERSIBLE ) {
         // ensure there are no reversible blocks
         auto itr = rbi.begin();
         if( itr != rbi.end() ) {
            wlog( "read_mode has changed to irreversible: erasing reversible blocks" );
         }
         for( ; itr != rbi.end(); itr = rbi.begin() )
            reversible_blocks.remove( *itr );
      } else {
         auto itr = rbi.begin();
         for( ; itr != rbi.end() && itr->blocknum <= lib_num; itr = rbi.begin() )
            reversible_blocks.remove( *itr );

         EOS_ASSERT( itr == rbi.end() || itr->blocknum == lib_num + 1, reversible_blocks_exception,
                     "gap exists between last irreversible block (${lib}) and first reversible block (${first_reversible_block_num})",
                     ("lib", lib_num)("first_reversible_block_num", itr->blocknum)
         );

         auto ritr = rbi.rbegin();

         if( ritr != rbi.rend() ) {
            last_block_num = ritr->blocknum;
         }

         EOS_ASSERT( head->block_num <= last_block_num, reversible_blocks_exception,
                     "head block (${head_num}) is greater than the last locally stored block (${last_block_num})",
                     ("head_num", head->block_num)("last_block_num", last_block_num)
         );

         auto pending_head = fork_db.pending_head();

         if( ritr != rbi.rend()
             && lib_num < pending_head->block_num
             && pending_head->block_num <= last_block_num
         ) {
            auto rbitr = rbi.find( pending_head->block_num );
            EOS_ASSERT( rbitr != rbi.end(), reversible_blocks_exception, "pending head block not found in reversible blocks");
            auto rev_id = rbitr->get_block_id();
            EOS_ASSERT( rev_id == pending_head->id,
                        reversible_blocks_exception,
                        "mismatch in block id of pending head block ${num} in reversible blocks database: "
                        "expected: ${expected}, actual: ${actual}",
                        ("num", pending_head->block_num)("expected", pending_head->id)("actual", rev_id)
            );
         } else if( ritr != rbi.rend() && last_block_num < pending_head->block_num ) {
            const auto b = fork_db.search_on_branch( pending_head->id, last_block_num );
            FC_ASSERT( b, "unexpected violation of invariants" );
            auto rev_id = ritr->get_block_id();
            EOS_ASSERT( rev_id == b->id,
                        reversible_blocks_exception,
                        "mismatch in block id of last block (${num}) in reversible blocks database: "
                        "expected: ${expected}, actual: ${actual}",
                        ("num", last_block_num)("expected", b->id)("actual", rev_id)
            );
         }
         // else no checks needed since fork_db will be completely reset on replay anyway
      }

      if (auto dm_logger = get_deep_mind_logger()) {
         // FIXME: We should probably feed that from CMake directly somehow ...
         fc_dlog(*dm_logger, "DEEP_MIND_VERSION 13 0");

         fc_dlog(*dm_logger, "ABIDUMP START ${block_num} ${global_sequence_num}",
            ("block_num", head->block_num)
            ("global_sequence_num", db.get<dynamic_global_property_object>().global_action_sequence)
         );
         const auto& idx = db.get_index<account_index>();
         for (auto& row : idx.indices()) {
            if (row.abi.size() != 0) {
               fc_dlog(*dm_logger, "ABIDUMP ABI ${contract} ${abi}",
                  ("contract", row.name)
                  ("abi", row.abi)
               );
            }
         }
         fc_dlog(*dm_logger, "ABIDUMP END");
      }

      if( last_block_num > head->block_num ) {
         replay( check_shutdown ); // replay any irreversible and reversible blocks ahead of current head
      }

      if( check_shutdown() ) return;

      if( read_mode != db_read_mode::IRREVERSIBLE
          && fork_db.pending_head()->id != fork_db.head()->id
          && fork_db.head()->id == fork_db.root()->id
      ) {
         wlog( "read_mode has changed from irreversible: applying best branch from fork database" );

         for( auto pending_head = fork_db.pending_head();
              pending_head->id != fork_db.head()->id;
              pending_head = fork_db.pending_head()
         ) {
            wlog( "applying branch from fork database ending with block: ${id}", ("id", pending_head->id) );
            maybe_switch_forks( pending_head, controller::block_status::complete, forked_branch_callback{}, trx_meta_cache_lookup{} );
         }
      }
   }

   ~controller_impl() {
      thread_pool.stop();
      pending.reset();
   }

   void add_indices() {
      reversible_blocks.add_index<reversible_block_index>();

      controller_index_set::add_indices(db);
      db.add_index<kv_index>();
      contract_database_index_set::add_indices(db);

      authorization.add_indices();
      resource_limits.add_indices();
   }

   sha256 calculate_integrity_hash() const {
      sha256::encoder enc;
      auto hash_writer = std::make_shared<integrity_hash_snapshot_writer>(enc);
      kv_db.add_to_snapshot(hash_writer, *fork_db.head(), authorization, resource_limits);
      hash_writer->finalize();

      return enc.result();
   }

   void create_native_account( const fc::time_point& initial_timestamp, account_name name, const authority& owner, const authority& active, bool is_privileged = false ) {
      db.create<account_object>([&](auto& a) {
         a.name = name;
         a.creation_date = initial_timestamp;

         if( name == config::system_account_name ) {
            // The initial eosio ABI value affects consensus; see  https://github.com/EOSIO/eos/issues/7794
            // TODO: This doesn't charge RAM; a fix requires a consensus upgrade.
            a.abi.assign(eosio_abi_bin, sizeof(eosio_abi_bin));
         }
      });
      db.create<account_metadata_object>([&](auto & a) {
         a.name = name;
         a.set_privileged( is_privileged );
      });

      const auto& owner_permission  = authorization.create_permission(name, config::owner_name, 0,
                                                                      owner, 0, initial_timestamp );
      const auto& active_permission = authorization.create_permission(name, config::active_name, owner_permission.id,
                                                                      active, 0, initial_timestamp );

      resource_limits.initialize_account(name);

      int64_t ram_delta = config::overhead_per_account_ram_bytes;
      ram_delta += 2*config::billable_size_v<permission_object>;
      ram_delta += owner_permission.auth.get_billable_size();
      ram_delta += active_permission.auth.get_billable_size();

      std::string event_id;
      if (get_deep_mind_logger() != nullptr) {
         event_id = STORAGE_EVENT_ID("${name}", ("name", name));
      }

      resource_limits.add_pending_ram_usage(name, ram_delta, storage_usage_trace(0, std::move(event_id), "account", "add", "newaccount"));
      resource_limits.verify_account_ram_usage(name);
   }

   void initialize_database(const genesis_state& genesis) {
      // create the database header sigil
      db.create<database_header_object>([&]( auto& header ){
         // nothing to do for now
      });

      // Initialize block summary index
      for (int i = 0; i < 0x10000; i++)
         db.create<block_summary_object>([&](block_summary_object&) {});

      const auto& tapos_block_summary = db.get<block_summary_object>(1);
      db.modify( tapos_block_summary, [&]( auto& bs ) {
         bs.block_id = head->id;
      });

      genesis.initial_configuration.validate();
      db.create<global_property_object>([&genesis,&chain_id=this->chain_id](auto& gpo ){
         gpo.configuration = genesis.initial_configuration;
         gpo.kv_configuration = kv_database_config{};
         // TODO: Update this when genesis protocol features are enabled.
         gpo.wasm_configuration = genesis_state::default_initial_wasm_configuration;
         gpo.chain_id = chain_id;
      });

      db.create<protocol_state_object>([&](auto& pso ){
         pso.num_supported_key_types = config::genesis_num_supported_key_types;
         for( const auto& i : genesis_intrinsics ) {
            add_intrinsic_to_whitelist( pso.whitelisted_intrinsics, i );
         }
      });

      db.create<dynamic_global_property_object>([](auto&){});
      db.create<kv_db_config_object>([](auto&){});

      authorization.initialize_database();
      resource_limits.initialize_database();

      authority system_auth(genesis.initial_key);
      create_native_account( genesis.initial_timestamp, config::system_account_name, system_auth, system_auth, true );

      auto empty_authority = authority(1, {}, {});
      auto active_producers_authority = authority(1, {}, {});
      active_producers_authority.accounts.push_back({{config::system_account_name, config::active_name}, 1});

      create_native_account( genesis.initial_timestamp, config::null_account_name, empty_authority, empty_authority );
      create_native_account( genesis.initial_timestamp, config::producers_account_name, empty_authority, active_producers_authority );
      const auto& active_permission       = authorization.get_permission({config::producers_account_name, config::active_name});
      const auto& majority_permission     = authorization.create_permission( config::producers_account_name,
                                                                             config::majority_producers_permission_name,
                                                                             active_permission.id,
                                                                             active_producers_authority,
                                                                             0,
                                                                             genesis.initial_timestamp );
      const auto& minority_permission     = authorization.create_permission( config::producers_account_name,
                                                                             config::minority_producers_permission_name,
                                                                             majority_permission.id,
                                                                             active_producers_authority,
                                                                             0,
                                                                             genesis.initial_timestamp );
   }

   // The returned scoped_exit should not exceed the lifetime of the pending which existed when make_block_restore_point was called.
   fc::scoped_exit<std::function<void()>> make_block_restore_point() {
      auto& bb = std::get<building_block>(pending->_block_stage);
      auto orig_trx_receipts_size           = bb._pending_trx_receipts.size();
      auto orig_trx_metas_size              = bb._pending_trx_metas.size();
      auto orig_trx_receipt_digests_size    = std::holds_alternative<digests_t>(bb._trx_mroot_or_receipt_digests) ?
            std::get<digests_t>(bb._trx_mroot_or_receipt_digests).size() : 0;
      auto orig_action_receipt_digests_size = bb._action_receipt_digests.size();

      std::function<void()> callback = [this,
            orig_trx_receipts_size,
            orig_trx_metas_size,
            orig_trx_receipt_digests_size,
            orig_action_receipt_digests_size]()
      {
        auto& bb = std::get<building_block>(pending->_block_stage);
         bb._pending_trx_receipts.resize(orig_trx_receipts_size);
         bb._pending_trx_metas.resize(orig_trx_metas_size);
        if( std::holds_alternative<digests_t>(bb._trx_mroot_or_receipt_digests) )
            std::get<digests_t>(bb._trx_mroot_or_receipt_digests).resize(orig_trx_receipt_digests_size);
         bb._action_receipt_digests.resize(orig_action_receipt_digests_size);
      };

      return fc::make_scoped_exit( std::move(callback) );
   }

   transaction_trace_ptr apply_onerror( const generated_transaction& gtrx,
                                        fc::time_point deadline,
                                        fc::time_point start,
                                        uint32_t& cpu_time_to_bill_us, // only set on failure
                                        uint32_t billed_cpu_time_us,
                                        bool explicit_billed_cpu_time = false,
                                        bool enforce_whiteblacklist = true
                                      )
   {
      signed_transaction etrx;
      // Deliver onerror action containing the failed deferred transaction directly back to the sender.
      etrx.actions.emplace_back( vector<permission_level>{{gtrx.sender, config::active_name}},
                                 onerror( gtrx.sender_id, gtrx.packed_trx.data(), gtrx.packed_trx.size() ) );
      if( self.is_builtin_activated( builtin_protocol_feature_t::no_duplicate_deferred_id ) ) {
         etrx.expiration = time_point_sec();
         etrx.ref_block_num = 0;
         etrx.ref_block_prefix = 0;
      } else {
         etrx.expiration = self.pending_block_time() + fc::microseconds(999'999); // Round up to nearest second to avoid appearing expired
         etrx.set_reference_block( self.head_block_id() );
      }

      if (auto dm_logger = get_deep_mind_logger()) {
         auto packed_trx = fc::raw::pack(etrx);

         fc_dlog(*dm_logger, "TRX_OP CREATE onerror ${id} ${trx}",
            ("id", etrx.id())
            ("trx", fc::to_hex(packed_trx))
         );
      }

      transaction_checktime_timer trx_timer(timer);
      const packed_transaction trx( std::move( etrx ), true );
      transaction_context trx_context( self, trx, std::move(trx_timer), start );
      trx_context.deadline = deadline;
      trx_context.explicit_billed_cpu_time = explicit_billed_cpu_time;
      trx_context.billed_cpu_time_us = billed_cpu_time_us;
      trx_context.enforce_whiteblacklist = enforce_whiteblacklist;
      transaction_trace_ptr trace = trx_context.trace;

      auto handle_exception = [&](const auto& e)
      {
         cpu_time_to_bill_us = trx_context.update_billed_cpu_time( fc::time_point::now() );
         trace->error_code = controller::convert_exception_to_error_code( e );
         trace->except = e;
         trace->except_ptr = std::current_exception();
      };

      try {
         trx_context.init_for_implicit_trx();
         trx_context.published = gtrx.published;
         trx_context.execute_action( trx_context.schedule_action( trx.get_transaction().actions.back(), gtrx.sender, false, 0, 0 ), 0 );
         trx_context.finalize(); // Automatically rounds up network and CPU usage in trace and bills payers if successful

         auto restore = make_block_restore_point();
         trace->receipt = push_receipt( gtrx.trx_id, transaction_receipt::soft_fail,
                                        trx_context.billed_cpu_time_us, trace->net_usage );
         fc::move_append( std::get<building_block>(pending->_block_stage)._action_receipt_digests,
                          std::move(trx_context.executed_action_receipt_digests) );

         trx_context.squash();
         restore.cancel();
         return trace;
      } catch( const objective_block_validation_exception& ) {
         throw;
      } catch ( const std::bad_alloc& ) {
         throw;
      } catch ( const boost::interprocess::bad_alloc& ) {
         throw;
      } catch( const fc::exception& e ) {
         handle_exception(e);
      } catch ( const std::exception& e ) {
         auto wrapper = fc::std_exception_wrapper::from_current_exception(e);
         handle_exception(wrapper);
      }
      return trace;
   }

   int64_t remove_scheduled_transaction( const generated_transaction_object& gto ) {
      std::string event_id;
      if (get_deep_mind_logger() != nullptr) {
         event_id = STORAGE_EVENT_ID("${id}", ("id", gto.id));
      }

      int64_t ram_delta = -(config::billable_size_v<generated_transaction_object> + gto.packed_trx.size());
      resource_limits.add_pending_ram_usage( gto.payer, ram_delta, storage_usage_trace(0, std::move(event_id), "deferred_trx", "remove", "deferred_trx_removed") );
      // No need to verify_account_ram_usage since we are only reducing memory

      db.remove( gto );
      return ram_delta;
   }

   bool failure_is_subjective( const fc::exception& e ) const {
      auto code = e.code();
      return    (code == subjective_block_production_exception::code_value)
             || (code == block_net_usage_exceeded::code_value)
             || (code == greylist_net_usage_exceeded::code_value)
             || (code == block_cpu_usage_exceeded::code_value)
             || (code == greylist_cpu_usage_exceeded::code_value)
             || (code == deadline_exception::code_value)
             || (code == leeway_deadline_exception::code_value)
             || (code == actor_whitelist_exception::code_value)
             || (code == actor_blacklist_exception::code_value)
             || (code == contract_whitelist_exception::code_value)
             || (code == contract_blacklist_exception::code_value)
             || (code == action_blacklist_exception::code_value)
             || (code == key_blacklist_exception::code_value)
             || (code == sig_variable_size_limit_exception::code_value)
             || (code == inline_action_too_big_nonprivileged::code_value);
   }

   bool scheduled_failure_is_subjective( const fc::exception& e ) const {
      auto code = e.code();
      return    (code == tx_cpu_usage_exceeded::code_value)
             || failure_is_subjective(e);
   }

   transaction_trace_ptr push_scheduled_transaction( const transaction_id_type& trxid, fc::time_point deadline, uint32_t billed_cpu_time_us, bool explicit_billed_cpu_time = false ) {
      const auto& idx = db.get_index<generated_transaction_multi_index,by_trx_id>();
      auto itr = idx.find( trxid );
      EOS_ASSERT( itr != idx.end(), unknown_transaction_exception, "unknown transaction" );
      return push_scheduled_transaction( *itr, deadline, billed_cpu_time_us, explicit_billed_cpu_time );
   }

   transaction_trace_ptr push_scheduled_transaction( const generated_transaction_object& gto, fc::time_point deadline, uint32_t billed_cpu_time_us, bool explicit_billed_cpu_time = false )
   { try {

      const bool validating = !self.is_producing_block();
      EOS_ASSERT( !validating || explicit_billed_cpu_time, transaction_exception, "validating requires explicit billing" );

      combined_session undo_session = !self.skip_db_sessions() ? kv_db.make_session() : kv_db.make_no_op_session();

      auto gtrx = generated_transaction(gto);

      // remove the generated transaction object after making a copy
      // this will ensure that anything which affects the GTO multi-index-container will not invalidate
      // data we need to successfully retire this transaction.
      //
      // IF the transaction FAILs in a subjective way, `undo_session` should expire without being squashed
      // resulting in the GTO being restored and available for a future block to retire.
      int64_t trx_removal_ram_delta = remove_scheduled_transaction(gto);

      fc::datastream<const char*> ds( gtrx.packed_trx.data(), gtrx.packed_trx.size() );

      EOS_ASSERT( gtrx.delay_until <= self.pending_block_time(), transaction_exception, "this transaction isn't ready",
                 ("gtrx.delay_until",gtrx.delay_until)("pbt",self.pending_block_time())          );

      signed_transaction dtrx;
      fc::raw::unpack(ds,static_cast<transaction&>(dtrx) );
      transaction_metadata_ptr trx =
            transaction_metadata::create_no_recover_keys( std::make_shared<packed_transaction>( std::move(dtrx), true ),
                                                          transaction_metadata::trx_type::scheduled );
      trx->accepted = true;

      transaction_trace_ptr trace;
      if( gtrx.expiration < self.pending_block_time() ) {
         trace = std::make_shared<transaction_trace>();
         trace->id = gtrx.trx_id;
         trace->block_num = self.head_block_num() + 1;
         trace->block_time = self.pending_block_time();
         trace->producer_block_id = self.pending_producer_block_id();
         trace->scheduled = true;
         trace->receipt = push_receipt( gtrx.trx_id, transaction_receipt::expired, billed_cpu_time_us, 0 ); // expire the transaction
         trace->account_ram_delta = account_delta( gtrx.payer, trx_removal_ram_delta );
         emit( self.accepted_transaction, trx );
         emit( self.applied_transaction, std::tie(trace, trx->packed_trx()) );
         undo_session.squash();
         return trace;
      }

      auto reset_in_trx_requiring_checks = fc::make_scoped_exit([old_value=in_trx_requiring_checks,this](){
         in_trx_requiring_checks = old_value;
      });
      in_trx_requiring_checks = true;

      uint32_t cpu_time_to_bill_us = billed_cpu_time_us;

      transaction_checktime_timer trx_timer(timer);
      transaction_context trx_context( self, *trx->packed_trx(), std::move(trx_timer) );
      trx_context.leeway =  fc::microseconds(0); // avoid stealing cpu resource
      trx_context.deadline = deadline;
      trx_context.explicit_billed_cpu_time = explicit_billed_cpu_time;
      trx_context.billed_cpu_time_us = billed_cpu_time_us;
      trx_context.enforce_whiteblacklist = gtrx.sender.empty() ? true : !sender_avoids_whitelist_blacklist_enforcement( gtrx.sender );
      trace = trx_context.trace;

      auto handle_exception = [&](const auto& e)
      {
         cpu_time_to_bill_us = trx_context.update_billed_cpu_time( fc::time_point::now() );
         trace->error_code = controller::convert_exception_to_error_code( e );
         trace->except = e;
         trace->except_ptr = std::current_exception();
         trace->elapsed = fc::time_point::now() - trx_context.start;

         if (auto dm_logger = get_deep_mind_logger()) {
            fc_dlog(*dm_logger, "DTRX_OP FAILED ${action_id}",
               ("action_id", trx_context.get_action_id())
            );
         }
      };

      try {
         trx_context.init_for_deferred_trx( gtrx.published );

         if( trx_context.enforce_whiteblacklist && pending->_block_status == controller::block_status::incomplete ) {
            flat_set<account_name> actors;
            for( const auto& act : trx->packed_trx()->get_transaction().actions ) {
               for( const auto& auth : act.authorization ) {
                  actors.insert( auth.actor );
               }
            }
            check_actor_list( actors );
         }

         trx_context.exec();
         trx_context.finalize(); // Automatically rounds up network and CPU usage in trace and bills payers if successful

         auto restore = make_block_restore_point();

         trace->receipt = push_receipt( gtrx.trx_id,
                                        transaction_receipt::executed,
                                        trx_context.billed_cpu_time_us,
                                        trace->net_usage );

         fc::move_append( std::get<building_block>(pending->_block_stage)._action_receipt_digests,
                          std::move(trx_context.executed_action_receipt_digests) );

         trace->account_ram_delta = account_delta( gtrx.payer, trx_removal_ram_delta );

         emit( self.accepted_transaction, trx );
         emit( self.applied_transaction, std::tie(trace, trx->packed_trx()) );

         trx_context.squash();
         undo_session.squash();

         restore.cancel();

         return trace;
      } catch( const objective_block_validation_exception& ) {
         throw;
      } catch ( const std::bad_alloc& ) {
         throw;
      } catch ( const boost::interprocess::bad_alloc& ) {
         throw;
      } catch( const fc::exception& e ) {
        handle_exception(e);
      } catch ( const std::exception& e) {
        auto wrapper = fc::std_exception_wrapper::from_current_exception(e);
        handle_exception(wrapper);
      }

      trx_context.undo();

      // Only subjective OR soft OR hard failure logic below:

      if( gtrx.sender != account_name() && !(validating ? failure_is_subjective(*trace->except) : scheduled_failure_is_subjective(*trace->except))) {
         // Attempt error handling for the generated transaction.

         auto error_trace = apply_onerror( gtrx, deadline, trx_context.pseudo_start,
                                           cpu_time_to_bill_us, billed_cpu_time_us, explicit_billed_cpu_time,
                                           trx_context.enforce_whiteblacklist );
         error_trace->failed_dtrx_trace = trace;
         trace = error_trace;
         if( !trace->except_ptr ) {
            trace->account_ram_delta = account_delta( gtrx.payer, trx_removal_ram_delta );
            emit( self.accepted_transaction, trx );
            emit( self.applied_transaction, std::tie(trace, trx->packed_trx()) );
            undo_session.squash();
            return trace;
         }
         trace->elapsed = fc::time_point::now() - trx_context.start;
      }

      // Only subjective OR hard failure logic below:

      // subjectivity changes based on producing vs validating
      bool subjective  = false;
      if (validating) {
         subjective = failure_is_subjective(*trace->except);
      } else {
         subjective = scheduled_failure_is_subjective(*trace->except);
      }

      if ( !subjective ) {
         // hard failure logic

         if( !validating ) {
            auto& rl = self.get_mutable_resource_limits_manager();
            rl.update_account_usage( trx_context.bill_to_accounts, block_timestamp_type(self.pending_block_time()).slot );
            int64_t account_cpu_limit = 0;
            std::tie( std::ignore, account_cpu_limit, std::ignore, std::ignore ) = trx_context.max_bandwidth_billed_accounts_can_pay( true );

            uint32_t limited_cpu_time_to_bill_us = static_cast<uint32_t>( std::min(
                  std::min( static_cast<int64_t>(cpu_time_to_bill_us), account_cpu_limit ),
                  trx_context.initial_objective_duration_limit.count() ) );
            EOS_ASSERT( !explicit_billed_cpu_time || (cpu_time_to_bill_us == limited_cpu_time_to_bill_us),
                        transaction_exception, "cpu to bill ${cpu} != limited ${limit}", ("cpu", cpu_time_to_bill_us)("limit", limited_cpu_time_to_bill_us) );
            cpu_time_to_bill_us = limited_cpu_time_to_bill_us;
         }

         resource_limits.add_transaction_usage( trx_context.bill_to_accounts, cpu_time_to_bill_us, 0,
                                                block_timestamp_type(self.pending_block_time()).slot ); // Should never fail

         trace->receipt = push_receipt(gtrx.trx_id, transaction_receipt::hard_fail, cpu_time_to_bill_us, 0);
         trace->account_ram_delta = account_delta( gtrx.payer, trx_removal_ram_delta );

         emit( self.accepted_transaction, trx );
         emit( self.applied_transaction, std::tie(trace, trx->packed_trx()) );

         undo_session.squash();
      } else {
         emit( self.accepted_transaction, trx );
         emit( self.applied_transaction, std::tie(trace, trx->packed_trx()) );
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
      EOS_ASSERT( net_usage_words*8 == net_usage, transaction_exception, "net_usage is not divisible by 8" );
      auto& receipts = std::get<building_block>(pending->_block_stage)._pending_trx_receipts;
      receipts.emplace_back( trx );
      transaction_receipt& r = receipts.back();
      r.cpu_usage_us         = cpu_usage_us;
      r.net_usage_words      = net_usage_words;
      r.status               = status;
      auto& bb = std::get<building_block>(pending->_block_stage);
      if( std::holds_alternative<digests_t>(bb._trx_mroot_or_receipt_digests) )
         std::get<digests_t>(bb._trx_mroot_or_receipt_digests).emplace_back( r.digest() );
      return r;
   }

   /**
    *  This is the entry point for new transactions to the block state. It will check authorization and
    *  determine whether to execute it now or to delay it. Lastly it inserts a transaction receipt into
    *  the pending block.
    */
   transaction_trace_ptr push_transaction( const transaction_metadata_ptr& trx,
                                           fc::time_point deadline,
                                           uint32_t billed_cpu_time_us,
                                           bool explicit_billed_cpu_time,
                                           std::optional<uint32_t> explicit_net_usage_words,
                                           uint32_t subjective_cpu_bill_us )
   {
      EOS_ASSERT(deadline != fc::time_point(), transaction_exception, "deadline cannot be uninitialized");

      transaction_trace_ptr trace;
      try {
         auto start = fc::time_point::now();
         const bool check_auth = !self.skip_auth_check() && !trx->implicit;
         const fc::microseconds sig_cpu_usage = trx->signature_cpu_usage();

         if( !explicit_billed_cpu_time ) {
            fc::microseconds already_consumed_time( EOS_PERCENT(sig_cpu_usage.count(), conf.sig_cpu_bill_pct) );

            if( start.time_since_epoch() <  already_consumed_time ) {
               start = fc::time_point();
            } else {
               start -= already_consumed_time;
            }
         }

         transaction_checktime_timer trx_timer(timer);
         transaction_context trx_context(self, *trx->packed_trx(), std::move(trx_timer), start);
         if ((bool)subjective_cpu_leeway && pending->_block_status == controller::block_status::incomplete) {
            trx_context.leeway = *subjective_cpu_leeway;
         }
         trx_context.deadline = deadline;
         trx_context.explicit_billed_cpu_time = explicit_billed_cpu_time;
         trx_context.billed_cpu_time_us = billed_cpu_time_us;
         trx_context.subjective_cpu_bill_us = subjective_cpu_bill_us;
         trace = trx_context.trace;

         auto handle_exception =[&](const auto& e)
         {
            trace->error_code = controller::convert_exception_to_error_code( e );
            trace->except = e;
            trace->except_ptr = std::current_exception();
            trace->elapsed = fc::time_point::now() - trx_context.start;
         };

         try {
            const transaction& trn = trx->packed_trx()->get_transaction();
            if( trx->implicit ) {
               EOS_ASSERT( !explicit_net_usage_words, transaction_exception, "NET usage cannot be explicitly set for implicit transactions" );
               trx_context.init_for_implicit_trx();
               trx_context.enforce_whiteblacklist = false;
            } else {
               bool skip_recording = replay_head_time && (time_point(trn.expiration) <= *replay_head_time);
               if( explicit_net_usage_words ) {
                  trx_context.init_for_input_trx_with_explicit_net( *explicit_net_usage_words, skip_recording );
               } else {
                  trx_context.init_for_input_trx( trx->packed_trx()->get_unprunable_size(),
                                                  trx->packed_trx()->get_prunable_size(),
                                                  skip_recording );
               }
            }

            trx_context.delay = fc::seconds(trn.delay_sec);

            if( check_auth ) {
               authorization.check_authorization(
                       trn.actions,
                       trx->recovered_keys(),
                       {},
                       trx_context.delay,
                       [&trx_context](){ trx_context.checktime(); },
                       false
               );
            }
            trx_context.exec();
            trx_context.finalize(); // Automatically rounds up network and CPU usage in trace and bills payers if successful

            auto restore = make_block_restore_point();

            if (!trx->implicit) {
               transaction_receipt::status_enum s = (trx_context.delay == fc::seconds(0))
                                                    ? transaction_receipt::executed
                                                    : transaction_receipt::delayed;
               trace->receipt = push_receipt(*trx->packed_trx(), s, trx_context.billed_cpu_time_us, trace->net_usage);
               trx->billed_cpu_time_us = trx_context.billed_cpu_time_us;
               std::get<building_block>(pending->_block_stage)._pending_trx_metas.emplace_back(trx);
            } else {
               transaction_receipt_header r;
               r.status = transaction_receipt::executed;
               r.cpu_usage_us = trx_context.billed_cpu_time_us;
               r.net_usage_words = trace->net_usage / 8;
               trace->receipt = r;
            }

            fc::move_append( std::get<building_block>(pending->_block_stage)._action_receipt_digests,
                             std::move(trx_context.executed_action_receipt_digests) );

            // call the accept signal but only once for this transaction
            if (!trx->accepted) {
               trx->accepted = true;
               emit( self.accepted_transaction, trx);
            }

            emit(self.applied_transaction, std::tie(trace, trx->packed_trx()));


            if ( read_mode != db_read_mode::SPECULATIVE && pending->_block_status == controller::block_status::incomplete ) {
               //this may happen automatically in destructor, but I prefere make it more explicit
               trx_context.undo();
            } else {
               restore.cancel();
               trx_context.squash();
            }

            return trace;
         } catch( const objective_block_validation_exception& ) {
            throw;
         } catch ( const std::bad_alloc& ) {
           throw;
         } catch ( const boost::interprocess::bad_alloc& ) {
           throw;
         } catch (const fc::exception& e) {
           handle_exception(e);
         } catch (const std::exception& e) {
           auto wrapper = fc::std_exception_wrapper::from_current_exception(e);
           handle_exception(wrapper);
         }

         emit( self.accepted_transaction, trx );
         emit( self.applied_transaction, std::tie(trace, trx->packed_trx()) );

         return trace;
      } FC_CAPTURE_AND_RETHROW((trace))
   } /// push_transaction

   void start_block( block_timestamp_type when,
                     uint16_t confirm_block_count,
                     const vector<digest_type>& new_protocol_feature_activations,
                     controller::block_status s,
                     const std::optional<block_id_type>& producer_block_id )
   {
      EOS_ASSERT( !pending, block_validate_exception, "pending block already exists" );

      if (auto dm_logger = get_deep_mind_logger()) {
         // The head block represents the block just before this one that is about to start, so add 1 to get this block num
         fc_dlog(*dm_logger, "START_BLOCK ${block_num}", ("block_num", head->block_num + 1));
      }

      emit( self.block_start, head->block_num + 1 );

      auto guard_pending = fc::make_scoped_exit([this, head_block_num=head->block_num](){
         protocol_features.popped_blocks_to( head_block_num );
         pending.reset();
      });

      if (!self.skip_db_sessions(s)) {
         EOS_ASSERT( db.revision() == head->block_num, database_exception, "db revision is not on par with head block",
                     ("db.revision()", db.revision())("controller_head_block", head->block_num)("fork_db_head_block", fork_db.head()->block_num) );

         pending.emplace( kv_db.make_session(), *head, when, confirm_block_count, new_protocol_feature_activations );
      } else {
         pending.emplace( kv_db.make_no_op_session(), *head, when, confirm_block_count, new_protocol_feature_activations );
      }

      pending->_block_status = s;
      pending->_producer_block_id = producer_block_id;

      auto& bb = std::get<building_block>(pending->_block_stage);
      const auto& pbhs = bb._pending_block_header_state;

      if ( read_mode == db_read_mode::SPECULATIVE || pending->_block_status != controller::block_status::incomplete )
      {
         const auto& pso = db.get<protocol_state_object>();

         auto num_preactivated_protocol_features = pso.preactivated_protocol_features.size();
         bool handled_all_preactivated_features = (num_preactivated_protocol_features == 0);

         if( new_protocol_feature_activations.size() > 0 ) {
            flat_map<digest_type, bool> activated_protocol_features;
            activated_protocol_features.reserve( std::max( num_preactivated_protocol_features,
                                                           new_protocol_feature_activations.size() ) );
            for( const auto& feature_digest : pso.preactivated_protocol_features ) {
               activated_protocol_features.emplace( feature_digest, false );
            }

            size_t num_preactivated_features_that_have_activated = 0;

            const auto& pfs = protocol_features.get_protocol_feature_set();
            for( const auto& feature_digest : new_protocol_feature_activations ) {
               const auto& f = pfs.get_protocol_feature( feature_digest );

               auto res = activated_protocol_features.emplace( feature_digest, true );
               if( res.second ) {
                  // feature_digest was not preactivated
                  EOS_ASSERT( !f.preactivation_required, protocol_feature_exception,
                              "attempted to activate protocol feature without prior required preactivation: ${digest}",
                              ("digest", feature_digest)
                  );
               } else {
                  EOS_ASSERT( !res.first->second, block_validate_exception,
                              "attempted duplicate activation within a single block: ${digest}",
                              ("digest", feature_digest)
                  );
                  // feature_digest was preactivated
                  res.first->second = true;
                  ++num_preactivated_features_that_have_activated;
               }

               if( f.builtin_feature ) {
                  trigger_activation_handler( *f.builtin_feature );
               }

               protocol_features.activate_feature( feature_digest, pbhs.block_num );

               ++bb._num_new_protocol_features_that_have_activated;
            }

            if( num_preactivated_features_that_have_activated == num_preactivated_protocol_features ) {
               handled_all_preactivated_features = true;
            }
         }

         EOS_ASSERT( handled_all_preactivated_features, block_validate_exception,
                     "There are pre-activated protocol features that were not activated at the start of this block"
         );

         if( new_protocol_feature_activations.size() > 0 ) {
            db.modify( pso, [&]( auto& ps ) {
               ps.preactivated_protocol_features.clear();

               ps.activated_protocol_features.reserve( ps.activated_protocol_features.size()
                                                         + new_protocol_feature_activations.size() );
               for( const auto& feature_digest : new_protocol_feature_activations ) {
                  ps.activated_protocol_features.emplace_back( feature_digest, pbhs.block_num );
               }
            });
         }

         const auto& gpo = self.get_global_properties();

         if( gpo.proposed_schedule_block_num && // if there is a proposed schedule that was proposed in a block ...
             ( *gpo.proposed_schedule_block_num <= pbhs.dpos_irreversible_blocknum ) && // ... that has now become irreversible ...
             pbhs.prev_pending_schedule.schedule.producers.size() == 0 // ... and there was room for a new pending schedule prior to any possible promotion
         )
         {
            // Promote proposed schedule to pending schedule.
            if( !replay_head_time ) {
               ilog( "promoting proposed schedule (set in block ${proposed_num}) to pending; current block: ${n} lib: ${lib} schedule: ${schedule} ",
                     ("proposed_num", *gpo.proposed_schedule_block_num)("n", pbhs.block_num)
                     ("lib", pbhs.dpos_irreversible_blocknum)
                     ("schedule", producer_authority_schedule::from_shared(gpo.proposed_schedule) ) );
            }

            EOS_ASSERT( gpo.proposed_schedule.version == pbhs.active_schedule_version + 1,
                        producer_schedule_exception, "wrong producer schedule version specified" );

            std::get<building_block>(pending->_block_stage)._new_pending_producer_schedule = producer_authority_schedule::from_shared(gpo.proposed_schedule);
            db.modify( gpo, [&]( auto& gp ) {
               gp.proposed_schedule_block_num = std::optional<block_num_type>();
               gp.proposed_schedule.version=0;
               gp.proposed_schedule.producers.clear();
            });
         }

         try {
            transaction_metadata_ptr onbtrx =
                  transaction_metadata::create_no_recover_keys( std::make_shared<packed_transaction>( get_on_block_transaction(), true ),
                                                                transaction_metadata::trx_type::implicit );
            auto reset_in_trx_requiring_checks = fc::make_scoped_exit([old_value=in_trx_requiring_checks,this](){
                  in_trx_requiring_checks = old_value;
               });
            in_trx_requiring_checks = true;
            push_transaction( onbtrx, fc::time_point::maximum(), gpo.configuration.min_transaction_cpu_usage, true, {}, 0 );
         } catch( const std::bad_alloc& e ) {
            elog( "on block transaction failed due to a std::bad_alloc" );
            throw;
         } catch( const boost::interprocess::bad_alloc& e ) {
            elog( "on block transaction failed due to a bad allocation" );
            throw;
         } catch( const fc::exception& e ) {
            wlog( "on block transaction failed, but shouldn't impact block generation, system contract needs update" );
            edump((e.to_detail_string()));
         } catch( const std::exception& e ) {
            wlog( "on block transaction failed due to unexpected exception" );
            edump((e.what()));
         } catch( ... ) {
            elog( "on block transaction failed due to unknown exception" );
         }

         clear_expired_input_transactions();
         update_producers_authority();
      }

      guard_pending.cancel();
   } /// start_block

   void finalize_block()
   {
      EOS_ASSERT( pending, block_validate_exception, "it is not valid to finalize when there is no pending block");
      EOS_ASSERT( std::holds_alternative<building_block>(pending->_block_stage), block_validate_exception, "already called finalize_block");

      try {

      auto& pbhs = pending->get_pending_block_header_state();

      auto& bb = std::get<building_block>(pending->_block_stage);

      auto action_merkle_fut = async_thread_pool( thread_pool.get_executor(),
                                                  [ids{std::move( bb._action_receipt_digests )}]() mutable {
                                                     return merkle( std::move( ids ) );
                                                  } );
      const bool calc_trx_merkle = !std::holds_alternative<checksum256_type>(bb._trx_mroot_or_receipt_digests);
      std::future<checksum256_type> trx_merkle_fut;
      if( calc_trx_merkle ) {
         trx_merkle_fut = async_thread_pool( thread_pool.get_executor(),
                                             [ids{std::move( std::get<digests_t>(bb._trx_mroot_or_receipt_digests) )}]() mutable {
                                                return merkle( std::move( ids ) );
                                             } );
      }

      // Update resource limits:
      resource_limits.process_account_limit_updates();
      const auto& chain_config = self.get_global_properties().configuration;
      uint64_t CPU_TARGET = EOS_PERCENT(chain_config.max_block_cpu_usage, chain_config.target_block_cpu_usage_pct);
      resource_limits.set_block_parameters(
         { CPU_TARGET, chain_config.max_block_cpu_usage, config::block_cpu_usage_average_window_ms / config::block_interval_ms, config::maximum_elastic_resource_multiplier, {99, 100}, {1000, 999}},
         {EOS_PERCENT(chain_config.max_block_net_usage, chain_config.target_block_net_usage_pct), chain_config.max_block_net_usage, config::block_size_average_window_ms / config::block_interval_ms, config::maximum_elastic_resource_multiplier, {99, 100}, {1000, 999}}
      );
      resource_limits.process_block_usage(pbhs.block_num);

      // Create (unsigned) block:
      auto block_ptr = std::make_shared<signed_block>( pbhs.make_block_header(
         calc_trx_merkle ? trx_merkle_fut.get() : std::get<checksum256_type>(bb._trx_mroot_or_receipt_digests),
         action_merkle_fut.get(),
         bb._new_pending_producer_schedule,
         std::move( bb._new_protocol_feature_activations ),
         protocol_features.get_protocol_feature_set()
      ) );

      block_ptr->transactions = std::move( bb._pending_trx_receipts );

      auto id = block_ptr->calculate_id();

      // Update TaPoS table:
      create_block_summary( id );

      /*
      ilog( "finalized block ${n} (${id}) at ${t} by ${p} (${signing_key}); schedule_version: ${v} lib: ${lib} #dtrxs: ${ndtrxs} ${np}",
            ("n",pbhs.block_num)
            ("id",id)
            ("t",pbhs.timestamp)
            ("p",pbhs.producer)
            ("signing_key", pbhs.block_signing_key)
            ("v",pbhs.active_schedule_version)
            ("lib",pbhs.dpos_irreversible_blocknum)
            ("ndtrxs",db.get_index<generated_transaction_multi_index,by_trx_id>().size())
            ("np",block_ptr->new_producers)
      );
      */

      pending->_block_stage = assembled_block{
                                 id,
                                 std::move( bb._pending_block_header_state ),
                                 std::move( bb._pending_trx_metas ),
                                 std::move( block_ptr ),
                                 std::move( bb._new_pending_producer_schedule )
                              };
   } FC_CAPTURE_AND_RETHROW() } /// finalize_block

   /**
    * @post regardless of the success of commit block there is no active pending block
    */
   void commit_block( bool add_to_fork_db ) {
      auto reset_pending_on_exit = fc::make_scoped_exit([this]{
         pending.reset();
      });

      try {
         EOS_ASSERT( std::holds_alternative<completed_block>(pending->_block_stage), block_validate_exception,
                     "cannot call commit_block until pending block is completed" );

         auto bsp = std::get<completed_block>(pending->_block_stage)._block_state;

         if( add_to_fork_db ) {
            fork_db.add( bsp );
            fork_db.mark_valid( bsp );
            emit( self.accepted_block_header, bsp );
            head = fork_db.head();
            EOS_ASSERT( bsp == head, fork_database_exception, "committed block did not become the new head in fork database");
         }

         if( !replay_head_time && read_mode != db_read_mode::IRREVERSIBLE ) {
            reversible_blocks.create<reversible_block_object>( [&]( auto& ubo ) {
               ubo.blocknum = bsp->block_num;
               ubo.set_block( bsp->block );
            });
         }

         emit( self.accepted_block, bsp );

         if( add_to_fork_db ) {
            log_irreversible();
         }
      } catch (...) {
         // dont bother resetting pending, instead abort the block
         reset_pending_on_exit.cancel();
         abort_block();
         throw;
      }

      // push the state for pending.
      pending->push();
   }

   /**
    *  This method is called from other threads. The controller_impl should outlive those threads.
    *  However, to avoid race conditions, it means that the behavior of this function should not change
    *  after controller_impl construction.

    *  This should not be an issue since the purpose of this function is to ensure all of the protocol features
    *  in the supplied vector are recognized by the software, and the set of recognized protocol features is
    *  determined at startup and cannot be changed without a restart.
    */
   void check_protocol_features( block_timestamp_type timestamp,
                                 const flat_set<digest_type>& currently_activated_protocol_features,
                                 const vector<digest_type>& new_protocol_features )
   {
      const auto& pfs = protocol_features.get_protocol_feature_set();

      for( auto itr = new_protocol_features.begin(); itr != new_protocol_features.end(); ++itr ) {
         const auto& f = *itr;

         auto status = pfs.is_recognized( f, timestamp );
         switch( status ) {
            case protocol_feature_set::recognized_t::unrecognized:
               EOS_THROW( protocol_feature_exception,
                          "protocol feature with digest '${digest}' is unrecognized", ("digest", f) );
            break;
            case protocol_feature_set::recognized_t::disabled:
               EOS_THROW( protocol_feature_exception,
                          "protocol feature with digest '${digest}' is disabled", ("digest", f) );
            break;
            case protocol_feature_set::recognized_t::too_early:
               EOS_THROW( protocol_feature_exception,
                          "${timestamp} is too early for the earliest allowed activation time of the protocol feature with digest '${digest}'", ("digest", f)("timestamp", timestamp) );
            break;
            case protocol_feature_set::recognized_t::ready:
            break;
            default:
               EOS_THROW( protocol_feature_exception, "unexpected recognized_t status" );
            break;
         }

         EOS_ASSERT( currently_activated_protocol_features.find( f ) == currently_activated_protocol_features.end(),
                     protocol_feature_exception,
                     "protocol feature with digest '${digest}' has already been activated",
                     ("digest", f)
         );

         auto dependency_checker = [&currently_activated_protocol_features, &new_protocol_features, &itr]
                                   ( const digest_type& f ) -> bool
         {
            if( currently_activated_protocol_features.find( f ) != currently_activated_protocol_features.end() )
               return true;

            return (std::find( new_protocol_features.begin(), itr, f ) != itr);
         };

         EOS_ASSERT( pfs.validate_dependencies( f, dependency_checker ), protocol_feature_exception,
                     "not all dependencies of protocol feature with digest '${digest}' have been activated",
                     ("digest", f)
         );
      }
   }

   void apply_block( const block_state_ptr& bsp, controller::block_status s, const trx_meta_cache_lookup& trx_lookup )
   { try {
      try {
         const signed_block_ptr& b = bsp->block;
         const auto& new_protocol_feature_activations = bsp->get_new_protocol_feature_activations();

         auto producer_block_id = bsp->id;
         start_block( b->timestamp, b->confirmed, new_protocol_feature_activations, s, producer_block_id);

         // validated in create_block_state_future()
         std::get<building_block>(pending->_block_stage)._trx_mroot_or_receipt_digests = b->transaction_mroot;

         const bool existing_trxs_metas = !bsp->trxs_metas().empty();
         const bool pub_keys_recovered = bsp->is_pub_keys_recovered();
         const bool skip_auth_checks = self.skip_auth_check();
         std::vector<std::tuple<transaction_metadata_ptr, recover_keys_future>> trx_metas;
         bool use_bsp_cached = false;
         if( pub_keys_recovered || (skip_auth_checks && existing_trxs_metas) ) {
            use_bsp_cached = true;
         } else {
            trx_metas.reserve( b->transactions.size() );
            for( const auto& receipt : b->transactions ) {
               if( std::holds_alternative<packed_transaction>(receipt.trx)) {
                  const auto& pt = std::get<packed_transaction>(receipt.trx);
                  transaction_metadata_ptr trx_meta_ptr = trx_lookup ? trx_lookup( pt.id() ) : transaction_metadata_ptr{};
                  if( trx_meta_ptr && *trx_meta_ptr->packed_trx() != pt ) trx_meta_ptr = nullptr;
                  if( trx_meta_ptr && ( skip_auth_checks || !trx_meta_ptr->recovered_keys().empty() ) ) {
                     trx_metas.emplace_back( std::move( trx_meta_ptr ), recover_keys_future{} );
                  } else if( skip_auth_checks ) {
                     packed_transaction_ptr ptrx( b, &pt ); // alias signed_block_ptr
                     trx_metas.emplace_back(
                           transaction_metadata::create_no_recover_keys( std::move(ptrx), transaction_metadata::trx_type::input ),
                           recover_keys_future{} );
                  } else {
                     packed_transaction_ptr ptrx( b, &pt ); // alias signed_block_ptr
                     auto fut = transaction_metadata::start_recover_keys(
                           std::move( ptrx ), thread_pool.get_executor(), chain_id, microseconds::maximum() );
                     trx_metas.emplace_back( transaction_metadata_ptr{}, std::move( fut ) );
                  }
               }
            }
         }

         transaction_trace_ptr trace;

         bool explicit_net = self.skip_trx_checks();

         size_t packed_idx = 0;
         const auto& trx_receipts = std::get<building_block>(pending->_block_stage)._pending_trx_receipts;
         for( const auto& receipt : b->transactions ) {
            auto num_pending_receipts = trx_receipts.size();
            if( std::holds_alternative<packed_transaction>(receipt.trx) ) {
               const auto& trx_meta = ( use_bsp_cached ? bsp->trxs_metas().at( packed_idx )
                                                       : ( !!std::get<0>( trx_metas.at( packed_idx ) ) ?
                                                             std::get<0>( trx_metas.at( packed_idx ) )
                                                             : std::get<1>( trx_metas.at( packed_idx ) ).get() ) );
               std::optional<uint32_t> explicit_net_usage_words;
               if( explicit_net ) {
                  explicit_net_usage_words = receipt.net_usage_words.value;
               }
               trace = push_transaction( trx_meta, fc::time_point::maximum(), receipt.cpu_usage_us, true, explicit_net_usage_words, 0 );
               ++packed_idx;
            } else if( std::holds_alternative<transaction_id_type>(receipt.trx) ) {
               trace = push_scheduled_transaction( std::get<transaction_id_type>(receipt.trx), fc::time_point::maximum(), receipt.cpu_usage_us, true );
            } else {
               EOS_ASSERT( false, block_validate_exception, "encountered unexpected receipt type" );
            }

            bool transaction_failed =  trace && trace->except;
            bool transaction_can_fail = receipt.status == transaction_receipt_header::hard_fail && std::holds_alternative<transaction_id_type>(receipt.trx);
            if( transaction_failed && !transaction_can_fail) {
               edump((*trace));
               throw *trace->except;
            }

            EOS_ASSERT( trx_receipts.size() > 0,
                        block_validate_exception, "expected a receipt",
                        ("block_num", b->block_num())("block_id", producer_block_id)("expected_receipt", receipt)
                      );
            EOS_ASSERT( trx_receipts.size() == num_pending_receipts + 1,
                        block_validate_exception, "expected receipt was not added",
                        ("block_num", b->block_num())("block_id", producer_block_id)("expected_receipt", receipt)
                      );
            const transaction_receipt_header& r = trx_receipts.back();
            EOS_ASSERT( r == static_cast<const transaction_receipt_header&>(receipt),
                        block_validate_exception, "receipt does not match",
                        ("producer_receipt", static_cast<const transaction_receipt_header&>(receipt))("validator_receipt", r) );
         }

         finalize_block();

         auto& ab = std::get<assembled_block>(pending->_block_stage);

         // this implicitly asserts that all header fields (less the signature) are identical
         EOS_ASSERT( producer_block_id == ab._id, block_validate_exception, "Block ID does not match",
                     ("producer_block_id",producer_block_id)("validator_block_id",ab._id) );

         if( !use_bsp_cached ) {
            bsp->set_trxs_metas( std::move( ab._trx_metas ), !skip_auth_checks );
         }
         // create completed_block with the existing block_state as we just verified it is the same as assembled_block
         pending->_block_stage = completed_block{ bsp };

         commit_block(false);
         return;
      } catch ( const std::bad_alloc& ) {
         throw;
      } catch ( const boost::interprocess::bad_alloc& ) {
         throw;
      } catch ( const fc::exception& e ) {
         edump((e.to_detail_string()));
         abort_block();
         throw;
      } catch ( const std::exception& e ) {
         edump((e.what()));
         abort_block();
         throw;
      }
   } FC_CAPTURE_AND_RETHROW() } /// apply_block

   std::future<block_state_ptr> create_block_state_future( const block_id_type& id, const signed_block_ptr& b ) {
      EOS_ASSERT( b, block_validate_exception, "null block" );

      // no reason for a block_state if fork_db already knows about block
      auto existing = fork_db.get_block( id );
      EOS_ASSERT( !existing, fork_database_exception, "we already know about this block: ${id}", ("id", id) );

      auto prev = fork_db.get_block_header( b->previous );
      EOS_ASSERT( prev, unlinkable_block_exception,
                  "unlinkable block ${id}", ("id", id)("previous", b->previous) );

      return async_thread_pool( thread_pool.get_executor(), [b, prev, id, control=this]() {
         const bool skip_validate_signee = false;

         auto trx_mroot = calculate_trx_merkle( b->transactions );
         EOS_ASSERT( b->transaction_mroot == trx_mroot, block_validate_exception,
                     "invalid block transaction merkle root ${b} != ${c}", ("b", b->transaction_mroot)("c", trx_mroot) );

         auto bsp = std::make_shared<block_state>(
                        *prev,
                        move( b ),
                        control->protocol_features.get_protocol_feature_set(),
                        [control]( block_timestamp_type timestamp,
                                   const flat_set<digest_type>& cur_features,
                                   const vector<digest_type>& new_features )
                        { control->check_protocol_features( timestamp, cur_features, new_features ); },
                        skip_validate_signee
         );

         EOS_ASSERT( id == bsp->id, block_validate_exception,
                     "provided id ${id} does not match block id ${bid}", ("id", id)("bid", bsp->id) );
         return bsp;
      } );
   }

   block_state_ptr push_block( std::future<block_state_ptr>& block_state_future,
                    const forked_branch_callback& forked_branch_cb, const trx_meta_cache_lookup& trx_lookup )
   {
      controller::block_status s = controller::block_status::complete;
      EOS_ASSERT(!pending, block_validate_exception, "it is not valid to push a block when there is a pending block");

      auto reset_prod_light_validation = fc::make_scoped_exit([old_value=trusted_producer_light_validation, this]() {
         trusted_producer_light_validation = old_value;
      });
      try {
         block_state_ptr bsp = block_state_future.get();
         const auto& b = bsp->block;

         if( conf.terminate_at_block > 0 && conf.terminate_at_block < b->block_num() ) {
            ilog("Reached configured maximum block ${num}; terminating", ("num", conf.terminate_at_block) );
            shutdown();
            return bsp;
         }

         emit( self.pre_accepted_block, b );

         fork_db.add( bsp );

         if (self.is_trusted_producer(b->producer)) {
            trusted_producer_light_validation = true;
         };

         emit( self.accepted_block_header, bsp );

         if( read_mode != db_read_mode::IRREVERSIBLE ) {
            maybe_switch_forks( fork_db.pending_head(), s, forked_branch_cb, trx_lookup );
         } else {
            log_irreversible();
         }
         return bsp;
      }
      FC_LOG_AND_RETHROW()
   }

   void replay_push_block( const signed_block_ptr& b, controller::block_status s ) {
      self.validate_db_available_size();
      self.validate_reversible_available_size();

      EOS_ASSERT(!pending, block_validate_exception, "it is not valid to push a block when there is a pending block");

      try {
         EOS_ASSERT( b, block_validate_exception, "trying to push empty block" );
         EOS_ASSERT( (s == controller::block_status::irreversible || s == controller::block_status::validated),
                     block_validate_exception, "invalid block status for replay" );

         if( conf.terminate_at_block > 0 && conf.terminate_at_block < b->block_num() ) {
            ilog("Reached configured maximum block ${num}; terminating", ("num", conf.terminate_at_block) );
            shutdown();
            return;
         }

         emit( self.pre_accepted_block, b );
         const bool skip_validate_signee = !conf.force_all_checks;

         auto bsp = std::make_shared<block_state>(
                        *head,
                        b,
                        protocol_features.get_protocol_feature_set(),
                        [this]( block_timestamp_type timestamp,
                                const flat_set<digest_type>& cur_features,
                                const vector<digest_type>& new_features )
                        { check_protocol_features( timestamp, cur_features, new_features ); },
                        skip_validate_signee
         );

         if( s != controller::block_status::irreversible ) {
            fork_db.add( bsp, true );
         }

         emit( self.accepted_block_header, bsp );

         if( s == controller::block_status::irreversible ) {
            apply_block( bsp, s, trx_meta_cache_lookup{} );
            head = bsp;

            // On replay, log_irreversible is not called and so no irreversible_block signal is emitted.
            // So emit it explicitly here.
            emit( self.irreversible_block, bsp );

            if (!self.skip_db_sessions(s)) {
               kv_db.commit(bsp->block_num);
            }

         } else {
            EOS_ASSERT( read_mode != db_read_mode::IRREVERSIBLE, block_validate_exception,
                        "invariant failure: cannot replay reversible blocks while in irreversible mode" );
            maybe_switch_forks( bsp, s, forked_branch_callback{}, trx_meta_cache_lookup{} );
         }

      } FC_LOG_AND_RETHROW( )
   }

   void maybe_switch_forks( const block_state_ptr& new_head, controller::block_status s,
                            const forked_branch_callback& forked_branch_cb, const trx_meta_cache_lookup& trx_lookup )
   {
      bool head_changed = true;
      if( new_head->header.previous == head->id ) {
         try {
            apply_block( new_head, s, trx_lookup );
            fork_db.mark_valid( new_head );
            head = new_head;
         } catch ( const std::exception& e ) {
            fork_db.remove( new_head->id );
            throw;
         }
      } else if( new_head->id != head->id ) {
         auto old_head = head;
         ilog("switching forks from ${current_head_id} (block number ${current_head_num}) to ${new_head_id} (block number ${new_head_num})",
              ("current_head_id", head->id)("current_head_num", head->block_num)("new_head_id", new_head->id)("new_head_num", new_head->block_num) );

         if (auto dm_logger = get_deep_mind_logger()) {
            fc_dlog(*dm_logger, "SWITCH_FORK ${from_id} ${to_id}",
               ("from_id", head->id)
               ("to_id", new_head->id)
            );
         }

         auto branches = fork_db.fetch_branch_from( new_head->id, head->id );

         if( branches.second.size() > 0 ) {
            for( auto itr = branches.second.begin(); itr != branches.second.end(); ++itr ) {
               pop_block();
            }
            EOS_ASSERT( self.head_block_id() == branches.second.back()->header.previous, fork_database_exception,
                     "loss of sync between fork_db and chainbase during fork switch" ); // _should_ never fail

            if( forked_branch_cb ) forked_branch_cb( branches.second );
         }

         for( auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr ) {
            auto except = std::exception_ptr{};
            try {
               apply_block( *ritr, (*ritr)->is_valid() ? controller::block_status::validated
                                                       : controller::block_status::complete, trx_lookup );
               fork_db.mark_valid( *ritr );
               head = *ritr;
            } catch ( const std::bad_alloc& ) {
              throw;
            } catch ( const boost::interprocess::bad_alloc& ) {
              throw;
            } catch (const fc::exception& e) {
               elog("exception thrown while switching forks ${e}", ("e", e.to_detail_string()));
               except = std::current_exception();
            } catch (const std::exception& e) {
               elog("exception thrown while switching forks ${e}", ("e", e.what()));
               except = std::current_exception();
            }

            if( except ) {
               // ritr currently points to the block that threw
               // Remove the block that threw and all forks built off it.
               fork_db.remove( (*ritr)->id );

               // pop all blocks from the bad fork, discarding their transactions
               // ritr base is a forward itr to the last block successfully applied
               auto applied_itr = ritr.base();
               for( auto itr = applied_itr; itr != branches.first.end(); ++itr ) {
                  pop_block();
               }
               EOS_ASSERT( self.head_block_id() == branches.second.back()->header.previous, fork_database_exception,
                           "loss of sync between fork_db and chainbase during fork switch reversal" ); // _should_ never fail

               // re-apply good blocks
               for( auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr ) {
                  apply_block( *ritr, controller::block_status::validated /* we previously validated these blocks*/, trx_lookup );
                  head = *ritr;
               }
               std::rethrow_exception(except);
            } // end if exception
         } /// end for each block in branch

         ilog("successfully switched fork to new head ${new_head_id}", ("new_head_id", new_head->id));
      } else {
         head_changed = false;
      }

      if( head_changed )
         log_irreversible();

   } /// push_block

   deque<transaction_metadata_ptr> abort_block() {
      deque<transaction_metadata_ptr> applied_trxs;
      if( pending ) {
         applied_trxs = pending->extract_trx_metas();
         pending.reset();
         protocol_features.popped_blocks_to( head->block_num );
      }
      return applied_trxs;
   }

   static checksum256_type calculate_trx_merkle( const deque<transaction_receipt>& trxs ) {
      deque<digest_type> trx_digests;
      for( const auto& a : trxs )
         trx_digests.emplace_back( a.digest() );
      return merkle( move( trx_digests ) );
   }

   void update_producers_authority() {
      const auto& producers = pending->get_pending_block_header_state().active_schedule.producers;

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

   bool sender_avoids_whitelist_blacklist_enforcement( account_name sender )const {
      if( conf.sender_bypass_whiteblacklist.size() > 0 &&
          ( conf.sender_bypass_whiteblacklist.find( sender ) != conf.sender_bypass_whiteblacklist.end() ) )
      {
         return true;
      }

      return false;
   }

   void check_actor_list( const flat_set<account_name>& actors )const {
      if( actors.size() == 0 ) return;

      if( conf.actor_whitelist.size() > 0 ) {
         // throw if actors is not a subset of whitelist
         const auto& whitelist = conf.actor_whitelist;
         bool is_subset = true;

         // quick extents check, then brute force the check actors
         if (*actors.cbegin() >= *whitelist.cbegin() && *actors.crbegin() <= *whitelist.crbegin() ) {
            auto lower_bound = whitelist.cbegin();
            for (const auto& actor: actors) {
               lower_bound = std::lower_bound(lower_bound, whitelist.cend(), actor);

               // if the actor is not found, this is not a subset
               if (lower_bound == whitelist.cend() || *lower_bound != actor ) {
                  is_subset = false;
                  break;
               }

               // if the actor was found, we are guaranteed that other actors are either not present in the whitelist
               // or will be present in the range defined as [next actor,end)
               lower_bound = std::next(lower_bound);
            }
         } else {
            is_subset = false;
         }

         // helper lambda to lazily calculate the actors for error messaging
         static auto generate_missing_actors = [](const flat_set<account_name>& actors, const flat_set<account_name>& whitelist) -> vector<account_name> {
            vector<account_name> excluded;
            excluded.reserve( actors.size() );
            set_difference( actors.begin(), actors.end(),
                            whitelist.begin(), whitelist.end(),
                            std::back_inserter(excluded) );
            return excluded;
         };

         EOS_ASSERT( is_subset,  actor_whitelist_exception,
                     "authorizing actor(s) in transaction are not on the actor whitelist: ${actors}",
                     ("actors", generate_missing_actors(actors, whitelist))
                   );
      } else if( conf.actor_blacklist.size() > 0 ) {
         // throw if actors intersects blacklist
         const auto& blacklist = conf.actor_blacklist;
         bool intersects = false;

         // quick extents check then brute force check actors
         if( *actors.cbegin() <= *blacklist.crbegin() && *actors.crbegin() >= *blacklist.cbegin() ) {
            auto lower_bound = blacklist.cbegin();
            for (const auto& actor: actors) {
               lower_bound = std::lower_bound(lower_bound, blacklist.cend(), actor);

               // if the lower bound in the blacklist is at the end, all other actors are guaranteed to
               // not exist in the blacklist
               if (lower_bound == blacklist.cend()) {
                  break;
               }

               // if the lower bound of an actor IS the actor, then we have an intersection
               if (*lower_bound == actor) {
                  intersects = true;
                  break;
               }
            }
         }

         // helper lambda to lazily calculate the actors for error messaging
         static auto generate_blacklisted_actors = [](const flat_set<account_name>& actors, const flat_set<account_name>& blacklist) -> vector<account_name> {
            vector<account_name> blacklisted;
            blacklisted.reserve( actors.size() );
            set_intersection( actors.begin(), actors.end(),
                              blacklist.begin(), blacklist.end(),
                              std::back_inserter(blacklisted)
                            );
            return blacklisted;
         };

         EOS_ASSERT( !intersects, actor_blacklist_exception,
                     "authorizing actor(s) in transaction are on the actor blacklist: ${actors}",
                     ("actors", generate_blacklisted_actors(actors, blacklist))
                   );
      }
   }

   void check_contract_list( account_name code )const {
      if( conf.contract_whitelist.size() > 0 ) {
         EOS_ASSERT( conf.contract_whitelist.find( code ) != conf.contract_whitelist.end(),
                     contract_whitelist_exception,
                     "account '${code}' is not on the contract whitelist", ("code", code)
                   );
      } else if( conf.contract_blacklist.size() > 0 ) {
         EOS_ASSERT( conf.contract_blacklist.find( code ) == conf.contract_blacklist.end(),
                     contract_blacklist_exception,
                     "account '${code}' is on the contract blacklist", ("code", code)
                   );
      }
   }

   void check_action_list( account_name code, action_name action )const {
      if( conf.action_blacklist.size() > 0 ) {
         EOS_ASSERT( conf.action_blacklist.find( std::make_pair(code, action) ) == conf.action_blacklist.end(),
                     action_blacklist_exception,
                     "action '${code}::${action}' is on the action blacklist",
                     ("code", code)("action", action)
                   );
      }
   }

   void check_key_list( const public_key_type& key )const {
      if( conf.key_blacklist.size() > 0 ) {
         EOS_ASSERT( conf.key_blacklist.find( key ) == conf.key_blacklist.end(),
                     key_blacklist_exception,
                     "public key '${key}' is on the key blacklist",
                     ("key", key)
                   );
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
      on_block_act.name = "onblock"_n;
      on_block_act.authorization = vector<permission_level>{{config::system_account_name, config::active_name}};
      on_block_act.data = fc::raw::pack(self.head_block_header());

      signed_transaction trx;
      trx.actions.emplace_back(std::move(on_block_act));
      if( self.is_builtin_activated( builtin_protocol_feature_t::no_duplicate_deferred_id ) ) {
         trx.expiration = time_point_sec();
         trx.ref_block_num = 0;
         trx.ref_block_prefix = 0;
      } else {
         trx.expiration = self.pending_block_time() + fc::microseconds(999'999); // Round up to nearest second to avoid appearing expired
         trx.set_reference_block( self.head_block_id() );
      }

      if (auto dm_logger = get_deep_mind_logger()) {
         auto packed_trx = fc::raw::pack(trx);

         fc_dlog(*dm_logger, "TRX_OP CREATE onblock ${id} ${trx}",
            ("id", trx.id())
            ("trx", fc::to_hex(packed_trx))
         );
      }

      return trx;
   }

   inline fc::logger* get_deep_mind_logger() const {
      return deep_mind_logger;
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

const protocol_feature_manager& controller::get_protocol_feature_manager()const
{
   return my->protocol_features;
}

uint32_t controller::get_max_nonprivileged_inline_action_size()const
{
   return my->conf.max_nonprivileged_inline_action_size;
}

const controller::config& controller::get_config()const
{
   return my->conf;
}

controller::controller( const controller::config& cfg, const chain_id_type& chain_id )
:my( new controller_impl( cfg, *this, protocol_feature_set{}, chain_id ) )
{
}

controller::controller( const config& cfg, protocol_feature_set&& pfs, const chain_id_type& chain_id )
:my( new controller_impl( cfg, *this, std::move(pfs), chain_id ) )
{
}

controller::~controller() {
   my->abort_block();
   /* Shouldn't be needed anymore.
   //close fork_db here, because it can generate "irreversible" signal to this controller,
   //in case if read-mode == IRREVERSIBLE, we will apply latest irreversible block
   //for that we need 'my' to be valid pointer pointing to valid controller_impl.
   my->fork_db.close();
   */

   my->kv_db.flush();
}

void controller::add_indices() {
   my->add_indices();
}

void controller::startup( std::function<void()> shutdown, std::function<bool()> check_shutdown, const snapshot_reader_ptr& snapshot ) {
   my->startup(shutdown, check_shutdown, snapshot);
}

void controller::startup( std::function<void()> shutdown, std::function<bool()> check_shutdown, const genesis_state& genesis ) {
   my->startup(shutdown, check_shutdown, genesis);
}

void controller::startup(std::function<void()> shutdown, std::function<bool()> check_shutdown) {
   my->startup(shutdown, check_shutdown);
}

const chainbase::database& controller::db()const { return my->db; }

chainbase::database& controller::mutable_db()const { return my->db; }

const fork_database& controller::fork_db()const { return my->fork_db; }
eosio::chain::combined_database& controller::kv_db() const { return my->kv_db; }

const chainbase::database& controller::reversible_db()const { return my->reversible_blocks; }

void controller::preactivate_feature( uint32_t action_id, const digest_type& feature_digest ) {
   const auto& pfs = my->protocol_features.get_protocol_feature_set();
   auto cur_time = pending_block_time();

   auto status = pfs.is_recognized( feature_digest, cur_time );
   switch( status ) {
      case protocol_feature_set::recognized_t::unrecognized:
         if( is_producing_block() ) {
            EOS_THROW( subjective_block_production_exception,
                       "protocol feature with digest '${digest}' is unrecognized", ("digest", feature_digest) );
         } else {
            EOS_THROW( protocol_feature_bad_block_exception,
                       "protocol feature with digest '${digest}' is unrecognized", ("digest", feature_digest) );
         }
      break;
      case protocol_feature_set::recognized_t::disabled:
         if( is_producing_block() ) {
            EOS_THROW( subjective_block_production_exception,
                       "protocol feature with digest '${digest}' is disabled", ("digest", feature_digest) );
         } else {
            EOS_THROW( protocol_feature_bad_block_exception,
                       "protocol feature with digest '${digest}' is disabled", ("digest", feature_digest) );
         }
      break;
      case protocol_feature_set::recognized_t::too_early:
         if( is_producing_block() ) {
            EOS_THROW( subjective_block_production_exception,
                       "${timestamp} is too early for the earliest allowed activation time of the protocol feature with digest '${digest}'", ("digest", feature_digest)("timestamp", cur_time) );
         } else {
            EOS_THROW( protocol_feature_bad_block_exception,
                       "${timestamp} is too early for the earliest allowed activation time of the protocol feature with digest '${digest}'", ("digest", feature_digest)("timestamp", cur_time) );
         }
      break;
      case protocol_feature_set::recognized_t::ready:
      break;
      default:
         if( is_producing_block() ) {
            EOS_THROW( subjective_block_production_exception, "unexpected recognized_t status" );
         } else {
            EOS_THROW( protocol_feature_bad_block_exception, "unexpected recognized_t status" );
         }
      break;
   }

   // The above failures depend on subjective information.
   // Because of deferred transactions, this complicates things considerably.

   // If producing a block, we throw a subjective failure if the feature is not properly recognized in order
   // to try to avoid retiring into a block a deferred transacton driven by subjective information.

   // But it is still possible for a producer to retire a deferred transaction that deals with this subjective
   // information. If they recognized the feature, they would retire it successfully, but a validator that
   // does not recognize the feature should reject the entire block (not just fail the deferred transaction).
   // Even if they don't recognize the feature, the producer could change their nodeos code to treat it like an
   // objective failure thus leading the deferred transaction to retire with soft_fail or hard_fail.
   // In this case, validators that don't recognize the feature would reject the whole block immediately, and
   // validators that do recognize the feature would likely lead to a different retire status which would
   // ultimately cause a validation failure and thus rejection of the block.
   // In either case, it results in rejection of the block which is the desired behavior in this scenario.

   // If the feature is properly recognized by producer and validator, we have dealt with the subjectivity and
   // now only consider the remaining failure modes which are deterministic and objective.
   // Thus the exceptions that can be thrown below can be regular objective exceptions
   // that do not cause immediate rejection of the block.

   EOS_ASSERT( !is_protocol_feature_activated( feature_digest ),
               protocol_feature_exception,
               "protocol feature with digest '${digest}' is already activated",
               ("digest", feature_digest)
   );

   const auto& pso = my->db.get<protocol_state_object>();

   EOS_ASSERT( std::find( pso.preactivated_protocol_features.begin(),
                          pso.preactivated_protocol_features.end(),
                          feature_digest
               ) == pso.preactivated_protocol_features.end(),
               protocol_feature_exception,
               "protocol feature with digest '${digest}' is already pre-activated",
               ("digest", feature_digest)
   );

   auto dependency_checker = [&]( const digest_type& d ) -> bool
   {
      if( is_protocol_feature_activated( d ) ) return true;

      return ( std::find( pso.preactivated_protocol_features.begin(),
                          pso.preactivated_protocol_features.end(),
                          d ) != pso.preactivated_protocol_features.end() );
   };

   EOS_ASSERT( pfs.validate_dependencies( feature_digest, dependency_checker ),
               protocol_feature_exception,
               "not all dependencies of protocol feature with digest '${digest}' have been activated or pre-activated",
               ("digest", feature_digest)
   );

   if (auto dm_logger = get_deep_mind_logger()) {
      const auto feature = pfs.get_protocol_feature(feature_digest);

      fc_dlog(*dm_logger, "FEATURE_OP PRE_ACTIVATE ${action_id} ${feature_digest} ${feature}",
         ("action_id", action_id)
         ("feature_digest", feature_digest)
         ("feature", feature.to_variant())
      );
   }

   my->db.modify( pso, [&]( auto& ps ) {
      ps.preactivated_protocol_features.push_back( feature_digest );
   } );
}

vector<digest_type> controller::get_preactivated_protocol_features()const {
   const auto& pso = my->db.get<protocol_state_object>();

   if( pso.preactivated_protocol_features.size() == 0 ) return {};

   vector<digest_type> preactivated_protocol_features;

   for( const auto& f : pso.preactivated_protocol_features ) {
      preactivated_protocol_features.emplace_back( f );
   }

   return preactivated_protocol_features;
}

void controller::validate_protocol_features( const vector<digest_type>& features_to_activate )const {
   my->check_protocol_features( my->head->header.timestamp,
                                my->head->activated_protocol_features->protocol_features,
                                features_to_activate );
}

void controller::start_block( block_timestamp_type when, uint16_t confirm_block_count )
{
   validate_db_available_size();

   EOS_ASSERT( !my->pending, block_validate_exception, "pending block already exists" );

   vector<digest_type> new_protocol_feature_activations;

   const auto& pso = my->db.get<protocol_state_object>();
   if( pso.preactivated_protocol_features.size() > 0 ) {
      for( const auto& f : pso.preactivated_protocol_features ) {
         new_protocol_feature_activations.emplace_back( f );
      }
   }

   if( new_protocol_feature_activations.size() > 0 ) {
      validate_protocol_features( new_protocol_feature_activations );
   }

   my->start_block( when, confirm_block_count, new_protocol_feature_activations,
                    block_status::incomplete, std::optional<block_id_type>() );
}

void controller::start_block( block_timestamp_type when,
                              uint16_t confirm_block_count,
                              const vector<digest_type>& new_protocol_feature_activations )
{
   validate_db_available_size();

   if( new_protocol_feature_activations.size() > 0 ) {
      validate_protocol_features( new_protocol_feature_activations );
   }

   my->start_block( when, confirm_block_count, new_protocol_feature_activations,
                    block_status::incomplete, std::optional<block_id_type>() );
}

block_state_ptr controller::finalize_block( const signer_callback_type& signer_callback ) {
   validate_db_available_size();

   my->finalize_block();

   auto& ab = std::get<assembled_block>(my->pending->_block_stage);

   auto bsp = std::make_shared<block_state>(
                  std::move( ab._pending_block_header_state ),
                  std::move( ab._unsigned_block ),
                  std::move( ab._trx_metas ),
                  my->protocol_features.get_protocol_feature_set(),
                  []( block_timestamp_type timestamp,
                      const flat_set<digest_type>& cur_features,
                      const vector<digest_type>& new_features )
                  {},
                  signer_callback
              );

   my->pending->_block_stage = completed_block{ bsp };

   return bsp;
}

void controller::commit_block() {
   validate_db_available_size();
   validate_reversible_available_size();
   my->commit_block(true);
}

deque<transaction_metadata_ptr> controller::abort_block() {
   return my->abort_block();
}

boost::asio::io_context& controller::get_thread_pool() {
   return my->thread_pool.get_executor();
}

std::future<block_state_ptr> controller::create_block_state_future( const block_id_type& id, const signed_block_ptr& b ) {
   return my->create_block_state_future( id, b );
}

block_state_ptr controller::push_block( std::future<block_state_ptr>& block_state_future,
                             const forked_branch_callback& forked_branch_cb, const trx_meta_cache_lookup& trx_lookup )
{
   validate_db_available_size();
   validate_reversible_available_size();
   return my->push_block( block_state_future, forked_branch_cb, trx_lookup );
}

transaction_trace_ptr controller::push_transaction( const transaction_metadata_ptr& trx, fc::time_point deadline,
                                                    uint32_t billed_cpu_time_us, bool explicit_billed_cpu_time,
                                                    uint32_t subjective_cpu_bill_us ) {
   validate_db_available_size();
   EOS_ASSERT( get_read_mode() != db_read_mode::IRREVERSIBLE, transaction_type_exception, "push transaction not allowed in irreversible mode" );
   EOS_ASSERT( trx && !trx->implicit && !trx->scheduled, transaction_type_exception, "Implicit/Scheduled transaction not allowed" );
   return my->push_transaction(trx, deadline, billed_cpu_time_us, explicit_billed_cpu_time, {}, subjective_cpu_bill_us );
}

transaction_trace_ptr controller::push_scheduled_transaction( const transaction_id_type& trxid, fc::time_point deadline,
                                                              uint32_t billed_cpu_time_us, bool explicit_billed_cpu_time )
{
   EOS_ASSERT( get_read_mode() != db_read_mode::IRREVERSIBLE, transaction_type_exception, "push scheduled transaction not allowed in irreversible mode" );
   validate_db_available_size();
   return my->push_scheduled_transaction( trxid, deadline, billed_cpu_time_us, explicit_billed_cpu_time );
}

const flat_set<account_name>& controller::get_actor_whitelist() const {
   return my->conf.actor_whitelist;
}
const flat_set<account_name>& controller::get_actor_blacklist() const {
   return my->conf.actor_blacklist;
}
const flat_set<account_name>& controller::get_contract_whitelist() const {
   return my->conf.contract_whitelist;
}
const flat_set<account_name>& controller::get_contract_blacklist() const {
   return my->conf.contract_blacklist;
}
const flat_set< pair<account_name, action_name> >& controller::get_action_blacklist() const {
   return my->conf.action_blacklist;
}
const flat_set<public_key_type>& controller::get_key_blacklist() const {
   return my->conf.key_blacklist;
}

void controller::set_actor_whitelist( const flat_set<account_name>& new_actor_whitelist ) {
   my->conf.actor_whitelist = new_actor_whitelist;
}
void controller::set_actor_blacklist( const flat_set<account_name>& new_actor_blacklist ) {
   my->conf.actor_blacklist = new_actor_blacklist;
}
void controller::set_contract_whitelist( const flat_set<account_name>& new_contract_whitelist ) {
   my->conf.contract_whitelist = new_contract_whitelist;
}
void controller::set_contract_blacklist( const flat_set<account_name>& new_contract_blacklist ) {
   my->conf.contract_blacklist = new_contract_blacklist;
}
void controller::set_action_blacklist( const flat_set< pair<account_name, action_name> >& new_action_blacklist ) {
   for (auto& act: new_action_blacklist) {
      EOS_ASSERT(act.first != account_name(), name_type_exception, "Action blacklist - contract name should not be empty");
      EOS_ASSERT(act.second != action_name(), action_type_exception, "Action blacklist - action name should not be empty");
   }
   my->conf.action_blacklist = new_action_blacklist;
}
void controller::set_key_blacklist( const flat_set<public_key_type>& new_key_blacklist ) {
   my->conf.key_blacklist = new_key_blacklist;
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

uint32_t controller::fork_db_head_block_num()const {
   return my->fork_db.head()->block_num;
}

block_id_type controller::fork_db_head_block_id()const {
   return my->fork_db.head()->id;
}

time_point controller::fork_db_head_block_time()const {
   return my->fork_db.head()->header.timestamp;
}

account_name  controller::fork_db_head_block_producer()const {
   return my->fork_db.head()->header.producer;
}

uint32_t controller::fork_db_pending_head_block_num()const {
   return my->fork_db.pending_head()->block_num;
}

block_id_type controller::fork_db_pending_head_block_id()const {
   return my->fork_db.pending_head()->id;
}

time_point controller::fork_db_pending_head_block_time()const {
   return my->fork_db.pending_head()->header.timestamp;
}

account_name  controller::fork_db_pending_head_block_producer()const {
   return my->fork_db.pending_head()->header.producer;
}

time_point controller::pending_block_time()const {
   EOS_ASSERT( my->pending, block_validate_exception, "no pending block" );

   if( std::holds_alternative<completed_block>(my->pending->_block_stage) )
      return std::get<completed_block>(my->pending->_block_stage)._block_state->header.timestamp;

   return my->pending->get_pending_block_header_state().timestamp;
}

account_name controller::pending_block_producer()const {
   EOS_ASSERT( my->pending, block_validate_exception, "no pending block" );

   if( std::holds_alternative<completed_block>(my->pending->_block_stage) )
      return std::get<completed_block>(my->pending->_block_stage)._block_state->header.producer;

   return my->pending->get_pending_block_header_state().producer;
}

const block_signing_authority& controller::pending_block_signing_authority()const {
   EOS_ASSERT( my->pending, block_validate_exception, "no pending block" );

   if( std::holds_alternative<completed_block>(my->pending->_block_stage) )
      return std::get<completed_block>(my->pending->_block_stage)._block_state->valid_block_signing_authority;

   return my->pending->get_pending_block_header_state().valid_block_signing_authority;
}

std::optional<block_id_type> controller::pending_producer_block_id()const {
   EOS_ASSERT( my->pending, block_validate_exception, "no pending block" );
   return my->pending->_producer_block_id;
}

const deque<transaction_receipt>& controller::get_pending_trx_receipts()const {
   EOS_ASSERT( my->pending, block_validate_exception, "no pending block" );
   return my->pending->get_trx_receipts();
}

uint32_t controller::last_irreversible_block_num() const {
   return my->fork_db.root()->block_num;
}

block_id_type controller::last_irreversible_block_id() const {
   auto lib_num = last_irreversible_block_num();

   return get_block_id_for_num( lib_num );
}

time_point controller::last_irreversible_block_time() const {
   return my->fork_db.root()->header.timestamp.to_time_point();
}

const signed_block_ptr controller::last_irreversible_block() const {
  return my->blog.head();
}

const dynamic_global_property_object& controller::get_dynamic_global_properties()const {
  return my->db.get<dynamic_global_property_object>();
}
const global_property_object& controller::get_global_properties()const {
  return my->db.get<global_property_object>();
}

signed_block_ptr controller::fetch_block_by_id( block_id_type id )const {
   auto state = my->fork_db.get_block(id);
   if( state && state->block ) return state->block;
   auto bptr = fetch_block_by_number( block_header::num_from_id(id) );
   if( bptr && bptr->calculate_id() == id ) return bptr;
   return signed_block_ptr();
}

signed_block_ptr controller::fetch_block_by_number( uint32_t block_num )const  { try {
   auto blk_state = fetch_block_state_by_number( block_num );
   if( blk_state ) {
      return blk_state->block;
   }

   return my->blog.read_signed_block_by_num(block_num);
} FC_CAPTURE_AND_RETHROW( (block_num) ) }

block_state_ptr controller::fetch_block_state_by_id( block_id_type id )const {
   auto state = my->fork_db.get_block(id);
   return state;
}

block_state_ptr controller::fetch_block_state_by_number( uint32_t block_num )const  { try {
   const auto& rev_blocks = my->reversible_blocks.get_index<reversible_block_index,by_num>();
   auto objitr = rev_blocks.find(block_num);

   if( objitr == rev_blocks.end() ) {
      if( my->read_mode == db_read_mode::IRREVERSIBLE ) {
         return my->fork_db.search_on_branch( my->fork_db.pending_head()->id, block_num );
      } else {
         return block_state_ptr();
      }
   }

   return my->fork_db.get_block( objitr->get_block_id() );
} FC_CAPTURE_AND_RETHROW( (block_num) ) }

block_id_type controller::get_block_id_for_num( uint32_t block_num )const { try {
   const auto& tapos_block_summary = db().get<block_summary_object>((uint16_t)block_num);

   if( block_header::num_from_id(tapos_block_summary.block_id) == block_num )
      return tapos_block_summary.block_id;

   const auto& blog_head = my->blog.head();

   bool find_in_blog = (blog_head && block_num <= blog_head->block_num());

   if( !find_in_blog ) {
      if( my->read_mode != db_read_mode::IRREVERSIBLE ) {
         const auto& rev_blocks = my->reversible_blocks.get_index<reversible_block_index,by_num>();
         auto objitr = rev_blocks.find(block_num);
         if( objitr != rev_blocks.end() ) {
            return objitr->get_block_id();
         }
      } else {
         auto bsp = my->fork_db.search_on_branch( my->fork_db.pending_head()->id, block_num );

         if( bsp ) return bsp->id;
      }
   }

   auto id = my->blog.read_block_id_by_num(block_num);

   EOS_ASSERT( BOOST_LIKELY( id != block_id_type() ), unknown_block_exception,
               "Could not find block: ${block}", ("block", block_num) );

   return id;
} FC_CAPTURE_AND_RETHROW( (block_num) ) }

sha256 controller::calculate_integrity_hash()const { try {
   return my->calculate_integrity_hash();
} FC_LOG_AND_RETHROW() }

void controller::write_snapshot( const snapshot_writer_ptr& snapshot ) const {
   EOS_ASSERT( !my->pending, block_validate_exception, "cannot take a consistent snapshot with a pending block" );
   return my->kv_db.add_to_snapshot(snapshot, *my->fork_db.head(), my->authorization, my->resource_limits);
}

int64_t controller::set_proposed_producers( vector<producer_authority> producers ) {
   const auto& gpo = get_global_properties();
   auto cur_block_num = head_block_num() + 1;

   if( producers.size() == 0 && is_builtin_activated( builtin_protocol_feature_t::disallow_empty_producer_schedule ) ) {
      return -1;
   }

   if( gpo.proposed_schedule_block_num ) {
      if( *gpo.proposed_schedule_block_num != cur_block_num )
         return -1; // there is already a proposed schedule set in a previous block, wait for it to become pending

      if( std::equal( producers.begin(), producers.end(),
                      gpo.proposed_schedule.producers.begin(), gpo.proposed_schedule.producers.end() ) )
         return -1; // the proposed producer schedule does not change
   }

   producer_authority_schedule sch;

   decltype(sch.producers.cend()) end;
   decltype(end)                  begin;

   const auto& pending_sch = pending_producers();

   if( pending_sch.producers.size() == 0 ) {
      const auto& active_sch = active_producers();
      begin = active_sch.producers.begin();
      end   = active_sch.producers.end();
      sch.version = active_sch.version + 1;
   } else {
      begin = pending_sch.producers.begin();
      end   = pending_sch.producers.end();
      sch.version = pending_sch.version + 1;
   }

   if( std::equal( producers.begin(), producers.end(), begin, end ) )
      return -1; // the producer schedule would not change

   sch.producers = std::move(producers);

   int64_t version = sch.version;

   ilog( "proposed producer schedule with version ${v}", ("v", version) );

   my->db.modify( gpo, [&]( auto& gp ) {
      gp.proposed_schedule_block_num = cur_block_num;
      gp.proposed_schedule = sch.to_shared(gp.proposed_schedule.producers.get_allocator());
   });
   return version;
}

const producer_authority_schedule&    controller::active_producers()const {
   if( !(my->pending) )
      return  my->head->active_schedule;

   if( std::holds_alternative<completed_block>(my->pending->_block_stage) )
      return std::get<completed_block>(my->pending->_block_stage)._block_state->active_schedule;

   return my->pending->get_pending_block_header_state().active_schedule;
}

const producer_authority_schedule& controller::pending_producers()const {
   if( !(my->pending) )
      return  my->head->pending_schedule.schedule;

   if( std::holds_alternative<completed_block>(my->pending->_block_stage) )
      return std::get<completed_block>(my->pending->_block_stage)._block_state->pending_schedule.schedule;

   if( std::holds_alternative<assembled_block>(my->pending->_block_stage) ) {
      const auto& new_prods_cache = std::get<assembled_block>(my->pending->_block_stage)._new_producer_authority_cache;
      if( new_prods_cache ) {
         return *new_prods_cache;
      }
   }

   const auto& bb = std::get<building_block>(my->pending->_block_stage);

   if( bb._new_pending_producer_schedule )
      return *bb._new_pending_producer_schedule;

   return bb._pending_block_header_state.prev_pending_schedule.schedule;
}

std::optional<producer_authority_schedule> controller::proposed_producers()const {
   const auto& gpo = get_global_properties();
   if( !gpo.proposed_schedule_block_num )
      return std::optional<producer_authority_schedule>();

   return producer_authority_schedule::from_shared(gpo.proposed_schedule);
}

bool controller::light_validation_allowed() const {
   if (!my->pending || my->in_trx_requiring_checks) {
      return false;
   }

   const auto pb_status = my->pending->_block_status;

   // in a pending irreversible or previously validated block and we have forcing all checks
   const bool consider_skipping_on_replay =
         (pb_status == block_status::irreversible || pb_status == block_status::validated) && !my->conf.force_all_checks;

   // OR in a signed block and in light validation mode
   const bool consider_skipping_on_validate = (pb_status == block_status::complete &&
         (my->conf.block_validation_mode == validation_mode::LIGHT || my->trusted_producer_light_validation));

   return consider_skipping_on_replay || consider_skipping_on_validate;
}


bool controller::skip_auth_check() const {
   return light_validation_allowed();
}

bool controller::skip_trx_checks() const {
   return light_validation_allowed();
}

bool controller::skip_db_sessions( block_status bs ) const {
   bool consider_skipping = bs == block_status::irreversible;
   return consider_skipping
      && !my->conf.disable_replay_opts
      && !my->in_trx_requiring_checks;
}

bool controller::skip_db_sessions() const {
   if (my->pending) {
      return skip_db_sessions(my->pending->_block_status);
   } else {
      return false;
   }
}

bool controller::is_trusted_producer( const account_name& producer) const {
   return get_validation_mode() == chain::validation_mode::LIGHT || my->conf.trusted_producers.count(producer);
}

bool controller::contracts_console()const {
   return my->conf.contracts_console;
}

chain_id_type controller::get_chain_id()const {
   return my->chain_id;
}

db_read_mode controller::get_read_mode()const {
   return my->read_mode;
}

validation_mode controller::get_validation_mode()const {
   return my->conf.block_validation_mode;
}

const flat_set<account_name>& controller::get_trusted_producers()const {
   return my->conf.trusted_producers;
}

uint32_t controller::get_terminate_at_block()const {
   return my->conf.terminate_at_block;
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

bool controller::sender_avoids_whitelist_blacklist_enforcement( account_name sender )const {
   return my->sender_avoids_whitelist_blacklist_enforcement( sender );
}

void controller::check_actor_list( const flat_set<account_name>& actors )const {
   my->check_actor_list( actors );
}

void controller::check_contract_list( account_name code )const {
   my->check_contract_list( code );
}

void controller::check_action_list( account_name code, action_name action )const {
   my->check_action_list( code, action );
}

void controller::check_key_list( const public_key_type& key )const {
   my->check_key_list( key );
}

bool controller::is_building_block()const {
   return my->pending.has_value();
}

bool controller::is_producing_block()const {
   if( !my->pending ) return false;

   return (my->pending->_block_status == block_status::incomplete);
}

bool controller::is_ram_billing_in_notify_allowed()const {
   return my->conf.disable_all_subjective_mitigations || !is_producing_block() || my->conf.allow_ram_billing_in_notify;
}

uint32_t controller::configured_subjective_signature_length_limit()const {
   return my->conf.maximum_variable_signature_length;
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

void controller::validate_db_available_size() const {
   const auto free = db().get_segment_manager()->get_free_memory();
   const auto guard = my->conf.state_guard_size;
   EOS_ASSERT(free >= guard, database_guard_exception, "database free: ${f}, guard size: ${g}", ("f", free)("g",guard));
}

void controller::validate_reversible_available_size() const {
   const auto free = my->reversible_blocks.get_segment_manager()->get_free_memory();
   const auto guard = my->conf.reversible_guard_size;
   EOS_ASSERT(free >= guard, reversible_guard_exception, "reversible free: ${f}, guard size: ${g}", ("f", free)("g",guard));
}

bool controller::is_protocol_feature_activated( const digest_type& feature_digest )const {
   if( my->pending )
      return my->pending->is_protocol_feature_activated( feature_digest );

   const auto& activated_features = my->head->activated_protocol_features->protocol_features;
   return (activated_features.find( feature_digest ) != activated_features.end());
}

bool controller::is_builtin_activated( builtin_protocol_feature_t f )const {
   uint32_t current_block_num = head_block_num();

   if( my->pending ) {
      ++current_block_num;
   }

   return my->protocol_features.is_builtin_activated( f, current_block_num );
}

bool controller::is_known_unexpired_transaction( const transaction_id_type& id) const {
   return db().find<transaction_object, by_trx_id>(id);
}

void controller::set_subjective_cpu_leeway(fc::microseconds leeway) {
   my->subjective_cpu_leeway = leeway;
}

std::optional<fc::microseconds> controller::get_subjective_cpu_leeway() const {
    return my->subjective_cpu_leeway;
}

void controller::set_greylist_limit( uint32_t limit ) {
   EOS_ASSERT( 0 < limit && limit <= chain::config::maximum_elastic_resource_multiplier,
               misc_exception,
               "Invalid limit (${limit}) passed into set_greylist_limit. "
               "Must be between 1 and ${max}.",
               ("limit", limit)("max", chain::config::maximum_elastic_resource_multiplier)
   );
   my->conf.greylist_limit = limit;
}

uint32_t controller::get_greylist_limit()const {
   return my->conf.greylist_limit;
}

void controller::add_resource_greylist(const account_name &name) {
   my->conf.resource_greylist.insert(name);
}

void controller::remove_resource_greylist(const account_name &name) {
   my->conf.resource_greylist.erase(name);
}

bool controller::is_resource_greylisted(const account_name &name) const {
   return my->conf.resource_greylist.find(name) !=  my->conf.resource_greylist.end();
}

const flat_set<account_name> &controller::get_resource_greylist() const {
   return  my->conf.resource_greylist;
}


void controller::add_to_ram_correction( account_name account, uint64_t ram_bytes, uint32_t action_id, const char* event_id ) {
   int64_t correction_object_id = 0;

   if( auto ptr = my->db.find<account_ram_correction_object, by_name>( account ) ) {
      my->db.modify<account_ram_correction_object>( *ptr, [&]( auto& rco ) {
         correction_object_id = rco.id._id;
         rco.ram_correction += ram_bytes;
      } );
   } else {
      my->db.create<account_ram_correction_object>( [&]( auto& rco ) {
         correction_object_id = rco.id._id;
         rco.name = account;
         rco.ram_correction = ram_bytes;
      } );
   }

   if (auto dm_logger = get_deep_mind_logger()) {
      fc_dlog(*dm_logger, "RAM_CORRECTION_OP ${action_id} ${correction_id} ${event_id} ${payer} ${delta}",
         ("action_id", action_id)
         ("correction_id", correction_object_id)
         ("event_id", event_id)
         ("payer", account)
         ("delta", ram_bytes)
      );
   }
}

fc::microseconds controller::get_abi_serializer_max_time()const {
   return my->conf.abi_serializer_max_time_us;
}

bool controller::all_subjective_mitigations_disabled()const {
   return my->conf.disable_all_subjective_mitigations;
}

fc::logger* controller::get_deep_mind_logger()const {
   return my->get_deep_mind_logger();
}

void controller::enable_deep_mind(fc::logger* logger) {
   EOS_ASSERT( logger != nullptr, misc_exception, "Invalid logger passed into enable_deep_mind, must be set" );
   my->deep_mind_logger = logger;
}

#if defined(EOSIO_EOS_VM_RUNTIME_ENABLED) || defined(EOSIO_EOS_VM_JIT_RUNTIME_ENABLED)
vm::wasm_allocator& controller::get_wasm_allocator() {
   return my->wasm_alloc;
}
#endif

std::optional<uint64_t> controller::convert_exception_to_error_code( const fc::exception& e ) {
   const chain_exception* e_ptr = dynamic_cast<const chain_exception*>( &e );

   if( e_ptr == nullptr ) return {};

   if( !e_ptr->error_code ) return static_cast<uint64_t>(system_error_code::generic_system_error);

   return e_ptr->error_code;
}

chain_id_type controller::extract_chain_id(snapshot_reader& snapshot) {
   chain_snapshot_header header;
   snapshot.read_section<chain_snapshot_header>([&header]( auto &section ){
      section.read_row(header);
      header.validate();
   });

   // check if this is a legacy version of the snapshot, which has a genesis state instead of chain id
   std::optional<genesis_state> genesis = extract_legacy_genesis_state(snapshot, header.version);
   if (genesis) {
      return genesis->compute_chain_id();
   }

   chain_id_type chain_id;
   using v4 = legacy::snapshot_global_property_object_v4;
   if (header.version <= v4::maximum_version) {
      snapshot.read_section<global_property_object>([&chain_id]( auto &section ){
         v4 global_properties;
         section.read_row(global_properties);
         chain_id = global_properties.chain_id;
      }); 
   }
   else {
      snapshot.read_section<global_property_object>([&chain_id]( auto &section ){
         snapshot_global_property_object global_properties;
         section.read_row(global_properties);
         chain_id = global_properties.chain_id;
      });      
   }

   return chain_id;
}

std::optional<chain_id_type> controller::extract_chain_id_from_db( const path& state_dir ) {
   try {
      chainbase::database db( state_dir, chainbase::database::read_only );

      db.add_index<database_header_multi_index>();
      db.add_index<global_property_multi_index>();

      controller_impl::validate_db_version( db );

      if( db.revision() < 1 ) return {};

      return db.get<global_property_object>().chain_id;
   } catch( const bad_database_version_exception& ) {
      throw;
   } catch( ... ) {
   }

   return {};
}

void controller::replace_producer_keys( const public_key_type& key ) {
   ilog("Replace producer keys with ${k}", ("k", key));
   mutable_db().modify( db().get<global_property_object>(), [&]( auto& gp ) {
      gp.proposed_schedule_block_num = {};
      gp.proposed_schedule.version = 0;
      gp.proposed_schedule.producers.clear();
   });
   auto version = my->head->pending_schedule.schedule.version;
   my->head->pending_schedule = {};
   my->head->pending_schedule.schedule.version = version;
   for (auto& prod: my->head->active_schedule.producers ) {
      ilog("${n}", ("n", prod.producer_name));
      std::visit([&](auto &auth) {
         auth.threshold = 1;
         auth.keys = {key_weight{key, 1}};
      }, prod.authority);
   }
}

void controller::replace_account_keys( name account, name permission, const public_key_type& key ) {
   auto& rlm = get_mutable_resource_limits_manager();
   auto* perm = db().find<permission_object, by_owner>(boost::make_tuple(account, permission));
   if (!perm)
      return;
   int64_t old_size = (int64_t)(chain::config::billable_size_v<permission_object> + perm->auth.get_billable_size());
   mutable_db().modify(*perm, [&](auto& p) {
      p.auth = authority(key);
   });
   int64_t new_size = (int64_t)(chain::config::billable_size_v<permission_object> + perm->auth.get_billable_size());
   rlm.add_pending_ram_usage(account, new_size - old_size, generic_storage_usage_trace(0));
   rlm.verify_account_ram_usage(account);
}

/// Protocol feature activation handlers:

template<>
void controller_impl::on_activation<builtin_protocol_feature_t::preactivate_feature>() {
   db.modify( db.get<protocol_state_object>(), [&]( auto& ps ) {
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "preactivate_feature" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "is_feature_activated" );
   } );
}

template<>
void controller_impl::on_activation<builtin_protocol_feature_t::get_sender>() {
   db.modify( db.get<protocol_state_object>(), [&]( auto& ps ) {
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "get_sender" );
   } );
}

template<>
void controller_impl::on_activation<builtin_protocol_feature_t::replace_deferred>() {
   const auto& indx = db.get_index<account_ram_correction_index, by_id>();
   for( auto itr = indx.begin(); itr != indx.end(); itr = indx.begin() ) {
      int64_t current_ram_usage = resource_limits.get_account_ram_usage( itr->name );
      int64_t ram_delta = -static_cast<int64_t>(itr->ram_correction);
      if( itr->ram_correction > static_cast<uint64_t>(current_ram_usage) ) {
         ram_delta = -current_ram_usage;
         elog( "account ${name} was to be reduced by ${adjust} bytes of RAM despite only using ${current} bytes of RAM",
               ("name", itr->name)("adjust", itr->ram_correction)("current", current_ram_usage) );
      }

      std::string event_id;
      if (get_deep_mind_logger() != nullptr) {
         event_id = STORAGE_EVENT_ID("${id}", ("id", itr->id._id));
      }

      resource_limits.add_pending_ram_usage( itr->name, ram_delta, storage_usage_trace(0, std::move(event_id), "deferred_trx", "correction", "deferred_trx_ram_correction") );
      db.remove( *itr );
   }
}

template<>
void controller_impl::on_activation<builtin_protocol_feature_t::webauthn_key>() {
   db.modify( db.get<protocol_state_object>(), [&]( auto& ps ) {
      ps.num_supported_key_types = 3;
   } );
}

template<>
void controller_impl::on_activation<builtin_protocol_feature_t::wtmsig_block_signatures>() {
   db.modify( db.get<protocol_state_object>(), [&]( auto& ps ) {
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "set_proposed_producers_ex" );
   } );
}

template<>
void controller_impl::on_activation<builtin_protocol_feature_t::action_return_value>() {
   db.modify( db.get<protocol_state_object>(), [&]( auto& ps ) {
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "set_action_return_value" );
   } );
}

template<>
void controller_impl::on_activation<builtin_protocol_feature_t::kv_database>() {
   db.modify( db.get<protocol_state_object>(), [&]( auto& ps ) {
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_erase" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_set" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_get" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_get_data" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_create" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_destroy" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_status" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_compare" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_key_compare" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_move_to_end" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_next" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_prev" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_lower_bound" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_key" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "kv_it_value" );
      // resource management
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "set_resource_limit" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "get_resource_limit" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "set_kv_parameters_packed" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "get_kv_parameters_packed" );
   } );
}

template<>
void controller_impl::on_activation<builtin_protocol_feature_t::configurable_wasm_limits>() {
   db.modify( db.get<protocol_state_object>(), [&]( auto& ps ) {
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "set_wasm_parameters_packed" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "get_wasm_parameters_packed" );
   } );
}

template<>
void controller_impl::on_activation<builtin_protocol_feature_t::blockchain_parameters>() {
   db.modify( db.get<protocol_state_object>(), [&]( auto& ps ) {
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "get_parameters_packed" );
      add_intrinsic_to_whitelist( ps.whitelisted_intrinsics, "set_parameters_packed" );
   } );
}

/// End of protocol feature activation handlers

} } /// eosio::chain
