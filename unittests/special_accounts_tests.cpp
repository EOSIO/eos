/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <algorithm>
#include <vector>
#include <iterator>
#include <boost/test/unit_test.hpp>

#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/global_property_object.hpp>

#include <eosio/testing/tester.hpp>

#include <eosio/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/permutation.hpp>

using namespace eosio;
using namespace chain;
using tester = eosio::testing::tester;

BOOST_AUTO_TEST_SUITE(special_account_tests)

//Check special accounts exits in genesis
BOOST_FIXTURE_TEST_CASE(accounts_exists, tester)
{ try {

      tester test;
      chain::controller *control = test.control.get();
      const chain::database& chain1_db = control->db();

      auto nobody = chain1_db.find<account_object, by_name>(config::null_account_name);
      BOOST_CHECK(nobody != nullptr);
      const auto& nobody_active_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::null_account_name, config::active_name));
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.threshold, 1);
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.keys.size(), 0);

      const auto& nobody_owner_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::null_account_name, config::owner_name));
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.threshold, 1);
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.keys.size(), 0);

      auto producers = chain1_db.find<account_object, by_name>(config::producers_account_name);
      BOOST_CHECK(producers != nullptr);

      const auto& active_producers = control->head_block_state()->active_schedule;

      const auto& producers_active_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::producers_account_name, config::active_name));
      auto expected_threshold = (active_producers.producers.size() * 2)/3 + 1;
      BOOST_CHECK_EQUAL(producers_active_authority.auth.threshold, expected_threshold);
      BOOST_CHECK_EQUAL(producers_active_authority.auth.accounts.size(), active_producers.producers.size());
      BOOST_CHECK_EQUAL(producers_active_authority.auth.keys.size(), 0);

      std::vector<account_name> active_auth;
      for(auto& apw : producers_active_authority.auth.accounts) {
         active_auth.emplace_back(apw.permission.actor);
      }

      std::vector<account_name> diff;
      for (int i = 0; i < std::max(active_auth.size(), active_producers.producers.size()); ++i) {
         account_name n1 = i < active_auth.size() ? active_auth[i] : (account_name)0;
         account_name n2 = i < active_producers.producers.size() ? active_producers.producers[i].producer_name : (account_name)0;
         if (n1 != n2) diff.push_back((uint64_t)n2 - (uint64_t)n1);
      }

      BOOST_CHECK_EQUAL(diff.size(), 0);

      const auto& producers_owner_authority = chain1_db.get<permission_object, by_owner>(boost::make_tuple(config::producers_account_name, config::owner_name));
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.threshold, 1);
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.keys.size(), 0);

      //TODO: Add checks on the other permissions of the producers account

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
