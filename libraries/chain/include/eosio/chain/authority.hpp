/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <chainbase/chainbase.hpp>
#include <eosio/chain/transaction.hpp>

namespace eosio { namespace chain {


struct permission_level_weight {
   permission_level  permission;
   weight_type       weight;
};

struct key_weight {
   public_key_type key;
   weight_type     weight;
};

struct authority {
  authority( public_key_type k ):threshold(1),keys({{k,1}}){}
  authority( uint32_t t, vector<key_weight> k, vector<permission_level_weight> p = {} )
  :threshold(t),keys(move(k)),accounts(move(p)){}
  authority(){}


  uint32_t                          threshold = 0;
  vector<key_weight>                keys;
  vector<permission_level_weight>   accounts;
};


struct shared_authority {
   shared_authority( chainbase::allocator<char> alloc )
   :keys(alloc),accounts(alloc){}

   shared_authority& operator=(const authority& a) {
      threshold = a.threshold;
      keys = decltype(keys)(a.keys.begin(), a.keys.end(), keys.get_allocator());
      accounts = decltype(accounts)(a.accounts.begin(), a.accounts.end(), accounts.get_allocator());
      return *this;
   }

   uint32_t                                   threshold = 0;
   shared_vector<key_weight>                  keys;
   shared_vector<permission_level_weight>     accounts;

   operator authority()const { return to_authority(); }
   authority to_authority()const {
      authority auth;
      auth.threshold = threshold;
      auth.keys.reserve(keys.size());
      auth.accounts.reserve(accounts.size());
      for( const auto& k : keys ) { auth.keys.emplace_back( k ); }
      for( const auto& a : accounts ) { auth.accounts.emplace_back( a ); }
      return auth;
   }
};

inline bool operator< (const permission_level& a, const permission_level& b) {
   return std::tie(a.actor, a.permission) < std::tie(b.actor, b.permission);
}

/**
 * Makes sure all keys are unique and sorted and all account permissions are unique and sorted and that authority can
 * be satisfied
 */
template<typename Authority>
inline bool validate( const Authority& auth ) {
   const key_weight* prev = nullptr;
   decltype(auth.threshold) total_weight = 0;

   for( const auto& k : auth.keys ) {
      if( !prev ) prev = &k;
      else if( prev->key < k.key ) return false;
      total_weight += k.weight;
   }
   const permission_level_weight* pa = nullptr;
   for( const auto& a : auth.accounts ) {
      if( !pa ) pa = &a;
      else if( pa->permission < a.permission ) return false;
      total_weight += a.weight;
   }
   return total_weight >= auth.threshold;
}

} } // namespace eosio::chain


FC_REFLECT(eosio::chain::permission_level_weight, (permission)(weight) )
FC_REFLECT(eosio::chain::key_weight, (key)(weight) )
FC_REFLECT(eosio::chain::authority, (threshold)(accounts)(keys))
FC_REFLECT(eosio::chain::shared_authority, (threshold)(accounts)(keys))
