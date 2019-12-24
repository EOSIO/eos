#include <limits>

#include <apifinylib/action.hpp>
#include <apifinylib/db.h>
#include <apifinylib/apifiny.hpp>
#include <apifinylib/permission.h>
#include <apifinylib/print.hpp>
#include <apifinylib/serialize.hpp>

#include "test_api.hpp"



struct check_auth_msg {
   apifiny::name                    account;
   apifiny::name                    permission;
   std::vector<apifiny::public_key> pubkeys;

   APIFINYLIB_SERIALIZE( check_auth_msg, (account)(permission)(pubkeys)  )
};

void test_permission::check_authorization( uint64_t receiver, uint64_t code, uint64_t action ) {
   (void)code;
   (void)action;
   using namespace apifiny;

   auto self = receiver;
   auto params = unpack_action_data<check_auth_msg>();
   auto packed_pubkeys = pack(params.pubkeys);
   int64_t res64 = ::check_permission_authorization( params.account.value,
                                                     params.permission.value,
                                                     packed_pubkeys.data(), packed_pubkeys.size(),
                                                     (const char*)0,        0,
                                                     static_cast<uint64_t>( std::numeric_limits<int64_t>::max() )
                                                   );

   auto itr = db_lowerbound_i64( self, self, self, 1 );
   if(itr == -1) {
      db_store_i64( self, self, self, 1, &res64, sizeof(int64_t) );
   } else {
      db_update_i64( itr, self, &res64, sizeof(int64_t) );
   }
}

struct test_permission_last_used_msg {
   apifiny::name account;
   apifiny::name permission;
   int64_t     last_used_time;

   APIFINYLIB_SERIALIZE( test_permission_last_used_msg, (account)(permission)(last_used_time) )
};

void test_permission::test_permission_last_used( uint64_t /* receiver */, uint64_t code, uint64_t action ) {
   (void)code;
   (void)action;
   using namespace apifiny;

   auto params = unpack_action_data<test_permission_last_used_msg>();

   apifiny_assert( get_permission_last_used(params.account.value, params.permission.value) == params.last_used_time, "unexpected last used permission time" );
}

void test_permission::test_account_creation_time( uint64_t /* receiver */, uint64_t code, uint64_t action ) {
   (void)code;
   (void)action;
   using namespace apifiny;

   auto params = unpack_action_data<test_permission_last_used_msg>();

   apifiny_assert( get_account_creation_time(params.account.value) == params.last_used_time, "unexpected account creation time" );
}
