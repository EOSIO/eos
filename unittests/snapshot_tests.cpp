/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>

#include <eosio/chain/snapshot.hpp>

#include <snapshot_test/snapshot_test.wast.hpp>
#include <snapshot_test/snapshot_test.abi.hpp>


using namespace eosio;
using namespace testing;
using namespace chain;

class snapshotted_tester : public base_tester {
public:
   snapshotted_tester(controller::config config, const snapshot_reader_ptr& snapshot, int ordinal) {
      FC_ASSERT(config.blocks_dir.filename().generic_string() != "."
         && config.state_dir.filename().generic_string() != ".", "invalid path names in controller::config");

      controller::config copied_config = config;
      copied_config.blocks_dir =
              config.blocks_dir.parent_path() / std::to_string(ordinal).append(config.blocks_dir.filename().generic_string());
      copied_config.state_dir =
              config.state_dir.parent_path() / std::to_string(ordinal).append(config.state_dir.filename().generic_string());

      init(copied_config, snapshot);
   }

   signed_block_ptr produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = 0/*skip_missed_block_penalty*/ )override {
      return _produce_block(skip_time, false, skip_flag);
   }

   signed_block_ptr produce_empty_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = 0/*skip_missed_block_penalty*/ )override {
      control->abort_block();
      return _produce_block(skip_time, true, skip_flag);
   }

   bool validate() { return true; }
};

BOOST_AUTO_TEST_SUITE(snapshot_tests)

BOOST_AUTO_TEST_CASE(test_multi_index_snapshot)
{
   tester chain;

   chain.create_account(N(snapshot));
   chain.produce_blocks(1);
   chain.set_code(N(snapshot), snapshot_test_wast);
   chain.set_abi(N(snapshot), snapshot_test_abi);
   chain.produce_blocks(1);
   chain.control->abort_block();


   static const int generation_count = 20;
   std::list<snapshotted_tester> sub_testers;

   for (int generation = 0; generation < generation_count; generation++) {
      // create a new snapshot child
      variant_snapshot_writer writer;
      auto writer_p = std::shared_ptr<snapshot_writer>(&writer, [](snapshot_writer *){});
      chain.control->write_snapshot(writer_p);
      auto snapshot = writer.finalize();

      // create a new child at this snapshot
      sub_testers.emplace_back(chain.get_config(), std::make_shared<variant_snapshot_reader>(snapshot), generation);

      // increment the test contract

      // produce block
      auto new_block = chain.produce_block();

      // undo the auto-pending from tester
      chain.control->abort_block();

      auto integrity_value = chain.control->calculate_integrity_hash();

      // push that block to all sub testers and validate the integrity of the database after it.
      for (auto& other: sub_testers) {
         other.push_block(new_block);
         BOOST_REQUIRE_EQUAL(integrity_value.str(), other.control->calculate_integrity_hash().str());
      }
   }
}

BOOST_AUTO_TEST_SUITE_END()
