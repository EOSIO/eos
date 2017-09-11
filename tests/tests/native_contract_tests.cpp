#include <boost/test/unit_test.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/permission_link_object.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/authority_checker.hpp>

#include <eos/native_contract/producer_objects.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/permutation.hpp>

#include "../common/database_fixture.hpp"

using namespace eos;
using namespace chain;


BOOST_AUTO_TEST_SUITE(native_contract_tests)

//Simple test of account creation
BOOST_FIXTURE_TEST_CASE(create_account, testing_fixture)
{ try {
      Make_Blockchain(chain);
      chain.produce_blocks(10);

      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), Asset(100000));

      Make_Account(chain, joe, inita, Asset(1000));
      chain.produce_blocks();
      Transfer_Asset(chain, inita, joe, Asset(1000));

      { // test in the pending state
         BOOST_CHECK_EQUAL(chain.get_liquid_balance("joe"), Asset(1000));
         BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), Asset(100000 - 2000));

         const auto& joe_owner_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple("joe", "owner"));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_owner_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys[0].weight, 1);

         const auto& joe_active_authority =
            chain_db.get<permission_object, by_owner>(boost::make_tuple("joe", "active"));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_active_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_active_authority.auth.keys[0].weight, 1);
      }

      chain.produce_blocks(1); /// verify changes survived creating a new block
      {
         BOOST_CHECK_EQUAL(chain.get_liquid_balance("joe"), Asset(1000));
         BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), Asset(100000 - 2000));

         const auto& joe_owner_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple("joe", "owner"));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.threshold, 1);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.accounts.size(), 0);
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys.size(), 1);
         BOOST_CHECK_EQUAL(string(joe_owner_authority.auth.keys[0].key), string(joe_public_key));
         BOOST_CHECK_EQUAL(joe_owner_authority.auth.keys[0].weight, 1);

         const auto& joe_active_authority =
         chain_db.get<permission_object, by_owner>(boost::make_tuple("joe", "active"));
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
      Make_Blockchain(chain)

      BOOST_CHECK_EQUAL(chain.head_block_num(), 0);
      chain.produce_blocks(10);
      BOOST_CHECK_EQUAL(chain.head_block_num(), 10);

      SignedTransaction trx;
      BOOST_REQUIRE_THROW(chain.push_transaction(trx), transaction_exception); // no messages
      trx.messages.resize(1);
      transaction_set_reference_block(trx, chain.head_block_id());
      trx.expiration = chain.head_block_time() + 100;
      trx.scope = sort_names( {"inita", "initb"} );

      types::transfer trans = { "inita", "initb", (100), "" };

      UInt64 value(5);
      auto packed = fc::raw::pack(value);
      auto unpacked = fc::raw::unpack<UInt64>(packed);
      BOOST_CHECK_EQUAL( value, unpacked );
      trx.messages[0].type = "transfer";
      trx.messages[0].authorization = {{"inita", "active"}};
      trx.messages[0].code = config::EosContractName;
      transaction_set_message(trx, 0, "transfer", trans);
      chain.push_transaction(trx, chain_controller::skip_transaction_signatures);

      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), Asset(100000 - 100));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("initb"), Asset(100000 + 100));
      chain.produce_blocks(1);

      BOOST_REQUIRE_THROW(chain.push_transaction(trx), transaction_exception); // not unique

      Transfer_Asset(chain, initb, inita, Asset(100));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), Asset(100000));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("initb"), Asset(100000));
} FC_LOG_AND_RETHROW() }

// Simple test of creating/updating a new block producer
BOOST_FIXTURE_TEST_CASE(producer_creation, testing_fixture)
{ try {
      Make_Blockchain(chain)
      Make_Account(chain, producer);
      chain.produce_blocks();
      BOOST_CHECK_EQUAL(chain.head_block_num(), 1);

      Make_Producer(chain, producer, producer_public_key);

      while (chain.head_block_num() < 3) {
         auto& producer = chain.get_producer("producer");
         BOOST_CHECK_EQUAL(chain.get_producer(producer.owner).owner, "producer");
         BOOST_CHECK_EQUAL(producer.signing_key, producer_public_key);
         BOOST_CHECK_EQUAL(producer.last_aslot, 0);
         BOOST_CHECK_EQUAL(producer.total_missed, 0);
         BOOST_CHECK_EQUAL(producer.last_confirmed_block_num, 0);
         chain.produce_blocks();
      }

      Make_Key(signing);
      Update_Producer(chain, "producer", signing_public_key);
      auto& producer = chain.get_producer("producer");
      BOOST_CHECK_EQUAL(producer.signing_key, signing_public_key);
} FC_LOG_AND_RETHROW() }

