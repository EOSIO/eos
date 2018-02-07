#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;

struct auth_tester : public tester {
   transaction_trace push_nonce(account_name from, std::initializer_list<permission_level> &&permissions, std::initializer_list<private_key_type> &&sign_with) {
      variant pretty_trx = fc::mutable_variant_object()
      ("actions", fc::variants({
         fc::mutable_variant_object()
            ("account", name(config::system_account_name))
            ("name", "nonce")
            ("authorization", vector<permission_level>(permissions))
            ("data", fc::mutable_variant_object()
               ("from", from)
               ("value", fc::time_point::now())
            )
         })
     );

      signed_transaction trx;
      contracts::abi_serializer::from_variant(pretty_trx, trx, get_resolver());
      set_tapos( trx );
      for(auto iter = sign_with.begin(); iter != sign_with.end(); iter++)
         trx.sign( *iter, chain_id_type() );
      return push_transaction( trx );
   }
};

BOOST_AUTO_TEST_SUITE(auth_tests)

BOOST_FIXTURE_TEST_CASE( missing_sigs, auth_tester ) { try {
   create_accounts( {N(alice)} );
   produce_block();

   BOOST_REQUIRE_THROW( push_nonce( N(alice), {permission_level{N(alice), config::active_name}}, {} ), tx_missing_sigs );
   auto trace = push_nonce(N(alice), {permission_level{N(alice), config::active_name}}, { get_private_key(N(alice), "active") } );

   produce_block();
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));

} FC_LOG_AND_RETHROW() } /// missing_sigs

BOOST_FIXTURE_TEST_CASE( missing_auths, auth_tester ) { try {
   create_accounts( {N(alice), N(bob)} );
   produce_block();

   /// action not provided from authority
   BOOST_REQUIRE_THROW( push_nonce( N(alice), {permission_level{N(bob), config::active_name}}, { get_private_key(N(bob), "active") } ), tx_missing_auth);

} FC_LOG_AND_RETHROW() } /// transfer_test


/**
 *  This test case will attempt to allow one account to transfer on behalf
 *  of another account by updating the active authority.
 */
BOOST_FIXTURE_TEST_CASE( delegate_auth, auth_tester ) { try {
   create_accounts( {N(alice),N(bob)});
   produce_block();

   auto delegated_auth = authority( 1, {},
                          {
                            { .permission = {N(bob),config::active_name}, .weight = 1}
                          });

   set_authority( N(alice), config::active_name,  delegated_auth );

   produce_block( fc::hours(2) ); ///< skip 2 hours

   /// execute nonce from alice signed by bob
   auto trace = push_nonce(N(alice), {permission_level{N(alice), config::active_name}}, { get_private_key(N(bob), "active") } );

   produce_block();
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));
} FC_LOG_AND_RETHROW() }



BOOST_AUTO_TEST_SUITE_END()
