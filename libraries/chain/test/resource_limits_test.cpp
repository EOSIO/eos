#include <boost/test/unit_test.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/test/chainbase_fixture.hpp>

#include <algorithm>

using namespace eosio::chain::resource_limits;
using namespace eosio::chain::test;
using namespace eosio::chain;



class resource_limits_fixture: private chainbase_fixture<512*1024>, public resource_limits_manager
{
   public:
      resource_limits_fixture()
      :chainbase_fixture()
      ,resource_limits_manager(*chainbase_fixture::_db)
      {
         initialize_database();
         initialize_chain();
      }

      ~resource_limits_fixture() {}

      chainbase::database::session start_session() {
         return chainbase_fixture::_db->start_undo_session(true);
      }
};

constexpr int expected_elastic_iterations(uint64_t from, uint64_t to, uint64_t rate_num, uint64_t rate_den ) {
   int result = 0;
   uint64_t cur = from;

   while((from < to && cur < to) || (from > to && cur > to)) {
      cur = cur * rate_num / rate_den;
      result += 1;
   }

   return result;
}


constexpr int expected_exponential_average_iterations( uint64_t from, uint64_t to, uint64_t value, uint64_t window_size ) {
   int result = 0;
   uint64_t cur = from;

   while((from < to && cur < to) || (from > to && cur > to)) {
      cur = cur * (uint128_t)(window_size - 1) / (uint64_t)(window_size);
      cur += value / (uint64_t)(window_size);
      result += 1;
   }

   return result;
}