// Test producer votes on blockchain parameters in full blockchain context
BOOST_FIXTURE_TEST_CASE(producer_voting_parameters, testing_fixture)
{ try {
      Make_Blockchain(chain)
      chain.produce_blocks(21);

      vector<BlockchainConfiguration> votes{
         {1024  , 512   , 4096  , Asset(5000   ).amount, Asset(4000   ).amount, Asset(100  ).amount, 512   , 6, 5000, 4, 4096, 65536 },
         {10000 , 100   , 4096  , Asset(3333   ).amount, Asset(27109  ).amount, Asset(10   ).amount, 100   , 6, 5000, 4, 4096, 65536 },
         {2048  , 1500  , 1000  , Asset(5432   ).amount, Asset(2000   ).amount, Asset(50   ).amount, 1500  , 6, 5000, 4, 4096, 65536 },
         {100   , 25    , 1024  , Asset(90000  ).amount, Asset(0      ).amount, Asset(433  ).amount, 25    , 6, 5000, 4, 4096, 65536 },
         {1024  , 1000  , 100   , Asset(10     ).amount, Asset(50     ).amount, Asset(200  ).amount, 1000  , 6, 5000, 4, 4096, 65536 },
         {420   , 400   , 2710  , Asset(27599  ).amount, Asset(1177   ).amount, Asset(27720).amount, 400   , 6, 5000, 4, 4096, 65536 },
         {271   , 200   , 66629 , Asset(2666   ).amount, Asset(99991  ).amount, Asset(277  ).amount, 200   , 6, 5000, 4, 4096, 65536 },
         {1057  , 1000  , 2770  , Asset(972    ).amount, Asset(302716 ).amount, Asset(578  ).amount, 1000  , 6, 5000, 4, 4096, 65536 },
         {9926  , 27    , 990   , Asset(99999  ).amount, Asset(39651  ).amount, Asset(4402 ).amount, 27    , 6, 5000, 4, 4096, 65536 },
         {1005  , 1000  , 1917  , Asset(937111 ).amount, Asset(2734   ).amount, Asset(1    ).amount, 1000  , 6, 5000, 4, 4096, 65536 },
         {80    , 70    , 5726  , Asset(63920  ).amount, Asset(231561 ).amount, Asset(27100).amount, 70    , 6, 5000, 4, 4096, 65536 },
         {471617, 333333, 100   , Asset(2666   ).amount, Asset(2650   ).amount, Asset(2772 ).amount, 33333 , 6, 5000, 4, 4096, 65536 },
         {2222  , 1000  , 100   , Asset(33619  ).amount, Asset(1046   ).amount, Asset(10577).amount, 1000  , 6, 5000, 4, 4096, 65536 },
         {8     , 7     , 100   , Asset(5757267).amount, Asset(2257   ).amount, Asset(2888 ).amount, 7     , 6, 5000, 4, 4096, 65536 },
         {2717  , 2000  , 57797 , Asset(3366   ).amount, Asset(205    ).amount, Asset(4472 ).amount, 2000  , 6, 5000, 4, 4096, 65536 },
         {9997  , 5000  , 27700 , Asset(29199  ).amount, Asset(100    ).amount, Asset(221  ).amount, 5000  , 6, 5000, 4, 4096, 65536 },
         {163900, 200   , 882   , Asset(100    ).amount, Asset(5720233).amount, Asset(105  ).amount, 200   , 6, 5000, 4, 4096, 65536 },
         {728   , 80    , 27100 , Asset(28888  ).amount, Asset(6205   ).amount, Asset(5011 ).amount, 80    , 6, 5000, 4, 4096, 65536 },
         {91937 , 44444 , 652589, Asset(87612  ).amount, Asset(123    ).amount, Asset(2044 ).amount, 44444 , 6, 5000, 4, 4096, 65536 },
         {171   , 96    , 123456, Asset(8402   ).amount, Asset(321    ).amount, Asset(816  ).amount, 96    , 6, 5000, 4, 4096, 65536 },
         {17177 , 6767  , 654321, Asset(9926   ).amount, Asset(9264   ).amount, Asset(8196 ).amount, 6767  , 6, 5000, 4, 4096, 65536 },
      };
      BlockchainConfiguration medians =
         {1057, 512, 2770, Asset(9926).amount, Asset(2650).amount, Asset(816).amount, 512, 6, 5000, 4, 4096, 65536 };
      // If this fails, the medians variable probably needs to be updated to have the medians of the votes above
      BOOST_REQUIRE_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);

      for (int i = 0; i < votes.size(); ++i) {
         auto name = std::string("init") + char('a' + i);
         Update_Producer(chain, name, chain.get_producer(name).signing_key, votes[i]);
      }

      BOOST_CHECK_NE(chain.get_global_properties().configuration, medians);
      chain.produce_blocks(20);
      BOOST_CHECK_NE(chain.get_global_properties().configuration, medians);
      chain.produce_blocks();
      BOOST_CHECK_EQUAL(chain.get_global_properties().configuration, medians);
} FC_LOG_AND_RETHROW() }

