/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <chainbase/chainbase.hpp>
#include <eosio/chain/transaction.hpp>

#include <type_traits>

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

   size_t get_billable_size() const {
      /**
       *  public_key_type contains a static_variant and so its size could change if we later wanted to add a new public key type of
       *  of larger size, thus increasing the returned value from shared_authority::get_billable_size() for old authorities that
       *  do not even use the new public key type.
       *
       * Although adding a new public key type is a hardforking change anyway, the current implementation means we would need to:
       *  - track historical sizes of public_key_type,
       *  - branch on hardfork versions within this function, and
       *  - calculate billable size of the authority based on the appropriate historical size of public_key_type,
       * all in order to avoid retroactively changing the billable size of authorities.
       * TODO: Better implementation of get_billable_size()?
       *       Perhaps it would require changes to how public_key_type is stored in shared_authority?
       *       For example: change keys (of type shared_vector<key_weight>) to packed_keys (of type shared_vector<char>)
       *                    which store the packed data of vector<key_weight>, and then charge based on that packed size.
       */
      return keys.size() * sizeof(key_weight) + accounts.size() * sizeof(permission_level_weight);
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

   static_assert( std::is_same<decltype(auth.threshold), uint32_t>::value &&
                  std::is_same<weight_type, uint16_t>::value &&
                  std::is_same<typename decltype(auth.keys)::value_type, key_weight>::value &&
                  std::is_same<typename decltype(auth.accounts)::value_type, permission_level_weight>::value,
                  "unexpected type for threshold and/or weight in authority" );

   if( ( auth.keys.size() + auth.accounts.size() ) > (1 << 16) )
      return false; // overflow protection (assumes weight_type is uint16_t and threshold is of type uint32_t)

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
FC_REFLECT(eosio::chain::authority, (threshold)(keys)(accounts))
FC_REFLECT(eosio::chain::shared_authority, (threshold)(keys)(accounts))
