#include <eosio/chain/controller.hpp>
#include <chainbase/chainbase.hpp>

#include <eosio/chain/block_log.hpp>
#include <eosio/chain/fork_database.hpp>

#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contracts/contract_table_objects.hpp>
#include <eosio/chain/action_objects.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/producer_object.hpp>
#include <eosio/chain/permission_link_object.hpp>

#include <eosio/chain/resource_limits.hpp>

namespace eosio { namespace chain {

using resource_limits::resource_limits_manager;

struct pending_state {
   pending_state( database::session&& s )
   :_db_session( move(s) ){}

   block_timestamp_type           _pending_time;
   database::session              _db_session;
   vector<transaction_metadata>   _transaction_metas;
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
   }


   controller_impl( const controller::config& cfg, controller& s  )
   :db( cfg.shared_memory_dir, 
        cfg.read_only ? database::read_only : database::read_write,
        cfg.shared_memory_size ),
    blog( cfg.block_log_dir ),
    wasmif( cfg.wasm_runtime ),
    resource_limits( db ),
    conf( cfg ),
    self( s )
   {
      initialize_indicies();

      /**
       * The undable state contains state transitions from blocks
       * in the fork database that could be reversed. Because this
       * is a new startup and the fork database is empty, we must
       * unwind that pending state. This state will be regenerated
       * when we catch up to the head block later.
       */
      clear_all_undo(); 
      initialize_fork_db();
   }

   ~controller_impl() {
      pending.reset();
      db.flush();
   }

   void initialize_indicies() {
      /*
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
      */

      resource_limits.initialize_database();
   }

   void abort_pending_block() {
      pending.reset();
   }

   void clear_all_undo() {
      // Rewind the database to the last irreversible block
      db.with_write_lock([&] {
         db.undo_all();
         FC_ASSERT(db.revision() == self.head_block_num(), 
                   "Chainbase revision does not match head block num",
                   ("rev", db.revision())("head_block", self.head_block_num()));
      });
   }