// Test producer votes on blockchain parameters in full blockchain context, with missed blocks
BOOST_FIXTURE_TEST_CASE(producer_voting_parameters_2, testing_fixture)
{ try {
      Make_Blockchain(chain)
      chain.produce_blocks(21);

      vector<BlockchainConfiguration> votes{
         {1024  , 512   , 4096  , Asset(5000   ).amount, Asset(4000   ).amount, Asset(100  ).amount, 512    , 6, 5000, 4, 4096, 65536 },
         {10000 , 100   , 4096  , Asset(3333   ).amount, Asset(27109  ).amount, Asset(10   ).amount, 100    , 6, 5000, 4, 4096, 65536 },
         {2048  , 1500  , 1000  , Asset(5432   ).amount, Asset(2000   ).amount, Asset(50   ).amount, 1500   , 6, 5000, 4, 4096, 65536 },
         {100   , 25    , 1024  , Asset(90000  ).amount, Asset(0      ).amount, Asset(433  ).amount, 25     , 6, 5000, 4, 4096, 65536 },
         {1024  , 1000  , 100   , Asset(10     ).amount, Asset(50     ).amount, Asset(200  ).amount, 1000   , 6, 5000, 4, 4096, 65536 },
         {420   , 400   , 2710  , Asset(27599  ).amount, Asset(1177   ).amount, Asset(27720).amount, 400    , 6, 5000, 4, 4096, 65536 },
         {271   , 200   , 66629 , Asset(2666   ).amount, Asset(99991  ).amount, Asset(277  ).amount, 200    , 6, 5000, 4, 4096, 65536 },
         {1057  , 1000  , 2770  , Asset(972    ).amount, Asset(302716 ).amount, Asset(578  ).amount, 1000   , 6, 5000, 4, 4096, 65536 },
         {9926  , 27    , 990   , Asset(99999  ).amount, Asset(39651  ).amount, Asset(4402 ).amount, 27     , 6, 5000, 4, 4096, 65536 },
         {1005  , 1000  , 1917  , Asset(937111 ).amount, Asset(2734   ).amount, Asset(1    ).amount, 1000   , 6, 5000, 4, 4096, 65536 },
         {80    , 70    , 5726  , Asset(63920  ).amount, Asset(231561 ).amount, Asset(27100).amount, 70     , 6, 5000, 4, 4096, 65536 },
         {471617, 333333, 100   , Asset(2666   ).amount, Asset(2650   ).amount, Asset(2772 ).amount, 333333 , 6, 5000, 4, 4096, 65536 },
         {2222  , 1000  , 100   , Asset(33619  ).amount, Asset(1046   ).amount, Asset(10577).amount, 1000   , 6, 5000, 4, 4096, 65536 },
         {8     , 7     , 100   , Asset(5757267).amount, Asset(2257   ).amount, Asset(2888 ).amount, 7      , 6, 5000, 4, 4096, 65536 },
         {2717  , 2000  , 57797 , Asset(3366   ).amount, Asset(205    ).amount, Asset(4472 ).amount, 2000   , 6, 5000, 4, 4096, 65536 },
         {9997  , 5000  , 27700 , Asset(29199  ).amount, Asset(100    ).amount, Asset(221  ).amount, 5000   , 6, 5000, 4, 4096, 65536 },
         {163900, 200   , 882   , Asset(100    ).amount, Asset(5720233).amount, Asset(105  ).amount, 200    , 6, 5000, 4, 4096, 65536 },
         {728   , 80    , 27100 , Asset(28888  ).amount, Asset(6205   ).amount, Asset(5011 ).amount, 80     , 6, 5000, 4, 4096, 65536 },
         {91937 , 44444 , 652589, Asset(87612  ).amount, Asset(123    ).amount, Asset(2044 ).amount, 44444  , 6, 5000, 4, 4096, 65536 },
         {171   , 96    , 123456, Asset(8402   ).amount, Asset(321    ).amount, Asset(816  ).amount, 96     , 6, 5000, 4, 4096, 65536 },
         {17177 , 6767  , 654321, Asset(9926   ).amount, Asset(9264   ).amount, Asset(8196 ).amount, 6767   , 6, 5000, 4, 4096, 65536 },
      };
      BlockchainConfiguration medians =
         {1057, 512, 2770, Asset(9926).amount, Asset(2650).amount, Asset(816).amount, 512, 6, 5000, 4, 4096, 65536 };
      // If this fails, the medians variable probably needs to be updated to have the medians of the votes above
      BOOST_REQUIRE_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);

      for (int i = 0; i < votes.size(); ++i) {
         auto name = std::string("init") + char('a' + i);
         Update_Producer(chain, name, chain.get_producer(name).signing_key, votes[i]);
      }

      BOOST_CHECK_NE(chain.get_global_properties().configuration, medians);
      chain.produce_blocks(2);
      chain.produce_blocks(18, 5);
      BOOST_CHECK_NE(chain.get_global_properties().configuration, medians);
      chain.produce_blocks();
      BOOST_CHECK_EQUAL(chain.get_global_properties().configuration, medians);
} FC_LOG_AND_RETHROW() }

