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
#include <eosio/chain/producer_object.hpp>
#include <eosio/chain/permission_link_object.hpp>

#include <eosio/chain/resource_limits.hpp>

#include <chainbase/chainbase.hpp>
#include <fc/io/json.hpp>

namespace eosio { namespace chain {

using resource_limits::resource_limits_manager;

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
   //wasm_interface                 wasmif;
   resource_limits_manager        resource_limits;
   controller::config             conf;
   controller&                    self;

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


   controller_impl( const controller::config& cfg, controller& s  )
   :db( cfg.shared_memory_dir, 
        cfg.read_only ? database::read_only : database::read_write,
        cfg.shared_memory_size ),
    blog( cfg.block_log_dir ),
    fork_db( cfg.shared_memory_dir ),
    //wasmif( cfg.wasm_runtime ),
    resource_limits( db ),
    conf( cfg ),
    self( s )
   {
      head = fork_db.head();
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

      db.add_index<contracts::table_id_multi_index>();
      db.add_index<contracts::key_value_index>();
      db.add_index<contracts::index64_index>();
      db.add_index<contracts::index128_index>();
      db.add_index<contracts::index256_index>();
      db.add_index<contracts::index_double_index>();

      db.add_index<global_property_multi_index>();
      db.add_index<dynamic_global_property_multi_index>();
      db.add_index<block_summary_multi_index>();
      db.add_index<transaction_multi_index>();
      db.add_index<generated_transaction_multi_index>();
      db.add_index<producer_multi_index>();
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
         db.set_revision( head->block_num );
         fork_db.set( head );
  
         // Initialize block summary index
         for (int i = 0; i < 0x10000; i++)
            db.create<block_summary_object>([&](block_summary_object&) {});

      }
      FC_ASSERT( db.revision() == head->block_num, "fork database is inconsistant with shared memory",
                 ("db",db.revision())("head",head->block_num) );
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

time_point controller::head_block_time()const {
   return my->head->header.timestamp;
}

uint64_t controller::next_global_sequence() {
   return 0;
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

   
} } /// eosio::chain
