#include <eosio/chain/controller.hpp>
#include <eosio/chain/context.hpp>

#include <eosio/chain/block_log.hpp>
#include <eosio/chain/fork_database.hpp>

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/scope_sequence_object.hpp>
#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/action_objects.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/permission_link_object.hpp>

#include <eosio/chain/resource_limits.hpp>

#include <chainbase/chainbase.hpp>
#include <fc/io/json.hpp>

#include <eosio/chain/eosio_contract.hpp>

namespace eosio { namespace chain {

using resource_limits::resource_limits_manager;

abi_def eos_contract_abi(const abi_def& eosio_system_abi);

struct pending_state {
   pending_state( database::session&& s )
   :_db_session( move(s) ){}

   database::session                  _db_session;
   vector<transaction_metadata_ptr>   _applied_transaction_metas;

   block_state_ptr                    _pending_block_state;

   vector<action_receipt>             _actions;


   void push() {
      _db_session.push();
   }
};

struct controller_impl {
   chainbase::database            db;
   block_log                      blog;
   optional<pending_state>        pending;
   block_state_ptr                head; 
   fork_database                  fork_db;            
   wasm_interface                 wasmif;
   resource_limits_manager        resource_limits;
   controller::config             conf;
   controller&                    self;

   typedef pair<scope_name,action_name>                   handler_key;
   map< account_name, map<handler_key, apply_handler> >   apply_handlers;

   block_id_type head_block_id()const {
      return head->id;
   }
   time_point head_block_time()const {
      return head->header.timestamp;
   }
   const block_header& head_block_header()const {
      return head->header;
   }

   void pop_block() {
      /// head = fork_db.get( head->previous );
      /// db.undo();
   }


   void set_apply_handler( account_name contract, scope_name scope, action_name action, apply_handler v ) {
      apply_handlers[contract][make_pair(scope,action)] = v;
   }

   controller_impl( const controller::config& cfg, controller& s  )
   :db( cfg.shared_memory_dir, 
        cfg.read_only ? database::read_only : database::read_write,
        cfg.shared_memory_size ),
    blog( cfg.block_log_dir ),
    fork_db( cfg.shared_memory_dir ),
    wasmif( cfg.wasm_runtime ),
    resource_limits( db ),
    conf( cfg ),
    self( s )
   {
      head = fork_db.head();


#define SET_APP_HANDLER( contract, scope, action, nspace ) \
   set_apply_handler( #contract, #scope, #action, &BOOST_PP_CAT(apply_, BOOST_PP_CAT(contract, BOOST_PP_CAT(_,action) ) ) )
   SET_APP_HANDLER( eosio, eosio, newaccount, eosio );
   SET_APP_HANDLER( eosio, eosio, setcode, eosio );
   SET_APP_HANDLER( eosio, eosio, setabi, eosio );
   SET_APP_HANDLER( eosio, eosio, updateauth, eosio );
   SET_APP_HANDLER( eosio, eosio, deleteauth, eosio );
   SET_APP_HANDLER( eosio, eosio, linkauth, eosio );
   SET_APP_HANDLER( eosio, eosio, unlinkauth, eosio );
   SET_APP_HANDLER( eosio, eosio, onerror, eosio );
   SET_APP_HANDLER( eosio, eosio, postrecovery, eosio );
   SET_APP_HANDLER( eosio, eosio, passrecovery, eosio );
   SET_APP_HANDLER( eosio, eosio, vetorecovery, eosio );
   SET_APP_HANDLER( eosio, eosio, canceldelay, eosio );


   }

   void init() {
      // ilog( "${c}", ("c",fc::json::to_pretty_string(cfg)) );
      initialize_indicies();

      initialize_fork_db();
      /**
       * The undable state contains state transitions from blocks
       * in the fork database that could be reversed. Because this
       * is a new startup and the fork database is empty, we must
       * unwind that pending state. This state will be regenerated
       * when we catch up to the head block later.
       */
      //clear_all_undo(); 
   }

   ~controller_impl() {
      pending.reset();

      edump((db.revision())(head->block_num));

      db.flush();
   }

   void initialize_indicies() {
      db.add_index<account_index>();
      db.add_index<permission_index>();
      db.add_index<permission_usage_index>();
      db.add_index<permission_link_index>();
      db.add_index<action_permission_index>();

      db.add_index<table_id_multi_index>();
      db.add_index<key_value_index>();
      db.add_index<index64_index>();
      db.add_index<index128_index>();
      db.add_index<index256_index>();
      db.add_index<index_double_index>();

      db.add_index<global_property_multi_index>();
      db.add_index<dynamic_global_property_multi_index>();
      db.add_index<block_summary_multi_index>();
      db.add_index<transaction_multi_index>();
      db.add_index<generated_transaction_multi_index>();
      db.add_index<scope_sequence_multi_index>();

      resource_limits.initialize_database();
   }

   void abort_pending_block() {
      pending.reset();
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
    *  The fork database needs an initial block_state to be set before
    *  it can accept any new blocks. This initial block state can be found
    *  in the database (whose head block state should be irreversible) or
    *  it would be the genesis state.
    */
   void initialize_fork_db() {
      head = fork_db.head();
      if( !head ) {
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
         signed_block genblock(genheader.header);

         edump((genheader.header));
         edump((genblock));
         blog.append( genblock );

         db.set_revision( head->block_num );
         fork_db.set( head );
  
         initialize_database();
      }
      FC_ASSERT( db.revision() == head->block_num, "fork database is inconsistant with shared memory",
                 ("db",db.revision())("head",head->block_num) );
   }

   void create_native_account( account_name name ) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = conf.genesis.initial_timestamp;
         a.privileged = true;

         if( name == config::system_account_name ) {
            a.set_abi(eos_contract_abi(abi_def()));
         }
      });
      const auto& owner = db.create<permission_object>([&](permission_object& p) {
         p.owner = name;
         p.name = "owner";
         p.auth.threshold = 1;
         p.auth.keys.push_back( key_weight{ .key = conf.genesis.initial_key, .weight = 1 } );
      });
      db.create<permission_object>([&](permission_object& p) {
         p.owner = name;
         p.parent = owner.id;
         p.name = "active";
         p.auth.threshold = 1;
         p.auth.keys.push_back( key_weight{ .key = conf.genesis.initial_key, .weight = 1 } );
      });

