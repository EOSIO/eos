#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;

auto make_postrecovery(const tester &t, account_name account, string role) {
   signed_transaction trx;
   trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                             postrecovery{
                                .account = account,
                                .data    = authority(t.get_public_key(account, role)),
                                .memo    = "Test recovery"
                             } );
   t.set_tapos(trx);
   trx.sign(t.get_private_key(account, "active"), chain_id_type());
   return trx;
}

auto make_vetorecovery(const tester &t, account_name account, permission_name vetoperm = N(active), optional<private_key_type> signing_key = optional<private_key_type>()) {
   signed_transaction trx;
   trx.actions.emplace_back( vector<permission_level>{{account,vetoperm}},
                             vetorecovery{
                                .account = account
                             } );
   t.set_tapos(trx);
   if (signing_key) {
      trx.sign(*signing_key, chain_id_type());
   } else {
      trx.sign(t.get_private_key(account, (string)vetoperm), chain_id_type());
   }
   return trx;
}

struct recov_tester : public tester {
   transaction_trace push_reqauth(account_name from, string role) {
      return tester::push_reqauth(from, vector<permission_level>{{from, config::owner_name}}, {get_private_key(from, role)} );
   }
};



BOOST_AUTO_TEST_SUITE(recovery_tests)


BOOST_FIXTURE_TEST_CASE( test_recovery_owner, recov_tester ) try {
   produce_blocks(1000);
   create_account(N(alice));
   produce_block();

   fc::time_point expected_recovery(fc::seconds(control->head_block_time().sec_since_epoch()) +fc::days(30));

   transaction_id_type recovery_txid;
   {
      signed_transaction trx = make_postrecovery(*this, N(alice), "owner.recov");
      auto trace = push_transaction(trx);
      BOOST_REQUIRE_EQUAL(trace.deferred_transactions.size(), 1);
      recovery_txid = trace.deferred_transactions.front().id();
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }


   auto skip_time = expected_recovery - control->head_block_time() - fc::milliseconds(config::block_interval_ms);
   produce_block(skip_time);
   control->push_deferred_transactions(true);
   auto last_old_nonce_id = push_reqauth(N(alice), "owner").id;
   produce_block();
   control->push_deferred_transactions(true);

   BOOST_REQUIRE_EQUAL(chain_has_transaction(last_old_nonce_id), true);
   BOOST_REQUIRE_THROW(push_reqauth(N(alice), "owner"), tx_missing_sigs);
   auto first_new_nonce_id = push_reqauth(N(alice), "owner.recov").id;
   produce_block();
   BOOST_REQUIRE_EQUAL(chain_has_transaction(first_new_nonce_id), true);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_recovery_owner_veto, recov_tester ) try {
   produce_blocks(1000);
   create_account(N(alice));
   produce_block();

   fc::time_point expected_recovery(fc::seconds(control->head_block_time().sec_since_epoch()) +fc::days(30));

   transaction_id_type recovery_txid;
   {
      signed_transaction trx = make_postrecovery(*this, N(alice), "owner.recov");
      auto trace = push_transaction(trx);
      BOOST_REQUIRE_EQUAL(trace.deferred_transactions.size(), 1);
      recovery_txid = trace.deferred_transactions.front().id();
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }

   auto skip_time = expected_recovery - control->head_block_time() - fc::milliseconds(config::block_interval_ms);
   produce_block(skip_time);
   control->push_deferred_transactions(true);
   auto last_old_nonce_id = push_reqauth(N(alice), "owner").id;

   // post the veto at the last possible time
   {
      signed_transaction trx = make_vetorecovery(*this, N(alice));
      auto trace = push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }
   BOOST_REQUIRE_EQUAL(chain_has_transaction(last_old_nonce_id), true);

   control->push_deferred_transactions(true);

   // make sure the old owner is still in control

   BOOST_REQUIRE_THROW(push_reqauth(N(alice), "owner.recov"), tx_missing_sigs);
   auto first_new_nonce_id = push_reqauth(N(alice), "owner").id;
   produce_block();
   BOOST_REQUIRE_EQUAL(chain_has_transaction(first_new_nonce_id), true);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_recovery_bad_creator, recov_tester ) try {
   produce_blocks(1000);
   create_account(N(alice), config::system_account_name, true);
   produce_block();

   fc::time_point expected_recovery(fc::seconds(control->head_block_time().sec_since_epoch()) +fc::days(30));

   transaction_id_type recovery_txid;
   {
      signed_transaction trx = make_postrecovery(*this, N(alice), "owner");
      auto trace = push_transaction(trx);
      BOOST_REQUIRE_EQUAL(trace.deferred_transactions.size(), 1);
      recovery_txid = trace.deferred_transactions.front().id();
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }

   auto skip_time = expected_recovery - control->head_block_time() - fc::milliseconds(config::block_interval_ms);
   produce_block(skip_time);
   control->push_deferred_transactions(true);

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
   control->push_deferred_transactions(true);

   // make sure the recovery goes through
   auto first_new_nonce_id = push_reqauth(N(alice), "owner").id;
   produce_block();
   BOOST_REQUIRE_EQUAL(chain_has_transaction(first_new_nonce_id), true);

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
