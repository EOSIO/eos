#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(auth_tests)

BOOST_FIXTURE_TEST_CASE( missing_sigs, tester ) { try {
   create_accounts( {N(alice)} );
   produce_block();

   BOOST_REQUIRE_THROW( push_reqauth( N(alice), {permission_level{N(alice), config::active_name}}, {} ), tx_missing_sigs );
   auto trace = push_reqauth(N(alice), "owner");

                    produce_block();
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));

} FC_LOG_AND_RETHROW() } /// missing_sigs

BOOST_FIXTURE_TEST_CASE( missing_multi_sigs, tester ) { try {
    produce_block();
    create_account(N(alice), config::system_account_name, true);
    produce_block();

    BOOST_REQUIRE_THROW(push_reqauth(N(alice), "owner"), tx_missing_sigs); // without multisig
    auto trace = push_reqauth(N(alice), "owner", true); // with multisig

    produce_block();
    BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));

 } FC_LOG_AND_RETHROW() } /// missing_multi_sigs

BOOST_FIXTURE_TEST_CASE( missing_auths, tester ) { try {
   create_accounts( {N(alice), N(bob)} );
   produce_block();

   /// action not provided from authority
   BOOST_REQUIRE_THROW( push_reqauth( N(alice), {permission_level{N(bob), config::active_name}}, { get_private_key(N(bob), "active") } ), tx_missing_auth);

} FC_LOG_AND_RETHROW() } /// transfer_test


/**
 *  This test case will attempt to allow one account to transfer on behalf
 *  of another account by updating the active authority.
 */
BOOST_FIXTURE_TEST_CASE( delegate_auth, tester ) { try {
   create_accounts( {N(alice),N(bob)});
   produce_block();

   auto delegated_auth = authority( 1, {},
                          {
                            { .permission = {N(bob),config::active_name}, .weight = 1}
                          });

   set_authority( N(alice), config::active_name,  delegated_auth );

   produce_block( fc::hours(2) ); ///< skip 2 hours

   /// execute nonce from alice signed by bob
   auto trace = push_reqauth(N(alice), {permission_level{N(alice), config::active_name}}, { get_private_key(N(bob), "active") } );

   produce_block();
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));
} FC_LOG_AND_RETHROW() }



BOOST_AUTO_TEST_SUITE_END()
