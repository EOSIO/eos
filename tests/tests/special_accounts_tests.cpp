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
      auto threshold = uint32_t(gpo.active_producers.size()*config::ProducersAuthorityThreshold);

      const auto& producers_active_authority = chain_db.get<permission_object, by_owner>(boost::make_tuple(config::ProducersAccountName, config::ActiveName));
      BOOST_CHECK_EQUAL(producers_active_authority.auth.threshold, threshold);
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