// Test that if I create a producer and vote for him, he gets in on the next round (but not before)
BOOST_FIXTURE_TEST_CASE(producer_voting_1, testing_fixture) {
   try {
      Make_Blockchain(chain)
      Make_Account(chain, joe);
      Make_Account(chain, bob);
      chain.produce_blocks();

      Make_Producer(chain, joe);
      Approve_Producer(chain, bob, joe, true);
      // Produce blocks up to, but not including, the last block in the round
      chain.produce_blocks(config::BlocksPerRound - chain.head_block_num() - 1);

      {
         BOOST_CHECK_EQUAL(chain.get_approved_producers("bob").count("joe"), 1);
         BOOST_CHECK_EQUAL(chain.get_staked_balance("bob"), Asset(100));
         const auto& joeVotes = chain_db.get<native::eos::ProducerVotesObject, native::eos::byOwnerName>("joe");
         BOOST_CHECK_EQUAL(joeVotes.getVotes(), chain.get_staked_balance("bob"));
      }

      // OK, let's go to the next round
      chain.produce_blocks();

      const auto& gpo = chain.get_global_properties();
      BOOST_REQUIRE(boost::find(gpo.active_producers, "joe") != gpo.active_producers.end());

      Approve_Producer(chain, bob, joe, false);
      chain.produce_blocks();

      {
         BOOST_CHECK_EQUAL(chain.get_approved_producers("bob").count("joe"), 0);
         const auto& joeVotes = chain_db.get<native::eos::ProducerVotesObject, native::eos::byOwnerName>("joe");
         BOOST_CHECK_EQUAL(joeVotes.getVotes(), 0);
      }
   } FC_LOG_AND_RETHROW()
}

