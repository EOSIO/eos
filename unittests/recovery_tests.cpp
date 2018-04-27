#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/authorization_manager.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

auto make_postrecovery(const TESTER &t, account_name account, string role) {
   signed_transaction trx;
   trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                             postrecovery{
                                .account = account,
                                .auth    = authority(t.get_public_key(account, role)),
                                .memo    = "Test recovery"
                             } );
   t.set_transaction_headers(trx);
   trx.sign(t.get_private_key(account, "active"), chain_id_type());
   return trx;
}

auto make_vetorecovery(const TESTER &t, account_name account, permission_name vetoperm = N(active), optional<private_key_type> signing_key = optional<private_key_type>()) {
   signed_transaction trx;
   trx.actions.emplace_back( vector<permission_level>{{account,vetoperm}},
                             vetorecovery{
                                .account = account
                             } );
   t.set_transaction_headers(trx);
   if (signing_key) {
      trx.sign(*signing_key, chain_id_type());
   } else {
      trx.sign(t.get_private_key(account, (string)vetoperm), chain_id_type());
   }
   return trx;
}


BOOST_AUTO_TEST_SUITE(recovery_tests)

BOOST_FIXTURE_TEST_CASE( test_recovery_multisig_owner, TESTER ) try {
    produce_block();
    create_account(N(alice), config::system_account_name, true);
    produce_block();

    BOOST_REQUIRE_THROW(push_reqauth(N(alice), "owner"), tx_missing_sigs); // requires multisig authorization
    push_reqauth(N(alice), "owner", true);
    produce_block();


    transaction_id_type recovery_txid;
    {
        signed_transaction trx = make_postrecovery(*this, N(alice), "owner.recov");
        auto trace = push_transaction(trx);
        auto gen_size = control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
        BOOST_CHECK_EQUAL(1, gen_size);
        recovery_txid = control->db().get_index<generated_transaction_multi_index,by_trx_id>().begin()->trx_id;
        produce_block();
        BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
    }

    authority org_auth = (authority)control->get_authorization_manager().get_permission({N(alice), N(owner)}).auth;

    produce_block(fc::days(30)+fc::milliseconds(config::block_interval_ms));

    authority auth = (authority)control->get_authorization_manager().get_permission({N(alice), N(owner)}).auth;
    if (org_auth == auth) BOOST_TEST_FAIL("authority should have changed");

    produce_block();
    BOOST_REQUIRE_THROW(push_reqauth(N(alice), "owner", true), tx_missing_sigs);
    auto first_new_nonce_id = push_reqauth(N(alice), "owner.recov")->id;
    produce_block();
    BOOST_REQUIRE_EQUAL(chain_has_transaction(first_new_nonce_id), true);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_recovery_owner, TESTER ) try {
   produce_block();
   create_account(N(alice));
   produce_block();

   fc::time_point expected_recovery(fc::seconds(control->head_block_time().sec_since_epoch()) +fc::days(30));

   transaction_id_type recovery_txid;
   {
      signed_transaction trx = make_postrecovery(*this, N(alice), "owner.recov");
      auto trace = push_transaction(trx);
      auto gen_size = control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_CHECK_EQUAL(gen_size, 1);
      recovery_txid = control->db().get_index<generated_transaction_multi_index,by_trx_id>().begin()->trx_id;
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }


   auto skip_time = expected_recovery - control->head_block_time();
   produce_block(skip_time);
   auto last_old_nonce_id = push_reqauth(N(alice), "owner")->id;
   produce_block();

   BOOST_REQUIRE_EQUAL(chain_has_transaction(last_old_nonce_id), true);
   BOOST_REQUIRE_THROW(push_reqauth(N(alice), "owner"), tx_missing_sigs);
   auto first_new_nonce_id = push_reqauth(N(alice), "owner.recov")->id;
   produce_block();
   BOOST_REQUIRE_EQUAL(chain_has_transaction(first_new_nonce_id), true);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_recovery_owner_veto, TESTER ) try {
   produce_block();
   create_account(N(alice));
   produce_block();

   fc::time_point expected_recovery(fc::seconds(control->head_block_time().sec_since_epoch()) +fc::days(30));

   transaction_id_type recovery_txid;
   {
      signed_transaction trx = make_postrecovery(*this, N(alice), "owner.recov");
      auto trace = push_transaction(trx);
      auto gen_size = control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_CHECK_EQUAL(gen_size, 1);
      recovery_txid = control->db().get_index<generated_transaction_multi_index,by_trx_id>().begin()->trx_id;
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }

   auto skip_time = expected_recovery - control->head_block_time();
   produce_block(skip_time);
   auto last_old_nonce_id = push_reqauth(N(alice), "owner")->id;

   // post the veto at the last possible time
   {
      signed_transaction trx = make_vetorecovery(*this, N(alice));
      auto trace = push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }
   BOOST_REQUIRE_EQUAL(chain_has_transaction(last_old_nonce_id), true);

   // make sure the old owner is still in control

   BOOST_REQUIRE_THROW(push_reqauth(N(alice), "owner.recov"), tx_missing_sigs);
   auto first_new_nonce_id = push_reqauth(N(alice), "owner")->id;
   produce_block();
   BOOST_REQUIRE_EQUAL(chain_has_transaction(first_new_nonce_id), true);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_recovery_bad_creator, TESTER ) try {
   produce_block();
   create_account(N(alice), config::system_account_name, true);
   produce_block();

   fc::time_point expected_recovery(fc::seconds(control->head_block_time().sec_since_epoch()) +fc::days(30));

   transaction_id_type recovery_txid;
   {
      signed_transaction trx = make_postrecovery(*this, N(alice), "owner");
      auto trace = push_transaction(trx);
      auto gen_size = control->db().get_index<generated_transaction_multi_index,by_trx_id>().size();
      BOOST_CHECK_EQUAL(gen_size, 1);
      recovery_txid = control->db().get_index<generated_transaction_multi_index,by_trx_id>().begin()->trx_id;
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }

   auto skip_time = expected_recovery - control->head_block_time();
   produce_block(skip_time);

   // try all types of veto from the bad partner
   {
      signed_transaction trx = make_vetorecovery(*this, N(alice), N(active), get_private_key(N(inita),"active"));
      BOOST_REQUIRE_THROW(push_transaction(trx), tx_missing_sigs);
   }

   {
      signed_transaction trx = make_vetorecovery(*this, N(alice), N(active), get_private_key(N(inita),"owner"));
      BOOST_REQUIRE_THROW(push_transaction(trx), tx_missing_sigs);
   }

   {
      signed_transaction trx = make_vetorecovery(*this, N(alice), N(owner), get_private_key(N(inita),"active"));
      BOOST_REQUIRE_THROW(push_transaction(trx), tx_missing_sigs);
   }

   {
      signed_transaction trx = make_vetorecovery(*this, N(alice), N(owner), get_private_key(N(inita),"owner"));
      BOOST_REQUIRE_THROW(push_transaction(trx), tx_missing_sigs);
   }

   produce_block();

   // make sure the recovery goes through
   auto first_new_nonce_id = push_reqauth(N(alice), "owner")->id;
   produce_block();
   BOOST_REQUIRE_EQUAL(chain_has_transaction(first_new_nonce_id), true);

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
