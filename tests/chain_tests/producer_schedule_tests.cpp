#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <boost/range/algorithm.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio::testing;
using namespace eosio::chain;
using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(producer_schedule_tests)

   // Calculate expected producer given the schedule and slot number
   account_name get_expected_producer(const producer_schedule_type& schedule, const uint64_t slot) {
      const auto& index = (slot % (schedule.producers.size() * config::producer_repetitions)) / config::producer_repetitions;
      return schedule.producers.at(index).producer_name;
   };

   // Check if two schedule is equal
   bool is_schedule_equal(const producer_schedule_type& first, const producer_schedule_type& second) {
      bool is_equal = first.producers.size() == second.producers.size();
      for (uint32_t i = 0; i < first.producers.size(); i++) {
         is_equal = is_equal && first.producers.at(i) == second.producers.at(i);
      }
      return is_equal;
   };

   // Calculate the block num of the next round first block
   // The new producer schedule will become effective when it's in the block of the next round first block
   // However, it won't be applied until the effective block num is deemed irreversible
   uint64_t calc_block_num_of_next_round_first_block(const chain_controller& control){
      auto res = control.get_dynamic_global_properties().head_block_number + 1;
      const auto blocks_per_round = control.get_global_properties().active_producers.producers.size() * config::producer_repetitions;
      while((res % blocks_per_round) != 0) {
         res++;
      }
      return res;
   };

   BOOST_FIXTURE_TEST_CASE( verify_producer_schedule, TESTER ) try {

      // Utility function to ensure that producer schedule work as expected
      const auto& confirm_schedule_correctness = [&](const producer_schedule_type& new_prod_schd, const uint64_t eff_new_prod_schd_block_num)  {
         const uint32_t check_duration = 1000; // number of blocks
         for (uint32_t i = 0; i < check_duration; ++i) {
            const producer_schedule_type current_schedule = control->get_global_properties().active_producers;
            const auto& current_absolute_slot = control->get_dynamic_global_properties().current_absolute_slot ;
            // Determine expected producer
            const auto& expected_producer = get_expected_producer(current_schedule, current_absolute_slot + 1);

            // The new schedule will only be applied once the effective block num is deemed irreversible
            const bool is_new_schedule_applied = control->last_irreversible_block_num() > eff_new_prod_schd_block_num;

            // Ensure that we have the correct schedule at the right time
            if (is_new_schedule_applied) {
               BOOST_TEST(is_schedule_equal(new_prod_schd, current_schedule));
            } else {
               BOOST_TEST(!is_schedule_equal(new_prod_schd, current_schedule));
            }

            // Produce block
            produce_block();

            // Check if the producer is the same as what we expect
            BOOST_TEST(control->head_block_producer() == expected_producer);
         }
      };

      // Create producer accounts
      vector<account_name> producers = {
              "inita", "initb", "initc", "initd", "inite", "initf", "initg",
              "inith", "initi", "initj", "initk", "initl", "initm", "initn",
              "inito", "initp", "initq", "initr", "inits", "initt", "initu"
      };
      create_accounts(producers);

      // ---- Test first set of producers ----
      // Send set prods action and confirm schedule correctness
      const auto& first_prod_schd = set_producers(producers, 1);
      const auto& eff_first_prod_schd_block_num = calc_block_num_of_next_round_first_block(*control);
      confirm_schedule_correctness(first_prod_schd, eff_first_prod_schd_block_num);

      // ---- Test second set of producers ----
      vector<account_name> second_set_of_producer = {
              producers[3], producers[6], producers[9], producers[12], producers[15], producers[18], producers[20]
      };
      // Send set prods action and confirm schedule correctness
      const auto& second_prod_schd = set_producers(second_set_of_producer, 2);
      const auto& eff_second_prod_schd_block_num = calc_block_num_of_next_round_first_block(*control);
      confirm_schedule_correctness(second_prod_schd, eff_second_prod_schd_block_num);

      // ---- Test deliberately miss some blocks ----
      const int64_t num_of_missed_blocks = 5000;
      produce_block(fc::microseconds(500 * 1000 * num_of_missed_blocks));
      // Ensure schedule is still correct
      confirm_schedule_correctness(second_prod_schd, eff_second_prod_schd_block_num);
      produce_block();

      // ---- Test third set of producers ----
      vector<account_name> third_set_of_producer = {
              producers[2], producers[5], producers[8], producers[11], producers[14], producers[17], producers[20],
              producers[0], producers[3], producers[6], producers[9], producers[12], producers[15], producers[18],
              producers[1], producers[4], producers[7], producers[10], producers[13], producers[16], producers[19]
      };
      // Send set prods action and confirm schedule correctness
      const auto& third_prod_schd = set_producers(third_set_of_producer, 3);
      const auto& eff_third_prod_schd_block_num = calc_block_num_of_next_round_first_block(*control);
      confirm_schedule_correctness(third_prod_schd, eff_third_prod_schd_block_num);

   } FC_LOG_AND_RETHROW()


   BOOST_FIXTURE_TEST_CASE( verify_producers, TESTER ) try {
      
      vector<account_name> valid_producers = {
         "inita", "initb", "initc", "initd", "inite", "initf", "initg",
         "inith", "initi", "initj", "initk", "initl", "initm", "initn",
         "inito", "initp", "initq", "initr", "inits", "initt", "initu"
      };
      create_accounts(valid_producers);
      set_producers(valid_producers);

      // account initz does not exist
      vector<account_name> nonexisting_producer = { "initz" };
      BOOST_CHECK_THROW(set_producers(nonexisting_producer), wasm_execution_error);
      
      // replace initg with inita, inita is now duplicate
      vector<account_name> invalid_producers = {
         "inita", "initb", "initc", "initd", "inite", "initf", "inita",
         "inith", "initi", "initj", "initk", "initl", "initm", "initn",
         "inito", "initp", "initq", "initr", "inits", "initt", "initu"
      };

      BOOST_CHECK_THROW(set_producers(invalid_producers), wasm_execution_error);

   } FC_LOG_AND_RETHROW()

   BOOST_FIXTURE_TEST_CASE( verify_header_schedule_version, TESTER ) try {

      // Utility function to ensure that producer schedule version in the header is correct
      const auto& confirm_header_schd_ver_correctness = [&](const uint64_t expected_version, const uint64_t eff_new_prod_schd_block_num)  {
         const uint32_t check_duration = 1000; // number of blocks
         for (uint32_t i = 0; i < check_duration; ++i) {
            // The new schedule will only be applied once the effective block num is deemed irreversible
            const bool is_new_schedule_applied = control->last_irreversible_block_num() > eff_new_prod_schd_block_num;

            // Produce block
            produce_block();

            // Ensure that the head block header is updated at the right time
            const auto current_schd_ver = control->head_block_header().schedule_version;
            if (is_new_schedule_applied) {
               BOOST_TEST(current_schd_ver == expected_version);
            } else {
               BOOST_TEST(current_schd_ver != expected_version);
            }
         }
      };

      // Create producer accounts
      vector<account_name> producers = {
              "inita", "initb", "initc", "initd", "inite", "initf", "initg",
              "inith", "initi", "initj", "initk", "initl", "initm", "initn",
              "inito", "initp", "initq", "initr", "inits", "initt", "initu"
      };
      create_accounts(producers);

      // Send set prods action and confirm schedule correctness
      set_producers(producers, 1);
      const auto& eff_first_prod_schd_block_num = calc_block_num_of_next_round_first_block(*control);
      // Confirm the version is correct
      confirm_header_schd_ver_correctness(1, eff_first_prod_schd_block_num);

      // Shuffle the producers and set the new one with smaller version
      boost::range::random_shuffle(producers);
      set_producers(producers, 0);
      const auto& eff_second_prod_schd_block_num = calc_block_num_of_next_round_first_block(*control);
      // Even though we set it with smaller version number, the version should be incremented instead
      confirm_header_schd_ver_correctness(2, eff_second_prod_schd_block_num);

      // Shuffle the producers and set the new one with much larger version number
      boost::range::random_shuffle(producers);
      set_producers(producers, 1000);
      const auto& eff_third_prod_schd_block_num = calc_block_num_of_next_round_first_block(*control);
      // Even though we set it with much large version number, the version should be incremented instead
      confirm_header_schd_ver_correctness(3, eff_third_prod_schd_block_num);

   } FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
