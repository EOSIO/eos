/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <algorithm>
#include <vector>
#include <iterator>
#include <boost/test/unit_test.hpp>

#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/permission_object.hpp>

#include <eosio/testing/tester.hpp>
//#include <eosio/chain/key_value_object.hpp>

//#include <eosio/chain/producer_objects.hpp>

#include <eosio/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/permutation.hpp>

//#include "../common/database_fixture.hpp"

using namespace eosio;
using namespace chain;
using tester = eosio::testing::tester;

BOOST_AUTO_TEST_SUITE(special_account_tests)

//Check special accounts exits in genesis
BOOST_FIXTURE_TEST_CASE(accounts_exists, tester)
{ try {

      tester test;
      chain::chain_controller *control = test.control.get();
      chain::database &chain1_db = control->get_mutable_database();
      //chain::fork_database &fdb = control->_fork_db;
      //chain::block_log &blog = control->_block_log;

      //Make_Blockchain(chain1);

      auto nobody = chain1_db.find<account_object, by_name>(config::nobody_account_name);
      BOOST_CHECK(nobody != nullptr);
      const auto& nobody_active_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::nobody_account_name, config::active_name));
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.threshold, 0);
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.keys.size(), 0);

      const auto& nobody_owner_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::nobody_account_name, config::owner_name));
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.threshold, 0);
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.keys.size(), 0);

      // TODO: check for anybody account
      //auto anybody = chain1_db.find<account_object, by_name>(config::anybody_account_name);
      //BOOST_CHECK(anybody == nullptr);

      auto producers = chain1_db.find<account_object, by_name>(config::producers_account_name);
      BOOST_CHECK(producers != nullptr);

      auto& gpo = chain1_db.get<global_property_object>();

      const auto& producers_active_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::producers_account_name, config::active_name));
      BOOST_CHECK_EQUAL(producers_active_authority.auth.threshold, config::producers_authority_threshold);
      BOOST_CHECK_EQUAL(producers_active_authority.auth.accounts.size(), gpo.active_producers.producers.size());
      BOOST_CHECK_EQUAL(producers_active_authority.auth.keys.size(), 0);

      std::vector<account_name> active_auth;
      for(auto& apw : producers_active_authority.auth.accounts) {
         active_auth.emplace_back(apw.permission.actor);
      }

      std::vector<account_name> diff;
      for (int i = 0; i < std::max(active_auth.size(), gpo.active_producers.producers.size()); ++i) {
         account_name n1 = i < active_auth.size() ? active_auth[i] : (account_name)0;
         account_name n2 = i < gpo.active_producers.producers.size() ? gpo.active_producers.producers[i].producer_name : (account_name)0;
         if (n1 != n2) diff.push_back((uint64_t)n2 - (uint64_t)n1);
      }

      BOOST_CHECK_EQUAL(diff.size(), 0);

      const auto& producers_owner_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::producers_account_name, config::owner_name));
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.threshold, 0);
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.keys.size(), 0);

} FC_LOG_AND_RETHROW() }

//Check correct authority when new set of producers are elected
BOOST_FIXTURE_TEST_CASE(producers_authority, tester)
{ try {
      tester test;
      chain::chain_controller *control = test.control.get();
      chain::database &chain1_db = control->get_mutable_database();

      test.create_account(N(alice));
      test.create_account(N(bob));
      test.create_account(N(charlie));
      test.create_account(N(newproducer1));
      test.create_account(N(newproducer2));
      test.create_account(N(newproducer3));

      // Make_Blockchain(chain)

      // Make_Account(chain, alice);
      // Make_Account(chain, bob);
      // Make_Account(chain, charlie);

      // Make_Account(chain, newproducer1);
      // Make_Account(chain, newproducer2);
      // Make_Account(chain, newproducer3);

      test.produce_block();

      //chain.produce_blocks();

      // Make_Producer(chain, newproducer1);
      // Make_Producer(chain, newproducer2);
      // Make_Producer(chain, newproducer3);

      // Approve_Producer(chain, alice, newproducer1, true);
      // Approve_Producer(chain, bob, newproducer2, true);
      // Approve_Producer(chain, charlie, newproducer3, true);

      // chain.produce_blocks(config::blocks_per_round - chain.head_block_num() );

      // auto& gpo = chain_db.get<global_property_object>();

      // BOOST_REQUIRE(boost::find(gpo.active_producers, "newproducer1") != gpo.active_producers.end());
      // BOOST_REQUIRE(boost::find(gpo.active_producers, "newproducer2") != gpo.active_producers.end());
      // BOOST_REQUIRE(boost::find(gpo.active_producers, "newproducer3") != gpo.active_producers.end());

      // const auto& producers_active_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple(config::producers_account_name, config::active_name));
      // BOOST_CHECK_EQUAL(producers_active_authority.auth.threshold, config::producers_authority_threshold);
      // BOOST_CHECK_EQUAL(producers_active_authority.auth.accounts.size(), gpo.active_producers.size());
      // BOOST_CHECK_EQUAL(producers_active_authority.auth.keys.size(), 0);

      // std::vector<account_name> active_auth;
      // for(auto& apw : producers_active_authority.auth.accounts) {
      //    active_auth.emplace_back(apw.permission.account);
      // }

      // std::vector<account_name> diff;
      // std::set_difference(
      //    active_auth.begin(),
      //    active_auth.end(),
      //    gpo.active_producers.begin(),
      //    gpo.active_producers.end(),
      //    std::inserter(diff, diff.begin())
      // );

      // BOOST_CHECK_EQUAL(diff.size(), 0);

      // const auto& producers_owner_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple(config::producers_account_name, config::owner_name));
      // BOOST_CHECK_EQUAL(producers_owner_authority.auth.threshold, 0);
      // BOOST_CHECK_EQUAL(producers_owner_authority.auth.accounts.size(), 0);
      // BOOST_CHECK_EQUAL(producers_owner_authority.auth.keys.size(), 0);

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
