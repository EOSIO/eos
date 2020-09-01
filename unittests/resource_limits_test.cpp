#include <algorithm>

#include <eosio/chain/config.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/testing/chainbase_fixture.hpp>
#include <eosio/testing/tester.hpp>
#include "fork_test_utilities.hpp"

#include <boost/test/unit_test.hpp>

using namespace eosio::chain::resource_limits;
using namespace eosio::testing;
using namespace eosio::chain;

class resource_limits_fixture: private chainbase_fixture<1024*1024>, public resource_limits_manager
{
   public:
      resource_limits_fixture()
      :chainbase_fixture()
      ,resource_limits_manager(*chainbase_fixture::_db, []() { return nullptr; })
      {
         add_indices();
         initialize_database();
      }

      ~resource_limits_fixture() {}

      chainbase::database::session start_session() {
         return chainbase_fixture::_db->start_undo_session(true);
      }
};

constexpr uint64_t expected_elastic_iterations(uint64_t from, uint64_t to, uint64_t rate_num, uint64_t rate_den ) {
   uint64_t result = 0;
   uint64_t cur = from;

   while((from < to && cur < to) || (from > to && cur > to)) {
      cur = cur * rate_num / rate_den;
      result += 1;
   }

   return result;
}


constexpr uint64_t expected_exponential_average_iterations( uint64_t from, uint64_t to, uint64_t value, uint64_t window_size ) {
   uint64_t result = 0;
   uint64_t cur = from;

   while((from < to && cur < to) || (from > to && cur > to)) {
      cur = cur * (uint64_t)(window_size - 1) / (uint64_t)(window_size);
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
      const uint64_t desired_virtual_limit = config::default_max_block_cpu_usage * config::maximum_elastic_resource_multiplier;
      const uint64_t expected_relax_iterations = expected_elastic_iterations( config::default_max_block_cpu_usage, desired_virtual_limit, 1000, 999 );

      // this is enough iterations for the average to reach/exceed the target (triggering congestion handling) and then the iterations to contract down to the min
      // subtracting 1 for the iteration that pulls double duty as reaching/exceeding the target and starting congestion handling
      const uint64_t expected_contract_iterations =
              expected_exponential_average_iterations(0, EOS_PERCENT(config::default_max_block_cpu_usage, config::default_target_block_cpu_usage_pct), config::default_max_block_cpu_usage, config::block_cpu_usage_average_window_ms / config::block_interval_ms ) +
              expected_elastic_iterations( desired_virtual_limit, config::default_max_block_cpu_usage, 99, 100 ) - 1;

      const account_name account(1);
      initialize_account(account);
      set_account_limits(account, -1, -1, -1);
      process_account_limit_updates();

      // relax from the starting state (congested) to the idle state as fast as possible
      uint32_t iterations = 0;
      while( get_virtual_block_cpu_limit() < desired_virtual_limit && iterations <= expected_relax_iterations ) {
         add_transaction_usage({account},0,0,iterations);
         process_block_usage(iterations++);
      }

      BOOST_REQUIRE_EQUAL(iterations, expected_relax_iterations);
      BOOST_REQUIRE_EQUAL(get_virtual_block_cpu_limit(), desired_virtual_limit);

      // push maximum resources to go from idle back to congested as fast as possible
      while( get_virtual_block_cpu_limit() > config::default_max_block_cpu_usage
              && iterations <= expected_relax_iterations + expected_contract_iterations ) {
         add_transaction_usage({account}, config::default_max_block_cpu_usage, 0, iterations);
         process_block_usage(iterations++);
      }

      BOOST_REQUIRE_EQUAL(iterations, expected_relax_iterations + expected_contract_iterations);
      BOOST_REQUIRE_EQUAL(get_virtual_block_cpu_limit(), config::default_max_block_cpu_usage);
   } FC_LOG_AND_RETHROW();

   /**
    * Test to make sure that the elastic limits for blocks relax and contract as expected
    */
   BOOST_FIXTURE_TEST_CASE(elastic_net_relax_contract, resource_limits_fixture) try {
      const uint64_t desired_virtual_limit = config::default_max_block_net_usage * config::maximum_elastic_resource_multiplier;
      const uint64_t expected_relax_iterations = expected_elastic_iterations( config::default_max_block_net_usage, desired_virtual_limit, 1000, 999 );

      // this is enough iterations for the average to reach/exceed the target (triggering congestion handling) and then the iterations to contract down to the min
      // subtracting 1 for the iteration that pulls double duty as reaching/exceeding the target and starting congestion handling
      const uint64_t expected_contract_iterations =
              expected_exponential_average_iterations(0, EOS_PERCENT(config::default_max_block_net_usage, config::default_target_block_net_usage_pct), config::default_max_block_net_usage, config::block_size_average_window_ms / config::block_interval_ms ) +
              expected_elastic_iterations( desired_virtual_limit, config::default_max_block_net_usage, 99, 100 ) - 1;

      const account_name account(1);
      initialize_account(account);
      set_account_limits(account, -1, -1, -1);
      process_account_limit_updates();

      // relax from the starting state (congested) to the idle state as fast as possible
      uint32_t iterations = 0;
      while( get_virtual_block_net_limit() < desired_virtual_limit && iterations <= expected_relax_iterations ) {
         add_transaction_usage({account},0,0,iterations);
         process_block_usage(iterations++);
      }

      BOOST_REQUIRE_EQUAL(iterations, expected_relax_iterations);
      BOOST_REQUIRE_EQUAL(get_virtual_block_net_limit(), desired_virtual_limit);

      // push maximum resources to go from idle back to congested as fast as possible
      while( get_virtual_block_net_limit() > config::default_max_block_net_usage
              && iterations <= expected_relax_iterations + expected_contract_iterations ) {
         add_transaction_usage({account},0, config::default_max_block_net_usage, iterations);
         process_block_usage(iterations++);
      }

      BOOST_REQUIRE_EQUAL(iterations, expected_relax_iterations + expected_contract_iterations);
      BOOST_REQUIRE_EQUAL(get_virtual_block_net_limit(), config::default_max_block_net_usage);
   } FC_LOG_AND_RETHROW();

   /**
    * create 5 accounts with different weights, verify that the capacities are as expected and that usage properly enforces them
    */


// TODO: restore weighted capacity cpu tests
#if 0
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
         BOOST_CHECK_EQUAL(get_account_cpu_limit(account), expected_limits.at(idx));

         {  // use the expected limit, should succeed ... roll it back
            auto s = start_session();
            add_transaction_usage({account}, expected_limits.at(idx), 0, 0);
            s.undo();
         }

         // use too much, and expect failure;
         BOOST_REQUIRE_THROW(add_transaction_usage({account}, expected_limits.at(idx) + 1, 0, 0), tx_cpu_usage_exceeded);
      }
   } FC_LOG_AND_RETHROW();

   /**
    * create 5 accounts with different weights, verify that the capacities are as expected and that usage properly enforces them
    */
   BOOST_FIXTURE_TEST_CASE(weighted_capacity_net, resource_limits_fixture) try {
      const vector<int64_t> weights = { 234, 511, 672, 800, 1213 };
      const int64_t total = std::accumulate(std::begin(weights), std::end(weights), 0LL);
      vector<int64_t> expected_limits;
      std::transform(std::begin(weights), std::end(weights), std::back_inserter(expected_limits), [total](const auto& v){ return v * config::default_max_block_net_usage / total; });

      for (int64_t idx = 0; idx < weights.size(); idx++) {
         const account_name account(idx + 100);
         initialize_account(account);
         set_account_limits(account, -1, weights.at(idx), -1 );
      }

      process_account_limit_updates();

      for (int64_t idx = 0; idx < weights.size(); idx++) {
         const account_name account(idx + 100);
         BOOST_CHECK_EQUAL(get_account_net_limit(account), expected_limits.at(idx));

         {  // use the expected limit, should succeed ... roll it back
            auto s = start_session();
            add_transaction_usage({account}, 0, expected_limits.at(idx), 0);
            s.undo();
         }

         // use too much, and expect failure;
         BOOST_REQUIRE_THROW(add_transaction_usage({account}, 0, expected_limits.at(idx) + 1, 0), tx_net_usage_exceeded);
      }
   } FC_LOG_AND_RETHROW();
