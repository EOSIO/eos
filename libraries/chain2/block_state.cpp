#include <eosio/chain/block_state.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain {


   /**
    *  Perform context free validation of transaction state
    */
   void validate_transaction( const transaction& trx ) {
      EOS_ASSERT( !trx.actions.empty(), tx_no_action, "transaction must have at least one action" );

      // Check for at least one authorization in the context-aware actions
      bool has_auth = false;
      for( const auto& act : trx.actions ) {
         has_auth |= !act.authorization.empty();
         if( has_auth ) break;
      }
      EOS_ASSERT( has_auth, tx_no_auths, "transaction must have at least one authorization" );

      // Check that there are no authorizations in any of the context-free actions
      for (const auto &act : trx.context_free_actions) {
         EOS_ASSERT( act.authorization.empty(), cfa_irrelevant_auth, 
                     "context-free actions cannot require authorization" );
      }

      EOS_ASSERT( trx.max_kcpu_usage.value < UINT32_MAX / 1024UL, transaction_exception, "declared max_kcpu_usage overflows when expanded to max cpu usage" );
      EOS_ASSERT( trx.max_net_usage_words.value < UINT32_MAX / 8UL, transaction_exception, "declared max_net_usage_words overflows when expanded to max net usage" );
   } /// validate_transaction


   void validate_shard_locks_unique(const vector<shard_lock>& locks, const string& tag) {
      if (locks.size() < 2) {
         return;
      }

      for (auto cur = locks.begin() + 1; cur != locks.end(); ++cur) {
         auto prev = cur - 1;
         EOS_ASSERT(*prev != *cur, block_lock_exception, "${tag} lock \"${a}::${s}\" is not unique", ("tag",tag)("a",cur->account)("s",cur->scope));
         EOS_ASSERT(*prev < *cur,  block_lock_exception, "${tag} locks are not sorted", ("tag",tag));
      }
   }


   void validate_shard_locks( const shard_summary& shard, uint32_t shard_index ) {
      validate_shard_locks_unique( shard.read_locks, "read" );
      validate_shard_locks_unique( shard.write_locks, "write" );

      // validate that no read_scope is used as a write scope in this cycle and that no two shards
      // share write scopes
      set<shard_lock> read_locks;
      map<shard_lock, uint32_t> write_locks;

      for (const auto& s: shard.read_locks) {
         EOS_ASSERT(write_locks.count(s) == 0, block_concurrency_exception,
            "shard ${i} requires read lock \"${a}::${s}\" which is locked for write by shard ${j}",
            ("i", shard_index)("s", s)("j", write_locks[s]));
         read_locks.emplace(s);
      }

      for (const auto& s: shard.write_locks) {
         EOS_ASSERT(write_locks.count(s) == 0, block_concurrency_exception,
            "shard ${i} requires write lock \"${a}::${s}\" which is locked for write by shard ${j}",
            ("i", shard_index)("a", s.account)("s", s.scope)("j", write_locks[s]));
         EOS_ASSERT(read_locks.count(s) == 0, block_concurrency_exception,
            "shard ${i} requires write lock \"${a}::${s}\" which is locked for read",
            ("i", shard_index)("a", s.account)("s", s.scope));
         write_locks[s] = shard_index;
      }
   }


   block_state::block_state( block_header_state h, signed_block_ptr b )
   :block_header_state( move(h) ), block(move(b))
   {
      if( block ) {
         for( const auto& packed : b->input_transactions ) {
            auto meta_ptr = std::make_shared<transaction_metadata>( packed, chain_id_type(), block->timestamp );

            /** perform context-free validation of transactions */
            const auto& trx = meta_ptr->trx();
            FC_ASSERT( time_point(trx.expiration) > header.timestamp, "transaction is expired" );
            validate_transaction( trx );

            auto id = meta_ptr->id;
            input_transactions[id] = move(meta_ptr);
         }

         FC_ASSERT( block->regions.size() >= 1, "must be at least one region" );
         /// regions must be listed in order
         for( uint32_t i = 1; i < block->regions.size(); ++i )
            FC_ASSERT( block->regions[i-1].region < block->regions[i].region );

         reset_trace();

      } // end if block
   } 

   void block_state::reset_trace() {
      validated      = false;
      bool found_trx = false;
      trace = std::make_shared<block_trace>( block );

      auto num_regions = block->regions.size();
      trace->region_traces.resize( num_regions );
      /// reserve region_trace
      for( uint32_t r = 0; r < num_regions; ++r ) {
         auto num_cycles = block->regions[r].cycles_summary.size();
         trace->region_traces[r].cycle_traces.resize( num_cycles );
         // FC_ASSERT( block->regions[r].cycles_summary.size() >= 1, "must be at least one cycle" );
         /// reserve cycle traces
         for( uint32_t c = 0; c < num_cycles; c++ ) {
            auto num_shards = block->regions[r].cycles_summary[c].size();
            trace->region_traces[r].cycle_traces[c].shard_traces.resize( num_shards );
            // FC_ASSERT( block->regions[r].cycles_summary.size() >= 1, "must be at least one shard" );
            /// reserve shard traces
            for( uint32_t s = 0; s < num_shards; ++s ) {
               validate_shard_locks( block->regions[r].cycles_summary[c][s], s );

               auto num_trx = block->regions[r].cycles_summary[c][s].transactions.size();
               trace->region_traces[r].cycle_traces[c].shard_traces[s].transaction_traces.resize(num_trx);
               for( uint32_t t = 0; t < num_trx; ++t ) {
                  found_trx = true;
               }
            }
         }
      }
      FC_ASSERT( found_trx, "a block must contain at least one transaction (the implicit on block trx)" );
   }



} } /// eosio::chain
