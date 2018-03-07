/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/types.h>

extern "C" {
   /**
       *  Checks if a set of public keys can authorize an account permission
       *  @brief Checks if a set of public keys can authorize an account permission
       *  @param account - the account owner of the permission
       *  @param permission - the permission name to check for authorization
       *  @param pubkeys - a pointer to an array of public keys (public_key)
       *  @param pubkeys_len - the lenght of the array of public keys
       *  @return 1 if the account permission can be authorized, 0 otherwise
   */
   int32_t check_authorization( account_name account, permission_name permission, char* pubkeys, uint32_t pubkeys_len );
}

   
