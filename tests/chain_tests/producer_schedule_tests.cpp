#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <iostream>
#include <string>
#include <tuple>
using namespace eosio::testing;
using namespace eosio::chain;
using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(producer_schedule_tests)

   BOOST_FIXTURE_TEST_CASE( verify_producer_schedule, tester ) try {

      // --- Define some utility functions -----

      using block_num_and_schd_tuple = std::tuple<block_num_type, producer_schedule_type>;

      // Utility function to send setprods action
      uint32_t version = 1;
      const auto& set_producers = [&](const vector<account_name>& producers) -> block_num_and_schd_tuple {
         // Construct producer schedule object
         producer_schedule_type schedule;
         schedule.version = version++;
         for (auto& producer_name: producers) {
            producer_key pk = { producer_name, get_public_key( producer_name, "active" )};
            schedule.producers.emplace_back(pk);
         }

         // Send setprods action
         vector<mvo> producer_keys;
         for (auto& producer: schedule.producers) {
            producer_keys.emplace_back( mvo()("producer_name", producer.producer_name)("signing_key", producer.block_signing_key));
         }
         push_action(N(eosio), N(setprods), N(eosio), mvo()("version", schedule.version)("producers", producer_keys));

         // Calculate effective block num, which is the start of the next round
         auto eff_block_num = control->get_dynamic_global_properties().head_block_number + 1;
         const auto blocks_per_round = control->get_global_properties().active_producers.producers.size() * config::producer_repetitions;
         while((eff_block_num % blocks_per_round) != 0) {
            eff_block_num++;
         }

         // Return the effective block num and new schedule
         return block_num_and_schd_tuple{ eff_block_num, schedule };
      };

      // Utility function to calculate expected producer
      const auto& get_expected_producer = [&](const producer_schedule_type& schedule, const uint64_t slot) -> account_name {
         const auto& index = (slot % (schedule.producers.size() * config::producer_repetitions)) / config::producer_repetitions;
         return schedule.producers.at(index).producer_name;
      };

      // Utility function to check if two schedule is equal
      const auto& is_schedule_equal = [&](const producer_schedule_type& first, const producer_schedule_type& second) -> bool {
         bool is_equal = first.producers.size() == second.producers.size();
         for (uint32_t i = 0; i < first.producers.size(); i++) {
            is_equal = is_equal && first.producers.at(i) == second.producers.at(i);
         }
         return is_equal;
      };

      // Utility function to ensure that producer schedule work as expected
      const auto& confirm_schedule_correctness = [&](const block_num_and_schd_tuple& new_eff_block_num_and_schd_tuple)  {
         const uint32_t check_duration = 1000; // number of blocks
         for (uint32_t i = 0; i < check_duration; ++i) {
            const producer_schedule_type current_schedule = control->get_global_properties().active_producers;
            const auto& current_absolute_slot = control->get_dynamic_global_properties().current_absolute_slot ;
            // Determine expected producer
            const auto& expected_producer = get_expected_producer(current_schedule, current_absolute_slot + 1);

            // Ensure that we have the correct schedule at the right time
            // The new schedule will only be used once the effective block num is deemed irreversible
            const auto& new_schd_eff_block_num = std::get<0>(new_eff_block_num_and_schd_tuple);
            const auto& new_schedule = std::get<1>(new_eff_block_num_and_schd_tuple);
            if (control->last_irreversible_block_num() > new_schd_eff_block_num) {
               BOOST_TEST(is_schedule_equal(new_schedule, current_schedule));
            } else {
               BOOST_TEST(!is_schedule_equal(new_schedule, current_schedule));
            }

            // Produce block
            produce_block();

            // Check if the producer is the same as what we expect
            BOOST_TEST(control->head_block_producer() == expected_producer);
         }
      };

      // ---- End of utility function definitions ----

      // Create producer accounts
      vector<account_name> producers = {
              "inita", "initb", "initc", "initd", "inite", "initf", "initg",
              "inith", "initi", "initj", "initk", "initl", "initm", "initn",
              "inito", "initp", "initq", "initr", "inits", "initt", "initu"
      };
      create_accounts(producers);

      // ---- Test first set of producers ----
      vector<account_name> first_set_of_producer = {
              producers[0], producers[1], producers[2], producers[3], producers[4], producers[5], producers[6],
              producers[7], producers[8], producers[9], producers[10], producers[11], producers[12], producers[13],
              producers[14], producers[15], producers[16], producers[17], producers[18], producers[19], producers[20]
      };
      // Send set prods action and confirm schedule correctness
      const auto& first_eff_block_num_and_schd_tuple = set_producers(first_set_of_producer);
      confirm_schedule_correctness(first_eff_block_num_and_schd_tuple);

      // ---- Test second set of producers ----
      vector<account_name> second_set_producer_order = {
              producers[3], producers[6], producers[9], producers[12], producers[15], producers[18], producers[20]
      };
      // Send set prods action and confirm schedule correctness
      const auto& second_eff_block_num_and_schd_tuple = set_producers(second_set_producer_order);
      confirm_schedule_correctness(second_eff_block_num_and_schd_tuple);


      // ---- Test deliberately miss some blocks ----
      const int64_t num_of_missed_blocks = 5000;
      produce_block(fc::microseconds(500 * 1000 * num_of_missed_blocks));
      // Ensure schedule is still correct
      confirm_schedule_correctness(second_eff_block_num_and_schd_tuple);
      produce_block();


      // ---- Test third set of producers ----
      vector<account_name> third_set_producer_order = {
              producers[2], producers[5], producers[8], producers[11], producers[14], producers[17], producers[20],
              producers[0], producers[3], producers[6], producers[9], producers[12], producers[15], producers[18],
              producers[1], producers[4], producers[7], producers[10], producers[13], producers[16], producers[19]
      };
      // Send set prods action and confirm schedule correctness
      const auto& third_eff_block_num_and_schd_tuple = set_producers(third_set_producer_order);
      confirm_schedule_correctness(third_eff_block_num_and_schd_tuple);

   } FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
