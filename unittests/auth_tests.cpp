#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/authorization_manager.hpp>

#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>

#include <eosio/testing/tester_network.hpp>
#include <eosio/chain/producer_object.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(auth_tests)

BOOST_FIXTURE_TEST_CASE( missing_sigs, TESTER ) { try {
   create_accounts( {N(alice)} );
   produce_block();

   BOOST_REQUIRE_THROW( push_reqauth( N(alice), {permission_level{N(alice), config::active_name}}, {} ), unsatisfied_authorization );
   auto trace = push_reqauth(N(alice), "owner");

   produce_block();
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace->id));

} FC_LOG_AND_RETHROW() } /// missing_sigs

BOOST_FIXTURE_TEST_CASE( missing_multi_sigs, TESTER ) { try {
    produce_block();
    create_account(N(alice), config::system_account_name, true);
    produce_block();

    BOOST_REQUIRE_THROW(push_reqauth(N(alice), "owner"), unsatisfied_authorization); // without multisig
    auto trace = push_reqauth(N(alice), "owner", true); // with multisig

    produce_block();
    BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace->id));

 } FC_LOG_AND_RETHROW() } /// missing_multi_sigs

BOOST_FIXTURE_TEST_CASE( missing_auths, TESTER ) { try {
   create_accounts( {N(alice), N(bob)} );
   produce_block();

   /// action not provided from authority
   BOOST_REQUIRE_THROW( push_reqauth( N(alice), {permission_level{N(bob), config::active_name}}, { get_private_key(N(bob), "active") } ), missing_auth_exception);

} FC_LOG_AND_RETHROW() } /// transfer_test

/**
 *  This test case will attempt to allow one account to transfer on behalf
 *  of another account by updating the active authority.
 */