      resource_limits.initialize_account(name);
   }

   void initialize_database() {
      // Initialize block summary index
      for (int i = 0; i < 0x10000; i++)
         db.create<block_summary_object>([&](block_summary_object&) {});

      db.create<global_property_object>([](auto){});
      db.create<dynamic_global_property_object>([](auto){});
      db.create<permission_object>([](auto){}); /// reserve perm 0 (used else where)

      resource_limits.initialize_chain();

      create_native_account( config::system_account_name );

      // Create special accounts
      auto create_special_account = [&](account_name name, const auto& owner, const auto& active) {
         db.create<account_object>([&](account_object& a) {
            a.name = name;
            a.creation_date = conf.genesis.initial_timestamp;
         });
         const auto& owner_permission = db.create<permission_object>([&](permission_object& p) {
            p.name = config::owner_name;
            p.parent = 0;
            p.owner = name;
            p.auth = move(owner);
         });
         db.create<permission_object>([&](permission_object& p) {
            p.name = config::active_name;
            p.parent = owner_permission.id;
            p.owner = owner_permission.owner;
            p.auth = move(active);
         });
      };

      auto empty_authority = authority(0, {}, {});
      auto active_producers_authority = authority(0, {}, {});
      active_producers_authority.accounts.push_back({{config::system_account_name, config::active_name}, 1});

      create_special_account(config::nobody_account_name, empty_authority, empty_authority);
      create_special_account(config::producers_account_name, empty_authority, active_producers_authority);
   }



   /**
    *  This method will backup all tranasctions in the current pending block,
    *  undo the pending block, call f(), and then push the pending transactions
    *  on top of the new state.
    */
   template<typename Function>
   auto without_pending_transactions( Function&& f )
   {
#if 0
      vector<transaction_metadata> old_input;

      if( _pending_block )
         old_input = move(_pending_transaction_metas);

      clear_pending();

      /** after applying f() push previously input transactions on top */
      auto on_exit = fc::make_scoped_exit( [&](){
         for( auto& t : old_input ) {
            try {
               if (!is_known_transaction(t.id))
                  _push_transaction( std::move(t) );
            } catch ( ... ){}
         }
      });
#endif
      pending.reset();
      return f();
   }



   block_state_ptr push_block( const signed_block_ptr& b ) {
      return without_pending_transactions( [&](){ 
         return db.with_write_lock( [&](){ 
            return push_block_impl( b );
         });
      });
   }

