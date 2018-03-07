/**
 * @file action_test.cpp
 * @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/permission.h>
#include <eosiolib/db.h>

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/compiler_builtins.h>
#include <eosiolib/serialize.hpp>
#include <eosiolib/action.hpp>

#include "test_api.hpp"

using namespace eosio;

struct check_auth {
   account_name         account;
   permission_name      permission;
   vector<public_key>   pubkeys;

   EOSLIB_SERIALIZE( check_auth, (account)(permission)(pubkeys)  )
};

void test_permission::check_authorization() {
  auto params = unpack_action_data<check_auth>();

  uint64_t res64 = (uint64_t)::check_authorization( params.account, params.permission, 
        (char*)params.pubkeys.data(), params.pubkeys.size()*sizeof(public_key) );

  store_i64(current_receiver(), current_receiver(), current_receiver(), &res64, sizeof(uint64_t));
}
