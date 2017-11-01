#pragma once
#include <eosio/blockchain/database.hpp>
#include <eosio/blockchain/scheduler.hpp>
#include <eosio/blockchain/transaction.hpp>

namespace eosio { namespace blockchain {

   /**
    *  @class transaction_scheduler 
    *  @breif organizes transactions into shards and executes them
    *
    *  This class is responsible for scheduling transactions within a single
    *  cycle. Scheduled transactions are dispatched to the proper strand via
    *  the scheduler and the results can be gathered by get_results() so they
    *  can be organized into a cycle of the block_data and block_summary
    */
   class transaction_scheduler {
      public:
         transaction_scheduler( cycle_handle& c, scheduler& s );
         ~transaction_scheduler();

         /**
          * Attempts to schedule a transaction, returns true if success or false if
          * it was deferred. If it was scheduled it will be dispatched for execution
          * by the scheduler.
          */
         bool schedule( meta_transaction_ptr trx );

         /**
          * Returns an array of shards where a shard is a vector of
          * transactions. The result of executing the transaction (if any)
          * is stored in the meta_transaction_ptr
          */
         vector< vector<meta_transaction_ptr> > get_results();

      private:
         cycle_handle& _cycle_handle;
         scheduler&    _scheduler;

         struct shard {
            shard( shard_handle sh, scheduler::strand_handle strandh )
            :db_shard(sh),strand(strandh){}

            shard_handle                 db_shard;
            scheduler::strand_handle     strand;
            vector<meta_transaction_ptr> transactions;
         };

         /**
          *  New shards are allocated in the _shards list and then a 
          *  pointer to the shard is inserted into this map. The _shards
          *  list will delete the shard when this transaction scheduler
          *  is complete.
          */
         map<account_name, shard*> _write_scope_to_shard;
         set<account_name>         _read_scopes;

         list<shard>               _shards;
   };


} } /// eosio::blockchain
