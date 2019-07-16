/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <sstream>

#include <eosio/chain/snapshot.hpp>
#include <eosio/testing/tester.hpp>

#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

#include <contracts.hpp>
#include <snapshots.hpp>

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

   snapshotted_tester(controller::config config, const snapshot_reader_ptr& snapshot, int ordinal, int copy_block_log_from_ordinal) {
      FC_ASSERT(config.blocks_dir.filename().generic_string() != "."
         && config.state_dir.filename().generic_string() != ".", "invalid path names in controller::config");

      controller::config copied_config = config;
      copied_config.blocks_dir =
              config.blocks_dir.parent_path() / std::to_string(ordinal).append(config.blocks_dir.filename().generic_string());
      copied_config.state_dir =
              config.state_dir.parent_path() / std::to_string(ordinal).append(config.state_dir.filename().generic_string());

      // create a copy of the desired block log and reversible
      auto block_log_path = config.blocks_dir.parent_path() / std::to_string(copy_block_log_from_ordinal).append(config.blocks_dir.filename().generic_string());
      fc::create_directories(copied_config.blocks_dir);
      fc::copy(block_log_path / "blocks.log", copied_config.blocks_dir / "blocks.log");
      fc::copy(block_log_path / config::reversible_blocks_dir_name, copied_config.blocks_dir / config::reversible_blocks_dir_name );

      init(copied_config, snapshot);
   }
   signed_block_ptr produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) )override {
      return _produce_block(skip_time, false);
   }

   signed_block_ptr produce_empty_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) )override {
      control->abort_block();
      return _produce_block(skip_time, true);
   }

   signed_block_ptr finish_block()override {
      return _finish_block();
   }

   bool validate() { return true; }
};

struct variant_snapshot_suite {
   using writer_t = variant_snapshot_writer;
   using reader_t = variant_snapshot_reader;
   using write_storage_t = fc::mutable_variant_object;
   using snapshot_t = fc::variant;

   struct writer : public writer_t {
      writer( const std::shared_ptr<write_storage_t>& storage )
      :writer_t(*storage)
      ,storage(storage)
      {

      }

      std::shared_ptr<write_storage_t> storage;
   };

   struct reader : public reader_t {
      explicit reader(const snapshot_t& storage)
      :reader_t(storage)
      {}
   };


   static auto get_writer() {
      return std::make_shared<writer>(std::make_shared<write_storage_t>());
   }

   static auto finalize(const std::shared_ptr<writer>& w) {
      w->finalize();
      return snapshot_t(*w->storage);
   }

   static auto get_reader( const snapshot_t& buffer) {
      return std::make_shared<reader>(buffer);
   }

   template<typename Snapshot>
   static snapshot_t load_from_file() {
      return Snapshot::json();
   }
};

struct buffered_snapshot_suite {
   using writer_t = ostream_snapshot_writer;
   using reader_t = istream_snapshot_reader;
   using write_storage_t = std::ostringstream;
   using snapshot_t = std::string;
   using read_storage_t = std::istringstream;

   struct writer : public writer_t {
      writer( const std::shared_ptr<write_storage_t>& storage )
      :writer_t(*storage)
      ,storage(storage)
      {

      }

      std::shared_ptr<write_storage_t> storage;
   };

   struct reader : public reader_t {
      explicit reader(const std::shared_ptr<read_storage_t>& storage)
      :reader_t(*storage)
      ,storage(storage)
      {}

      std::shared_ptr<read_storage_t> storage;
   };


   static auto get_writer() {
      return std::make_shared<writer>(std::make_shared<write_storage_t>());
   }

   static auto finalize(const std::shared_ptr<writer>& w) {
      w->finalize();
      return w->storage->str();
   }

   static auto get_reader( const snapshot_t& buffer) {
      return std::make_shared<reader>(std::make_shared<read_storage_t>(buffer));
   }

   template<typename Snapshot>
   static snapshot_t load_from_file() {
      return Snapshot::bin();
   }
};

BOOST_AUTO_TEST_SUITE(snapshot_tests)

