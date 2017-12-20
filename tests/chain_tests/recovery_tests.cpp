#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>

#include <eosio/chain/contracts/staked_balance_objects.hpp>


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(recovery_tests)



BOOST_FIXTURE_TEST_CASE( test_recovery_owner, tester ) try {
   produce_blocks(1000);
   create_account(N(alice), asset::from_string("1000.000 EOS"));
   produce_block();

   fc::time_point expected_recovery(fc::seconds(control->head_block_time().sec_since_epoch()) +fc::days(30));

   transaction_id_type recovery_txid;
   {
      signed_transaction trx;
      trx.write_scope = {config::eosio_auth_scope, N(alice)};
      trx.actions.emplace_back( vector<permission_level>{{N(alice),config::active_name}},
                                postrecovery{
                                   .account = N(alice),
                                   .data    = authority(get_public_key(N(alice), "owner.recov")),
                                   .memo    = "Test recovery"
                                } );
      set_tapos(trx);
      trx.sign(get_private_key(N(alice), "active"), chain_id_type());
      auto trace = push_transaction(trx);
      BOOST_REQUIRE_EQUAL(trace.deferred_transactions.size(), 1);
      recovery_txid = trace.deferred_transactions.front().id();
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }

   auto push_nonce = [this](const string& role) {
      // ensure the old owner key is valid
      signed_transaction trx;
      trx.write_scope = {N(alice)};
      trx.actions.emplace_back( vector<permission_level>{{N(alice),config::owner_name}},
                                nonce{
                                   .value = control->head_block_num()
                                } );
      set_tapos(trx);
      trx.sign(get_private_key(N(alice), role), chain_id_type());
      push_transaction(trx);
      return trx.id();
   };

   auto skip_time = expected_recovery - control->head_block_time() - fc::milliseconds(config::block_interval_ms);
   produce_block(skip_time);
   control->push_deferred_transactions(true);
   auto last_old_nonce_id = push_nonce("owner");
   produce_block();
   control->push_deferred_transactions(true);

   BOOST_REQUIRE_EQUAL(chain_has_transaction(last_old_nonce_id), true);
   BOOST_REQUIRE_THROW(push_nonce("owner"), tx_missing_sigs);
   auto first_new_nonce_id = push_nonce("owner.recov");
   produce_block();
   BOOST_REQUIRE_EQUAL(chain_has_transaction(first_new_nonce_id), true);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_recovery_owner_veto, tester ) try {
   produce_blocks(1000);
   create_account(N(alice), asset::from_string("1000.000 EOS"));
   produce_block();

   fc::time_point expected_recovery(fc::seconds(control->head_block_time().sec_since_epoch()) +fc::days(30));

   transaction_id_type recovery_txid;
   {
      signed_transaction trx;
      trx.write_scope = {config::eosio_auth_scope, N(alice)};
      trx.actions.emplace_back( vector<permission_level>{{N(alice),config::active_name}},
                                postrecovery{
                                   .account = N(alice),
                                   .data    = authority(get_public_key(N(alice), "owner.recov")),
                                   .memo    = "Test recovery"
                                } );
      set_tapos(trx);
      trx.sign(get_private_key(N(alice), "active"), chain_id_type());
      auto trace = push_transaction(trx);
      BOOST_REQUIRE_EQUAL(trace.deferred_transactions.size(), 1);
      recovery_txid = trace.deferred_transactions.front().id();
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }

   auto push_nonce = [this](const string& role) {
      // ensure the old owner key is valid
      signed_transaction trx;
      trx.write_scope = {N(alice)};
      trx.actions.emplace_back( vector<permission_level>{{N(alice),config::owner_name}},
                                nonce{
                                   .value = control->head_block_num()
                                } );
      set_tapos(trx);
      trx.sign(get_private_key(N(alice), role), chain_id_type());
      push_transaction(trx);
      return trx.id();
   };

   auto skip_time = expected_recovery - control->head_block_time() - fc::milliseconds(config::block_interval_ms);
   produce_block(skip_time);
   control->push_deferred_transactions(true);
   auto last_old_nonce_id = push_nonce("owner");

   // post the veto at the last possible time
   {
      signed_transaction trx;
      trx.write_scope = {config::eosio_auth_scope, N(alice)};
      trx.actions.emplace_back( vector<permission_level>{{N(alice),config::active_name}},
                                vetorecovery{
                                   .account = N(alice)
                                } );
      set_tapos(trx);
      trx.sign(get_private_key(N(alice), "active"), chain_id_type());
      auto trace = push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(chain_has_transaction(trx.id()), true);
   }
   BOOST_REQUIRE_EQUAL(chain_has_transaction(last_old_nonce_id), true);

   control->push_deferred_transactions(true);

   // make sure the old owner is still in controll

   BOOST_REQUIRE_THROW(push_nonce("owner.recov"), tx_missing_sigs);
   auto first_new_nonce_id = push_nonce("owner");
   produce_block();
   BOOST_REQUIRE_EQUAL(chain_has_transaction(first_new_nonce_id), true);

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