#endif

   BOOST_FIXTURE_TEST_CASE(enforce_block_limits_cpu, resource_limits_fixture) try {
      const account_name account(1);
      initialize_account(account);
      set_account_limits(account, -1, -1, -1 );
      process_account_limit_updates();

      const uint64_t increment = 1000;
      const uint64_t expected_iterations = config::default_max_block_cpu_usage / increment;

      for (uint64_t idx = 0; idx < expected_iterations; idx++) {
         add_transaction_usage({account}, increment, 0, 0);
      }

      BOOST_REQUIRE_THROW(add_transaction_usage({account}, increment, 0, 0), block_resource_exhausted);

   } FC_LOG_AND_RETHROW();

   BOOST_FIXTURE_TEST_CASE(enforce_block_limits_net, resource_limits_fixture) try {
      const account_name account(1);
      initialize_account(account);
      set_account_limits(account, -1, -1, -1 );
      process_account_limit_updates();

      const uint64_t increment = 1000;
      const uint64_t expected_iterations = config::default_max_block_net_usage / increment;

      for (uint64_t idx = 0; idx < expected_iterations; idx++) {
         add_transaction_usage({account}, 0, increment, 0);
      }

      BOOST_REQUIRE_THROW(add_transaction_usage({account}, 0, increment, 0), block_resource_exhausted);

   } FC_LOG_AND_RETHROW();

   BOOST_FIXTURE_TEST_CASE(enforce_account_ram_limit, resource_limits_fixture) try {
      const uint64_t limit = 1000;
      const uint64_t increment = 77;
      const uint64_t expected_iterations = (limit + increment - 1 ) / increment;


      const account_name account(1);
      initialize_account(account);
      set_account_limits(account, limit, -1, -1 );
      process_account_limit_updates();

      for (uint64_t idx = 0; idx < expected_iterations - 1; idx++) {
         add_pending_ram_usage(account, increment, generic_storage_usage_trace(0));
         verify_account_ram_usage(account);
      }

      add_pending_ram_usage(account, increment, generic_storage_usage_trace(0));
      BOOST_REQUIRE_THROW(verify_account_ram_usage(account), ram_usage_exceeded);
   } FC_LOG_AND_RETHROW();

   BOOST_FIXTURE_TEST_CASE(enforce_account_ram_limit_underflow, resource_limits_fixture) try {
      const account_name account(1);
      initialize_account(account);
      set_account_limits(account, 100, -1, -1 );
      verify_account_ram_usage(account);
      process_account_limit_updates();
      BOOST_REQUIRE_THROW(add_pending_ram_usage(account, -101, generic_storage_usage_trace(0)), transaction_exception);

   } FC_LOG_AND_RETHROW();

   BOOST_FIXTURE_TEST_CASE(enforce_account_ram_limit_overflow, resource_limits_fixture) try {
      const account_name account(1);
      initialize_account(account);
      set_account_limits(account, UINT64_MAX, -1, -1 );
      verify_account_ram_usage(account);
      process_account_limit_updates();
      add_pending_ram_usage(account, UINT64_MAX/2, generic_storage_usage_trace(0));
      verify_account_ram_usage(account);
      add_pending_ram_usage(account, UINT64_MAX/2, generic_storage_usage_trace(0));
      verify_account_ram_usage(account);
      BOOST_REQUIRE_THROW(add_pending_ram_usage(account, 2, generic_storage_usage_trace(0)), transaction_exception);

   } FC_LOG_AND_RETHROW();

   BOOST_FIXTURE_TEST_CASE(enforce_account_ram_commitment, resource_limits_fixture) try {
      const int64_t limit = 1000;
      const int64_t commit = 600;
      const int64_t increment = 77;
      const int64_t expected_iterations = (limit - commit + increment - 1 ) / increment;


      const account_name account(1);
      initialize_account(account);
      set_account_limits(account, limit, -1, -1 );
      process_account_limit_updates();
      add_pending_ram_usage(account, commit, generic_storage_usage_trace(0));
      verify_account_ram_usage(account);

      for (int idx = 0; idx < expected_iterations - 1; idx++) {
         set_account_limits(account, limit - increment * idx, -1, -1);
         verify_account_ram_usage(account);
         process_account_limit_updates();
      }

      set_account_limits(account, limit - increment * expected_iterations, -1, -1);
      BOOST_REQUIRE_THROW(verify_account_ram_usage(account), ram_usage_exceeded);
   } FC_LOG_AND_RETHROW();


   BOOST_FIXTURE_TEST_CASE(sanity_check, resource_limits_fixture) try {
      int64_t  total_staked_tokens = 1'000'000'000'0000ll;
      int64_t  user_stake = 1'0000ll;
      uint64_t max_block_cpu = 200'000.; // us;
      uint64_t blocks_per_day = 2*60*60*24;
      uint64_t total_cpu_per_period = max_block_cpu * blocks_per_day;

      double congested_cpu_time_per_period = (double(total_cpu_per_period) * user_stake) / total_staked_tokens;
      wdump((congested_cpu_time_per_period));
      double uncongested_cpu_time_per_period = congested_cpu_time_per_period * config::maximum_elastic_resource_multiplier;
      wdump((uncongested_cpu_time_per_period));

      initialize_account( N(dan) );
      initialize_account( N(everyone) );
      set_account_limits( N(dan), 0, 0, user_stake );
      set_account_limits( N(everyone), 0, 0, (total_staked_tokens - user_stake) );
      process_account_limit_updates();

      // dan cannot consume more than 34 us per day
      BOOST_REQUIRE_THROW( add_transaction_usage( {N(dan)}, 35, 0, 1 ), tx_cpu_usage_exceeded );

      // Ensure CPU usage is 0 by "waiting" for one day's worth of blocks to pass.
      add_transaction_usage( {N(dan)}, 0, 0, 1 + blocks_per_day );

      // But dan should be able to consume up to 34 us per day.
      add_transaction_usage( {N(dan)}, 34, 0, 2 + blocks_per_day );
   } FC_LOG_AND_RETHROW()

   /**
     * Test to make sure that get_account_net_limit_ex and get_account_cpu_limit_ex returns proper results, including
     * 1. the last updated timestamp is always same as the time slot on accumulator.
     * 2. when no timestamp is given, the current_used should be same as used.
     * 3. when timestamp is given, if it is earlier than last_usage_update_time, current_used is same as used,
     *    otherwise, current_used should be the decay value (will be 0 after 1 day)
    */
   BOOST_FIXTURE_TEST_CASE(get_account_net_limit_ex_and_get_account_cpu_limit_ex, resource_limits_fixture) try {

      const account_name cpu_test_account("cpuacc");
      const account_name net_test_account("netacc");
      constexpr uint32_t net_window = eosio::chain::config::account_net_usage_average_window_ms / eosio::chain::config::block_interval_ms;
      constexpr uint32_t cpu_window = eosio::chain::config::account_cpu_usage_average_window_ms / eosio::chain::config::block_interval_ms;

      constexpr int64_t unlimited = -1;

      using get_account_limit_ex_func = std::function<std::pair<account_resource_limit, bool>(const resource_limits_manager*, const account_name&, uint32_t, const std::optional<block_timestamp_type>&)>;
      auto test_get_account_limit_ex = [this](const account_name& test_account, const uint32_t window, get_account_limit_ex_func get_account_limit_ex)
      {
         constexpr uint32_t delta_slot = 100;
         constexpr uint32_t greylist_limit = config::maximum_elastic_resource_multiplier;
         const block_timestamp_type time_stamp_now(delta_slot + 1);
         BOOST_CHECK_LT(delta_slot, window);
         initialize_account(test_account);
         set_account_limits(test_account, unlimited, unlimited, unlimited);
         process_account_limit_updates();
         // unlimited
         {
            const auto ret_unlimited_wo_time_stamp = get_account_limit_ex(this, test_account, greylist_limit, {});
            const auto ret_unlimited_with_time_stamp = get_account_limit_ex(this, test_account, greylist_limit, time_stamp_now);
            BOOST_CHECK_EQUAL(ret_unlimited_wo_time_stamp.first.current_used, (int64_t) -1);
            BOOST_CHECK_EQUAL(ret_unlimited_with_time_stamp.first.current_used, (int64_t) -1);
            BOOST_CHECK_EQUAL(ret_unlimited_wo_time_stamp.first.last_usage_update_time.slot,
                              ret_unlimited_with_time_stamp.first.last_usage_update_time.slot);

         }
         const int64_t cpu_limit = 2048;
         const int64_t net_limit = 1024;
         set_account_limits(test_account, -1, net_limit, cpu_limit);
         process_account_limit_updates();
         // limited, with no usage, current time stamp
         {
            const auto ret_limited_init_wo_time_stamp =  get_account_limit_ex(this, test_account, greylist_limit, {});
            const auto ret_limited_init_with_time_stamp =  get_account_limit_ex(this, test_account, greylist_limit, time_stamp_now);
            BOOST_CHECK_EQUAL(ret_limited_init_wo_time_stamp.first.current_used, ret_limited_init_wo_time_stamp.first.used);
            BOOST_CHECK_EQUAL(ret_limited_init_wo_time_stamp.first.current_used, 0);
            BOOST_CHECK_EQUAL(ret_limited_init_with_time_stamp.first.current_used, ret_limited_init_with_time_stamp.first.used);
            BOOST_CHECK_EQUAL(ret_limited_init_with_time_stamp.first.current_used, 0);
            BOOST_CHECK_EQUAL(ret_limited_init_wo_time_stamp.first.last_usage_update_time.slot, 0 );
            BOOST_CHECK_EQUAL( ret_limited_init_with_time_stamp.first.last_usage_update_time.slot, 0 );
         }
         const uint32_t update_slot = time_stamp_now.slot - delta_slot;
         const int64_t cpu_usage = 100;
         const int64_t net_usage = 200;
         add_transaction_usage({test_account}, cpu_usage, net_usage, update_slot );
         // limited, with some usages, current time stamp
         {
            const auto ret_limited_1st_usg_wo_time_stamp =  get_account_limit_ex(this, test_account, greylist_limit, {});
            const auto ret_limited_1st_usg_with_time_stamp =  get_account_limit_ex(this, test_account, greylist_limit, time_stamp_now);
            BOOST_CHECK_EQUAL(ret_limited_1st_usg_wo_time_stamp.first.current_used, ret_limited_1st_usg_wo_time_stamp.first.used);
            BOOST_CHECK_LT(ret_limited_1st_usg_with_time_stamp.first.current_used, ret_limited_1st_usg_with_time_stamp.first.used);
            BOOST_CHECK_EQUAL(ret_limited_1st_usg_with_time_stamp.first.current_used,
                              ret_limited_1st_usg_with_time_stamp.first.used * (window - delta_slot) / window);
            BOOST_CHECK_EQUAL(ret_limited_1st_usg_wo_time_stamp.first.last_usage_update_time.slot, update_slot);
            BOOST_CHECK_EQUAL(ret_limited_1st_usg_with_time_stamp.first.last_usage_update_time.slot, update_slot);
         }
         // limited, with some usages, earlier time stamp
         const block_timestamp_type earlier_time_stamp(time_stamp_now.slot - delta_slot - 1);
         {
            const auto ret_limited_wo_time_stamp =  get_account_limit_ex(this, test_account, greylist_limit, {});
            const auto ret_limited_with_earlier_time_stamp =  get_account_limit_ex(this, test_account, greylist_limit, earlier_time_stamp);
            BOOST_CHECK_EQUAL(ret_limited_with_earlier_time_stamp.first.current_used, ret_limited_with_earlier_time_stamp.first.used);
            BOOST_CHECK_EQUAL(ret_limited_wo_time_stamp.first.current_used, ret_limited_wo_time_stamp.first.used );
            BOOST_CHECK_EQUAL(ret_limited_wo_time_stamp.first.last_usage_update_time.slot, update_slot);
            BOOST_CHECK_EQUAL(ret_limited_with_earlier_time_stamp.first.last_usage_update_time.slot, update_slot);
         }
      };
      test_get_account_limit_ex(net_test_account, net_window, &resource_limits_manager::get_account_net_limit_ex);
      test_get_account_limit_ex(cpu_test_account, cpu_window, &resource_limits_manager::get_account_cpu_limit_ex);

   } FC_LOG_AND_RETHROW()

   BOOST_AUTO_TEST_CASE(light_net_validation) try {
      tester main( setup_policy::preactivate_feature_and_new_bios );
      tester validator( setup_policy::none );
      tester validator2( setup_policy::none );

      name test_account("alice");
      name test_account2("bob");

      constexpr int64_t net_weight = 1;
      constexpr int64_t other_net_weight = 1'000'000'000;

      signed_block_ptr trigger_block;

      // Create test account with limited NET quota
      main.create_accounts( {test_account, test_account2} );
      main.push_action( config::system_account_name, N(setalimits), config::system_account_name, fc::mutable_variant_object()
         ("account",    test_account)
         ("ram_bytes",  -1)
         ("net_weight", net_weight)
         ("cpu_weight", -1)
      );
      main.push_action( config::system_account_name, N(setalimits), config::system_account_name, fc::mutable_variant_object()
         ("account",    test_account2)
         ("ram_bytes",  -1)
         ("net_weight", other_net_weight)
         ("cpu_weight", -1)
      );
      main.produce_block();

      // Sync validator and validator2 nodes with the main node as of this state.
      push_blocks( main, validator );
      push_blocks( main, validator2 );

      // Restart validator2 node in light validation mode.
      {
         validator2.close();
         auto cfg = validator2.get_config();
         cfg.block_validation_mode = validation_mode::LIGHT;
         validator2.init( cfg );
      }

      const auto& rlm = main.control->get_resource_limits_manager();

      // NET quota of test account should be enough to support one but not two reqauth transactions.
      int64_t before_net_usage = rlm.get_account_net_limit_ex( test_account ).first.used;
      main.push_action( config::system_account_name, N(reqauth), test_account, fc::mutable_variant_object()("from", "alice"), 6 );
      int64_t after_net_usage = rlm.get_account_net_limit_ex( test_account ).first.used;
      int64_t reqauth_net_usage_delta = after_net_usage - before_net_usage;
      BOOST_REQUIRE_EXCEPTION(
         main.push_action( config::system_account_name, N(reqauth), test_account, fc::mutable_variant_object()("from", "alice"), 7 ),
         fc::exception, fc_exception_code_is( tx_net_usage_exceeded::code_value )
      );
      trigger_block = main.produce_block();

      auto main_schedule_hash = main.control->head_block_state()->pending_schedule.schedule_hash;
      auto main_block_mroot   = main.control->head_block_state()->blockroot_merkle.get_root();

      // Modify NET bill in transaction receipt within trigger block to cause the NET usage of test account to exceed its quota.
      {
         // Increase the NET usage in the reqauth transaction receipt.
         trigger_block->transactions.back().net_usage_words.value = 2*((reqauth_net_usage_delta + 7)/8); // double the NET bill

         // Re-calculate the transaction merkle
         eosio::chain::deque<digest_type> trx_digests;
         const auto& trxs = trigger_block->transactions;
         for( const auto& a : trxs )
            trx_digests.emplace_back( a.digest() );
         trigger_block->transaction_mroot = merkle( move(trx_digests) );

         // Re-sign the block
         auto header_bmroot = digest_type::hash( std::make_pair( trigger_block->digest(), main_block_mroot ) );
         auto sig_digest = digest_type::hash( std::make_pair(header_bmroot, main_schedule_hash ) );
         trigger_block->producer_signature = main.get_private_key(config::system_account_name, "active").sign(sig_digest);
      }

      // Push trigger block to validator node.
      // This should fail because the NET bill calculated by the fully-validating node will differ from the one in the block.
      {
         auto bs = validator.control->create_block_state_future( trigger_block->calculate_id(), trigger_block );
         validator.control->abort_block();
         BOOST_REQUIRE_EXCEPTION(
            validator.control->push_block( bs, forked_branch_callback{}, trx_meta_cache_lookup{} ),
            fc::exception, fc_exception_message_is( "receipt does not match" )
         );
      }

      // Push trigger block to validator2 node.
      // This should still cause a NET failure, but will no longer be due to a receipt mismatch.
      // Because validator2 is in light validation mode, it does not compute the NET bill itself and instead only relies on the value in the block.
      // The failure will be due to failing check_net_usage within transaction_context::finalize because the NET bill in the block is too high.
      {
         auto bs = validator2.control->create_block_state_future( trigger_block->calculate_id(), trigger_block );
         validator2.control->abort_block();
         BOOST_REQUIRE_EXCEPTION(
            validator2.control->push_block( bs, forked_branch_callback{}, trx_meta_cache_lookup{} ),
            fc::exception, fc_exception_code_is( tx_net_usage_exceeded::code_value )
         );
      }

      // Modify NET bill in transaction receipt within trigger block to be lower than what it originally was.
      {
         // Increase the NET usage in the reqauth transaction receipt.
         trigger_block->transactions.back().net_usage_words.value = ((reqauth_net_usage_delta + 7)/8)/2; // half the original NET bill

         // Re-calculate the transaction merkle
         eosio::chain::deque<digest_type> trx_digests;
         const auto& trxs = trigger_block->transactions;
         for( const auto& a : trxs )
            trx_digests.emplace_back( a.digest() );
         trigger_block->transaction_mroot = merkle( move(trx_digests) );

         // Re-sign the block
         auto header_bmroot = digest_type::hash( std::make_pair( trigger_block->digest(), main_block_mroot ) );
         auto sig_digest = digest_type::hash( std::make_pair(header_bmroot, main_schedule_hash ) );
         trigger_block->producer_signature = main.get_private_key(config::system_account_name, "active").sign(sig_digest);
      }

      // Push new trigger block to validator node.
      // This should still fail because the NET bill is incorrect.
      {
         auto bs = validator.control->create_block_state_future( trigger_block->calculate_id(), trigger_block );
         validator.control->abort_block();
         BOOST_REQUIRE_EXCEPTION(
            validator.control->push_block( bs, forked_branch_callback{}, trx_meta_cache_lookup{} ),
            fc::exception, fc_exception_message_is( "receipt does not match" )
         );
      }

      // Push new trigger block to validator2 node.
      // Because validator2 is in light validation mode, this will not fail despite the fact that the NET bill is incorrect.
      {
         auto bs = validator2.control->create_block_state_future( trigger_block->calculate_id(), trigger_block );
         validator2.control->abort_block();
         validator2.control->push_block( bs, forked_branch_callback{}, trx_meta_cache_lookup{} );
      }
   } FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
