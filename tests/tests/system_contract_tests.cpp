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

      MKACCT(db, joe, init1, Asset(1000));

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

// Simple test to verify a simple transfer transaction works
BOOST_FIXTURE_TEST_CASE(transfer, testing_fixture)
{ try {
      MKDB(db)

      BOOST_CHECK_EQUAL(db.head_block_num(), 0);
      db.produce_blocks(10);
      BOOST_CHECK_EQUAL(db.head_block_num(), 10);

      signed_transaction trx;
      BOOST_REQUIRE_THROW(db.push_transaction(trx), transaction_exception); // no messages
      trx.messages.resize(1);
      trx.set_reference_block(db.head_block_id());
      trx.expiration = db.head_block_time() + 100;
      trx.messages[0].sender = "init1";
      trx.messages[0].recipient = "sys";
      trx.messages[0].type = "Undefined";
      BOOST_REQUIRE_THROW( db.push_transaction(trx), message_validate_exception ); // "Type Undefined is not defined"

      Transfer trans = { "init1", "init2", Asset(100), "transfer 100" };

      UInt64 value(5);
      auto packed = fc::raw::pack(value);
      auto unpacked = fc::raw::unpack<UInt64>(packed);
      BOOST_CHECK_EQUAL( value, unpacked );
      trx.messages[0].type = "Transfer";
      trx.messages[0].set("Transfer", trans );

      auto unpack_trans = trx.messages[0].as<Transfer>();

      BOOST_REQUIRE_THROW(db.push_transaction(trx), message_validate_exception); // "fail to notify receiver, init2"
      trx.messages[0].notify = {"init2"};
      trx.messages[0].set("Transfer", trans );
      db.push_transaction(trx);

      BOOST_CHECK_EQUAL(db.get_account("init1").balance, Asset(100000 - 100));
      BOOST_CHECK_EQUAL(db.get_account("init2").balance, Asset(100000 + 100));
      db.produce_blocks(1);

      BOOST_REQUIRE_THROW(db.push_transaction(trx), transaction_exception); // not unique

      XFER(db, init2, init1, Asset(100));
      BOOST_CHECK_EQUAL(db.get_account("init1").balance, Asset(100000));
      BOOST_CHECK_EQUAL(db.get_account("init2").balance, Asset(100000));
} FC_LOG_AND_RETHROW() }

// Simple test of creating/updating a new block producer
BOOST_FIXTURE_TEST_CASE(producer_creation, testing_fixture)
{ try {
      MKDB(db)
      db.produce_blocks();
      BOOST_CHECK_EQUAL(db.head_block_num(), 1);

      MKACCT(db, producer);
      MKPDCR(db, producer, producer_public_key);

      while (db.head_block_num() < 3) {
         auto& producer = db.get_producer("producer");
         BOOST_CHECK_EQUAL(db.get(producer.owner).name, "producer");
         BOOST_CHECK_EQUAL(producer.signing_key, producer_public_key);
         BOOST_CHECK_EQUAL(producer.last_aslot, 0);
         BOOST_CHECK_EQUAL(producer.total_missed, 0);
         BOOST_CHECK_EQUAL(producer.last_confirmed_block_num, 0);
         db.produce_blocks();
      }

      MKKEY(signing);
      UPPDCR(db, producer, signing_public_key);
      auto& producer = db.get_producer("producer");
      BOOST_CHECK_EQUAL(producer.signing_key, signing_public_key);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