using snapshot_suites = boost::mpl::list<variant_snapshot_suite, buffered_snapshot_suite>;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_exhaustive_snapshot, SNAPSHOT_SUITE, snapshot_suites)
{
   tester chain;

   chain.create_account(N(snapshot));
   chain.produce_blocks(1);
   chain.set_code(N(snapshot), contracts::snapshot_test_wasm());
   chain.set_abi(N(snapshot), contracts::snapshot_test_abi().data());
   chain.produce_blocks(1);
   chain.control->abort_block();

   static const int generation_count = 8;
   std::list<snapshotted_tester> sub_testers;

   for (int generation = 0; generation < generation_count; generation++) {
      // create a new snapshot child
      auto writer = SNAPSHOT_SUITE::get_writer();
      chain.control->write_snapshot(writer);
      auto snapshot = SNAPSHOT_SUITE::finalize(writer);

      // create a new child at this snapshot
      sub_testers.emplace_back(chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), generation);

      // increment the test contract
      chain.push_action(N(snapshot), N(increment), N(snapshot), mutable_variant_object()
         ( "value", 1 )
      );

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

BOOST_AUTO_TEST_CASE_TEMPLATE(test_replay_over_snapshot, SNAPSHOT_SUITE, snapshot_suites)
{
   tester chain;

   chain.create_account(N(snapshot));
   chain.produce_blocks(1);
   chain.set_code(N(snapshot), contracts::snapshot_test_wasm());
   chain.set_abi(N(snapshot), contracts::snapshot_test_abi().data());
   chain.produce_blocks(1);
   chain.control->abort_block();

   static const int pre_snapshot_block_count = 12;
   static const int post_snapshot_block_count = 12;

   for (int itr = 0; itr < pre_snapshot_block_count; itr++) {
      // increment the contract
      chain.push_action(N(snapshot), N(increment), N(snapshot), mutable_variant_object()
         ( "value", 1 )
      );

      // produce block
      chain.produce_block();
   }

   chain.control->abort_block();
   auto expected_pre_integrity_hash = chain.control->calculate_integrity_hash();

   // create a new snapshot child
   auto writer = SNAPSHOT_SUITE::get_writer();
   chain.control->write_snapshot(writer);
   auto snapshot = SNAPSHOT_SUITE::finalize(writer);

   // create a new child at this snapshot
   snapshotted_tester snap_chain(chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), 1);
   BOOST_REQUIRE_EQUAL(expected_pre_integrity_hash.str(), snap_chain.control->calculate_integrity_hash().str());

   // push more blocks to build up a block log
   for (int itr = 0; itr < post_snapshot_block_count; itr++) {
      // increment the contract
      chain.push_action(N(snapshot), N(increment), N(snapshot), mutable_variant_object()
         ( "value", 1 )
      );

      // produce & push block
      snap_chain.push_block(chain.produce_block());
   }

   // verify the hash at the end
   chain.control->abort_block();
   auto expected_post_integrity_hash = chain.control->calculate_integrity_hash();
   BOOST_REQUIRE_EQUAL(expected_post_integrity_hash.str(), snap_chain.control->calculate_integrity_hash().str());

   // replay the block log from the snapshot child, from the snapshot
   snapshotted_tester replay_chain(chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), 2, 1);
   BOOST_REQUIRE_EQUAL(expected_post_integrity_hash.str(), snap_chain.control->calculate_integrity_hash().str());
}

