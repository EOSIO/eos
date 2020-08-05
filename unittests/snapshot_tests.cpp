#include <sstream>

#include <eosio/chain/block_log.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/snapshot.hpp>
#include <eosio/testing/tester.hpp>

#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

#include <contracts.hpp>
#include <snapshots.hpp>

using namespace eosio;
using namespace testing;
using namespace chain;

chainbase::bfs::path get_parent_path(chainbase::bfs::path blocks_dir, int ordinal) {
   chainbase::bfs::path leaf_dir = blocks_dir.filename();
   if (leaf_dir.generic_string() == std::string("blocks")) {
      blocks_dir = blocks_dir.parent_path();
      leaf_dir = blocks_dir.filename();
      try {
         auto ordinal_for_config = boost::lexical_cast<int>(leaf_dir.generic_string());
         blocks_dir = blocks_dir.parent_path();
      }
      catch(const boost::bad_lexical_cast& ) {
         // no extra ordinal directory added to path
      }
   }
   return blocks_dir / std::to_string(ordinal);
}

controller::config copy_config(const controller::config& config, int ordinal) {
   controller::config copied_config = config;
   auto parent_path = get_parent_path(config.blog.log_dir, ordinal);
   copied_config.blog.log_dir = parent_path / config.blog.log_dir.filename().generic_string();
   copied_config.state_dir = parent_path / config.state_dir.filename().generic_string();
   return copied_config;
}

controller::config copy_config_and_files(const controller::config& config, int ordinal) {
   controller::config copied_config = copy_config(config, ordinal);
   fc::create_directories(copied_config.blog.log_dir);
   fc::copy(config.blog.log_dir / "blocks.log", copied_config.blog.log_dir / "blocks.log");
   fc::copy(config.blog.log_dir / config::reversible_blocks_dir_name, copied_config.blog.log_dir / config::reversible_blocks_dir_name );
   return copied_config;
}

class snapshotted_tester : public base_tester {
public:
   enum config_file_handling { dont_copy_config_files, copy_config_files };
   snapshotted_tester(controller::config config, const snapshot_reader_ptr& snapshot, int ordinal,
           config_file_handling copy_files_from_config = config_file_handling::dont_copy_config_files) {
      FC_ASSERT(config.blog.log_dir.filename().generic_string() != "."
                && config.state_dir.filename().generic_string() != ".", "invalid path names in controller::config");

      controller::config copied_config = (copy_files_from_config == copy_config_files)
                                         ? copy_config_and_files(config, ordinal) : copy_config(config, ordinal);

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

   static snapshot_t load_from_file(const std::string& name) {
      return snapshots::json(name);
   }

   static void write_to_file( const std::string& basename, const snapshot_t& snapshot ) {
     std::ofstream file( basename + ".json", std::ios_base::binary );
     fc::json::to_stream( file, snapshot, fc::time_point::maximum() );
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

   static snapshot_t load_from_file(const std::string& filename) {
      return snapshots::bin(filename);
   }

   static void write_to_file( const std::string& basename, const snapshot_t& snapshot ) {
     std::ofstream file( basename + ".bin", std::ios_base::binary );
     file.write( snapshot.data(), snapshot.size() );
   }
};

BOOST_AUTO_TEST_SUITE(snapshot_tests)

using snapshot_suites = boost::mpl::list<variant_snapshot_suite, buffered_snapshot_suite>;

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

   template <typename SNAPSHOT_SUITE>
   void verify_integrity_hash(const controller& lhs, const controller& rhs) {
      const auto lhs_integrity_hash = lhs.calculate_integrity_hash();
      const auto rhs_integrity_hash = rhs.calculate_integrity_hash();
      if (std::is_same_v<SNAPSHOT_SUITE, variant_snapshot_suite> && lhs_integrity_hash.str() != rhs_integrity_hash.str()) {
         auto lhs_latest_writer = SNAPSHOT_SUITE::get_writer();
         lhs.write_snapshot(lhs_latest_writer);
         auto lhs_latest = SNAPSHOT_SUITE::finalize(lhs_latest_writer);

         auto rhs_latest_writer = SNAPSHOT_SUITE::get_writer();
         rhs.write_snapshot(rhs_latest_writer);
         auto rhs_latest = SNAPSHOT_SUITE::finalize(rhs_latest_writer);

         print_variant_diff(lhs_latest, rhs_latest);
      }
      BOOST_REQUIRE_EQUAL(lhs_integrity_hash.str(), rhs_integrity_hash.str());
   }
}

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
   const chainbase::bfs::path parent_path = chain.get_config().blog.log_dir.parent_path();

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

