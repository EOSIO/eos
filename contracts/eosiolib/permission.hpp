/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/permission.h>
#include <eosiolib/transaction.hpp>
#include <eosiolib/optional.hpp>

#include <set>
#include <limits>

namespace eosio {

   /**
    *  @brief Checks if a transaction is authorized by a provided set of keys and permissions
    *
    *  @param trx - the transaction for which to check authorizations
    *  @param provided_permissions - the set of permissions which have authorized the transaction (empty permission name acts as wildcard)
    *  @param provided_keys - the set of public keys which have authorized the transaction
    *
    *  @return an optional of the minimum delay (in microseconds) required to satisfy the authorization (returns empty optional if transaction was not authorized by provided keys and permissions)
    */
   optional<uint64_t>
   check_transaction_authorization( const transaction&                 trx,
                                    const std::set<permission_level>&  provided_permissions ,
                                    const std::set<public_key>&        provided_keys = std::set<public_key>()
                                  )
   {
      bytes packed_trx = pack(trx);

      bytes packed_keys;
      auto nkeys = provided_keys.size();
      if( nkeys > 0 ) {
         packed_keys = pack(provided_keys);
      }

      bytes packed_perms;
      auto nperms = provided_permissions.size();
      if( nperms > 0 ) {
         packed_perms = pack(provided_permissions);
      }

      auto res = ::check_transaction_authorization( packed_trx.data(),
                                                    packed_trx.size(),
                                                    (nkeys > 0)  ? packed_keys.data()  : (const char*)0,
                                                    (nkeys > 0)  ? packed_keys.size()  : 0,
                                                    (nperms > 0) ? packed_perms.data() : (const char*)0,
                                                    (nperms > 0) ? packed_perms.size() : 0
                                                  );

      if( res >= 0 )
         return static_cast<uint64_t>(res);
      else
         return optional<uint64_t>();
   }

   /**
    *  @brief Checks if a permission is authorized by a provided delay and a provided set of keys and permissions
    *
    *  @param account    - the account owner of the permission
    *  @param permission - the permission name to check for authorization
    *  @param provided_keys - the set of public keys which have authorized the transaction
    *  @param provided_permissions - the set of permissions which have authorized the transaction (empty permission name acts as wildcard)
    *  @param provided_delay_us - the provided delay in microseconds (cannot exceed INT64_MAX)
    *
    *  @return an optional of the minimum delay (in microseconds) required to satisfy the authorization (returns empty optional if permission was not authorized by provided delay, keys, and permissions)
    */
   optional<uint64_t>
   check_permission_authorization( account_name                       account,
                                   permission_name                    permission,
                                   const std::set<public_key>&        provided_keys,
                                   const std::set<permission_level>&  provided_permissions = std::set<permission_level>(),
                                   uint64_t                           provided_delay_us = static_cast<uint64_t>(std::numeric_limits<int64_t>::max())
                                 )
   {
      bytes packed_keys;
      auto nkeys = provided_keys.size();
      if( nkeys > 0 ) {
         packed_keys = pack(provided_keys);
      }

      bytes packed_perms;
      auto nperms = provided_permissions.size();
      if( nperms > 0 ) {
         packed_perms = pack(provided_permissions);
      }

      auto res = ::check_permission_authorization( account,
                                                   permission,
                                                   (nkeys > 0)  ? packed_keys.data()  : (const char*)0,
                                                   (nkeys > 0)  ? packed_keys.size()  : 0,
                                                   (nperms > 0) ? packed_perms.data() : (const char*)0,
                                                   (nperms > 0) ? packed_perms.size() : 0,
                                                   provided_delay_us
                                                 );

      if( res >= 0 )
         return static_cast<uint64_t>(res);
      else
         return optional<uint64_t>();
   }

}
