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


BOOST_AUTO_TEST_CASE(update_auths) {
try {
   tester chain;
   chain.create_account("alice");

   // Deleting active or owner should fail
   BOOST_CHECK_THROW(chain.delete_authority("alice", "active"), action_validate_exception);
   BOOST_CHECK_THROW(chain.delete_authority("alice", "owner"), action_validate_exception);

   // Change owner permission
   const auto new_owner_priv_key = chain.get_private_key("alice", "new_owner");
   const auto new_owner_pub_key = new_owner_priv_key.get_public_key();
   chain.set_authority("alice", "owner", authority(new_owner_pub_key), "");
   chain.produce_blocks();

   // Ensure the permission is updated
   permission_object::id_type owner_id;
   {
      auto obj = chain.find<permission_object, by_owner>(boost::make_tuple("alice", "owner"));
      BOOST_TEST(obj != nullptr);
      BOOST_TEST(obj->owner == "alice");
      BOOST_TEST(obj->name == "owner");
      BOOST_TEST(obj->parent == 0);
      owner_id = obj->id;
      auto auth = obj->auth.to_authority();
      BOOST_TEST(auth.threshold == 1);
      BOOST_TEST(auth.keys.size() == 1);
      BOOST_TEST(auth.accounts.size() == 0);
      BOOST_TEST(auth.keys[0].key == new_owner_pub_key);
      BOOST_TEST(auth.keys[0].weight == 1);
   }

   // Change active permission, remember that the owner key has been changed
   const auto new_active_priv_key = chain.get_private_key("alice", "new_active");
   const auto new_active_pub_key = new_active_priv_key.get_public_key();
   chain.set_authority("alice", "active", authority(new_active_pub_key), "owner",
                       { permission_level{"alice", "active"} }, { chain.get_private_key("alice", "active") });
   chain.produce_blocks();

   {
      auto obj = chain.find<permission_object, by_owner>(boost::make_tuple("alice", "active"));
      BOOST_TEST(obj != nullptr);
      BOOST_TEST(obj->owner == "alice");
      BOOST_TEST(obj->name == "active");
      BOOST_TEST(obj->parent == owner_id);
      auto auth = obj->auth.to_authority();
      BOOST_TEST(auth.threshold == 1);
      BOOST_TEST(auth.keys.size() == 1);
      BOOST_TEST(auth.accounts.size() == 0);
      BOOST_TEST(auth.keys[0].key == new_active_pub_key);
      BOOST_TEST(auth.keys[0].weight == 1);
   }

   auto spending_priv_key = chain.get_private_key("alice", "spending");
   auto spending_pub_key = spending_priv_key.get_public_key();
   auto trading_priv_key = chain.get_private_key("alice", "trading");
   auto trading_pub_key = trading_priv_key.get_public_key();

   // Create new spending auth
   chain.set_authority("alice", "spending", authority(spending_pub_key), "active",
                       { permission_level{"alice", "active"} }, { new_active_priv_key });
   chain.produce_blocks();
   {
      auto obj = chain.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"));
      BOOST_TEST(obj != nullptr);
      BOOST_TEST(obj->owner == "alice");
      BOOST_TEST(obj->name == "spending");
      BOOST_TEST(chain.get<permission_object>(obj->parent).owner == "alice");
      BOOST_TEST(chain.get<permission_object>(obj->parent).name == "active");
   }

   // Update spending auth parent to be its own, should fail
   BOOST_CHECK_THROW(chain.set_authority("alice", "spending", spending_pub_key, "spending",
                                         { permission_level{"alice", "spending"} }, { spending_priv_key }), action_validate_exception);
   // Update spending auth parent to be owner, should fail
   BOOST_CHECK_THROW(chain.set_authority("alice", "spending", spending_pub_key, "owner",
                                         { permission_level{"alice", "spending"} }, { spending_priv_key }), action_validate_exception);

   // Remove spending auth
   chain.delete_authority("alice", "spending", { permission_level{"alice", "active"} }, { new_active_priv_key });
   {
      auto obj = chain.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"));
      BOOST_TEST(obj == nullptr);
   }
   chain.produce_blocks();

   // Create new trading auth
   chain.set_authority("alice", "trading", trading_pub_key, "active",
                       { permission_level{"alice", "active"} }, { new_active_priv_key });
   // Recreate spending auth again, however this time, it's under trading instead of owner
   chain.set_authority("alice", "spending", spending_pub_key, "trading",
                       { permission_level{"alice", "trading"} }, { trading_priv_key });
   chain.produce_blocks();

   // Verify correctness of trading and spending
   {
      const auto* trading = chain.find<permission_object, by_owner>(boost::make_tuple("alice", "trading"));
      const auto* spending = chain.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"));
      BOOST_TEST(trading != nullptr);
      BOOST_TEST(spending != nullptr);
      BOOST_TEST(trading->owner == "alice");
      BOOST_TEST(spending->owner == "alice");
      BOOST_TEST(trading->name == "trading");
      BOOST_TEST(spending->name == "spending");
      BOOST_TEST(spending->parent == trading->id);
      BOOST_TEST(chain.get(trading->parent).owner == "alice");
      BOOST_TEST(chain.get(trading->parent).name == "active");

   }

   // Delete trading, should fail since it has children (spending)
   BOOST_CHECK_THROW(chain.delete_authority("alice", "trading",
                                            { permission_level{"alice", "active"} }, { new_active_priv_key }), action_validate_exception);
   // Update trading parent to be spending, should fail since changing parent authority is not supported
   BOOST_CHECK_THROW(chain.set_authority("alice", "trading", trading_pub_key, "spending",
                                         { permission_level{"alice", "trading"} }, { trading_priv_key }), action_validate_exception);

   // Delete spending auth
   chain.delete_authority("alice", "spending", { permission_level{"alice", "active"} }, { new_active_priv_key });
   BOOST_TEST((chain.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"))) == nullptr);
   // Delete trading auth, now it should succeed since it doesn't have any children anymore
   chain.delete_authority("alice", "trading", { permission_level{"alice", "active"} }, { new_active_priv_key });
   BOOST_TEST((chain.find<permission_object, by_owner>(boost::make_tuple("alice", "trading"))) == nullptr);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(link_auths) { try {
   tester chain;

   chain.create_accounts({"alice","bob"});

   const auto spending_priv_key = chain.get_private_key("alice", "spending");
   const auto spending_pub_key = spending_priv_key.get_public_key();
   const auto scud_priv_key = chain.get_private_key("alice", "scud");
   const auto scud_pub_key = scud_priv_key.get_public_key();

   chain.set_authority("alice", "spending", spending_pub_key, "active");
   chain.set_authority("alice", "scud", scud_pub_key, "spending");

   // Send req auth action with alice's spending key, it should fail
   BOOST_CHECK_THROW(chain.push_reqauth("alice", { permission_level{N(alice), "spending"} }, { spending_priv_key }), tx_irrelevant_auth);
   // Link authority for eosio reqauth action with alice's spending key
   chain.link_authority("alice", "eosio", "spending",  "reqauth");
   // Now, req auth action with alice's spending key should succeed
   chain.push_reqauth("alice", { permission_level{N(alice), "spending"} }, { spending_priv_key });

   chain.produce_block();

   // Relink the same auth should fail
   BOOST_CHECK_THROW( chain.link_authority("alice", "eosio", "spending",  "reqauth"), action_validate_exception);

   // Unlink alice with eosio reqauth
   chain.unlink_authority("alice", "eosio", "reqauth");
   // Now, req auth action with alice's spending key should fail
   BOOST_CHECK_THROW(chain.push_reqauth("alice", { permission_level{N(alice), "spending"} }, { spending_priv_key }), tx_irrelevant_auth);

   chain.produce_block();

   // Send req auth action with scud key, it should fail
   BOOST_CHECK_THROW(chain.push_reqauth("alice", { permission_level{N(alice), "scud"} }, { scud_priv_key }), tx_irrelevant_auth);
   // Link authority for any eosio action with alice's scud key
   chain.link_authority("alice", "eosio", "scud");
   // Now, req auth action with alice's scud key should succeed
   chain.push_reqauth("alice", { permission_level{N(alice), "scud"} }, { scud_priv_key });
   // req auth action with alice's spending key should also be fine, since it is the parent of alice's scud key
   chain.push_reqauth("alice", { permission_level{N(alice), "spending"} }, { spending_priv_key });
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(create_account) {
try {
   tester chain;
   chain.create_account("joe");
   chain.produce_block();

   // Verify account created properly
   const auto& joe_owner_authority = chain.get<permission_object, by_owner>(boost::make_tuple("joe", "owner"));
   BOOST_TEST(joe_owner_authority.auth.threshold == 1);
   BOOST_TEST(joe_owner_authority.auth.accounts.size() == 0);
   BOOST_TEST(joe_owner_authority.auth.keys.size() == 1);
   BOOST_TEST(string(joe_owner_authority.auth.keys[0].key) == string(chain.get_public_key("joe", "owner")));
   BOOST_TEST(joe_owner_authority.auth.keys[0].weight == 1);

   const auto& joe_active_authority = chain.get<permission_object, by_owner>(boost::make_tuple("joe", "active"));
   BOOST_TEST(joe_active_authority.auth.threshold == 1);
   BOOST_TEST(joe_active_authority.auth.accounts.size() == 0);
   BOOST_TEST(joe_active_authority.auth.keys.size() == 1);
   BOOST_TEST(string(joe_active_authority.auth.keys[0].key) == string(chain.get_public_key("joe", "active")));
   BOOST_TEST(joe_active_authority.auth.keys[0].weight == 1);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
