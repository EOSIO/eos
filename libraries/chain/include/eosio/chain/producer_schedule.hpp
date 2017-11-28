#pragma once
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

   /**
    *  Used as part of the producer_schedule_type, mapps the producer name to their key.
    */
   struct producer_key {
      account_name      producer_name;
      public_key_type   block_signing_key;

      friend bool operator == ( const producer_key& a, const producer_key& b ) { 
         return tie( a.producer_name, a.block_signing_key ) == tie( b.producer_name,b.block_signing_key );
      }
      friend bool operator != ( const producer_key& a, const producer_key& b ) { 
         return tie( a.producer_name, a.block_signing_key ) != tie( b.producer_name,b.block_signing_key );
      }
   };
   /**
    *  Defines both the order, account name, and signing keys of the active set of producers. 
    */
   struct producer_schedule_type {
      uint32_t                                       version = 0; ///< sequentially incrementing version number
      fc::array<producer_key,config::producer_count> producers;
   };


   inline bool operator == ( const producer_schedule_type& a, const producer_schedule_type& b ) 
   {
      if( a.version != b.version ) return false;
      for( uint32_t i = 0; i < a.producers.size(); ++i )
         if( a.producers[i] != b.producers[i] ) return false;
      return true;
   }
   inline bool operator != ( const producer_schedule_type& a, const producer_schedule_type& b )
   {
      return !(a==b);
   }


} } /// eosio::chain

FC_REFLECT( eosio::chain::producer_key, (producer_name)(block_signing_key) )
FC_REFLECT( eosio::chain::producer_schedule_type, (version)(producers) )