// Same as producer_voting_1, except we first cast the vote for the producer, _then_ get a stake
BOOST_FIXTURE_TEST_CASE(producer_voting_2, testing_fixture) {
   try {
      Make_Blockchain(chain)
      Make_Account(chain, joe);
      Make_Account(chain, bob);
      chain.produce_blocks();

      Make_Producer(chain, joe);
      Approve_Producer(chain, bob, joe, true);
      chain.produce_blocks();

      {
         BOOST_CHECK_EQUAL(chain.get_approved_producers("bob").count("joe"), 1);
         BOOST_CHECK_EQUAL(chain.get_staked_balance("bob"), Asset(100));
         const auto& joeVotes = chain_db.get<native::eos::ProducerVotesObject, native::eos::byOwnerName>("joe");
         BOOST_CHECK_EQUAL(joeVotes.getVotes(), chain.get_staked_balance("bob"));
      }

      // Produce blocks up to, but not including, the last block in the round
      chain.produce_blocks(config::BlocksPerRound - chain.head_block_num() - 1);

      {
         BOOST_CHECK_EQUAL(chain.get_approved_producers("bob").count("joe"), 1);
         BOOST_CHECK_EQUAL(chain.get_staked_balance("bob"), Asset(100));
         const auto& joeVotes = chain_db.get<native::eos::ProducerVotesObject, native::eos::byOwnerName>("joe");
         BOOST_CHECK_EQUAL(joeVotes.getVotes(), chain.get_staked_balance("bob"));
      }

      // OK, let's go to the next round
      chain.produce_blocks();

      const auto& gpo = chain.get_global_properties();
      BOOST_REQUIRE(boost::find(gpo.active_producers, "joe") != gpo.active_producers.end());

      Approve_Producer(chain, bob, joe, false);
      chain.produce_blocks();

      {
         BOOST_CHECK_EQUAL(chain.get_approved_producers("bob").count("joe"), 0);
         const auto& joeVotes = chain_db.get<native::eos::ProducerVotesObject, native::eos::byOwnerName>("joe");
         BOOST_CHECK_EQUAL(joeVotes.getVotes(), 0);
      }
   } FC_LOG_AND_RETHROW()
}