   // create a new snapshot child
   auto writer = SNAPSHOT_SUITE::get_writer();
   chain.control->write_snapshot(writer);
   auto snapshot = SNAPSHOT_SUITE::finalize(writer);

   // create a new child at this snapshot
   int ordinal = 1;
   snapshotted_tester snap_chain(chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), ordinal++);
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *snap_chain.control);

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
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *snap_chain.control);

   // replay the block log from the snapshot child, from the snapshot
   using config_file_handling = snapshotted_tester::config_file_handling;
   snapshotted_tester replay_chain(snap_chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), ordinal++, config_file_handling::copy_config_files);
   const auto replay_head = replay_chain.control->head_block_num();
   auto snap_head = snap_chain.control->head_block_num();
   BOOST_REQUIRE_EQUAL(replay_head, snap_chain.control->last_irreversible_block_num());
   for (auto block_num = replay_head + 1; block_num <= snap_head; ++block_num) {
      auto block = snap_chain.control->fetch_block_by_number(block_num);
      replay_chain.push_block(block);
   }
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *replay_chain.control);

   auto block = chain.produce_block();
   chain.control->abort_block();
   snap_chain.push_block(block);
   replay_chain.push_block(block);
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *snap_chain.control);
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *replay_chain.control);

   snapshotted_tester replay2_chain(snap_chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), ordinal++, config_file_handling::copy_config_files);
   const auto replay2_head = replay2_chain.control->head_block_num();
   snap_head = snap_chain.control->head_block_num();
   BOOST_REQUIRE_EQUAL(replay2_head, snap_chain.control->last_irreversible_block_num());
   for (auto block_num = replay2_head + 1; block_num <= snap_head; ++block_num) {
      auto block = snap_chain.control->fetch_block_by_number(block_num);
      replay2_chain.push_block(block);
   }
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *replay2_chain.control);

   // verifies that chain's block_log has a genesis_state (and blocks starting at 1)
   controller::config copied_config = copy_config_and_files(chain.get_config(), ordinal++);
   auto genesis = chain::block_log::extract_genesis_state(chain.get_config().blog.log_dir);
   BOOST_REQUIRE(genesis);
   tester from_block_log_chain(copied_config, *genesis);
   const auto from_block_log_head = from_block_log_chain.control->head_block_num();
   BOOST_REQUIRE_EQUAL(from_block_log_head, snap_chain.control->last_irreversible_block_num());
   for (auto block_num = from_block_log_head + 1; block_num <= snap_head; ++block_num) {
      auto block = snap_chain.control->fetch_block_by_number(block_num);
      from_block_log_chain.push_block(block);
   }
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *from_block_log_chain.control);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_chain_id_in_snapshot, SNAPSHOT_SUITE, snapshot_suites)
{
   tester chain;
   const chainbase::bfs::path parent_path = chain.get_config().blog.log_dir.parent_path();

   chain.create_account(N(snapshot));
   chain.produce_blocks(1);
   chain.set_code(N(snapshot), contracts::snapshot_test_wasm());
   chain.set_abi(N(snapshot), contracts::snapshot_test_abi().data());
   chain.produce_blocks(1);
   chain.control->abort_block();

   // create a new snapshot child
   auto writer = SNAPSHOT_SUITE::get_writer();
   chain.control->write_snapshot(writer);
   auto snapshot = SNAPSHOT_SUITE::finalize(writer);

   snapshotted_tester snap_chain(chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), 0);
   BOOST_REQUIRE_EQUAL(chain.control->get_chain_id(), snap_chain.control->get_chain_id());
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *snap_chain.control);
}

bool should_write_snapshot() {
   auto compute = [] {
      auto argc = boost::unit_test::framework::master_test_suite().argc;
      auto argv = boost::unit_test::framework::master_test_suite().argv;
      return std::find(argv, argv + argc, std::string("--save-snapshot")) != (argv + argc);
   };
   static bool result = compute();
   return result;
}