BOOST_AUTO_TEST_SUITE(resource_limits_test)

   /**
    * Test to make sure that the elastic limits for blocks relax and contract as expected
    */
   BOOST_FIXTURE_TEST_CASE(elastic_cpu_relax_contract, resource_limits_fixture) try {
      const uint64_t desired_virtual_limit = config::default_max_block_cpu_usage * 1000ULL;
      const int expected_relax_iterations = expected_elastic_iterations( config::default_max_block_cpu_usage, desired_virtual_limit, 1000, 999 );

      // this is enough iterations for the average to reach/exceed the target (triggering congestion handling) and then the iterations to contract down to the min
      // subtracting 1 for the iteration that pulls double duty as reaching/exceeding the target and starting congestion handling
      const int expected_contract_iterations =
              expected_exponential_average_iterations(0, config::default_target_block_cpu_usage, config::default_max_block_cpu_usage, config::block_cpu_usage_average_window_ms / config::block_interval_ms ) +
              expected_elastic_iterations( desired_virtual_limit, config::default_max_block_cpu_usage, 99, 100 ) - 1;

      // relax from the starting state (congested) to the idle state as fast as possible
      int iterations = 0;
      while (get_virtual_block_cpu_limit() < desired_virtual_limit && iterations <= expected_relax_iterations) {
         add_block_usage(0,0,iterations++);
      }

      BOOST_REQUIRE_EQUAL(iterations, expected_relax_iterations);
      BOOST_REQUIRE_EQUAL(get_virtual_block_cpu_limit(), desired_virtual_limit);

      // push maximum resources to go from idle back to congested as fast as possible
      iterations = 0;
      while (get_virtual_block_cpu_limit() > config::default_max_block_cpu_usage && iterations <= expected_contract_iterations) {
         add_block_usage(config::default_max_block_cpu_usage, 0, iterations++);
      }

      BOOST_REQUIRE_EQUAL(iterations, expected_contract_iterations);
      BOOST_REQUIRE_EQUAL(get_virtual_block_cpu_limit(), config::default_max_block_cpu_usage);
   } FC_LOG_AND_RETHROW();

   /**
    * Test to make sure that the elastic limits for blocks relax and contract as expected
    */
   BOOST_FIXTURE_TEST_CASE(elastic_net_relax_contract, resource_limits_fixture) try {
      const uint64_t desired_virtual_limit = config::default_max_block_size * 1000ULL;
      const int expected_relax_iterations = expected_elastic_iterations( config::default_max_block_size, desired_virtual_limit, 1000, 999 );

      // this is enough iterations for the average to reach/exceed the target (triggering congestion handling) and then the iterations to contract down to the min
      // subtracting 1 for the iteration that pulls double duty as reaching/exceeding the target and starting congestion handling
      const int expected_contract_iterations =
              expected_exponential_average_iterations(0, config::default_target_block_size, config::default_max_block_size, config::block_size_average_window_ms / config::block_interval_ms ) +
              expected_elastic_iterations( desired_virtual_limit, config::default_max_block_size, 99, 100 ) - 1;

      // relax from the starting state (congested) to the idle state as fast as possible
      int iterations = 0;
      while (get_virtual_block_net_limit() < desired_virtual_limit && iterations <= expected_relax_iterations) {
         add_block_usage(0,0,iterations++);
      }

      BOOST_REQUIRE_EQUAL(iterations, expected_relax_iterations);
      BOOST_REQUIRE_EQUAL(get_virtual_block_net_limit(), desired_virtual_limit);

      // push maximum resources to go from idle back to congested as fast as possible
      iterations = 0;
      while (get_virtual_block_net_limit() > config::default_max_block_size && iterations <= expected_contract_iterations) {
         add_block_usage(0, config::default_max_block_size, iterations++);
      }

      BOOST_REQUIRE_EQUAL(iterations, expected_contract_iterations);
      BOOST_REQUIRE_EQUAL(get_virtual_block_net_limit(), config::default_max_block_size);
   } FC_LOG_AND_RETHROW();

   /**
    * create 5 accounts with different weights, verify that the capacities are as expected and that usage properly enforces them
    */
   BOOST_FIXTURE_TEST_CASE(weighted_capacity_cpu, resource_limits_fixture) try {
      const vector<int64_t> weights = { 234, 511, 672, 800, 1213 };
      const int64_t total = std::accumulate(std::begin(weights), std::end(weights), 0LL);
      vector<int64_t> expected_limits;
      std::transform(std::begin(weights), std::end(weights), std::back_inserter(expected_limits), [total](const auto& v){ return v * config::default_max_block_cpu_usage / total; });

      for (int64_t idx = 0; idx < weights.size(); idx++) {
         const account_name account(idx + 100);
         initialize_account(account);
         set_account_limits(account, -1, -1, weights.at(idx));
      }

      process_account_limit_updates();

      for (int64_t idx = 0; idx < weights.size(); idx++) {
         const account_name account(idx + 100);
         BOOST_CHECK_EQUAL(get_account_block_cpu_limit(account), expected_limits.at(idx));

         {  // use the expected limit, should succeed ... roll it back
            auto s = start_session();
            add_account_usage({account}, expected_limits.at(idx), 0, 0);
            s.undo();
         }

         // use too much, and expect failure;
         BOOST_REQUIRE_THROW(add_account_usage({account}, expected_limits.at(idx) + 1, 0, 0), tx_resource_exhausted);
      }
   } FC_LOG_AND_RETHROW();

   /**
    * create 5 accounts with different weights, verify that the capacities are as expected and that usage properly enforces them
    */
   BOOST_FIXTURE_TEST_CASE(weighted_capacity_net, resource_limits_fixture) try {
      const vector<int64_t> weights = { 234, 511, 672, 800, 1213 };
      const int64_t total = std::accumulate(std::begin(weights), std::end(weights), 0LL);
      vector<int64_t> expected_limits;
      std::transform(std::begin(weights), std::end(weights), std::back_inserter(expected_limits), [total](const auto& v){ return v * config::default_max_block_size / total; });

      for (int64_t idx = 0; idx < weights.size(); idx++) {
         const account_name account(idx + 100);
         initialize_account(account);
         set_account_limits(account, -1, weights.at(idx), -1 );
      }

      process_account_limit_updates();

      for (int64_t idx = 0; idx < weights.size(); idx++) {
         const account_name account(idx + 100);
         BOOST_CHECK_EQUAL(get_account_block_net_limit(account), expected_limits.at(idx));

         {  // use the expected limit, should succeed ... roll it back
            auto s = start_session();
            add_account_usage({account}, 0, expected_limits.at(idx), 0);
            s.undo();
         }

         // use too much, and expect failure;
         BOOST_REQUIRE_THROW(add_account_usage({account}, 0, expected_limits.at(idx) + 1, 0), tx_resource_exhausted);
      }
   } FC_LOG_AND_RETHROW();



BOOST_AUTO_TEST_SUITE_END()