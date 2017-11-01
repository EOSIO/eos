#include <eosio/blockchain/transaction_scheduler.hpp>

namespace eosio { namespace blockchain {

   transaction_scheduler::transaction_scheduler( cycle_handle& c, scheduler& s )
   :_cycle_handle(c),_scheduler(s)
   {
      
   }

   transaction_scheduler::~transaction_scheduler() {
      
   }

   bool transaction_scheduler::schedule( meta_transaction_ptr mtrx ) {
      const auto& write_scopes = mtrx->trx->write_scope;
      const auto& read_scopes = mtrx->trx->read_scope;

      /**
       * If there is all ready an existing shard that uses referenced scopes, 
       * then the transaction must be scheduled in that shard, if there are
       * two or more existing shards then there is a conflict and the transaction
       * cannot be scheduled.
       */
      shard* exiting_shard = nullptr;

      /**
       * For each read scope used by the transaction, check to see if there is
       * an existing shard that has already claimed the write scope. If so then
       * this transaction must be scheduled in that shard.
       */
      for( auto rs : read_scopes ) {
         auto itr = _write_scope_to_shard.find( rs );
         if( itr != _write_scope_to_shard.end() )
         {
            if( exiting_shard && exiting_shard != itr->second ) 
               return false;
            exiting_shard = itr->second;
         }
      }

      /**
       * Now check the write scopes to see if there is conflict
       */
      for( auto ws : write_scopes ) {
         auto itr = _write_scope_to_shard.find( ws );
         if( itr != _write_scope_to_shard.end() )
         {
            if( exiting_shard && exiting_shard != itr->second ) 
               return false;
            exiting_shard = itr->second;
         }
      }

      /**
       * If there is an existing shard then we may need to grow its scope.
       */
      if( exiting_shard ) {
         exiting_shard->transactions.push_back( mtrx );

         _cycle_handle.extend_shard( exiting_shard->db_shard, 
                                     set<scope_name>( write_scopes.begin(), write_scopes.end() ),
                                     set<scope_name>( read_scopes.begin(), read_scopes.end() )  );

         for( const auto& s : write_scopes )
            _write_scope_to_shard.emplace( s, exiting_shard );

         for( const auto& s : read_scopes )
            _read_scopes.emplace( s );

         _scheduler.post( [mtrx](){
            /// TODO: execute transaction via controller
         }, exiting_shard->strand );

      } else { /// there is no existing shard, start a new one
         auto dbs       = _cycle_handle.start_shard( set<scope_name>( write_scopes.begin(), write_scopes.end() ),
                                                     set<scope_name>( read_scopes.begin(), read_scopes.end() ) );
         auto newstrand = _scheduler.create_strand();
         _shards.emplace_front( dbs, newstrand );
         auto& news = _shards.front();

         for( auto s : write_scopes ) {
            _write_scope_to_shard[s] = &news;
         }
         for( auto s : read_scopes ) {
            _read_scopes.emplace( s );
         }

         _scheduler.post( [=](){
            /// TODO: execute transaction via controller
         }, newstrand );
      }

      return false;
   }


} } /// eosio::blockchain