   void commit_block( bool add_to_fork_db ) {
      if( add_to_fork_db ) {
         pending->_pending_block_state->validated = true;
         head = fork_db.add( pending->_pending_block_state );
      } 
      pending->push();
      pending.reset();
   }

   void push_transaction( const transaction_metadata_ptr& trx ) {
      return db.with_write_lock( [&](){ 
         return apply_transaction( trx );
      });
   }

   void apply_transaction( const transaction_metadata_ptr& trx, bool implicit = false ) {
      transaction_context trx_context( self, trx );
      trx_context.exec();

      auto& acts = pending->_actions;
      fc::move_append( acts, move(trx_context.executed) );

      if( !implicit ) {
         pending->_pending_block_state->block->transactions.emplace_back( trx->packed_trx );
         pending->_applied_transaction_metas.emplace_back( trx );
      }
   }


   void record_transaction( const transaction_metadata_ptr& trx ) {
      try {
          db.create<transaction_object>([&](transaction_object& transaction) {
              transaction.trx_id = trx->id;
              transaction.expiration = trx->trx.expiration;
          });
      } catch ( ... ) {
          EOS_ASSERT( false, transaction_exception,
                     "duplicate transaction ${id}", ("id", trx->id ) );
      }
   }







   block_state_ptr push_block_impl( const signed_block_ptr& b ) {
#if  0
      auto head_state = fork_db.add( b );
      /// check to see if we entered a fork
      if( head_state->header.previous != head_block_id() ) {
          auto branches = fork_db.fetch_branch_from(head_state->id, head_block_id());
          while (head_block_id() != branches.second.back()->header.previous)
             pop_block();

          /** apply all blocks from new fork */
          for( auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr) {
             optional<fc::exception> except;
             try {
                apply_block( *ritr );
             }
             catch (const fc::exception& e) { except = e; }
             if (except) {
                wlog("exception thrown while switching forks ${e}", ("e",except->to_detail_string()));

                while (ritr != branches.first.rend() ) {
                   fork_db.set_validity( *ritr, false );
                   ++ritr;
                }

                // pop all blocks from the bad fork
                while( head_block_id() != branches.second.back()->header.previous )
                   pop_block();
                
                // re-apply good blocks
                for( auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr ) {
                   apply_block( (*ritr) );
                }
                throw *except;
             } // end if exception
          } /// end for each block in branch

          return head_state;
      } /// end if fork

      try {
         apply_block( head_state );
      } catch ( const fc::exception& e ) {
         elog("Failed to push new block:\n${e}", ("e", e.to_detail_string()));
         fork_db.set_validity( head_state, false );
         throw;
      }
      return head;
#endif
   } /// push_block_impl

   bool should_enforce_runtime_limits()const {
      return false;
   }

   /*
   void validate_net_usage( const transaction& trx, uint32_t min_net_usage ) {
      uint32_t net_usage_limit = trx.max_net_usage_words.value * 8; // overflow checked in validate_transaction_without_state
      EOS_ASSERT( net_usage_limit == 0 || min_net_usage <= net_usage_limit,
                  transaction_exception,
                  "Packed transaction and associated data does not fit into the space committed to by the transaction's header! [usage=${usage},commitment=${commit}]",
                  ("usage", min_net_usage)("commit", net_usage_limit));
   } /// validate_net_usage
   */


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
   { try {
      ilog( "finalize block" );

      set_action_merkle();
      set_trx_merkle();

      auto p = pending->_pending_block_state;
      p->id = p->header.id();

      create_block_summary();


      /* TODO RESTORE 
      const auto& b = trace.block;
      update_global_properties( b );
      update_global_dynamic_data( b );
      update_signing_producer(signing_producer, b);

      create_block_summary(b);
      clear_expired_transactions();

      update_last_irreversible_block();

      resource_limits.process_account_limit_updates();

      const auto& chain_config = self.get_global_properties().configuration;
      resource_limits.set_block_parameters(
         {EOS_PERCENT(chain_config.max_block_cpu_usage, chain_config.target_block_cpu_usage_pct), chain_config.max_block_cpu_usage, config::block_cpu_usage_average_window_ms / config::block_interval_ms, 1000, {99, 100}, {1000, 999}},
         {EOS_PERCENT(chain_config.max_block_net_usage, chain_config.target_block_net_usage_pct), chain_config.max_block_net_usage, config::block_size_average_window_ms / config::block_interval_ms, 1000, {99, 100}, {1000, 999}}
      );

      */
   } FC_CAPTURE_AND_RETHROW() }


