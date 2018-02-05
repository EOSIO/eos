/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/testing/tester.hpp>
#include <fc/crypto/digest.hpp>

#include <boost/test/unit_test.hpp>


using namespace eosio::chain;
using namespace eosio::testing;
namespace bfs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(database_tests)

   // Simple tests of undo infrastructure
   BOOST_AUTO_TEST_CASE(undo_test) {
      try {
         tester test;
         auto &db = test.control->get_mutable_database();

         auto ses = db.start_undo_session(true);

         // Create an account
         db.create<account_object>([](account_object &a) {
            a.name = "billy";
         });

         // Make sure we can retrieve that account by name
         auto ptr = db.find<account_object, by_name, std::string>("billy");
         BOOST_TEST(ptr != nullptr);

         // Undo creation of the account
         ses.undo();

         // Make sure we can no longer find the account
         ptr = db.find<account_object, by_name, std::string>("billy");
         BOOST_TEST(ptr == nullptr);
      } FC_LOG_AND_RETHROW()
   }

   // Test the block fetching methods on database, get_block_id_for_num, fetch_bock_by_id, and fetch_block_by_number
   BOOST_AUTO_TEST_CASE(get_blocks) {
      try {
         tester test;
         vector<block_id_type> block_ids;

         const uint32_t num_of_blocks_to_prod = 200;
         // Produce 200 blocks and check their IDs should match the above
         test.produce_blocks(num_of_blocks_to_prod);
         for (uint32_t i = 0; i < num_of_blocks_to_prod; ++i) {
            block_ids.emplace_back(test.control->fetch_block_by_number(i + 1)->id());
            BOOST_TEST(block_header::num_from_id(block_ids.back()) == i + 1);
            BOOST_TEST(test.control->get_block_id_for_num(i + 1) == block_ids.back());
            BOOST_TEST(test.control->fetch_block_by_number(i + 1)->id() == block_ids.back());
         }

         // Utility function to check expected irreversible block
         auto calc_exp_last_irr_block_num = [&](uint32_t head_block_num) {
            const global_property_object &gpo = test.control->get_global_properties();
            const auto producers_size = gpo.active_producers.producers.size();
            const auto min_producers = EOS_PERCENT(producers_size, config::irreversible_threshold_percent);
            return head_block_num - (((min_producers - 1) * config::producer_repititions) + 1 +
                   (head_block_num % config::producer_repititions));
         };

         // Check the last irreversible block number is set correctly
         const auto expected_last_irreversible_block_number = calc_exp_last_irr_block_num(num_of_blocks_to_prod);
         BOOST_TEST(test.control->last_irreversible_block_num() == expected_last_irreversible_block_number);
         // Check that block 201 cannot be found (only 20 blocks exist)
         BOOST_CHECK_THROW(test.control->get_block_id_for_num(num_of_blocks_to_prod + 1),
                           eosio::chain::unknown_block_exception);

         const uint32_t next_num_of_blocks_to_prod = 100;
         // Produce 100 blocks and check their IDs should match the above
         test.produce_blocks(next_num_of_blocks_to_prod);

         const auto next_expected_last_irreversible_block_number = calc_exp_last_irr_block_num(
                 num_of_blocks_to_prod + next_num_of_blocks_to_prod);
         // Check the last irreversible block number is updated correctly
         BOOST_TEST(test.control->last_irreversible_block_num() == next_expected_last_irreversible_block_number);
         // Check that block 201 can now be found
         BOOST_CHECK_NO_THROW(test.control->get_block_id_for_num(num_of_blocks_to_prod + 1));
         // Check the latest head block match
         BOOST_TEST(test.control->get_block_id_for_num(num_of_blocks_to_prod + next_num_of_blocks_to_prod) ==
                    test.control->head_block_id());
      } FC_LOG_AND_RETHROW()
   }


BOOST_AUTO_TEST_SUITE_END()
