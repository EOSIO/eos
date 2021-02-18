#include <sstream>

#include <eosio/chain/block_log.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/snapshot.hpp>
#include <eosio/testing/tester.hpp>

#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

#include <contracts.hpp>
#include <snapshots.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <fc/io/cfile.hpp>
#include "test_cfd_transaction.hpp"

using namespace eosio;
using namespace testing;
using namespace chain;

void remove_existing_blocks(controller::config& config) {
   auto block_log_path = config.blog.log_dir / "blocks.log";
   remove(block_log_path);
   auto block_index_path = config.blog.log_dir / "blocks.index";
   remove(block_index_path);
}

void remove_existing_states(controller::config& config) {
   auto state_path = config.state_dir;
   remove_all(state_path);
   fc::create_directories(state_path);
}

class replay_tester : public base_tester {
 public:
   template <typename OnAppliedTrx>
   replay_tester(controller::config config, const genesis_state& genesis, OnAppliedTrx&& on_applied_trx) {
      cfg = config;
      base_tester::open(make_protocol_feature_set(), genesis.compute_chain_id(), [&genesis,&control=this->control, &on_applied_trx]() {        
         control->applied_transaction.connect(on_applied_trx);
         control->startup( [](){}, []() { return false; }, genesis );
      });
   }
   using base_tester::produce_block;

   signed_block_ptr produce_block(fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms)) override {
      return _produce_block(skip_time, false);
   }

   signed_block_ptr
   produce_empty_block(fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms)) override {
      unapplied_transactions.add_aborted(control->abort_block());
      return _produce_block(skip_time, true);
   }

   signed_block_ptr finish_block() override { return _finish_block(); }

   bool validate() { return true; }
};

struct restart_from_block_log_test_fixture {
   tester   chain;
   uint32_t cutoff_block_num;
   bool     fix_irreversible_blocks = false;
   restart_from_block_log_test_fixture() {
      chain.create_account("replay1"_n);
      chain.produce_blocks(1);
      chain.create_account("replay2"_n);
      chain.produce_blocks(1);
      chain.create_account("replay3"_n);
      chain.produce_blocks(1);
      auto cutoff_block = chain.produce_block();
      cutoff_block_num  = cutoff_block->block_num();
      chain.produce_blocks(10);

      BOOST_REQUIRE_NO_THROW(chain.control->get_account("replay1"_n));
      BOOST_REQUIRE_NO_THROW(chain.control->get_account("replay2"_n));
      BOOST_REQUIRE_NO_THROW(chain.control->get_account("replay3"_n));

      chain.close();
   }
   ~restart_from_block_log_test_fixture() {
      controller::config copied_config      = chain.get_config();
      copied_config.blog.fix_irreversible_blocks = this->fix_irreversible_blocks;
      auto genesis                          = chain::block_log::extract_genesis_state(chain.get_config().blog.log_dir);
      BOOST_REQUIRE(genesis);

      // remove the state files to make sure we are starting from block log
      remove_existing_states(copied_config);
      tester from_block_log_chain(copied_config, *genesis);
      BOOST_REQUIRE_NO_THROW(from_block_log_chain.control->get_account("replay1"_n));
      BOOST_REQUIRE_NO_THROW(from_block_log_chain.control->get_account("replay2"_n));
      BOOST_REQUIRE_NO_THROW(from_block_log_chain.control->get_account("replay3"_n));
   }
};

   

