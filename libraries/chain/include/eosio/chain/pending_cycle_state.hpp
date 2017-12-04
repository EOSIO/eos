#pragma once 
#include <eosio/chain/transaction.hpp>

namespace eosio { namespace chain {

   struct pending_cycle_state : cycle_trace {
      set<scope_name>          read_scopes;
      map<scope_name,uint32_t> write_scope_to_shard;

      /**
       *  @return the shard number this transation goes in or (-1) if it
       *  cannot go on any shard due to conflict
       */
      uint32_t schedule( const transaction& trx ) {
         uint32_t current_shard = -1;

         for( const auto& ws : trx.write_scope ) {
            if( read_scopes.find(ws) != read_scopes.end() )
               return -1;
            
            auto itr = write_scope_to_shard.find(ws);
            if( itr != write_scope_to_shard.end() ) {
               if( current_shard == -1 ) {
                  current_shard = itr->second;
                  continue;
               }
               if( current_shard != itr->second )
                  return -1;  /// conflict detected
            }
         }

         for( const auto& rs : trx.read_scope )
         {
            auto itr = write_scope_to_shard.find(rs);
            if( itr != write_scope_to_shard.end() ) {
               if( current_shard == -1 ) {
                  current_shard = itr->second;
                  continue;
               }
               if( current_shard != itr->second )
                   return -1; /// schedule conflict
               current_shard = itr->second;
            }
         }

         if( current_shard == -1 ) {
            shards.resize( shards.size()+1 );
            current_shard = shards.size() - 1;
         }

         for( auto ws : trx.write_scope )
         {
            shards.back().write_scopes.insert( ws );
            write_scope_to_shard[ws] = current_shard;
         }

         for( auto rs : trx.read_scope )
            read_scopes.insert(rs);

         return current_shard;
      } /// schedule

      struct pending_shard {
         set<scope_name> write_scopes;
      };

      vector<pending_shard> shards;
   };


} } /// eosio::chain
