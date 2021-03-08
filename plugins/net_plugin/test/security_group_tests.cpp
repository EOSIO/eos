#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_set.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/net_plugin/security_group.hpp>

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
   auto dummy_add = [](eosio::chain::account_name) {};
   auto dummy_remove = [](eosio::chain::account_name) {};
}

BOOST_AUTO_TEST_SUITE(security_group_tests)
using namespace eosio::testing;
BOOST_AUTO_TEST_CASE(test_initial_population) {
   auto populate = create_list({ 1, 2, 3, 4, 5, 6});
   size_t add_count  = 0;
   auto on_add = [&](eosio::chain::account_name name) {
      BOOST_REQUIRE(populate.find(name) != populate.end());
      ++add_count;
   };

   auto on_rem = [&](eosio::chain::account_name name) {
      BOOST_FAIL("Name removed from security group");
   };

   eosio::security_group_manager manager;
   manager.update_cache(1, populate, on_add, on_rem);

   BOOST_REQUIRE_EQUAL(add_count, populate.size());
}

BOOST_AUTO_TEST_CASE(test_remove_all) {
   auto populate = create_list({1, 2, 3, 4, 5, 6});
   eosio::security_group_manager manager;
   manager.update_cache(1, populate, dummy_add, dummy_remove);

   auto check_add = [](eosio::chain::account_name) {
      BOOST_FAIL("Tried to add name from empty list");
   };

   size_t remove_count = 0;
   auto check_remove = [&remove_count](eosio::chain::account_name) { ++remove_count; };

   participant_list_t clear;
   manager.update_cache(2, clear, check_add, check_remove);
   BOOST_REQUIRE_EQUAL(populate.size(), remove_count);
}

BOOST_AUTO_TEST_CASE(test_add_only) {
      auto populate = create_list({1, 2, 3, 4, 5, 6});
      eosio::security_group_manager manager;
      manager.update_cache(1, populate, dummy_add, dummy_remove);

      size_t add_count = 0;
      auto check_add = [&add_count](eosio::chain::account_name) { ++add_count; };
      auto check_remove = [](eosio::chain::account_name) { BOOST_FAIL("Nothing should be removed");};

      auto update = create_list({1, 2, 3, 4, 5, 6, 7, 8, 9});
      manager.update_cache(2, update, check_add, check_remove);
      BOOST_REQUIRE_EQUAL((update.size() - populate.size()), add_count);
}

BOOST_AUTO_TEST_CASE(test_remove_only) {
      auto populate = create_list({1, 2, 3, 4, 5, 6});
      eosio::security_group_manager manager;
      manager.update_cache(1, populate, dummy_add, dummy_remove);

      auto check_add = [](eosio::chain::account_name) { BOOST_FAIL("Nothing should be added");};
      size_t remove_count = 0;
      auto check_remove = [&remove_count](eosio::chain::account_name) { ++remove_count; };

      auto update = create_list({2, 4, 6});
      manager.update_cache(2, update, check_add, check_remove);
      BOOST_REQUIRE_EQUAL(populate.size() - update.size(), remove_count);
}

BOOST_AUTO_TEST_CASE(test_update) {
      auto populate = create_list({1, 2, 3, 4, 5, 6});
      eosio::security_group_manager manager;
      manager.update_cache(1, populate, dummy_add, dummy_remove);

      size_t add_count = 0;
      auto check_add = [&add_count](eosio::chain::account_name) { ++add_count;};
      size_t remove_count = 0;
      auto check_remove = [&remove_count](eosio::chain::account_name) { ++remove_count; };

      auto update = create_list({2, 4, 6, 7, 8, 9});
      manager.update_cache(2, update, check_add, check_remove);
      BOOST_REQUIRE_EQUAL(add_count + remove_count, update.size());
}
BOOST_AUTO_TEST_SUITE_END()