namespace {
   void variant_diff_helper(const fc::variant& lhs, const fc::variant& rhs, std::function<void(const std::string&, const fc::variant&, const fc::variant&)>&& out){
      if (lhs.get_type() != rhs.get_type()) {
         out("", lhs, rhs);
      } else if (lhs.is_object() ) {
         const auto& l_obj = lhs.get_object();
         const auto& r_obj = rhs.get_object();
         static const std::string sep = ".";

         // test keys from LHS
         std::set<std::string_view> keys;
         for (const auto& entry: l_obj) {
            const auto& l_val = entry.value();
            const auto& r_iter = r_obj.find(entry.key());
            if (r_iter == r_obj.end()) {
               out(sep + entry.key(), l_val, fc::variant());
            } else {
               const auto& r_val = r_iter->value();
               variant_diff_helper(l_val, r_val, [&out, &entry](const std::string& path, const fc::variant& lhs, const fc::variant& rhs){
                  out(sep + entry.key() + path, lhs, rhs);
               });
            }

            keys.insert(entry.key());
         }

         // print keys in RHS that were not tested
         for (const auto& entry: r_obj) {
            if (keys.find(entry.key()) != keys.end()) {
               continue;
            }
            const auto& r_val = entry.value();
            out(sep + entry.key(), fc::variant(), r_val);
         }
      } else if (lhs.is_array()) {
         const auto& l_arr = lhs.get_array();
         const auto& r_arr = rhs.get_array();

         // diff common
         auto common_count = std::min(l_arr.size(), r_arr.size());
         for (size_t idx = 0; idx < common_count; idx++) {
            const auto& l_val = l_arr.at(idx);
            const auto& r_val = r_arr.at(idx);
            variant_diff_helper(l_val, r_val, [&](const std::string& path, const fc::variant& lhs, const fc::variant& rhs){
               out( std::string("[") + std::to_string(idx) + std::string("]") + path, lhs, rhs);
            });
         }

         // print lhs additions
         for (size_t idx = common_count; idx < lhs.size(); idx++) {
            const auto& l_val = l_arr.at(idx);
            out( std::string("[") + std::to_string(idx) + std::string("]"), l_val, fc::variant());
         }

         // print rhs additions
         for (size_t idx = common_count; idx < rhs.size(); idx++) {
            const auto& r_val = r_arr.at(idx);
            out( std::string("[") + std::to_string(idx) + std::string("]"), fc::variant(), r_val);
         }

      } else if (!(lhs == rhs)) {
         out("", lhs, rhs);
      }
   }

   void print_variant_diff(const fc::variant& lhs, const fc::variant& rhs) {
      variant_diff_helper(lhs, rhs, [](const std::string& path, const fc::variant& lhs, const fc::variant& rhs){
         std::cout << path << std::endl;
         if (!lhs.is_null()) {
            std::cout << " < " << fc::json::to_pretty_string(lhs) << std::endl;
         }

         if (!rhs.is_null()) {
            std::cout << " > " << fc::json::to_pretty_string(rhs) << std::endl;
         }
      });
   }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_compatible_versions, SNAPSHOT_SUITE, snapshot_suites)
{
   tester chain(setup_policy::preactivate_feature_and_new_bios);

   ///< Begin deterministic code to generate blockchain for comparison
   // TODO: create a utility that will write new bin/json gzipped files based on this
   chain.create_account(N(snapshot));
   chain.produce_blocks(1);
   chain.set_code(N(snapshot), contracts::snapshot_test_wasm());
   chain.set_abi(N(snapshot), contracts::snapshot_test_abi().data());
   chain.produce_blocks(1);
   chain.control->abort_block();
   ///< End deterministic code to generate blockchain for comparison
   auto base_integrity_value = chain.control->calculate_integrity_hash();

   // create a latest snapshot
   auto base_writer = SNAPSHOT_SUITE::get_writer();
   chain.control->write_snapshot(base_writer);
   auto base = SNAPSHOT_SUITE::finalize(base_writer);

   {
      static_assert(chain_snapshot_header::minimum_compatible_version <= 2, "version 2 unit test is no longer needed.  Please clean up data files");
      auto v2 = SNAPSHOT_SUITE::template load_from_file<snapshots::snap_v2>();
      snapshotted_tester v2_tester(chain.get_config(), SNAPSHOT_SUITE::get_reader(v2), 0);
      auto v2_integrity_value = v2_tester.control->calculate_integrity_hash();
      BOOST_CHECK_EQUAL(v2_integrity_value.str(), base_integrity_value.str());

      // create a latest snapshot
      auto latest_writer = SNAPSHOT_SUITE::get_writer();
      v2_tester.control->write_snapshot(latest_writer);
      auto latest = SNAPSHOT_SUITE::finalize(latest_writer);

      if (std::is_same_v<SNAPSHOT_SUITE, variant_snapshot_suite> && v2_integrity_value.str() != base_integrity_value.str()) {
         print_variant_diff(base, latest);
      }

      // load the latest snapshot
      snapshotted_tester latest_tester(chain.get_config(), SNAPSHOT_SUITE::get_reader(latest), 1);
      auto latest_integrity_value = latest_tester.control->calculate_integrity_hash();

      BOOST_REQUIRE_EQUAL(v2_integrity_value.str(), latest_integrity_value.str());
   }
}

BOOST_AUTO_TEST_SUITE_END()
