/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/global_property_object.hpp>
#include <eosio/testing/tester.hpp>

#include <boost/range/algorithm.hpp>
#include <boost/test/unit_test.hpp>

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
   account_name get_expected_producer(const vector<producer_key>& schedule, const uint64_t slot) {
      const auto& index = (slot % (schedule.size() * config::producer_repetitions)) / config::producer_repetitions;
      return schedule.at(index).producer_name;
   };

   // Check if two schedule is equal
   bool is_schedule_equal(const vector<producer_key>& first, const vector<producer_key>& second) {
      bool is_equal = first.size() == second.size();
      for (uint32_t i = 0; i < first.size(); i++) {
         is_equal = is_equal && first.at(i) == second.at(i);
      }
      return is_equal;
   };

   // Calculate the block num of the next round first block
   // The new producer schedule will become effective when it's in the block of the next round first block
   // However, it won't be applied until the effective block num is deemed irreversible
   uint64_t calc_block_num_of_next_round_first_block(const controller& control){
      auto res = control.head_block_num() + 1;
      const auto blocks_per_round = control.head_block_state()->active_schedule.producers.size() * config::producer_repetitions;
      while((res % blocks_per_round) != 0) {
         res++;
      }
      return res;
   };
#if 0
   BOOST_FIXTURE_TEST_CASE( verify_producer_schedule, TESTER ) try {

      // Utility function to ensure that producer schedule work as expected
      const auto& confirm_schedule_correctness = [&](const vector<producer_key>& new_prod_schd, const uint64_t eff_new_prod_schd_block_num)  {
         const uint32_t check_duration = 1000; // number of blocks
         for (uint32_t i = 0; i < check_duration; ++i) {
            const auto current_schedule = control->head_block_state()->active_schedule.producers;
            const auto& current_absolute_slot = control->get_global_properties().proposed_schedule_block_num;
            // Determine expected producer
            const auto& expected_producer = get_expected_producer(current_schedule, *current_absolute_slot + 1);

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
      set_producers(producers);
      const auto first_prod_schd = get_producer_keys(producers);
      const auto eff_first_prod_schd_block_num = calc_block_num_of_next_round_first_block(*control);
      confirm_schedule_correctness(first_prod_schd, eff_first_prod_schd_block_num);

      // ---- Test second set of producers ----
      vector<account_name> second_set_of_producer = {
              producers[3], producers[6], producers[9], producers[12], producers[15], producers[18], producers[20]
      };
      // Send set prods action and confirm schedule correctness
      set_producers(second_set_of_producer);
      const auto second_prod_schd = get_producer_keys(second_set_of_producer);
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
      set_producers(third_set_of_producer);
      const auto third_prod_schd = get_producer_keys(third_set_of_producer);
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
#endif


BOOST_FIXTURE_TEST_CASE( producer_schedule_promotion_test, TESTER ) try {
   create_accounts( {N(alice),N(bob),N(carol)} );
   produce_block();

   auto compare_schedules = [&]( const vector<producer_key>& a, const producer_schedule_type& b ) {
      return std::equal( a.begin(), a.end(), b.producers.begin(), b.producers.end() );
   };

   auto res = set_producers( {N(alice),N(bob)} );
   vector<producer_key> sch1 = {
                                 {N(alice), get_public_key(N(alice), "active")},
                                 {N(bob),   get_public_key(N(bob),   "active")}
                               };
   //wdump((fc::json::to_pretty_string(res)));
   wlog("set producer schedule to [alice,bob]");
   BOOST_REQUIRE_EQUAL( true, control->proposed_producers().valid() );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch1, *control->proposed_producers() ) );
   BOOST_CHECK_EQUAL( control->pending_producers().version, 0u );
   produce_block(); // Starts new block which promotes the proposed schedule to pending
   BOOST_CHECK_EQUAL( control->pending_producers().version, 1u );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch1, control->pending_producers() ) );
   BOOST_CHECK_EQUAL( control->active_producers().version, 0u );
   produce_block();
   produce_block(); // Starts new block which promotes the pending schedule to active
   BOOST_CHECK_EQUAL( control->active_producers().version, 1u );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch1, control->active_producers() ) );
   produce_blocks(7);

   res = set_producers( {N(alice),N(bob),N(carol)} );
   vector<producer_key> sch2 = {
                                 {N(alice), get_public_key(N(alice), "active")},
                                 {N(bob),   get_public_key(N(bob),   "active")},
                                 {N(carol), get_public_key(N(carol), "active")}
                               };
   wlog("set producer schedule to [alice,bob,carol]");
   BOOST_REQUIRE_EQUAL( true, control->proposed_producers().valid() );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch2, *control->proposed_producers() ) );

   produce_block();
   produce_blocks(23); // Alice produces the last block of her first round.
                    // Bob's first block (which advances LIB to Alice's last block) is started but not finalized.
   BOOST_REQUIRE_EQUAL( control->head_block_producer(), N(alice) );
   BOOST_REQUIRE_EQUAL( control->pending_block_state()->header.producer, N(bob) );
   BOOST_CHECK_EQUAL( control->pending_producers().version, 2u );

   produce_blocks(12); // Bob produces his first 11 blocks
   BOOST_CHECK_EQUAL( control->active_producers().version, 1u );
   produce_blocks(12); // Bob produces his 12th block.
                    // Alice's first block of the second round is started but not finalized (which advances LIB to Bob's last block).
   BOOST_REQUIRE_EQUAL( control->head_block_producer(), N(alice) );
   BOOST_REQUIRE_EQUAL( control->pending_block_state()->header.producer, N(bob) );
   BOOST_CHECK_EQUAL( control->active_producers().version, 2u );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch2, control->active_producers() ) );

   produce_block(); // Alice produces the first block of her second round which has changed the active schedule.

   // The next block will be produced according to the new schedule
   produce_block();
   BOOST_CHECK_EQUAL( control->head_block_producer(), N(carol) ); // And that next block happens to be produced by Carol.

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( producer_schedule_reduction, tester ) try {
   create_accounts( {N(alice),N(bob),N(carol)} );
   produce_block();

   auto compare_schedules = [&]( const vector<producer_key>& a, const producer_schedule_type& b ) {
      return std::equal( a.begin(), a.end(), b.producers.begin(), b.producers.end() );
   };

   auto res = set_producers( {N(alice),N(bob),N(carol)} );
   vector<producer_key> sch1 = {
                                 {N(alice), get_public_key(N(alice), "active")},
                                 {N(bob),   get_public_key(N(bob),   "active")},
                                 {N(carol),   get_public_key(N(carol),   "active")}
                               };
   wlog("set producer schedule to [alice,bob,carol]");
   BOOST_REQUIRE_EQUAL( true, control->proposed_producers().valid() );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch1, *control->proposed_producers() ) );
   BOOST_CHECK_EQUAL( control->pending_producers().version, 0u );
   produce_block(); // Starts new block which promotes the proposed schedule to pending
   BOOST_CHECK_EQUAL( control->pending_producers().version, 1u );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch1, control->pending_producers() ) );
   BOOST_CHECK_EQUAL( control->active_producers().version, 0u );
   produce_block();
   produce_block(); // Starts new block which promotes the pending schedule to active
   BOOST_CHECK_EQUAL( control->active_producers().version, 1u );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch1, control->active_producers() ) );
   produce_blocks(7);

   res = set_producers( {N(alice),N(bob)} );
   vector<producer_key> sch2 = {
                                 {N(alice), get_public_key(N(alice), "active")},
                                 {N(bob),   get_public_key(N(bob),   "active")}
                               };
   wlog("set producer schedule to [alice,bob]");
   BOOST_REQUIRE_EQUAL( true, control->proposed_producers().valid() );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch2, *control->proposed_producers() ) );

   produce_blocks(48);
   BOOST_REQUIRE_EQUAL( control->head_block_producer(), N(bob) );
   BOOST_REQUIRE_EQUAL( control->pending_block_state()->header.producer, N(carol) );
   BOOST_CHECK_EQUAL( control->pending_producers().version, 2u );

   produce_blocks(47);
   BOOST_CHECK_EQUAL( control->active_producers().version, 1u );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL( control->head_block_producer(), N(carol) );
   BOOST_REQUIRE_EQUAL( control->pending_block_state()->header.producer, N(alice) );
   BOOST_CHECK_EQUAL( control->active_producers().version, 2u );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch2, control->active_producers() ) );

   produce_blocks(2);
   BOOST_CHECK_EQUAL( control->head_block_producer(), N(bob) );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( empty_producer_schedule_has_no_effect, tester ) try {
   create_accounts( {N(alice),N(bob),N(carol)} );
   produce_block();

   auto compare_schedules = [&]( const vector<producer_key>& a, const producer_schedule_type& b ) {
      return std::equal( a.begin(), a.end(), b.producers.begin(), b.producers.end() );
   };

   auto res = set_producers( {N(alice),N(bob)} );
   vector<producer_key> sch1 = {
                                 {N(alice), get_public_key(N(alice), "active")},
                                 {N(bob),   get_public_key(N(bob),   "active")}
                               };
   wlog("set producer schedule to [alice,bob]");
   BOOST_REQUIRE_EQUAL( true, control->proposed_producers().valid() );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch1, *control->proposed_producers() ) );
   BOOST_CHECK_EQUAL( control->pending_producers().producers.size(), 0u );

   // Start a new block which promotes the proposed schedule to pending
   produce_block();
   BOOST_CHECK_EQUAL( control->pending_producers().version, 1u );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch1, control->pending_producers() ) );
   BOOST_CHECK_EQUAL( control->active_producers().version, 0u );

   // Start a new block which promotes the pending schedule to active
   produce_block();
   BOOST_CHECK_EQUAL( control->active_producers().version, 1u );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch1, control->active_producers() ) );
   produce_blocks(7);

   res = set_producers( {} );
   wlog("set producer schedule to []");
   BOOST_REQUIRE_EQUAL( true, control->proposed_producers().valid() );
   BOOST_CHECK_EQUAL( control->proposed_producers()->producers.size(), 0u );
   BOOST_CHECK_EQUAL( control->proposed_producers()->version, 2u );

   produce_blocks(12);
   BOOST_CHECK_EQUAL( control->pending_producers().version, 1u );

   // Empty producer schedule does get promoted from proposed to pending
   produce_block();
   BOOST_CHECK_EQUAL( control->pending_producers().version, 2u );
   BOOST_CHECK_EQUAL( false, control->proposed_producers().valid() );

   // However it should not get promoted from pending to active
   produce_blocks(24);
   BOOST_CHECK_EQUAL( control->active_producers().version, 1u );
   BOOST_CHECK_EQUAL( control->pending_producers().version, 2u );

   // Setting a new producer schedule should still use version 2
   res = set_producers( {N(alice),N(bob),N(carol)} );
   vector<producer_key> sch2 = {
                                 {N(alice), get_public_key(N(alice), "active")},
                                 {N(bob),   get_public_key(N(bob),   "active")},
                                 {N(carol), get_public_key(N(carol), "active")}
                               };
   wlog("set producer schedule to [alice,bob,carol]");
   BOOST_REQUIRE_EQUAL( true, control->proposed_producers().valid() );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch2, *control->proposed_producers() ) );
   BOOST_CHECK_EQUAL( control->proposed_producers()->version, 2u );

   // Produce enough blocks to promote the proposed schedule to pending, which it can do because the existing pending has zero producers
   produce_blocks(24);
   BOOST_CHECK_EQUAL( control->active_producers().version, 1u );
   BOOST_CHECK_EQUAL( control->pending_producers().version, 2u );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch2, control->pending_producers() ) );

   // Produce enough blocks to promote the pending schedule to active
   produce_blocks(24);
   BOOST_CHECK_EQUAL( control->active_producers().version, 2u );
   BOOST_CHECK_EQUAL( true, compare_schedules( sch2, control->active_producers() ) );

   BOOST_REQUIRE_EQUAL( validate(), true );
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
