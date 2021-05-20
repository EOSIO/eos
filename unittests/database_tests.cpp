#include <eosio/chain/global_property_object.hpp>
#include <eosio/testing/tester.hpp>

#include <fc/crypto/digest.hpp>

#include <boost/test/unit_test.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio::chain;
using namespace eosio::testing;
namespace bfs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(database_tests)

   // Simple tests of undo infrastructure
   BOOST_AUTO_TEST_CASE(undo_test) {
      try {
         TESTER test;

         // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.
         eosio::chain::database& db = const_cast<eosio::chain::database&>( test.control->db() );

         auto ses = db.start_undo_session(true);

         // Create an account
         db.create<account_object>([](account_object &a) {
            a.name = name("billy");
         });

         // Make sure we can retrieve that account by name
         auto ptr = db.find<account_object, by_name>(name("billy"));
         BOOST_TEST(ptr != nullptr);

         // Undo creation of the account
         ses.undo();

         // Make sure we can no longer find the account
         ptr = db.find<account_object, by_name>(name("billy"));
         BOOST_TEST(ptr == nullptr);
      } FC_LOG_AND_RETHROW()
   }

   // Test the block fetching methods on database, fetch_bock_by_id, and fetch_block_by_number
   BOOST_AUTO_TEST_CASE(get_blocks) {
      try {
         TESTER test;
         vector<block_id_type> block_ids;

         const uint32_t num_of_blocks_to_prod = 200;
         // Produce 200 blocks and check their IDs should match the above
         test.produce_blocks(num_of_blocks_to_prod);
         for (uint32_t i = 0; i < num_of_blocks_to_prod; ++i) {
            block_ids.emplace_back(test.control->fetch_block_by_number(i + 1)->calculate_id());
            BOOST_TEST(block_header::num_from_id(block_ids.back()) == i + 1);
            BOOST_TEST(test.control->fetch_block_by_number(i + 1)->calculate_id() == block_ids.back());
         }

         // Check the last irreversible block number is set correctly, with one producer, irreversibility should only just 1 block before
         const auto expected_last_irreversible_block_number = test.control->head_block_num() - 1;
         BOOST_TEST(test.control->head_block_state()->dpos_irreversible_blocknum == expected_last_irreversible_block_number);
         // Ensure that future block doesn't exist
         const auto nonexisting_future_block_num = test.control->head_block_num() + 1;
         BOOST_TEST(test.control->fetch_block_by_number(nonexisting_future_block_num) == nullptr);

         const uint32_t next_num_of_blocks_to_prod = 100;
         test.produce_blocks(next_num_of_blocks_to_prod);

         const auto next_expected_last_irreversible_block_number = test.control->head_block_num() - 1;
         // Check the last irreversible block number is updated correctly
         BOOST_TEST(test.control->head_block_state()->dpos_irreversible_blocknum == next_expected_last_irreversible_block_number);
         // Previous nonexisting future block should exist by now
         BOOST_CHECK_NO_THROW(test.control->fetch_block_by_number(nonexisting_future_block_num));
         // Check the latest head block match
         BOOST_TEST(test.control->fetch_block_by_number(test.control->head_block_num())->calculate_id() ==
                    test.control->head_block_id());
      } FC_LOG_AND_RETHROW()
   }

BOOST_AUTO_TEST_SUITE_END()
