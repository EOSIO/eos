#include <boost/test/unit_test.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/key_value_object.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace eos;
using namespace chain;

BOOST_AUTO_TEST_SUITE(system_contract_tests)

//Simple test of account creation
BOOST_FIXTURE_TEST_CASE(create_account, testing_fixture)
{ try {
      Make_Database(db);
      db.produce_blocks(10);

      BOOST_CHECK_EQUAL(db.get_liquid_balance("init1"), Asset(100000));

      Make_Account(db, joe, init1, Asset(1000));

      { // test in the pending state
         BOOST_CHECK_EQUAL(db.get_liquid_balance("joe"), Asset(1000));
         BOOST_CHECK_EQUAL(db.get_liquid_balance("init1"), Asset(100000 - 1000));

         const auto& joe_owner_authority = db_db.get<permission_object, by_owner>(boost::make_tuple("joe", "owner"));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_owner_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys[0].weight, 1);

         const auto& joe_active_authority = db_db.get<permission_object, by_owner>(boost::make_tuple("joe", "active"));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_active_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys[0].weight, 1);
      }

      db.produce_blocks(1); /// verify changes survived creating a new block
      {
         BOOST_CHECK_EQUAL(db.get_liquid_balance("joe"), Asset(1000));
         BOOST_CHECK_EQUAL(db.get_liquid_balance("init1"), Asset(100000 - 1000));

         const auto& joe_owner_authority = db_db.get<permission_object, by_owner>(boost::make_tuple("joe", "owner"));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_owner_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys[0].weight, 1);

         const auto& joe_active_authority = db_db.get<permission_object, by_owner>(boost::make_tuple("joe", "active"));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_active_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys[0].weight, 1);
      }
} FC_LOG_AND_RETHROW() }

// Simple test to verify a simple transfer transaction works
BOOST_FIXTURE_TEST_CASE(transfer, testing_fixture)
{ try {
      Make_Database(db)

      BOOST_CHECK_EQUAL(db.head_block_num(), 0);
      db.produce_blocks(10);
      BOOST_CHECK_EQUAL(db.head_block_num(), 10);

      SignedTransaction trx;
      BOOST_REQUIRE_THROW(db.push_transaction(trx), transaction_exception); // no messages
      trx.messages.resize(1);
      trx.set_reference_block(db.head_block_id());
      trx.expiration = db.head_block_time() + 100;
      trx.messages[0].sender = "init1";
      trx.messages[0].recipient = config::EosContractName;

      types::Transfer trans = { "init1", "init2", Asset(100), "transfer 100" };

      UInt64 value(5);
      auto packed = fc::raw::pack(value);
      auto unpacked = fc::raw::unpack<UInt64>(packed);
      BOOST_CHECK_EQUAL( value, unpacked );
      trx.messages[0].type = "Transfer";
      trx.setMessage(0, "Transfer", trans);

      auto unpack_trans = trx.messageAs<types::Transfer>(0);

      BOOST_REQUIRE_THROW(db.push_transaction(trx), message_validate_exception); // "fail to notify receiver, init2"
      trx.messages[0].notify = {"init2"};
      trx.setMessage(0, "Transfer", trans);
      db.push_transaction(trx);

      BOOST_CHECK_EQUAL(db.get_liquid_balance("init1"), Asset(100000 - 100));
      BOOST_CHECK_EQUAL(db.get_liquid_balance("init2"), Asset(100000 + 100));
      db.produce_blocks(1);

      BOOST_REQUIRE_THROW(db.push_transaction(trx), transaction_exception); // not unique

      Transfer_Asset(db, init2, init1, Asset(100));
      BOOST_CHECK_EQUAL(db.get_liquid_balance("init1"), Asset(100000));
      BOOST_CHECK_EQUAL(db.get_liquid_balance("init2"), Asset(100000));
} FC_LOG_AND_RETHROW() }

// Simple test of creating/updating a new block producer
BOOST_FIXTURE_TEST_CASE(producer_creation, testing_fixture)
{ try {
      Make_Database(db)
      db.produce_blocks();
      BOOST_CHECK_EQUAL(db.head_block_num(), 1);

      Make_Account(db, producer);
      Make_Producer(db, producer, producer_public_key);

      while (db.head_block_num() < 3) {
         auto& producer = db.get_producer("producer");
         BOOST_CHECK_EQUAL(db.get_producer(producer.owner).owner, "producer");
         BOOST_CHECK_EQUAL(producer.signing_key, producer_public_key);
         BOOST_CHECK_EQUAL(producer.last_aslot, 0);
         BOOST_CHECK_EQUAL(producer.total_missed, 0);
         BOOST_CHECK_EQUAL(producer.last_confirmed_block_num, 0);
         db.produce_blocks();
      }

      Make_Key(signing);
      Update_Producer(db, "producer", signing_public_key);
      auto& producer = db.get_producer("producer");
      BOOST_CHECK_EQUAL(producer.signing_key, signing_public_key);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