void  light_validation_restart_from_block_log_test_case(bool do_prune, uint32_t stride) {

   fc::temp_directory temp_dir;
   auto [ config, gen]  = tester::default_config(temp_dir);
   config.read_mode = db_read_mode::SPECULATIVE;
   config.blog.stride = stride;
   tester chain(config, gen);
   chain.execute_setup_policy(setup_policy::full);

   eosio::chain::transaction_trace_ptr trace;

   deploy_test_api(chain);
   chain.produce_blocks(10);
   trace = push_test_cfd_transaction(chain);
   chain.produce_blocks(10);

   BOOST_REQUIRE(trace->receipt);
   BOOST_CHECK_EQUAL(trace->receipt->status, transaction_receipt::executed);
   BOOST_CHECK_EQUAL(2, trace->action_traces.size());

   BOOST_CHECK(trace->action_traces.at(0).context_free);            // cfa
   BOOST_CHECK_EQUAL("test\n", trace->action_traces.at(0).console); // cfa executed

   BOOST_CHECK(!trace->action_traces.at(1).context_free); // non-cfa
   BOOST_CHECK_EQUAL("", trace->action_traces.at(1).console);

   chain.close();

   const auto&                      blocks_dir = chain.get_config().blog.log_dir;

   if (do_prune) {    
      block_log                        blog(chain.get_config().blog);
      std::vector<transaction_id_type> ids{trace->id};
      BOOST_CHECK(blog.prune_transactions(trace->block_num, ids) == 1);
      BOOST_CHECK(ids.empty());
      BOOST_REQUIRE_NO_THROW(block_log::repair_log(blocks_dir));
   }

   controller::config copied_config = chain.get_config();
   auto               genesis       = chain::block_log::extract_genesis_state(blocks_dir);
   BOOST_REQUIRE(genesis);

   // remove the state files to make sure we are starting from block log
   remove_existing_states(copied_config);

   transaction_trace_ptr other_trace;

   replay_tester from_block_log_chain(copied_config, *genesis,
                                       [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
                                          auto& t = std::get<0>(x);
                                          if (t && t->id == trace->id) {
                                             other_trace = t;
                                          }
                                       });

   BOOST_REQUIRE(other_trace);
   BOOST_REQUIRE(other_trace->receipt);
   BOOST_CHECK_EQUAL(other_trace->receipt->status, transaction_receipt::executed);
   BOOST_CHECK(*trace->receipt == *other_trace->receipt);
   BOOST_CHECK_EQUAL(2, other_trace->action_traces.size());

   BOOST_CHECK(other_trace->action_traces.at(0).context_free);      // cfa
   BOOST_CHECK_EQUAL("", other_trace->action_traces.at(0).console); // cfa not executed for replay
   BOOST_CHECK_EQUAL(trace->action_traces.at(0).receipt->global_sequence,
                     other_trace->action_traces.at(0).receipt->global_sequence);
   BOOST_CHECK_EQUAL(trace->action_traces.at(0).receipt->digest(),
                     other_trace->action_traces.at(0).receipt->digest());

   BOOST_CHECK(!other_trace->action_traces.at(1).context_free); // non-cfa
   BOOST_CHECK_EQUAL("", other_trace->action_traces.at(1).console);
   BOOST_CHECK_EQUAL(trace->action_traces.at(1).receipt->global_sequence,
                     other_trace->action_traces.at(1).receipt->global_sequence);
   BOOST_CHECK_EQUAL(trace->action_traces.at(1).receipt->digest(),
                     other_trace->action_traces.at(1).receipt->digest());
}


BOOST_AUTO_TEST_SUITE(restart_chain_tests)

BOOST_AUTO_TEST_CASE(test_existing_state_without_block_log) {
   tester chain;

   std::vector<signed_block_ptr> blocks;
   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());

   tester other;
   for (auto new_block : blocks) {
      other.push_block(new_block);
   }
   blocks.clear();

   other.close();
   auto cfg = other.get_config();
   remove_existing_blocks(cfg);
   // restarting chain with no block log and no genesis
   other.open();

   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());
   chain.control->abort_block();

   for (auto new_block : blocks) {
      other.push_block(new_block);
   }
}

BOOST_AUTO_TEST_CASE(test_restart_with_different_chain_id) {
   tester chain;

   std::vector<signed_block_ptr> blocks;
   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());
   blocks.push_back(chain.produce_block());

   tester other;
   for (auto new_block : blocks) {
      other.push_block(new_block);
   }
   blocks.clear();

   other.close();
   genesis_state genesis;
   genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:01.000");
   genesis.initial_key       = eosio::testing::base_tester::get_public_key(config::system_account_name, "active");
   std::optional<chain_id_type> chain_id = genesis.compute_chain_id();
   BOOST_REQUIRE_EXCEPTION(other.open(chain_id), chain_id_type_exception,
                           fc_exception_message_starts_with("chain ID in state "));
}

BOOST_FIXTURE_TEST_CASE(test_restart_from_block_log, restart_from_block_log_test_fixture) {
   BOOST_REQUIRE_NO_THROW(block_log::smoke_test(chain.get_config().blog.log_dir, 1));
}

