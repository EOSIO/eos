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
};

/**
 * @brief This class determines whether a set of signing keys are sufficient to satisfy an authority or not
 *
 * To determine whether an authority is satisfied or not, we first determine which keys have approved of a message, and
 * then determine whether that list of keys is sufficient to satisfy the authority. This class takes a list of keys and
 * provides the @ref satisfied method to determine whether that list of keys satisfies a provided authority.
 *
 * @tparam F A callable which takes a single argument of type @ref AccountPermission and returns the corresponding
 * authority
 */
template<typename F>
class AuthorityChecker {
   F PermissionToAuthority;
   const flat_set<public_key_type>& signingKeys;

public:
   AuthorityChecker(F PermissionToAuthority, const flat_set<public_key_type>& signingKeys)
      : PermissionToAuthority(PermissionToAuthority), signingKeys(signingKeys) {}

   bool satisfied(const types::AccountPermission& permission) const {
      return satisfied(PermissionToAuthority(permission));
   }
   template<typename AuthorityType>
   bool satisfied(const AuthorityType& authority) const {
      UInt32 weight = 0;
      for (const auto& kpw : authority.keys)
         if (signingKeys.count(kpw.key)) {
            weight += kpw.weight;
            if (weight >= authority.threshold)
               return true;
         }
      for (const auto& apw : authority.accounts)
//#warning TODO: Recursion limit? Yes: implement as producer-configurable parameter 
         if (satisfied(apw.permission)) {
            weight += apw.weight;
            if (weight >= authority.threshold)
               return true;
         }
      return false;
   }
};

inline bool operator < ( const types::AccountPermission& a, const types::AccountPermission& b ) {
   return std::tie( a.account, a.permission ) < std::tie( b.account, b.permission );
}
template<typename F>
AuthorityChecker<F> MakeAuthorityChecker(F&& pta, const flat_set<public_key_type>& signingKeys) {
   return AuthorityChecker<F>(std::forward<F>(pta), signingKeys);
}

/**
 *  Makes sure all keys are unique and sorted and all account permissions are unique and sorted
 */
inline bool validate( types::Authority& auth ) {
   const types::KeyPermissionWeight* prev = nullptr;
   for( const auto& k : auth.keys ) {
      if( !prev ) prev = &k;
      else if( prev->key < k.key ) return false;
   }
   const types::AccountPermissionWeight* pa = nullptr;
   for( const auto& a : auth.accounts ) {
      if( !pa ) pa = &a;
      else if( pa->permission < a.permission ) return false;
   }
   return true;
}

} } // namespace eos::chain

FC_REFLECT(eos::chain::shared_authority, (threshold)(accounts)(keys))
