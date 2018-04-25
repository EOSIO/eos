#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/permission_object.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;


BOOST_AUTO_TEST_SUITE( delay_tests )


BOOST_FIXTURE_TEST_CASE( delay_create_account, validating_tester) { try {

   produce_blocks(2);
   signed_transaction trx;

   account_name a = N(newco);
   account_name creator = config::system_account_name;

   auto owner_auth =  authority( get_public_key( a, "owner" ) );
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             newaccount{
                                .creator  = creator,
                                .name     = a,
                                .owner    = owner_auth,
                                .active   = authority( get_public_key( a, "active" ) ),
                                .recovery = authority( get_public_key( a, "recovery" ) ),
                             });
   set_transaction_headers(trx);
   trx.delay_sec = 3;
   trx.sign( get_private_key( creator, "active" ), chain_id_type()  );

   auto trace = push_transaction( trx );

   produce_blocks(8);


} FC_LOG_AND_RETHROW() }


BOOST_FIXTURE_TEST_CASE( delay_error_create_account, validating_tester) { try {

   produce_blocks(2);
   signed_transaction trx;

   account_name a = N(newco);
   account_name creator = config::system_account_name;

   auto owner_auth =  authority( get_public_key( a, "owner" ) );
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             newaccount{
                                .creator  = N(bad), /// a does not exist, this should error when execute
                                .name     = a,
                                .owner    = owner_auth,
                                .active   = authority( get_public_key( a, "active" ) ),
                                .recovery = authority( get_public_key( a, "recovery" ) ),
                             });
   set_transaction_headers(trx);
   trx.delay_sec = 3;
   trx.sign( get_private_key( creator, "active" ), chain_id_type()  );

   ilog( fc::json::to_pretty_string(trx) );
   auto trace = push_transaction( trx );
   edump((*trace));

   produce_blocks(8);


} FC_LOG_AND_RETHROW() }



BOOST_AUTO_TEST_SUITE_END()