BOOST_FIXTURE_TEST_CASE(test_restart_from_trimmed_block_log, restart_from_block_log_test_fixture) {
   auto& config = chain.get_config();
   auto blocks_path = config.blog.log_dir;
   remove_all(blocks_path/"reversible");
   BOOST_REQUIRE_NO_THROW(block_log::trim_blocklog_end(config.blog.log_dir, cutoff_block_num));
   block_log log(chain.get_config().blog);
   BOOST_CHECK(log.head()->block_num() == cutoff_block_num);
   BOOST_CHECK(fc::file_size(blocks_path / "blocks.index") == cutoff_block_num * sizeof(uint64_t));
}

BOOST_AUTO_TEST_CASE(test_light_validation_restart_from_block_log) {
   bool do_prune = false;
   uint32_t blocks_log_stride = UINT32_MAX;
   light_validation_restart_from_block_log_test_case(do_prune, blocks_log_stride);
}

BOOST_AUTO_TEST_CASE(test_light_validation_restart_from_block_log_with_pruned_trx) {
   bool do_prune = true;
   uint32_t blocks_log_stride = UINT32_MAX;
   light_validation_restart_from_block_log_test_case(do_prune, blocks_log_stride);
}

BOOST_AUTO_TEST_CASE(test_light_validation_restart_from_block_log_with_pruned_trx_and_split_log) {
   bool do_prune = true;
   uint32_t blocks_log_stride = 10;
   light_validation_restart_from_block_log_test_case(do_prune, blocks_log_stride);
}

BOOST_AUTO_TEST_CASE(test_split_log) {
   namespace bfs = boost::filesystem;
   fc::temp_directory temp_dir;

   tester chain(
         temp_dir,
         [](controller::config& config) {
            config.blog.archive_dir        = "archive";
            config.blog.stride             = 20;
            config.blog.max_retained_files = 5;
         },
         true);
   chain.produce_blocks(150);

   auto blocks_dir = chain.get_config().blog.log_dir;
   auto blocks_archive_dir = blocks_dir / chain.get_config().blog.archive_dir;

   BOOST_CHECK(bfs::exists( blocks_archive_dir / "blocks-1-20.log" ));
   BOOST_CHECK(bfs::exists( blocks_archive_dir / "blocks-1-20.index" ));
   BOOST_CHECK(bfs::exists( blocks_archive_dir / "blocks-21-40.log" ));
   BOOST_CHECK(bfs::exists( blocks_archive_dir / "blocks-21-40.index" ));

   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-41-60.log" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-41-60.index" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-61-80.log" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-61-80.index" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-81-100.log" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-81-100.index" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-101-120.log" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-101-120.index" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-121-140.log" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-121-140.index" ));

   BOOST_CHECK( ! chain.control->fetch_block_by_number(40) );
   
   BOOST_CHECK( chain.control->fetch_block_by_number(81)->block_num() == 81 );
   BOOST_CHECK( chain.control->fetch_block_by_number(90)->block_num() == 90 );
   BOOST_CHECK( chain.control->fetch_block_by_number(100)->block_num() == 100 );

   BOOST_CHECK( chain.control->fetch_block_by_number(41)->block_num() == 41 );
   BOOST_CHECK( chain.control->fetch_block_by_number(50)->block_num() == 50 );
   BOOST_CHECK( chain.control->fetch_block_by_number(60)->block_num() == 60 );

   BOOST_CHECK( chain.control->fetch_block_by_number(121)->block_num() == 121 );
   BOOST_CHECK( chain.control->fetch_block_by_number(130)->block_num() == 130 );
   BOOST_CHECK( chain.control->fetch_block_by_number(140)->block_num() == 140 );

   BOOST_CHECK( chain.control->fetch_block_by_number(145)->block_num() == 145);

   BOOST_CHECK( ! chain.control->fetch_block_by_number(160));
}