BOOST_FIXTURE_TEST_CASE( delegate_auth, TESTER ) { try {
   create_accounts( {N(alice),N(bob)});
   produce_block();

   auto delegated_auth = authority( 1, {},
                          {
                            { .permission = {N(bob),config::active_name}, .weight = 1}
                          });

   auto original_auth = static_cast<authority>(control->get_authorization_manager().get_permission({N(alice), config::active_name}).auth);
   wdump((original_auth));

   set_authority( N(alice), config::active_name,  delegated_auth );

   auto new_auth = static_cast<authority>(control->get_authorization_manager().get_permission({N(alice), config::active_name}).auth);
   wdump((new_auth));
   BOOST_CHECK_EQUAL((new_auth == delegated_auth), true);

   produce_block( fc::milliseconds(config::block_interval_ms*2) );

   auto auth = static_cast<authority>(control->get_authorization_manager().get_permission({N(alice), config::active_name}).auth);
   wdump((auth));
   BOOST_CHECK_EQUAL((new_auth == auth), true);

   /// execute nonce from alice signed by bob
   auto trace = push_reqauth(N(alice), {permission_level{N(alice), config::active_name}}, { get_private_key(N(bob), "active") } );

   produce_block();
   //todoBOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace->id));

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE(update_auths) {
try {
   TESTER chain;
   chain.create_account("alice");
   chain.create_account("bob");

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

   // Bob attempts to create new spending auth for Alice
   BOOST_CHECK_THROW( chain.set_authority( "alice", "spending", authority(spending_pub_key), "active",
                                           { permission_level{"bob", "active"} }, { chain.get_private_key("bob", "active") } ),
                      irrelevant_auth_exception );

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
   TESTER chain;

   chain.create_accounts({"alice","bob"});

   const auto spending_priv_key = chain.get_private_key("alice", "spending");
   const auto spending_pub_key = spending_priv_key.get_public_key();
   const auto scud_priv_key = chain.get_private_key("alice", "scud");
   const auto scud_pub_key = scud_priv_key.get_public_key();

   chain.set_authority("alice", "spending", spending_pub_key, "active");
   chain.set_authority("alice", "scud", scud_pub_key, "spending");

   // Send req auth action with alice's spending key, it should fail
   BOOST_CHECK_THROW(chain.push_reqauth("alice", { permission_level{N(alice), "spending"} }, { spending_priv_key }), irrelevant_auth_exception);
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
   BOOST_CHECK_THROW(chain.push_reqauth("alice", { permission_level{N(alice), "spending"} }, { spending_priv_key }), irrelevant_auth_exception);

   chain.produce_block();

   // Send req auth action with scud key, it should fail
   BOOST_CHECK_THROW(chain.push_reqauth("alice", { permission_level{N(alice), "scud"} }, { scud_priv_key }), irrelevant_auth_exception);
   // Link authority for any eosio action with alice's scud key
   chain.link_authority("alice", "eosio", "scud");
   // Now, req auth action with alice's scud key should succeed
   chain.push_reqauth("alice", { permission_level{N(alice), "scud"} }, { scud_priv_key });
   // req auth action with alice's spending key should also be fine, since it is the parent of alice's scud key
   chain.push_reqauth("alice", { permission_level{N(alice), "spending"} }, { spending_priv_key });

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(link_then_update_auth) { try {
   TESTER chain;

   chain.create_account("alice");

   const auto first_priv_key = chain.get_private_key("alice", "first");
   const auto first_pub_key = first_priv_key.get_public_key();
   const auto second_priv_key = chain.get_private_key("alice", "second");
   const auto second_pub_key = second_priv_key.get_public_key();

   chain.set_authority("alice", "first", first_pub_key, "active");

   chain.link_authority("alice", "eosio", "first",  "reqauth");
   chain.push_reqauth("alice", { permission_level{N(alice), "first"} }, { first_priv_key });

   chain.produce_blocks(13); // Wait at least 6 seconds for first push_reqauth transaction to expire.

   // Update "first" auth public key
   chain.set_authority("alice", "first", second_pub_key, "active");
   // Authority updated, using previous "first" auth should fail on linked auth
   BOOST_CHECK_THROW(chain.push_reqauth("alice", { permission_level{N(alice), "first"} }, { first_priv_key }), unsatisfied_authorization);
   // Using updated authority, should succeed
   chain.push_reqauth("alice", { permission_level{N(alice), "first"} }, { second_priv_key });

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(create_account) {
try {
   TESTER chain;
   chain.create_account("joe");
   chain.produce_block();

   // Verify account created properly
   const auto& joe_owner_authority = chain.get<permission_object, by_owner>(boost::make_tuple("joe", "owner"));
   BOOST_TEST(joe_owner_authority.auth.threshold == 1);
   BOOST_TEST(joe_owner_authority.auth.accounts.size() == 1);
   BOOST_TEST(joe_owner_authority.auth.keys.size() == 1);
   BOOST_TEST(string(joe_owner_authority.auth.keys[0].key) == string(chain.get_public_key("joe", "owner")));
   BOOST_TEST(joe_owner_authority.auth.keys[0].weight == 1);

   const auto& joe_active_authority = chain.get<permission_object, by_owner>(boost::make_tuple("joe", "active"));
   BOOST_TEST(joe_active_authority.auth.threshold == 1);
   BOOST_TEST(joe_active_authority.auth.accounts.size() == 1);
   BOOST_TEST(joe_active_authority.auth.keys.size() == 1);
   BOOST_TEST(string(joe_active_authority.auth.keys[0].key) == string(chain.get_public_key("joe", "active")));
   BOOST_TEST(joe_active_authority.auth.keys[0].weight == 1);

   // Create duplicate name
   BOOST_CHECK_EXCEPTION(chain.create_account("joe"), action_validate_exception,
                         fc_exception_message_is("Cannot create account named joe, as that name is already taken"));

   // Creating account with name more than 12 chars
   BOOST_CHECK_EXCEPTION(chain.create_account("aaaaaaaaaaaaa"), action_validate_exception,
                         fc_exception_message_is("account names can only be 12 chars long"));


   // Creating account with eosio. prefix with privileged account
   chain.create_account("eosio.test1");

   // Creating account with eosio. prefix with non-privileged account, should fail
   BOOST_CHECK_EXCEPTION(chain.create_account("eosio.test2", "joe"), action_validate_exception,
                         fc_exception_message_is("only privileged accounts can have names that start with 'eosio.'"));

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( any_auth ) { try {
   TESTER chain;
   chain.create_accounts( {"alice","bob"} );
   chain.produce_block();

   const auto spending_priv_key = chain.get_private_key("alice", "spending");
   const auto spending_pub_key = spending_priv_key.get_public_key();
   const auto bob_spending_priv_key = chain.get_private_key("bob", "spending");
   const auto bob_spending_pub_key = spending_priv_key.get_public_key();

   chain.set_authority("alice", "spending", spending_pub_key, "active");
   chain.set_authority("bob", "spending", bob_spending_pub_key, "active");

   /// this should fail because spending is not active which is default for reqauth
   BOOST_REQUIRE_THROW( chain.push_reqauth("alice", { permission_level{N(alice), "spending"} }, { spending_priv_key }),
                        irrelevant_auth_exception );

   chain.produce_block();

   //test.push_reqauth( N(alice), { permission_level{N(alice),"spending"} }, { spending_priv_key });

   chain.link_authority( "alice", "eosio", "eosio.any", "reqauth" );
   chain.link_authority( "bob", "eosio", "eosio.any", "reqauth" );

   /// this should succeed because eosio::reqauth is linked to any permission
   chain.push_reqauth("alice", { permission_level{N(alice), "spending"} }, { spending_priv_key });

   /// this should fail because bob cannot authorize for alice, the permission given must be one-of alices
   BOOST_REQUIRE_THROW( chain.push_reqauth("alice", { permission_level{N(bob), "spending"} }, { spending_priv_key }),
                        missing_auth_exception );


   chain.produce_block();

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(no_double_billing) {
try {
   TESTER chain;

   chain.produce_block();

   account_name acc1 = N("bill1");
   account_name acc2 = N("bill2");
   account_name acc1a = N("bill1a");

   chain.create_account(acc1);
   chain.create_account(acc1a);
   chain.produce_block();

   chainbase::database &db = chain.control->db();

   using resource_usage_object = eosio::chain::resource_limits::resource_usage_object;
   using by_owner = eosio::chain::resource_limits::by_owner;

   auto create_acc = [&](account_name a) {

      signed_transaction trx;
      chain.set_transaction_headers(trx);

      authority owner_auth =  authority( chain.get_public_key( a, "owner" ) );

      vector<permission_level> pls = {{acc1, "active"}};
      pls.push_back({acc1, "owner"}); // same account but different permission names
      pls.push_back({acc1a, "owner"});
      trx.actions.emplace_back( pls,
                                newaccount{
                                   .creator  = acc1,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( chain.get_public_key( a, "active" ) )
                                });

      chain.set_transaction_headers(trx);
      trx.sign( chain.get_private_key( acc1, "active" ), chain.control->get_chain_id()  );
      trx.sign( chain.get_private_key( acc1, "owner" ), chain.control->get_chain_id()  );
      trx.sign( chain.get_private_key( acc1a, "owner" ), chain.control->get_chain_id()  );
      return chain.push_transaction( trx );
   };

   create_acc(acc2);

   const auto &usage = db.get<resource_usage_object,by_owner>(acc1);

   const auto &usage2 = db.get<resource_usage_object,by_owner>(acc1a);

   BOOST_TEST(usage.cpu_usage.average() > 0);
   BOOST_TEST(usage.net_usage.average() > 0);
   BOOST_REQUIRE_EQUAL(usage.cpu_usage.average(), usage2.cpu_usage.average());
   BOOST_REQUIRE_EQUAL(usage.net_usage.average(), usage2.net_usage.average());
   chain.produce_block();

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(stricter_auth) {
try {
   TESTER chain;

   chain.produce_block();

   account_name acc1 = N("acc1");
   account_name acc2 = N("acc2");
   account_name acc3 = N("acc3");
   account_name acc4 = N("acc4");

   chain.create_account(acc1);
   chain.produce_block();

   auto create_acc = [&](account_name a, account_name creator, int threshold) {

      signed_transaction trx;
      chain.set_transaction_headers(trx);

      authority invalid_auth = authority(threshold, {key_weight{chain.get_public_key( a, "owner" ), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});

      vector<permission_level> pls;
      pls.push_back({creator, "active"});
      trx.actions.emplace_back( pls,
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = authority( chain.get_public_key( a, "owner" ) ),
                                   .active   = invalid_auth//authority( chain.get_public_key( a, "active" ) ),
                                });

      chain.set_transaction_headers(trx);
      trx.sign( chain.get_private_key( creator, "active" ), chain.control->get_chain_id()  );
      return chain.push_transaction( trx );
   };

   try {
     create_acc(acc2, acc1, 0);
     BOOST_FAIL("threshold can't be zero");
   } catch (...) { }

   try {
     create_acc(acc4, acc1, 3);
     BOOST_FAIL("threshold can't be more than total weight");
   } catch (...) { }

   create_acc(acc3, acc1, 1);

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( linkauth_special ) { try {
   TESTER chain;

   const auto& tester_account = N(tester);
   std::vector<transaction_id_type> ids;

   chain.produce_blocks();
   chain.create_account(N(currency));

   chain.produce_blocks();
   chain.create_account(N(tester));
   chain.create_account(N(tester2));
   chain.produce_blocks();

   chain.push_action(config::system_account_name, updateauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("permission", "first")
           ("parent", "active")
           ("auth",  authority(chain.get_public_key(tester_account, "first"), 5))
   );

   auto validate_disallow = [&] (const char *type) {
   BOOST_REQUIRE_EXCEPTION(
   chain.push_action(config::system_account_name, linkauth::get_name(), tester_account, fc::mutable_variant_object()
           ("account", "tester")
           ("code", "eosio")
           ("type", type)
           ("requirement", "first")),
   action_validate_exception,
   [] (const action_validate_exception &ex)->bool {
      BOOST_REQUIRE_EQUAL(std::string("action exception"), ex.what());
      return true;
   });
   };

   validate_disallow("linkauth");
   validate_disallow("unlinkauth");
   validate_disallow("deleteauth");
   validate_disallow("updateauth");
   validate_disallow("canceldelay");

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
