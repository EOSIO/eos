#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>

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
   uint64_t calc_block_num_of_next_round_first_block(const chain_controller& control){
      auto eff_block_num = control.get_dynamic_global_properties().head_block_number + 1;
      const auto blocks_per_round = control.get_global_properties().active_producers.producers.size() * config::producer_repetitions;
      while((eff_block_num % blocks_per_round) != 0) {
         eff_block_num++;
      }
      return eff_block_num;
   };

   BOOST_FIXTURE_TEST_CASE( verify_producer_schedule, tester ) try {

      // Utility function to ensure that producer schedule work as expected
      const auto& confirm_schedule_correctness = [&](const producer_schedule_type& new_prod_schd, const uint64_t eff_new_prod_schd_block_num)  {
         const uint32_t check_duration = 1000; // number of blocks
         for (uint32_t i = 0; i < check_duration; ++i) {
            const producer_schedule_type current_schedule = control->get_global_properties().active_producers;
            const auto& current_absolute_slot = control->get_dynamic_global_properties().current_absolute_slot ;
            // Determine expected producer
            const auto& expected_producer = get_expected_producer(current_schedule, current_absolute_slot + 1);

            // Ensure that we have the correct schedule at the right time
            // The new schedule will only be used once the effective block num is deemed irreversible
            if (control->last_irreversible_block_num() > eff_new_prod_schd_block_num) {
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


BOOST_AUTO_TEST_SUITE_END()