BOOST_AUTO_TEST_CASE(test_split_log_util1) {
   namespace bfs = boost::filesystem;
   fc::temp_directory temp_dir;

   tester chain;
   chain.produce_blocks(160);

   uint32_t head_block_num = chain.control->head_block_num();

   controller::config copied_config = chain.get_config();
   auto               genesis       = chain::block_log::extract_genesis_state(chain.get_config().blog.log_dir);
   BOOST_REQUIRE(genesis);

   chain.close();

   auto blocks_dir = chain.get_config().blog.log_dir;
   auto retained_dir = blocks_dir / "retained";
   block_log::split_blocklog(blocks_dir, retained_dir, 50);

   BOOST_CHECK(bfs::exists( retained_dir / "blocks-1-50.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "blocks-1-50.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "blocks-51-100.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "blocks-51-100.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "blocks-101-150.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "blocks-101-150.index" ));
   char buf[64];
   snprintf(buf, 64, "blocks-151-%u.log", head_block_num-1);
   bfs::path last_block_file = retained_dir / buf;
   snprintf(buf, 64, "blocks-151-%u.index", head_block_num-1);
   bfs::path last_index_file = retained_dir / buf;
   BOOST_CHECK(bfs::exists(last_block_file));
   BOOST_CHECK(bfs::exists( last_index_file ));

   bfs::rename(last_block_file, blocks_dir / "blocks.log");
   bfs::rename(last_index_file, blocks_dir / "blocks.index");


   // remove the state files to make sure we are starting from block log
   remove_existing_states(copied_config);
   // we need to remove the reversible blocks so that new blocks can be produced from the new chain
   bfs::remove_all(copied_config.blog.log_dir/"reversible");
   copied_config.blog.retained_dir       = retained_dir;
   copied_config.blog.stride             = 50;
   copied_config.blog.max_retained_files = 5;
   tester from_block_log_chain(copied_config, *genesis);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(1)->block_num() == 1);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(75)->block_num() == 75);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(100)->block_num() == 100);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(150)->block_num() == 150);
}

BOOST_AUTO_TEST_CASE(test_split_log_no_archive) {

   namespace bfs = boost::filesystem;
   fc::temp_directory temp_dir;

   tester chain(
         temp_dir,
         [](controller::config& config) {
            config.blog.archive_dir        = "";
            config.blog.stride             = 10;
            config.blog.max_retained_files = 5;
         },
         true);
   chain.produce_blocks(75);

   auto blocks_dir = chain.get_config().blog.log_dir;
   auto blocks_archive_dir = chain.get_config().blog.log_dir / chain.get_config().blog.archive_dir;

   BOOST_CHECK(!bfs::exists( blocks_archive_dir / "blocks-1-10.log" ));
   BOOST_CHECK(!bfs::exists( blocks_archive_dir / "blocks-1-10.index" ));
   BOOST_CHECK(!bfs::exists( blocks_archive_dir / "blocks-11-20.log" ));
   BOOST_CHECK(!bfs::exists( blocks_archive_dir / "blocks-11-20.index" ));
   BOOST_CHECK(!bfs::exists( blocks_dir / "blocks-1-10.log" ));
   BOOST_CHECK(!bfs::exists( blocks_dir / "blocks-1-10.index" ));
   BOOST_CHECK(!bfs::exists( blocks_dir / "blocks-11-20.log" ));
   BOOST_CHECK(!bfs::exists( blocks_dir / "blocks-11-20.index" ));

   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-21-30.log" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-21-30.index" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-31-40.log" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-31-40.index" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-41-50.log" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-41-50.index" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-51-60.log" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-51-60.index" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-61-70.log" ));
   BOOST_CHECK(bfs::exists( blocks_dir / "blocks-61-70.index" ));

   BOOST_CHECK( ! chain.control->fetch_block_by_number(10));
   BOOST_CHECK( chain.control->fetch_block_by_number(70));
   BOOST_CHECK( ! chain.control->fetch_block_by_number(80));
}

void split_log_replay(uint32_t replay_max_retained_block_files) {
   namespace bfs = boost::filesystem;
   fc::temp_directory temp_dir;

   const uint32_t stride = 20;

   tester chain(
         temp_dir,
         [](controller::config& config) {
            config.blog.stride             = stride;
            config.blog.max_retained_files = 10;
         },
         true);
   chain.produce_blocks(150);
   
   controller::config copied_config = chain.get_config();
   auto               genesis       = chain::block_log::extract_genesis_state(chain.get_config().blog.log_dir);
   BOOST_REQUIRE(genesis);

   chain.close();

   // remove the state files to make sure we are starting from block log
   remove_existing_states(copied_config);
   // we need to remove the reversible blocks so that new blocks can be produced from the new chain
   bfs::remove_all(copied_config.blog.log_dir/"reversible");
   copied_config.blog.stride        = stride;
   copied_config.blog.max_retained_files = replay_max_retained_block_files;
   tester from_block_log_chain(copied_config, *genesis);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(1)->block_num() == 1);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(75)->block_num() == 75);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(100)->block_num() == 100);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(150)->block_num() == 150);

   // produce new blocks to cross the blocks_log_stride boundary
   from_block_log_chain.produce_blocks(stride);

   const auto previous_chunk_end_block_num = (from_block_log_chain.control->head_block_num() / stride) * stride;
   const auto num_removed_blocks = std::min(stride * replay_max_retained_block_files, previous_chunk_end_block_num);
   const auto min_retained_block_number = previous_chunk_end_block_num - num_removed_blocks + 1;

   if (min_retained_block_number > 1) {
      // old blocks beyond the max_retained_block_files will no longer be available
      BOOST_CHECK(! from_block_log_chain.control->fetch_block_by_number(min_retained_block_number - 1));
   }
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(min_retained_block_number)->block_num() == min_retained_block_number);
}

