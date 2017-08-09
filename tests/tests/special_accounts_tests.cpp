#include <algorithm>
#include <vector>
#include <iterator>
#include <boost/test/unit_test.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/permission_object.hpp>
#include <eos/chain/key_value_object.hpp>

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

BOOST_AUTO_TEST_SUITE(special_account_tests)

//Check special accounts exits in genesis
BOOST_FIXTURE_TEST_CASE(accounts_exists, testing_fixture)
{ try {

      Make_Blockchain(chain);

      auto nobody = chain_db.find<account_object, by_name>(config::NobodyAccountName);
      BOOST_CHECK(nobody != nullptr);
      const auto& nobody_active_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple(config::NobodyAccountName, config::ActiveName));
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.threshold, 0);
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(nobody_active_authority.auth.keys.size(), 0);

      const auto& nobody_owner_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple(config::NobodyAccountName, config::OwnerName));
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.threshold, 0);
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(nobody_owner_authority.auth.keys.size(), 0);

      // TODO: check for anybody account
      //auto anybody = chain_db.find<account_object, by_name>(config::AnybodyAccountName);
      //BOOST_CHECK(anybody == nullptr);

      auto producers = chain_db.find<account_object, by_name>(config::ProducersAccountName);
      BOOST_CHECK(producers != nullptr);

      auto& gpo = chain_db.get<global_property_object>();

      const auto& producers_active_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple(config::ProducersAccountName, config::ActiveName));
      BOOST_CHECK_EQUAL(producers_active_authority.auth.threshold, config::ProducersAuthorityThreshold);
      BOOST_CHECK_EQUAL(producers_active_authority.auth.accounts.size(), gpo.active_producers.size());
      BOOST_CHECK_EQUAL(producers_active_authority.auth.keys.size(), 0);

      std::vector<AccountName> active_auth;
      for(auto& apw : producers_active_authority.auth.accounts) {
         active_auth.emplace_back(apw.permission.account);
      }

      std::vector<AccountName> diff;
      std::set_difference(
         active_auth.begin(),
         active_auth.end(),
         gpo.active_producers.begin(),
         gpo.active_producers.end(),
         std::inserter(diff, diff.begin())
      );

      BOOST_CHECK_EQUAL(diff.size(), 0);

      const auto& producers_owner_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple(config::ProducersAccountName, config::OwnerName));
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.threshold, 0);
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.keys.size(), 0);

} FC_LOG_AND_RETHROW() }

//Check correct authority when new set of producers are elected
BOOST_FIXTURE_TEST_CASE(producers_authority, testing_fixture)
{ try {

      Make_Blockchain(chain)

      Make_Account(chain, alice);
      Make_Account(chain, bob);
      Make_Account(chain, charlie);

      Make_Account(chain, newproducer1);
      Make_Account(chain, newproducer2);
      Make_Account(chain, newproducer3);
      chain.produce_blocks();

      Make_Producer(chain, newproducer1);
      Make_Producer(chain, newproducer2);
      Make_Producer(chain, newproducer3);

      Approve_Producer(chain, alice, newproducer1, true);
      Approve_Producer(chain, bob, newproducer2, true);
      Approve_Producer(chain, charlie, newproducer3, true);

      chain.produce_blocks(config::BlocksPerRound - chain.head_block_num() );

      auto& gpo = chain_db.get<global_property_object>();

      BOOST_REQUIRE(boost::find(gpo.active_producers, "newproducer1") != gpo.active_producers.end());
      BOOST_REQUIRE(boost::find(gpo.active_producers, "newproducer2") != gpo.active_producers.end());
      BOOST_REQUIRE(boost::find(gpo.active_producers, "newproducer3") != gpo.active_producers.end());

      const auto& producers_active_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple(config::ProducersAccountName, config::ActiveName));
      BOOST_CHECK_EQUAL(producers_active_authority.auth.threshold, config::ProducersAuthorityThreshold);
      BOOST_CHECK_EQUAL(producers_active_authority.auth.accounts.size(), gpo.active_producers.size());
      BOOST_CHECK_EQUAL(producers_active_authority.auth.keys.size(), 0);

      std::vector<AccountName> active_auth;
      for(auto& apw : producers_active_authority.auth.accounts) {
         active_auth.emplace_back(apw.permission.account);
      }

      std::vector<AccountName> diff;
      std::set_difference(
         active_auth.begin(),
         active_auth.end(),
         gpo.active_producers.begin(),
         gpo.active_producers.end(),
         std::inserter(diff, diff.begin())
      );

      BOOST_CHECK_EQUAL(diff.size(), 0);

      const auto& producers_owner_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple(config::ProducersAccountName, config::OwnerName));
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.threshold, 0);
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.accounts.size(), 0);
      BOOST_CHECK_EQUAL(producers_owner_authority.auth.keys.size(), 0);

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()