// Test voting for producers by proxy
BOOST_FIXTURE_TEST_CASE(producer_proxy_voting, testing_fixture) {
   try {
      using Action = std::function<void(testing_blockchain&)>;
      Action approve = [](auto& chain) {
         Approve_Producer(chain, proxy, producer, true);
      };
      Action setproxy = [](auto& chain) {
         Set_Proxy(chain, stakeholder, proxy);
      };
      Action stake = [](auto& chain) {
         Stake_Asset(chain, stakeholder, Asset(100).amount);
      };

      auto run = [this](std::vector<Action> actions) {
         Make_Blockchain(chain)
         Make_Account(chain, stakeholder);
         Make_Account(chain, proxy);
         Make_Account(chain, producer);
         chain.produce_blocks();

         Make_Producer(chain, producer);

         for (auto& action : actions)
            action(chain);

         // Produce blocks up to, but not including, the last block in the round
         chain.produce_blocks(config::BlocksPerRound - chain.head_block_num() - 1);

         {
            BOOST_CHECK_EQUAL(chain.get_approved_producers("proxy").count("producer"), 1);
            BOOST_CHECK_EQUAL(chain.get_staked_balance("stakeholder"), Asset(100));
            const auto& producerVotes = chain_db.get<native::eos::ProducerVotesObject, native::eos::byOwnerName>("producer");
            BOOST_CHECK_EQUAL(producerVotes.getVotes(), chain.get_staked_balance("stakeholder"));
         }

         // OK, let's go to the next round
         chain.produce_blocks();

         const auto& gpo = chain.get_global_properties();
         BOOST_REQUIRE(boost::find(gpo.active_producers, "producer") != gpo.active_producers.end());

         Approve_Producer(chain, proxy, producer, false);
         chain.produce_blocks();

         {
            BOOST_CHECK_EQUAL(chain.get_approved_producers("proxy").count("producer"), 0);
            const auto& producerVotes = chain_db.get<native::eos::ProducerVotesObject, native::eos::byOwnerName>("producer");
            BOOST_CHECK_EQUAL(producerVotes.getVotes(), 0);
         }
      };

      // Eh, I'm not going to try all legal permutations here... just several of them
     /*
      run({approve, setproxy, stake});
      run({approve, setproxy, stake});
      run({approve, stake, setproxy});
      run({setproxy, approve, stake});
      run({setproxy, stake, approve});
      run({stake, setproxy, approve});
      run({stake, approve, setproxy});
      // Make sure it throws if I try to proxy to an account before the account accepts proxying
      BOOST_CHECK_THROW(run({setproxy, approve, stake}), chain::message_precondition_exception);
      */
   } FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(auth_tests, testing_fixture) {
   try {
   Make_Blockchain(chain)
   Make_Account(chain, alice);
   chain.produce_blocks();

   BOOST_CHECK_THROW(Delete_Authority(chain, alice, "active"), message_validate_exception);
   BOOST_CHECK_THROW(Delete_Authority(chain, alice, "owner"), message_validate_exception);

   Make_Key(k1);
   Make_Key(k2);
   Set_Authority(chain, alice, "spending", "active", Key_Authority(k1_public_key));

   // Ensure authority doesn't appear until next block
   BOOST_CHECK_EQUAL((chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"))), nullptr);
   chain.produce_blocks();

   {
      auto obj = chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"));
      BOOST_CHECK_NE(obj, nullptr);
      BOOST_CHECK_EQUAL(obj->owner, "alice");
      BOOST_CHECK_EQUAL(obj->name, "spending");
      BOOST_CHECK_EQUAL(chain_db.get(obj->parent).owner, "alice");
      BOOST_CHECK_EQUAL(chain_db.get(obj->parent).name, "active");
   }

   BOOST_CHECK_THROW(Set_Authority(chain, alice, "spending", "spending", Key_Authority(k1_public_key)),
                     message_validate_exception);
   BOOST_CHECK_THROW(Set_Authority(chain, alice, "spending", "owner", Key_Authority(k1_public_key)),
                     message_precondition_exception);
   Delete_Authority(chain, alice, "spending");
   BOOST_CHECK_NE((chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"))), nullptr);
   chain.produce_blocks();

   {
      auto obj = chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"));
      BOOST_CHECK_EQUAL(obj, nullptr);
   }

   chain.produce_blocks();

   Set_Authority(chain, alice, "trading", "active", Key_Authority(k1_public_key));
   BOOST_CHECK_EQUAL((chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "trading"))), nullptr);
   chain.produce_blocks();
   Set_Authority(chain, alice, "spending", "trading", Key_Authority(k2_public_key));
   BOOST_CHECK_EQUAL((chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"))), nullptr);
   chain.produce_blocks();

   {
      auto trading = chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "trading"));
      auto spending = chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"));
      BOOST_CHECK_NE(trading, nullptr);
      BOOST_CHECK_NE(spending, nullptr);
      BOOST_CHECK_EQUAL(trading->owner, "alice");
      BOOST_CHECK_EQUAL(spending->owner, "alice");
      BOOST_CHECK_EQUAL(trading->name, "trading");
      BOOST_CHECK_EQUAL(spending->name, "spending");
      BOOST_CHECK_EQUAL(spending->parent, trading->id);
      BOOST_CHECK_EQUAL(chain_db.get(trading->parent).owner, "alice");
      BOOST_CHECK_EQUAL(chain_db.get(trading->parent).name, "active");

      // Abort if this gets run; it shouldn't get run in this test
      auto PermissionToAuthority = [](auto)->Authority{abort();};

      auto tradingChecker = MakeAuthorityChecker(PermissionToAuthority, 2, {k1_public_key});
      auto spendingChecker = MakeAuthorityChecker(PermissionToAuthority, 2, {k2_public_key});

      BOOST_CHECK(tradingChecker.satisfied(trading->auth));
      BOOST_CHECK(spendingChecker.satisfied(spending->auth));
      BOOST_CHECK(!spendingChecker.satisfied(trading->auth));
      BOOST_CHECK(!tradingChecker.satisfied(spending->auth));
   }

   BOOST_CHECK_THROW(Delete_Authority(chain, alice, "trading"), message_precondition_exception);
   BOOST_CHECK_THROW(Set_Authority(chain, alice, "trading", "spending", Key_Authority(k1_public_key)),
                     message_precondition_exception);
   BOOST_CHECK_THROW(Set_Authority(chain, alice, "spending", "active", Key_Authority(k1_public_key)),
                     message_precondition_exception);
   Delete_Authority(chain, alice, "spending");
   BOOST_CHECK_NE((chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"))), nullptr);
   chain.produce_blocks();
   Delete_Authority(chain, alice, "trading");
   BOOST_CHECK_NE((chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "trading"))), nullptr);
   chain.produce_blocks();

   {
      auto trading = chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "trading"));
      auto spending = chain_db.find<permission_object, by_owner>(boost::make_tuple("alice", "spending"));
      BOOST_CHECK_EQUAL(trading, nullptr);
      BOOST_CHECK_EQUAL(spending, nullptr);
   }
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(auth_links, testing_fixture) { try {
   Make_Blockchain(chain);
   Make_Account(chain, alice);
   Make_Account(chain, bob);
   chain.produce_blocks();

   Make_Key(spending);
   Make_Key(scud);

   Set_Authority(chain, alice, "spending", "active", Key_Authority(spending_public_key));
   chain.produce_blocks();
   Set_Authority(chain, alice, "scud", "spending", Key_Authority(scud_public_key));
   chain.produce_blocks();
   Link_Authority(chain, alice, "spending", eos, "transfer");
   BOOST_CHECK_EQUAL(
           (chain_db.find<permission_link_object, by_message_type>(boost::make_tuple("alice", "eos", "transfer"))),
            nullptr);
   chain.produce_blocks();

   {
      auto obj = chain_db.find<permission_link_object, by_message_type>(boost::make_tuple("alice", "eos", "transfer"));
      BOOST_CHECK_NE(obj, nullptr);
      BOOST_CHECK_EQUAL(obj->account, "alice");
      BOOST_CHECK_EQUAL(obj->code, "eos");
      BOOST_CHECK_EQUAL(obj->message_type, "transfer");
   }

   Transfer_Asset(chain, inita, alice, Asset(1000));
   chain.produce_blocks();
   // Take off the training wheels, we're gonna fully validate transactions now
   chain.set_auto_sign_transactions(false);
   chain.set_skip_transaction_signature_checking(false);
   chain.set_hold_transactions_for_review(true);

   // This won't run yet; it'll get held for review
   Transfer_Asset(chain, alice, bob, Asset(10));
   // OK, set the above transfer's authorization level to scud and check that it fails
   BOOST_CHECK_THROW(chain.review_transaction([&chain](SignedTransaction& trx, auto) {
                        trx.messages.front().authorization = {{"alice", "scud"}};
                        chain.sign_transaction(trx);
                        return true;
                     }), tx_irrelevant_auth);
   // OK, now set the auth level to spending, and it should succeed
   chain.review_transaction([&chain](SignedTransaction& trx, auto) {
      trx.messages.front().authorization = {{"alice", "spending"}};
      trx.signatures.clear();
      chain.sign_transaction(trx);
      return true;
   });
   // Finally, set it to active, it should still succeed
   chain.review_transaction([&chain](SignedTransaction& trx, auto) {
      trx.messages.front().authorization = {{"alice", "active"}};
      trx.signatures.clear();
      chain.sign_transaction(trx);
      return true;
   });

   BOOST_CHECK_EQUAL(chain.get_liquid_balance("bob"), Asset(20));

   SignedTransaction backup;
   // Now we'll lock some funds, but we'll use the scud authority to do it. First, this should fail, but back up the
   // transaction for later
   Stake_Asset(chain, alice, 100);
   BOOST_CHECK_THROW(chain.review_transaction([&chain, &backup](SignedTransaction& trx, auto) {
                        trx.messages.front().authorization = {{"alice", "scud"}};
                        trx.scope = {"alice", config::EosContractName};
                        chain.sign_transaction(trx);
                        backup = trx;
                        return true;
                     }), tx_irrelevant_auth);
   // Now set the default authority to scud...
   Link_Authority(chain, alice, "scud", eos);
   chain.review_transaction([&chain](SignedTransaction& trx, auto) { chain.sign_transaction(trx); return true; });
   chain.produce_blocks();
   // And now the backed up transaction should succeed, because scud is sufficient authority for all except "transfer"
   chain.chain_controller::push_transaction(backup);

   // But transfers with scud authority should still not work, because there's an overriding link to spending
   Transfer_Asset(chain, alice, bob, Asset(10));
   BOOST_CHECK_THROW(chain.review_transaction([&chain](SignedTransaction& trx, auto) {
                        trx.messages.front().authorization = {{"alice", "scud"}};
                        chain.sign_transaction(trx);
                        return true;
                     }), tx_irrelevant_auth);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
