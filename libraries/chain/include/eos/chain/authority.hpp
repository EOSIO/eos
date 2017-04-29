#pragma once
#include <eos/chain/types.hpp>
#include <eos/types/generated.hpp>

namespace eos {
   inline bool operator < ( const AccountPermission& a, const AccountPermission& b ) {
      return std::tie( a.account, a.permission ) < std::tie( b.account, b.permission );
   }

   /**
    *  Makes sure all keys are unique and sorted and all account permissions are unique and sorted
    */
   inline bool validate( eos::Authority& auth ) {
      const KeyPermissionWeight* prev = nullptr;
      for( const auto& k : auth.keys ) {
         if( !prev ) prev = &k;
         else if( prev->key < k.key ) return false;
      }
      const AccountPermissionWeight* pa = nullptr;
      for( const auto& a : auth.accounts ) {
         if( !pa ) pa = &a;
         else if( pa->permission < a.permission ) return false;
      }
      return true;
   }
}

