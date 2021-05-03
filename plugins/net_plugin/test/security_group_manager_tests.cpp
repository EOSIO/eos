#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_set.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/net_plugin/security_group_manager.hpp>

#include <array>
#include <utility>
#include <vector>

#include <eosio/testing/backing_store_tester_macros.hpp>
#include <eosio/testing/eosio_system_tester.hpp>
#include <fc/log/log_message.hpp>
#include <fc/log/logger.hpp>

namespace {
   using participant_list_t = boost::container::flat_set<eosio::chain::account_name>;
   participant_list_t create_list(std::vector<uint64_t> participants) {
      participant_list_t participant_list;
      for(auto participant : participants) {
         participant_list.emplace(participant);
      }
      return participant_list;
   }
}

BOOST_AUTO_TEST_SUITE(security_group_tests)
using namespace eosio::testing;

BOOST_AUTO_TEST_CASE(test_initial_population) {
   auto populate = create_list({ 1, 2, 3, 4, 5, 6});
   eosio::security_group_manager manager;
   BOOST_REQUIRE(manager.update_cache(1, populate));
   BOOST_REQUIRE(!manager.update_cache(1, populate));

   for(auto participant : populate) {
      BOOST_REQUIRE(manager.is_in_security_group(participant));
   }
}

BOOST_AUTO_TEST_CASE(test_version) {
   eosio::security_group_manager manager;
   BOOST_REQUIRE(manager.current_version() == 0);

   auto populate = create_list({ 1, 2, 3, 4, 5, 6});
   BOOST_REQUIRE(manager.update_cache(1, populate));
   BOOST_REQUIRE(manager.current_version() == 1);
}

BOOST_AUTO_TEST_CASE(test_remove_all) {
   auto populate = create_list({1, 2, 3, 4, 5, 6});
   eosio::security_group_manager manager;
   manager.update_cache(1, populate);

   participant_list_t clear;
   BOOST_REQUIRE(manager.update_cache(2, clear));

   for(auto participant : populate) {
      BOOST_REQUIRE(manager.is_in_security_group(participant));
   }
}

BOOST_AUTO_TEST_CASE(test_add_only) {
      auto populate = create_list({1, 2, 3, 4, 5, 6});
      eosio::security_group_manager manager;
      manager.update_cache(1, populate);

      auto add = create_list({7, 8, 9});
      for(auto participant : add) {
         BOOST_REQUIRE(!manager.is_in_security_group(participant));
      }

      populate.insert(add.begin(), add.end());
      manager.update_cache(2, populate);
      for(auto participant : populate) {
         BOOST_REQUIRE(manager.is_in_security_group(participant));
      }
}

BOOST_AUTO_TEST_CASE(test_remove_only) {
      auto populate = create_list({1, 2, 3, 4, 5, 6});
      eosio::security_group_manager manager;
      manager.update_cache(1, populate);

      auto update = create_list({2, 4, 6});
      manager.update_cache(2, update);

      auto removed = create_list({1, 3, 5});
      for(auto participant : removed) {
         BOOST_REQUIRE(!manager.is_in_security_group(participant));
      }

      for (auto participant : update) {
         BOOST_REQUIRE(manager.is_in_security_group(participant));
      }
}

BOOST_AUTO_TEST_CASE(test_update) {
      auto populate = create_list({1, 2, 3, 4, 5, 6});
      eosio::security_group_manager manager;
      manager.update_cache(1, populate);

      auto update = create_list({2, 4, 6, 7, 8, 9});
      manager.update_cache(2, update);

      auto removed = create_list({1, 3, 5});
      for(auto participant : removed) {
         BOOST_REQUIRE(!manager.is_in_security_group(participant));
      }

      auto added = create_list({7, 8, 9});
      for (auto participant : added) {
         BOOST_REQUIRE(manager.is_in_security_group(participant));
      }
}
BOOST_AUTO_TEST_SUITE_END()
