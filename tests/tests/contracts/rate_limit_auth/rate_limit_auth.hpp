/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include <eoslib/token.hpp>
#include <eoslib/db.hpp>

/**
 *  @defgroup testcontract Test Contract
 *  @brief Test Contract
 *  @ingroup contractapi
 *
 */


namespace rate_limit_auth {

  /**
   *  @defgroup ratelimitcontract Test Contract for rate limiting
   *  @brief Test Contract for verifying rate limiting on all authorities
   *  @ingroup testcontract
   *
   *  @{
   */

   /**
    *  Mesage with 2 authorities
    */
   struct Auths2 {
      AccountName       acct1;
      AccountName       acct2;
   };

   /**
    *  Mesage with 3 authorities
    */
   struct Auths3 {
      AccountName       acct1;
      AccountName       acct2;
      AccountName       acct3;
   };

} /// @} /// ratelimitcontract
