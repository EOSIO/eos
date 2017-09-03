#pragma once
#include <eos/chain/types.hpp>
#include <eos/types/generated.hpp>

namespace eos { namespace chain {
struct shared_authority {
   shared_authority( chainbase::allocator<char> alloc )
      :accounts(alloc),keys(alloc)
   {}

   shared_authority& operator=(const Authority& a) {
      threshold = a.threshold;
      accounts = decltype(accounts)(a.accounts.begin(), a.accounts.end(), accounts.get_allocator());
      keys = decltype(keys)(a.keys.begin(), a.keys.end(), keys.get_allocator());
      return *this;
   }
   shared_authority& operator=(Authority&& a) {
      threshold = a.threshold;
      accounts.reserve(a.accounts.size());
      for (auto& p : a.accounts)
         accounts.emplace_back(std::move(p));
      keys.reserve(a.keys.size());
      for (auto& p : a.keys)
         keys.emplace_back(std::move(p));
      return *this;
   }

   UInt32                                        threshold = 0;
   shared_vector<types::AccountPermissionWeight> accounts;
   shared_vector<types::KeyPermissionWeight>     keys;

   Authority to_authority()const {
      Authority auth;
      auth.threshold = threshold;
      auth.keys.reserve(keys.size());
      auth.accounts.reserve(accounts.size());
      for( const auto& k : keys ) { auth.keys.emplace_back( k ); }
      for( const auto& a : accounts ) { auth.accounts.emplace_back( a ); }
      return auth;
   }
};

inline bool operator< (const types::AccountPermission& a, const types::AccountPermission& b) {
   return std::tie(a.account, a.permission) < std::tie(b.account, b.permission);
}

/**
 * Makes sure all keys are unique and sorted and all account permissions are unique and sorted and that authority can
 * be satisfied
 */
inline bool validate(const types::Authority& auth) {
   const types::KeyPermissionWeight* prev = nullptr;
   decltype(auth.threshold) totalWeight = 0;

   for( const auto& k : auth.keys ) {
      if( !prev ) prev = &k;
      else if( prev->key < k.key ) return false;
      totalWeight += k.weight;
   }
   const types::AccountPermissionWeight* pa = nullptr;
   for( const auto& a : auth.accounts ) {
      if( !pa ) pa = &a;
      else if( pa->permission < a.permission ) return false;
      totalWeight += a.weight;
   }
   return totalWeight >= auth.threshold;
}

} } // namespace eos::chain

FC_REFLECT(eos::chain::shared_authority, (threshold)(accounts)(keys))