   /**
    *  The fork database needs an initial block_state to be set before
    *  it can accept any new blocks. This initial block state can be found
    *  in the database (whose head block state should be irreversible) or
    *  it would be the genesis state.
    */
   void initialize_fork_db() {

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


   block_state_ptr push_block_impl( const signed_block_ptr& b ) {
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
   } /// push_block_impl

   bool should_enforce_runtime_limits()const {
      return false;
   }

   void validate_net_usage( const transaction& trx, uint32_t min_net_usage ) {
      uint32_t net_usage_limit = trx.max_net_usage_words.value * 8; // overflow checked in validate_transaction_without_state
      EOS_ASSERT( net_usage_limit == 0 || min_net_usage <= net_usage_limit,
                  transaction_exception,
                  "Packed transaction and associated data does not fit into the space committed to by the transaction's header! [usage=${usage},commitment=${commit}]",
                  ("usage", min_net_usage)("commit", net_usage_limit));
   } /// validate_net_usage

   void finalize_block( const block_trace& trace ) 
   { try {
      const auto& b = trace.block;

      /* TODO RESTORE 
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

   bool should_check_tapos()const { return true; }

   void validate_tapos( const transaction& trx )const {
      if( !should_check_tapos() ) return;

      const auto& tapos_block_summary = db.get<block_summary_object>((uint16_t)trx.ref_block_num);

      //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
      EOS_ASSERT(trx.verify_reference_block(tapos_block_summary.block_id), invalid_ref_block_exception,
                 "Transaction's reference block did not match. Is this transaction from a different fork?",
                 ("tapos_summary", tapos_block_summary));
   }


   /**
    *  At the start of each block we notify the system contract with a transaction that passes in
    *  the block header of the prior block (which is currently our head block)
    */
   transaction get_on_block_transaction()
   {
      action on_block_act;
      on_block_act.account = config::system_account_name;
      on_block_act.name = N(onblock);
      on_block_act.authorization = vector<permission_level>{{config::system_account_name, config::active_name}};
      on_block_act.data = fc::raw::pack(head_block_header());

      transaction trx;
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
    */
   void apply_block( block_state_ptr bstate ) {
       auto session = db.start_undo_session(true);

       optional<fc::time_point> processing_deadline;
       if( should_enforce_runtime_limits() ) {
          processing_deadline = fc::time_point::now() + conf.limits.max_push_block_us;
       }

       auto& next_block_trace = bstate->trace;
       next_block_trace->implicit_transactions.emplace_back( get_on_block_transaction() );

       /** these validation steps are independent of processing order, in theory they could be
        * performed in parallel */
       for( const auto& item : bstate->input_transactions ) {
          const auto& trx = item.second->trx();
          validate_tapos( trx );
          validate_net_usage( trx, item.second->billable_packed_size );
       }

       for( uint32_t region_index = 0; region_index < bstate->block->regions.size(); ++region_index ) {
         apply_region( region_index, *bstate );
       }

       FC_ASSERT( bstate->header.action_mroot == next_block_trace->calculate_action_merkle_root(), 
                  "action merkle root does not match" ); finalize_block( *next_block_trace );
       fork_db.set_validity( bstate, true );

       self.applied_block( next_block_trace ); /// emit signal to plugins before exiting undo state
       session.push();

       head = move(bstate);
   } /// apply_block


   void apply_region( uint32_t region_index, const block_state& bstate ) {
      const auto& r = bstate.block->regions[region_index];

      EOS_ASSERT(!r.cycles_summary.empty(), tx_empty_region,"region[${r_index}] has no cycles", ("r_index",region_index));
      for( uint32_t cycle_index = 0; cycle_index < r.cycles_summary.size(); cycle_index++ ) {
         apply_cycle( region_index, cycle_index, bstate );
      }
   }

   void apply_cycle( uint32_t region_index, uint32_t cycle_index,  const block_state& bstate ) {
      const auto& c = bstate.block->regions[region_index].cycles_summary[cycle_index];

      for( uint32_t shard_index = 0; shard_index < c.size(); ++shard_index ) {
         apply_shard( region_index, cycle_index, shard_index, bstate );
      }
      resource_limits.synchronize_account_ram_usage();
      //auto& c_trace = bstate.trace->region_traces[region_index].cycle_traces[cycle_index];
      //_apply_cycle_trace(c_trace);
   }

   void apply_shard( uint32_t region_index, 
                     uint32_t cycle_index, 
                     uint32_t shard_index, 
                      const block_state& bstate ) {

      shard_trace& s_trace = bstate.trace->region_traces[region_index].cycle_traces[cycle_index].shard_traces[shard_index];

      const auto& shard = bstate.block->regions[region_index].cycles_summary[cycle_index][shard_index];
      EOS_ASSERT(!shard.empty(), tx_empty_shard,"region[${r_index}] cycle[${c_index}] shard[${s_index}] is empty",
                 ("r_index",region_index)("c_index",cycle_index)("s_index",shard_index));

      flat_set<shard_lock> used_read_locks;
      flat_set<shard_lock> used_write_locks;

      for( const auto& receipt : shard.transactions ) {
         // apply_transaction( ... );
      }

      EOS_ASSERT( boost::equal( used_read_locks, shard.read_locks ),
         block_lock_exception, "Read locks for executing shard: ${s} do not match those listed in the block", 
                               ("s", shard_index));

      EOS_ASSERT( boost::equal( used_write_locks, shard.write_locks ),
         block_lock_exception, "Write locks for executing shard: ${s} do not match those listed in the block", 
                               ("s", shard_index));

      s_trace.finalize_shard();
   }


};



controller::controller( const controller::config& cfg )
:my( new controller_impl( cfg, *this ) )
{
}

controller::~controller() {
}


void controller::startup() {
   auto head = my->blog.read_head();
   if( head && head_block_num() < head->block_num() ) {
      wlog( "\nDatabase in inconsistant state, replaying block log..." );
      //replay();
   }
}

const chainbase::database& controller::db()const { return my->db; }


block_state_ptr controller::push_block( const signed_block_ptr& b ) {
  return my->push_block( b );  
}

transaction_trace           controller::push_transaction( const signed_transaction& t ) {

}

optional<transaction_trace> controller::push_deferred_transaction() {

}

   
} } /// eosio::chain
