/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/account_object.hpp>

#include <chainbase/chainbase.hpp>

#include <fc/crypto/digest.hpp>

#include <boost/test/unit_test.hpp>

#include "../common/database_fixture.hpp"

namespace eos {
using namespace chain;
namespace bfs = boost::filesystem;

// Simple tests of undo infrastructure
BOOST_FIXTURE_TEST_CASE(undo_test, testing_fixture)
{ try {
      auto db = database(get_temp_dir(), database::read_write, 8*1024*1024);
      db.add_index<account_index>();
      auto ses = db.start_undo_session(true);

      // Create an account
      db.create<account_object>([](account_object& a) {
          a.name = "billy";
      });

      // Make sure we can retrieve that account by name
      auto ptr = db.find<account_object, by_name, std::string>("billy");
      BOOST_CHECK(ptr != nullptr);

      // Undo creation of the account
      ses.undo();

      // Make sure we can no longer find the account
      ptr = db.find<account_object, by_name, std::string>("billy");
      BOOST_CHECK(ptr == nullptr);
} FC_LOG_AND_RETHROW() }

// Test the block fetching methods on database, get_block_id_for_num, fetch_bock_by_id, and fetch_block_by_number
BOOST_FIXTURE_TEST_CASE(get_blocks, testing_fixture)
{ try {
      Make_Blockchain(chain)
      vector<block_id_type> block_ids;

      // Produce 20 blocks and check their IDs should match the above
        chain.produce_blocks(20);
      for (int i = 0; i < 20; ++i) {
         block_ids.emplace_back(chain.fetch_block_by_number(i+1)->id());
         BOOST_CHECK_EQUAL(block_header::num_from_id(block_ids.back()), i+1);
         BOOST_CHECK_EQUAL(chain.get_block_id_for_num(i+1), block_ids.back());
         BOOST_CHECK_EQUAL(chain.fetch_block_by_number(i+1)->id(), block_ids.back());
      }

      // Check the last irreversible block number is set correctly
      BOOST_CHECK_EQUAL(chain.last_irreversible_block_num(), 6);
      // Check that block 21 cannot be found (only 20 blocks exist)
      BOOST_CHECK_THROW(chain.get_block_id_for_num(21), eos::chain::unknown_block_exception);

        chain.produce_blocks();
      // Check the last irreversible block number is updated correctly
      BOOST_CHECK_EQUAL(chain.last_irreversible_block_num(), 7);
      // Check that block 21 can now be found
      BOOST_CHECK_EQUAL(chain.get_block_id_for_num(21), chain.head_block_id());
} FC_LOG_AND_RETHROW() }
} // namespace eos