   void create_block_summary() {
      auto p = pending->_pending_block_state;
      auto sid = p->block_num & 0xffff;
      db.modify( db.get<block_summary_object,by_id>(sid), [&](block_summary_object& bso ) {
          bso.block_id = p->id;
      });
   }

   /**
    *  This method only works for blocks within the TAPOS range, (last 65K blocks). It
    *  will return block_id_type() for older blocks.
    */
   block_id_type get_block_id_for_num( uint32_t block_num ) {
      auto sid = block_num & 0xffff;
      auto id  = db.get<block_summary_object,by_id>(sid).block_id;
      auto num = block_header::num_from_id( id );
      if( num == block_num ) return id;
      return block_id_type();
   }

   void clear_expired_transactions() {
      //Look for expired transactions in the deduplication list, and remove them.
      auto& transaction_idx = db.get_mutable_index<transaction_multi_index>();
      const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
      while( (!dedupe_index.empty()) && (head_block_time() > fc::time_point(dedupe_index.begin()->expiration) ) ) {
         transaction_idx.remove(*dedupe_index.begin());
      }

      // Look for expired transactions in the pending generated list, and remove them.
      // TODO: expire these by sending error to handler
      auto& generated_transaction_idx = db.get_mutable_index<generated_transaction_multi_index>();
      const auto& generated_index = generated_transaction_idx.indices().get<by_expiration>();
      while( (!generated_index.empty()) && (head_block_time() > generated_index.begin()->expiration) ) {
      // TODO:   destroy_generated_transaction(*generated_index.begin());
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
      on_block_act.data = fc::raw::pack(head_block_header());

      signed_transaction trx;
      trx.actions.emplace_back(std::move(on_block_act));
      trx.set_reference_block(head_block_id());
      trx.expiration = head_block_time() + fc::seconds(1);
      return trx;
   }


   /**
    *  apply_block 
    *
    *  This method starts an undo session, whose revision number should match 
    *  the block number. 
    *
    *  It first does some parallel read-only sanity checks on all input transactions.
    *
    *  It then follows the transaction delivery schedule defined by the block_summary. 
    *
   void apply_block( block_state_ptr bstate ) {
      try {
         start_block();
         for( const auto& receipt : bstate.block->transactions ) {
            receipt.trx.visit( [&]( const auto& t ){ 
               apply_transaction( t );
            });
         }
         finalize_block();
         validate_signature();
         commit_block();
      } catch ( const fc::exception& e ) {
         abort_block();
         edump((e.to_detail_string()));
         throw;
      }
   }

   void start_block( block_timestamp_type time ) {
      FC_ASSERT( !pending );
      pending = _db.start_undo_session();
      /// TODO: FC_ASSERT( _db.revision() == blocknum );

      auto trx = get_on_block_transaction();
      apply_transaction( trx );

   }

   vector<transaction_metadata> abort_block() {
      FC_ASSERT( pending.valid() );
      self.abort_apply_block();
      auto tmp = move( pending->_transaction_metas );
      pending.reset();
      return tmp;
   }

   void commit_block() {
      FC_ASSERT( pending.valid() );

      self.applied_block( pending->_pending_block_state );
      pending->push();
      pending.reset();
   }
    */

};

const resource_limits_manager&   controller::get_resource_limits_manager()const 
{
   return my->resource_limits;
}
resource_limits_manager&         controller::get_mutable_resource_limits_manager() 
{
   return my->resource_limits;
}


controller::controller( const controller::config& cfg )
:my( new controller_impl( cfg, *this ) )
{
   my->init();
}

controller::~controller() {
}


void controller::startup() {
   my->head = my->fork_db.head();
   if( !my->head ) {
   }

   /*
   auto head = my->blog.read_head();
   if( head && head_block_num() < head->block_num() ) {
      wlog( "\nDatabase in inconsistant state, replaying block log..." );
      //replay();
   }
   */
}

chainbase::database& controller::db()const { return my->db; }


void controller::start_block( block_timestamp_type when ) {
  wlog( "start_block" );
  FC_ASSERT( !my->pending );

  FC_ASSERT( my->db.revision() == my->head->block_num );

  my->pending = my->db.start_undo_session(true);
  my->pending->_pending_block_state = std::make_shared<block_state>( *my->head, when );

  auto onbtrx = std::make_shared<transaction_metadata>( my->get_on_block_transaction() );
  my->apply_transaction( onbtrx, true );
}

void controller::finalize_block() {
   my->finalize_block();
}

void controller::sign_block( std::function<signature_type( const digest_type& )> signer_callback ) {
   auto p = my->pending->_pending_block_state;
   p->header.sign( signer_callback, p->pending_schedule_hash );
   FC_ASSERT( p->block_signing_key == p->header.signee( p->pending_schedule_hash ),
              "block was not signed by expected producer key" );
   static_cast<signed_block_header&>(*p->block) = p->header;
}

void controller::commit_block() {
   my->commit_block(true);
}

block_state_ptr controller::head_block_state()const {
   return my->head;
}


void controller::apply_block( const signed_block_ptr& b ) {
   try {
      start_block( b->timestamp );

      for( const auto& receipt : b->transactions ) {
         if( receipt.trx.contains<packed_transaction>() ) {
            auto& pt = receipt.trx.get<packed_transaction>();
            auto mtrx = std::make_shared<transaction_metadata>(pt);
            push_transaction( mtrx );
         }
      }

      finalize_block();
      sign_block( [&]( const auto& ){ return b->producer_signature; } );

      FC_ASSERT( b->id() == my->pending->_pending_block_state->block->id(),
                 "applying block didn't produce expected block id" );

      my->commit_block(false);
      return;
   } catch ( const fc::exception& e ) {
      edump((e.to_detail_string()));
      abort_block();
      throw;
   }
}

vector<transaction_metadata_ptr> controller::abort_block() {
   vector<transaction_metadata_ptr> pushed = move(my->pending->_applied_transaction_metas);
   my->pending.reset();
   return pushed;
}

void controller::push_block( const signed_block_ptr& b ) {
   FC_ASSERT( !my->pending );

   auto new_head = my->fork_db.add( b );

   edump((new_head->block_num));
   if( new_head->header.previous == my->head->id ) {
      try {
         apply_block( b );
         my->fork_db.set_validity( new_head, true );
         my->head = new_head;
      } catch ( const fc::exception& e ) {
         my->fork_db.set_validity( new_head, false );
         throw;
      }
   } else {
      /// potential forking logic
      elog( "new head??? not building on current head?" );
   }
}

void controller::push_transaction( const transaction_metadata_ptr& trx ) { 
   my->push_transaction(trx);
}

void controller::push_transaction() {
}
void controller::push_transaction( const transaction_id_type& trxid ) {
   /// lookup scheduled trx and then apply it...
}

uint32_t controller::head_block_num()const {
   return my->head->block_num;
}
block_id_type controller::head_block_id()const {
   return my->head->id;
}

time_point controller::head_block_time()const {
   return my->head->header.timestamp;
}

uint64_t controller::next_global_sequence() {
   const auto& p = get_dynamic_global_properties();
   my->db.modify( p, [&]( auto& dgp ) {
      ++dgp.global_action_sequence;
   });
   return p.global_action_sequence;
}

uint64_t controller::next_recv_sequence( account_name receiver ) {
   return 0;
}
uint64_t controller::next_auth_sequence( account_name actor ) {
   return 0;
}
void     controller::record_transaction( const transaction_metadata_ptr& trx ) {
   my->record_transaction( trx );
}

const dynamic_global_property_object& controller::get_dynamic_global_properties()const {
  return my->db.get<dynamic_global_property_object>();
}
const global_property_object& controller::get_global_properties()const {
  return my->db.get<global_property_object>();
}

/**
 *  This method reads the current dpos_irreverible block number, if it is higher
 *  than the last block number of the log, it grabs the next block from the
 *  fork database, saves it to disk, then removes the block from the fork database. 
 *
 *  Any forks built off of a different block with the same number are also pruned.
 */
void controller::log_irreversible_blocks() {
   if( !my->blog.head() ) 
      my->blog.read_head();
 
   const auto& log_head = my->blog.head();
   auto lib = my->head->dpos_last_irreversible_blocknum;

   if( lib > 1 ) {
      while( log_head && log_head->block_num() < lib ) {
         auto lhead = log_head->block_num();
         auto blk_id = my->get_block_id_for_num( lhead + 1 );
         auto blk = my->fork_db.get_block( blk_id );
         FC_ASSERT( blk, "unable to find block state", ("id",blk_id));
         my->blog.append( *blk->block );
         my->fork_db.prune( blk );
         my->db.commit( lhead );
      }
   }
}

void controller::pop_block() {
   auto prev = my->fork_db.get_block( my->head->header.previous );
   FC_ASSERT( prev, "attempt to pop beyond last irreversible block" );
   my->db.undo();
   my->head = prev;
}


abi_def eos_contract_abi(const abi_def& eosio_system_abi)
{
   abi_def eos_abi(eosio_system_abi);
   eos_abi.types.push_back( type_def{"account_name","name"} );
   eos_abi.types.push_back( type_def{"table_name","name"} );
   eos_abi.types.push_back( type_def{"share_type","int64"} );
   eos_abi.types.push_back( type_def{"onerror","bytes"} );
   eos_abi.types.push_back( type_def{"context_free_type","bytes"} );
   eos_abi.types.push_back( type_def{"weight_type","uint16"} );
   eos_abi.types.push_back( type_def{"fields","field[]"} );
   eos_abi.types.push_back( type_def{"time_point_sec","time"} );

   // TODO add ricardian contracts
   eos_abi.actions.push_back( action_def{name("setcode"), "setcode",""} );
   eos_abi.actions.push_back( action_def{name("setabi"), "setabi",""} );
   eos_abi.actions.push_back( action_def{name("linkauth"), "linkauth",""} );
   eos_abi.actions.push_back( action_def{name("unlinkauth"), "unlinkauth",""} );
   eos_abi.actions.push_back( action_def{name("updateauth"), "updateauth",""} );
   eos_abi.actions.push_back( action_def{name("deleteauth"), "deleteauth",""} );
   eos_abi.actions.push_back( action_def{name("newaccount"), "newaccount",""} );
   eos_abi.actions.push_back( action_def{name("postrecovery"), "postrecovery",""} );
   eos_abi.actions.push_back( action_def{name("passrecovery"), "passrecovery",""} );
   eos_abi.actions.push_back( action_def{name("vetorecovery"), "vetorecovery",""} );
   eos_abi.actions.push_back( action_def{name("onerror"), "onerror",""} );
   eos_abi.actions.push_back( action_def{name("onblock"), "onblock",""} );
   eos_abi.actions.push_back( action_def{name("canceldelay"), "canceldelay",""} );
   
   // TODO add any ricardian_clauses 
   //
   // ACTION PAYLOADS


   eos_abi.structs.emplace_back( struct_def {
      "setcode", "", {
         {"account", "account_name"},
         {"vmtype", "uint8"},
         {"vmversion", "uint8"},
         {"code", "bytes"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "setabi", "", {
         {"account", "account_name"},
         {"abi", "abi_def"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "updateauth", "", {
         {"account", "account_name"},
         {"permission", "permission_name"},
         {"parent", "permission_name"},
         {"data", "authority"},
         {"delay", "uint32"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "linkauth", "", {
         {"account", "account_name"},
         {"code", "account_name"},
         {"type", "action_name"},
         {"requirement", "permission_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "unlinkauth", "", {
         {"account", "account_name"},
         {"code", "account_name"},
         {"type", "action_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "deleteauth", "", {
         {"account", "account_name"},
         {"permission", "permission_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "newaccount", "", {
         {"creator", "account_name"},
         {"name", "account_name"},
         {"owner", "authority"},
         {"active", "authority"},
         {"recovery", "authority"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "postrecovery", "", {
         {"account", "account_name"},
         {"data", "authority"},
         {"memo", "string"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "passrecovery", "", {
         {"account", "account_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "vetorecovery", "", {
         {"account", "account_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "canceldelay", "", {
         {"trx_id", "transaction_id_type"},
      }
   });

   // DATABASE RECORDS

   eos_abi.structs.emplace_back( struct_def {
      "pending_recovery", "", {
         {"account",    "name"},
         {"request_id", "uint128"},
         {"update",     "updateauth"},
         {"memo",       "string"}
      }
   });

   eos_abi.tables.emplace_back( table_def {
      "recovery", "i64", {
         "account",
      }, {
         "name"
      },
      "pending_recovery"
   });

   // abi_def fields

   eos_abi.structs.emplace_back( struct_def {
      "field", "", {
         {"name", "field_name"},
         {"type", "type_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "struct_def", "", {
         {"name", "type_name"},
         {"base", "type_name"},
         {"fields", "fields"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "permission_level", "", {
         {"actor", "account_name"},
         {"permission", "permission_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "action", "", {
         {"account", "account_name"},
         {"name", "action_name"},
         {"authorization", "permission_level[]"},
         {"data", "bytes"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "permission_level_weight", "", {
         {"permission", "permission_level"},
         {"weight", "weight_type"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "transaction_header", "", {
         {"expiration", "time_point_sec"},
         {"region", "uint16"},
         {"ref_block_num", "uint16"},
         {"ref_block_prefix", "uint32"},
         {"max_net_usage_words", "varuint32"},
         {"max_kcpu_usage", "varuint32"},
         {"delay_sec", "varuint32"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "transaction", "transaction_header", {
         {"context_free_actions", "action[]"},
         {"actions", "action[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "signed_transaction", "transaction", {
         {"signatures", "signature[]"},
         {"context_free_data", "bytes[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "key_weight", "", {
         {"key", "public_key"},
         {"weight", "weight_type"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "authority", "", {
         {"threshold", "uint32"},
         {"keys", "key_weight[]"},
         {"accounts", "permission_level_weight[]"}
      }
   });
   eos_abi.structs.emplace_back( struct_def {
         "clause_pair", "", {
            {"id", "string"},
            {"body", "string"} 
         }
   });
   eos_abi.structs.emplace_back( struct_def {
      "type_def", "", {
         {"new_type_name", "type_name"},
         {"type", "type_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "action_def", "", {
         {"name", "action_name"},
         {"type", "type_name"},
         {"ricardian_contract", "string"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "table_def", "", {
         {"name", "table_name"},
         {"index_type", "type_name"},
         {"key_names", "field_name[]"},
         {"key_types", "type_name[]"},
         {"type", "type_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "abi_def", "", {
         {"types", "type_def[]"},
         {"structs", "struct_def[]"},
         {"actions", "action_def[]"},
         {"tables", "table_def[]"},
         {"ricardian_clauses", "clause_pair[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "block_header", "", {
         {"previous", "checksum256"},
         {"timestamp", "uint32"},
         {"transaction_mroot", "checksum256"},
         {"action_mroot", "checksum256"},
         {"block_mroot", "checksum256"},
         {"producer", "account_name"},
         {"schedule_version", "uint32"},
         {"new_producers", "producer_schedule?"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
         "onblock", "", {
            {"header", "block_header"}
      }
   });

   return eos_abi;
}
/**
 * @param actions - the actions to check authorization across
 * @param provided_keys - the set of public keys which have authorized the transaction
 * @param allow_unused_signatures - true if method should not assert on unused signatures
 * @param provided_accounts - the set of accounts which have authorized the transaction (presumed to be owner)
 *
 * @return fc::microseconds set to the max delay that this authorization requires to complete
 */
fc::microseconds controller::check_authorization( const vector<action>& actions,
                                      const flat_set<public_key_type>& provided_keys,
                                      bool                             allow_unused_signatures,
                                      flat_set<account_name>           provided_accounts,
                                      flat_set<permission_level>       provided_levels
                                    )const {
   return fc::microseconds();
}

/**
 * @param account - the account owner of the permission
 * @param permission - the permission name to check for authorization
 * @param provided_keys - a set of public keys
 *
 * @return true if the provided keys are sufficient to authorize the account permission
 */
bool controller::check_authorization( account_name account, permission_name permission,
                       flat_set<public_key_type> provided_keys,
                       bool allow_unused_signatures)const {
   return true;
}

void controller::set_active_producers( const producer_schedule_type& sch ) {
   FC_ASSERT( !my->pending->_pending_block_state->header.new_producers, "this block has already set new producers" );
   FC_ASSERT( !my->pending->_pending_block_state->pending_schedule.producers.size(), "there is already a pending schedule, wait for it to become active" );
   my->pending->_pending_block_state->header.new_producers = sch;
}
const producer_schedule_type& controller::active_producers()const {
   return my->pending->_pending_block_state->active_schedule;
}

const producer_schedule_type& controller::pending_producers()const {
   return my->pending->_pending_block_state->pending_schedule;
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

   
} } /// eosio::chain