BOOST_AUTO_TEST_CASE(test_split_log_replay_retained_block_files_10) {
   split_log_replay(10);
}

BOOST_AUTO_TEST_CASE(test_split_log_replay_retained_block_files_5) {
   split_log_replay(5);
}

BOOST_AUTO_TEST_CASE(test_split_log_replay_retained_block_files_1) {
   split_log_replay(1);
}

BOOST_AUTO_TEST_CASE(test_split_log_replay_retained_block_files_0) {
   split_log_replay(0);
}

BOOST_AUTO_TEST_CASE(test_restart_without_blocks_log_file) {

   namespace bfs = boost::filesystem;
   fc::temp_directory temp_dir;

   const uint32_t stride = 20;

   tester chain(
         temp_dir,
         [](controller::config& config) {
            config.blog.stride             = stride;
            config.blog.max_retained_files = 10;
         },
         true);
   chain.produce_blocks(160);
   
   controller::config copied_config = chain.get_config();
   auto               genesis       = chain::block_log::extract_genesis_state(chain.get_config().blog.log_dir);
   BOOST_REQUIRE(genesis);

   chain.close();

   // remove the state files to make sure we are starting from block log
   remove_existing_states(copied_config);
   // we need to remove the reversible blocks so that new blocks can be produced from the new chain
   bfs::remove_all(copied_config.blog.log_dir/"reversible");
   bfs::remove(copied_config.blog.log_dir/"blocks.log");
   bfs::remove(copied_config.blog.log_dir/"blocks.index");
   copied_config.blog.stride             = stride;
   copied_config.blog.max_retained_files = 10;
   tester from_block_log_chain(copied_config, *genesis);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(1)->block_num() == 1);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(75)->block_num() == 75);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(100)->block_num() == 100);
   BOOST_CHECK( from_block_log_chain.control->fetch_block_by_number(160)->block_num() == 160);

   from_block_log_chain.produce_blocks(10);
}

BOOST_FIXTURE_TEST_CASE(auto_fix_with_incomplete_head,restart_from_block_log_test_fixture) {
   auto& config = chain.get_config();
   auto blocks_path = config.blog.log_dir;
   // write a few random bytes to block log indicating the last block entry is incomplete
   fc::cfile logfile;
   logfile.set_file_path(config.blog.log_dir / "blocks.log");
   logfile.open("ab");
   const char random_data[] = "12345678901231876983271649837";
   logfile.write(random_data, sizeof(random_data));
   fix_irreversible_blocks = true;
}

BOOST_FIXTURE_TEST_CASE(auto_fix_with_corrupted_index,restart_from_block_log_test_fixture) {
   auto& config     = chain.get_config();
   auto blocks_path = config.blog.log_dir;
   // write a few random index to block log indicating the index is corrupted
   fc::cfile indexfile;
   indexfile.set_file_path(config.blog.log_dir / "blocks.index");
   indexfile.open("ab");
   uint64_t data = UINT64_MAX;
   indexfile.write(reinterpret_cast<const char*>(&data), sizeof(data));
   fix_irreversible_blocks = true;
}

BOOST_FIXTURE_TEST_CASE(auto_fix_with_corrupted_log_and_index,restart_from_block_log_test_fixture) {
   auto& config     = chain.get_config();
   auto blocks_path = config.blog.log_dir;
   // write a few random bytes to block log and index
   fc::cfile indexfile;
   indexfile.set_file_path(config.blog.log_dir / "blocks.index");
   indexfile.open("ab");
   const char random_index[] = "1234";
   indexfile.write(reinterpret_cast<const char*>(&random_index), sizeof(random_index));

   fc::cfile logfile;
   logfile.set_file_path(config.blog.log_dir / "blocks.log");
   logfile.open("ab");
   const char random_data[] = "12345678901231876983271649837";
   logfile.write(random_data, sizeof(random_data));

   fix_irreversible_blocks = true;
}

