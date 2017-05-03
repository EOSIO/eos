#include <boost/test/unit_test.hpp>

#include <eos/chain/database.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/key_value_object.hpp>

#include <eos/native_system_contract_plugin/native_system_contract_plugin.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace eos;
using namespace chain;
using namespace types;

BOOST_AUTO_TEST_SUITE(system_contract_tests)

//Simple test of account creation
BOOST_FIXTURE_TEST_CASE(create_account, testing_fixture)
{ try {
      MKDB(db);
      db.produce_blocks(10);

      const auto& init1_account = db.get_account("init1");

      BOOST_CHECK_EQUAL(init1_account.balance, Asset(100000));

      auto joe_private_key = fc::ecc::private_key::regenerate(fc::sha256::hash("joe"));
      public_key_type joe_public_key = joe_private_key.get_public_key();
      CreateAccount ca{"init1", "joe", {1, {{joe_public_key, 1}}, {}},
                       {1, {{joe_public_key, 1}}, {}}, {}, Asset(1000)};

      signed_transaction trx;
      trx.set_reference_block(db.head_block_id());
      trx.expiration = db.head_block_time() + 100;
      trx.messages.emplace_back("init1", "sys", vector<AccountName>{}, "CreateAccount", ca);
      db.push_transaction(trx);

      { // test in the pending state
         const auto& joe_account = db.get_account("joe");
         BOOST_CHECK_EQUAL(joe_account.balance, Asset(1000));
         BOOST_CHECK_EQUAL(init1_account.balance, Asset(100000 - 1000));

         const auto& joe_owner_authority = db.get<permission_object, by_owner>(boost::make_tuple(joe_account.id, "owner"));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_owner_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys[0].weight, 1);

         const auto& joe_active_authority = db.get<permission_object, by_owner>(boost::make_tuple(joe_account.id, "active"));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_active_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys[0].weight, 1);
      }

      db.produce_blocks(1); /// verify changes survived creating a new block
      {
         const auto& joe_account = db.get_account("joe");
         BOOST_CHECK_EQUAL(joe_account.balance, Asset(1000));
         BOOST_CHECK_EQUAL(init1_account.balance, Asset(100000 - 1000));

         const auto& joe_owner_authority = db.get<permission_object, by_owner>(boost::make_tuple(joe_account.id, "owner"));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_owner_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys[0].weight, 1);

         const auto& joe_active_authority = db.get<permission_object, by_owner>(boost::make_tuple(joe_account.id, "active"));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_active_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys[0].weight, 1);
      }
} FC_LOG_AND_RETHROW() }

// Simple test of creating/updating a new block producer
BOOST_FIXTURE_TEST_CASE(producer_creation, testing_fixture)
{ try {
      MKDB(db)
      db.produce_blocks();
      BOOST_CHECK_EQUAL(db.head_block_num(), 1);

      signed_transaction trx;
      trx.set_reference_block(db.head_block_id());
      trx.expiration = db.head_block_time() + 100;

      auto producer_priv_key = private_key_type::regenerate(fc::digest("producer"));
      PublicKey producer_pub_key = producer_priv_key.get_public_key();
      Authority producer_authority{1, {{producer_pub_key, 1}}, {}};

      CreateAccount ca{"init0", "producer", producer_authority, producer_authority, {}, Asset(100)};
      trx.messages.emplace_back("init0", "sys", vector<AccountName>{}, "CreateAccount", ca);
      db.push_transaction(trx);
      trx.clear();

      CreateProducer cp{"producer", producer_pub_key};
      trx.messages.emplace_back("producer", "sys", vector<AccountName>{}, "CreateProducer", cp);
      db.push_transaction(trx);

      while (db.head_block_num() < 3) {
         auto& producer = db.get_producer("producer");
         BOOST_CHECK_EQUAL(db.get(producer.owner).name, "producer");
         BOOST_CHECK_EQUAL(producer.signing_key, producer_pub_key);
         BOOST_CHECK_EQUAL(producer.last_aslot, 0);
         BOOST_CHECK_EQUAL(producer.total_missed, 0);
         BOOST_CHECK_EQUAL(producer.last_confirmed_block_num, 0);
         db.produce_blocks();
      }

      auto signing_key = private_key_type::regenerate(fc::digest("producer signing key"));
      PublicKey signing_public_key = signing_key.get_public_key();
      trx.clear();
      trx.messages.emplace_back("producer", "sys", vector<AccountName>{}, "UpdateProducer",
                                UpdateProducer{"producer", signing_key.get_public_key()});
      db.push_transaction(trx);
      auto& producer = db.get_producer("producer");
      BOOST_CHECK_EQUAL(producer.signing_key, signing_public_key);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
