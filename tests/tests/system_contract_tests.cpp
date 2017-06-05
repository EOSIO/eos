#include <boost/test/unit_test.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/key_value_object.hpp>

#include <eos/native_system_contract_plugin/native_system_contract_plugin.hpp>

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
      auto model = db.get_model();

      const auto& init1_account = model.get_account("init1");
      BOOST_CHECK_EQUAL(init1_account.balance, Asset(100000));

      Make_Account(db, joe, init1, Asset(1000));

      { // test in the pending state
         const auto& joe_account = model.get_account("joe");
         BOOST_CHECK_EQUAL(joe_account.balance, Asset(1000));
         BOOST_CHECK_EQUAL(init1_account.balance, Asset(100000 - 1000));

         const auto& joe_owner_authority = model.get<permission_object, by_owner>(boost::make_tuple(joe_account.id, "owner"));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_owner_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys[0].weight, 1);

         const auto& joe_active_authority = model.get<permission_object, by_owner>(boost::make_tuple(joe_account.id, "active"));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_active_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys[0].weight, 1);
      }

      db.produce_blocks(1); /// verify changes survived creating a new block
      {
         const auto& joe_account = model.get_account("joe");
         BOOST_CHECK_EQUAL(joe_account.balance, Asset(1000));
         BOOST_CHECK_EQUAL(init1_account.balance, Asset(100000 - 1000));

         const auto& joe_owner_authority = model.get<permission_object, by_owner>(boost::make_tuple(joe_account.id, "owner"));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_owner_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys[0].weight, 1);

         const auto& joe_active_authority = model.get<permission_object, by_owner>(boost::make_tuple(joe_account.id, "active"));
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
      auto model = db.get_model();

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
      trx.messages[0].type = "Undefined";
      BOOST_REQUIRE_THROW( db.push_transaction(trx), message_validate_exception ); // "Type Undefined is not defined"

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

      BOOST_CHECK_EQUAL(model.get_account("init1").balance, Asset(100000 - 100));
      BOOST_CHECK_EQUAL(model.get_account("init2").balance, Asset(100000 + 100));
      db.produce_blocks(1);

      BOOST_REQUIRE_THROW(db.push_transaction(trx), transaction_exception); // not unique

      Transfer_Asset(db, init2, init1, Asset(100));
      BOOST_CHECK_EQUAL(model.get_account("init1").balance, Asset(100000));
      BOOST_CHECK_EQUAL(model.get_account("init2").balance, Asset(100000));
} FC_LOG_AND_RETHROW() }

// Simple test of creating/updating a new block producer
BOOST_FIXTURE_TEST_CASE(producer_creation, testing_fixture)
{ try {
      Make_Database(db)
      auto model = db.get_model();
      db.produce_blocks();
      BOOST_CHECK_EQUAL(db.head_block_num(), 1);

      Make_Account(db, producer);
      Make_Producer(db, producer, producer_public_key);

      while (db.head_block_num() < 3) {
         auto& producer = model.get_producer("producer");
         BOOST_CHECK_EQUAL(model.get(producer.owner).name, "producer");
         BOOST_CHECK_EQUAL(producer.signing_key, producer_public_key);
         BOOST_CHECK_EQUAL(producer.last_aslot, 0);
         BOOST_CHECK_EQUAL(producer.total_missed, 0);
         BOOST_CHECK_EQUAL(producer.last_confirmed_block_num, 0);
         db.produce_blocks();
      }

      Make_Key(signing);
      Update_Producer(db, "producer", signing_public_key);
      auto& producer = model.get_producer("producer");
      BOOST_CHECK_EQUAL(producer.signing_key, signing_public_key);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