struct blocklog_version_setter {
   blocklog_version_setter(uint32_t ver) { block_log::set_version(ver); };
   ~blocklog_version_setter() { block_log::set_version(block_log::max_supported_version); };
};

BOOST_AUTO_TEST_CASE(test_split_from_v1_log) {
   namespace bfs = boost::filesystem;
   fc::temp_directory temp_dir;
   blocklog_version_setter set_version(1); 
   tester chain(
         temp_dir,
         [](controller::config& config) {
            config.blog.stride             = 20;
            config.blog.max_retained_files = 5;
         },
         true);
   chain.produce_blocks(75);

   BOOST_CHECK( chain.control->fetch_block_by_number(1)->block_num() == 1 );
   BOOST_CHECK( chain.control->fetch_block_by_number(21)->block_num() == 21 );
   BOOST_CHECK( chain.control->fetch_block_by_number(41)->block_num() == 41 );
   BOOST_CHECK( chain.control->fetch_block_by_number(75)->block_num() == 75 );
}

void trim_blocklog_front(uint32_t version) {
   blocklog_version_setter set_version(version); 
   tester chain;
   chain.produce_blocks(10);
   chain.produce_blocks(20);
   chain.close();

   namespace bfs = boost::filesystem;

   auto  blocks_dir     = chain.get_config().blog.log_dir;
   auto  old_index_size = fc::file_size(blocks_dir / "blocks.index");

   scoped_temp_path temp1, temp2;
   boost::filesystem::create_directory(temp1.path);
   bfs::copy(blocks_dir / "blocks.log", temp1.path / "blocks.log");
   bfs::copy(blocks_dir / "blocks.index", temp1.path / "blocks.index");
   BOOST_REQUIRE_NO_THROW(block_log::trim_blocklog_front(temp1.path, temp2.path, 10));
   BOOST_REQUIRE_NO_THROW(block_log::smoke_test(temp1.path, 1));

   block_log old_log(chain.get_config().blog);
   block_log new_log({ .log_dir = temp1.path});
   // double check if the version has been set to the desired version
   BOOST_CHECK(old_log.version() == version); 
   BOOST_CHECK(new_log.first_block_num() == 10);
   BOOST_CHECK(new_log.head()->block_num() == old_log.head()->block_num());

   int num_blocks_trimmed = 10 - 1;
   BOOST_CHECK(fc::file_size(temp1.path / "blocks.index") == old_index_size - sizeof(uint64_t) * num_blocks_trimmed);
}

BOOST_AUTO_TEST_CASE(test_trim_blocklog_front) { 
   trim_blocklog_front(block_log::max_supported_version); 
}

BOOST_AUTO_TEST_CASE(test_trim_blocklog_front_v1) {
   trim_blocklog_front(1);
}

BOOST_AUTO_TEST_CASE(test_trim_blocklog_front_v2) {
   trim_blocklog_front(2);
}

BOOST_AUTO_TEST_CASE(test_trim_blocklog_front_v3) {
   trim_blocklog_front(3);
}

BOOST_AUTO_TEST_CASE(test_split_log_util2) {
   namespace bfs = boost::filesystem;
   

   tester chain;
   chain.produce_blocks(160);
   chain.close();

   auto blocks_dir = chain.get_config().blog.log_dir;
   auto retained_dir = blocks_dir / "retained";
   fc::temp_directory temp_dir;

   BOOST_REQUIRE_NO_THROW(block_log::trim_blocklog_front(blocks_dir, temp_dir.path(), 50));
   BOOST_REQUIRE_NO_THROW(block_log::trim_blocklog_end(blocks_dir, 150));

   BOOST_CHECK_NO_THROW(block_log::split_blocklog(blocks_dir, retained_dir, 50));

   BOOST_CHECK(bfs::exists( retained_dir / "blocks-50-50.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "blocks-50-50.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "blocks-51-100.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "blocks-51-100.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "blocks-101-150.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "blocks-101-150.index" ));

   bfs::remove(blocks_dir/"blocks.log");
   bfs::remove(blocks_dir/"blocks.index");

   block_log blog({.log_dir = blocks_dir, .retained_dir = retained_dir });
   
   BOOST_CHECK(blog.version() != 0);
   BOOST_CHECK(blog.head().get());
}

BOOST_AUTO_TEST_SUITE_END()