#warning TODO: restore snapshot_tests/test_compatible_versions
/*
BOOST_AUTO_TEST_CASE_TEMPLATE(test_compatible_versions, SNAPSHOT_SUITE, snapshot_suites)
{
   const uint32_t legacy_default_max_inline_action_size = 4 * 1024;
   tester chain(setup_policy::preactivate_feature_and_new_bios, db_read_mode::SPECULATIVE, {legacy_default_max_inline_action_size});

   ///< Begin deterministic code to generate blockchain for comparison
   // TODO: create a utility that will write new bin/json gzipped files based on this
   chain.create_account(N(snapshot));
   chain.produce_blocks(1);
   chain.set_code(N(snapshot), contracts::snapshot_test_wasm());
   chain.set_abi(N(snapshot), contracts::snapshot_test_abi().data());
   chain.produce_blocks(1);
   chain.control->abort_block();
   std::string current_version = "v4";

   int ordinal = 0;
   for(std::string version : {"v2", "v3", "v4"})
   {
      if(should_write_snapshot() && version == current_version) continue;
      static_assert(chain_snapshot_header::minimum_compatible_version <= 2, "version 2 unit test is no longer needed.  Please clean up data files");
      auto old_snapshot = SNAPSHOT_SUITE::load_from_file("snap_" + version);
      BOOST_TEST_CHECKPOINT("loading snapshot: " << version);
      snapshotted_tester old_snapshot_tester(chain.get_config(), SNAPSHOT_SUITE::get_reader(old_snapshot), ordinal++);
      verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *old_snapshot_tester.control);

      // create a latest snapshot
      auto latest_writer = SNAPSHOT_SUITE::get_writer();
      old_snapshot_tester.control->write_snapshot(latest_writer);
      auto latest = SNAPSHOT_SUITE::finalize(latest_writer);

      // load the latest snapshot
      snapshotted_tester latest_tester(chain.get_config(), SNAPSHOT_SUITE::get_reader(latest), ordinal++);
      verify_integrity_hash<SNAPSHOT_SUITE>(*old_snapshot_tester.control, *latest_tester.control);
   }
   // This isn't quite fully automated.  The snapshots still need to be gzipped and moved to
   // the correct place in the source tree.
   if (should_write_snapshot())
   {
      // create a latest snapshot
      auto latest_writer = SNAPSHOT_SUITE::get_writer();
      chain.control->write_snapshot(latest_writer);
      auto latest = SNAPSHOT_SUITE::finalize(latest_writer);

      SNAPSHOT_SUITE::write_to_file("snap_" + current_version, latest);
   }
}
*/

/* TODO: need new bin/json gzipped files
// TODO: make this insensitive to abi_def changes, which isn't part of consensus or part of the database format
BOOST_AUTO_TEST_CASE_TEMPLATE(test_pending_schedule_snapshot, SNAPSHOT_SUITE, snapshot_suites)
{

   const uint32_t legacy_default_max_inline_action_size = 4 * 1024;
   tester chain(setup_policy::preactivate_feature_and_new_bios, db_read_mode::SPECULATIVE, {legacy_default_max_inline_action_size});
   auto genesis = chain::block_log::extract_genesis_state(chain.get_config().blocks_dir);
   BOOST_REQUIRE(genesis);
   BOOST_REQUIRE_EQUAL(genesis->compute_chain_id(), chain.control->get_chain_id());
   const auto& gpo = chain.control->get_global_properties();
   BOOST_REQUIRE_EQUAL(gpo.chain_id, chain.control->get_chain_id());
   auto block = chain.produce_block();
   BOOST_REQUIRE_EQUAL(block->block_num(), 3); // ensure that test setup stays consistent with original snapshot setup
   chain.create_account(N(snapshot));
   block = chain.produce_block();
   BOOST_REQUIRE_EQUAL(block->block_num(), 4);

   BOOST_REQUIRE_EQUAL(gpo.proposed_schedule.version, 0);
   BOOST_REQUIRE_EQUAL(gpo.proposed_schedule.producers.size(), 0);

   auto res = chain.set_producers_legacy( {N(snapshot)} );
   block = chain.produce_block();
   BOOST_REQUIRE_EQUAL(block->block_num(), 5);
   chain.control->abort_block();
   ///< End deterministic code to generate blockchain for comparison

   BOOST_REQUIRE_EQUAL(gpo.proposed_schedule.version, 1);
   BOOST_REQUIRE_EQUAL(gpo.proposed_schedule.producers.size(), 1);
   BOOST_REQUIRE_EQUAL(gpo.proposed_schedule.producers[0].producer_name.to_string(), "snapshot");

   static_assert(chain_snapshot_header::minimum_compatible_version <= 2, "version 2 unit test is no longer needed.  Please clean up data files");
   auto v2 = SNAPSHOT_SUITE::load_from_file("snap_v2_prod_sched");
   int ordinal = 0;

   ////////////////////////////////////////////////////////////////////////
   // Verify that the controller gets its chain_id from the snapshot
   ////////////////////////////////////////////////////////////////////////

   auto reader = SNAPSHOT_SUITE::get_reader(v2);
   snapshotted_tester v2_tester(chain.get_config(), reader, ordinal++);
   auto chain_id = chain::controller::extract_chain_id(*reader);
   BOOST_REQUIRE_EQUAL(chain_id, v2_tester.control->get_chain_id());
   BOOST_REQUIRE_EQUAL(chain.control->get_chain_id(), v2_tester.control->get_chain_id());
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *v2_tester.control);

   // create a latest version snapshot from the loaded v2 snapthos
   auto latest_from_v2_writer = SNAPSHOT_SUITE::get_writer();
   v2_tester.control->write_snapshot(latest_from_v2_writer);
   auto latest_from_v2 = SNAPSHOT_SUITE::finalize(latest_from_v2_writer);

   // load the latest snapshot in a new tester and compare integrity
   snapshotted_tester latest_from_v2_tester(chain.get_config(), SNAPSHOT_SUITE::get_reader(latest_from_v2), ordinal++);
   verify_integrity_hash<SNAPSHOT_SUITE>(*v2_tester.control, *latest_from_v2_tester.control);

   const auto& v2_gpo = v2_tester.control->get_global_properties();
   BOOST_REQUIRE_EQUAL(v2_gpo.proposed_schedule.version, 1);
   BOOST_REQUIRE_EQUAL(v2_gpo.proposed_schedule.producers.size(), 1);
   BOOST_REQUIRE_EQUAL(v2_gpo.proposed_schedule.producers[0].producer_name.to_string(), "snapshot");

   // produce block
   auto new_block = chain.produce_block();
   // undo the auto-pending from tester
   chain.control->abort_block();

   // push that block to all sub testers and validate the integrity of the database after it.
   v2_tester.push_block(new_block);
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *v2_tester.control);

   latest_from_v2_tester.push_block(new_block);
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *latest_from_v2_tester.control);
}
*/

BOOST_AUTO_TEST_CASE_TEMPLATE(test_restart_with_existing_state_and_truncated_block_log, SNAPSHOT_SUITE, snapshot_suites)
{
   tester chain;
   const chainbase::bfs::path parent_path = chain.get_config().blog.log_dir.parent_path();

   chain.create_account(N(snapshot));
   chain.produce_blocks(1);
   chain.set_code(N(snapshot), contracts::snapshot_test_wasm());
   chain.set_abi(N(snapshot), contracts::snapshot_test_abi().data());
   chain.produce_blocks(1);
   chain.control->abort_block();

   static const int pre_snapshot_block_count = 12;

   for (int itr = 0; itr < pre_snapshot_block_count; itr++) {
      // increment the contract
      chain.push_action(N(snapshot), N(increment), N(snapshot), mutable_variant_object()
                        ( "value", 1 )
                        );

      // produce block
      chain.produce_block();
   }

   chain.control->abort_block();

   // create a new snapshot child
   auto writer = SNAPSHOT_SUITE::get_writer();
   chain.control->write_snapshot(writer);
   auto snapshot = SNAPSHOT_SUITE::finalize(writer);

   // create a new child at this snapshot
   int ordinal = 1;
   snapshotted_tester snap_chain(chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), ordinal++);
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *snap_chain.control);
   auto block = chain.produce_block();
   chain.control->abort_block();
   snap_chain.push_block(block);
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *snap_chain.control);

   snap_chain.close();
   auto cfg = snap_chain.get_config();
   // restart chain with truncated block log and existing state, but no genesis state (chain_id)
   snap_chain.open();
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *snap_chain.control);

   block = chain.produce_block();
   chain.control->abort_block();
   snap_chain.push_block(block);
   verify_integrity_hash<SNAPSHOT_SUITE>(*chain.control, *snap_chain.control);
}

BOOST_AUTO_TEST_SUITE_END()
