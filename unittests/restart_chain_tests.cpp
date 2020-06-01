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
#include "test_cfd_transaction.hpp"

using namespace eosio;
using namespace testing;
using namespace chain;

void remove_existing_blocks(controller::config& config) {
   auto block_log_path = config.blocks_dir / "blocks.log";
   remove(block_log_path);
   auto block_index_path = config.blocks_dir / "blocks.index";
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
   restart_from_block_log_test_fixture() {
      chain.create_account(N(replay1));
      chain.produce_blocks(1);
      chain.create_account(N(replay2));
      chain.produce_blocks(1);
      chain.create_account(N(replay3));
      chain.produce_blocks(1);
      auto cutoff_block = chain.produce_block();
      cutoff_block_num  = cutoff_block->block_num();
      chain.produce_blocks(10);

      BOOST_REQUIRE_NO_THROW(chain.control->get_account(N(replay1)));
      BOOST_REQUIRE_NO_THROW(chain.control->get_account(N(replay2)));
      BOOST_REQUIRE_NO_THROW(chain.control->get_account(N(replay3)));

      chain.close();
   }
   ~restart_from_block_log_test_fixture() {
      controller::config copied_config = chain.get_config();
      auto               genesis       = chain::block_log::extract_genesis_state(chain.get_config().blocks_dir);
      BOOST_REQUIRE(genesis);

      // remove the state files to make sure we are starting from block log
      remove_existing_states(copied_config);
      tester from_block_log_chain(copied_config, *genesis);
      BOOST_REQUIRE_NO_THROW(from_block_log_chain.control->get_account(N(replay1)));
      BOOST_REQUIRE_NO_THROW(from_block_log_chain.control->get_account(N(replay2)));
      BOOST_REQUIRE_NO_THROW(from_block_log_chain.control->get_account(N(replay3)));
   }
};

struct light_validation_restart_from_block_log_test_fixture {
   tester chain;
   eosio::chain::transaction_trace_ptr trace;
   light_validation_restart_from_block_log_test_fixture()
       : chain(setup_policy::full) {

      deploy_test_api(chain);
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
   }

   ~light_validation_restart_from_block_log_test_fixture() {
      controller::config copied_config = chain.get_config();
      auto               genesis       = chain::block_log::extract_genesis_state(chain.get_config().blocks_dir);
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
};

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
   genesis.initial_timestamp = fc::from_iso_string<fc::time_point>("2020-01-01T00:00:01.000");
   genesis.initial_key       = eosio::testing::base_tester::get_public_key(config::system_account_name, "active");
   fc::optional<chain_id_type> chain_id = genesis.compute_chain_id();
   BOOST_REQUIRE_EXCEPTION(other.open(chain_id), chain_id_type_exception,
                           fc_exception_message_starts_with("chain ID in state "));
}

BOOST_FIXTURE_TEST_CASE(test_restart_from_block_log, restart_from_block_log_test_fixture) {
   BOOST_REQUIRE_NO_THROW(block_log::smoke_test(chain.get_config().blocks_dir, 1));
}

BOOST_FIXTURE_TEST_CASE(test_restart_from_trimmed_block_log, restart_from_block_log_test_fixture) {
   auto& config = chain.get_config();
   auto blocks_path = config.blocks_dir;
   remove_all(blocks_path/"reversible");
   BOOST_REQUIRE_NO_THROW(block_log::trim_blocklog_end(config.blocks_dir, cutoff_block_num));
}
   
BOOST_FIXTURE_TEST_CASE(test_light_validation_restart_from_block_log, light_validation_restart_from_block_log_test_fixture) {
}

BOOST_FIXTURE_TEST_CASE(test_light_validation_restart_from_block_log_with_pruned_trx, light_validation_restart_from_block_log_test_fixture) {
   const auto& blocks_dir = chain.get_config().blocks_dir;
   block_log blog(blocks_dir);
   std::vector<transaction_id_type> ids{trace->id};
   BOOST_CHECK(blog.prune_transactions(trace->block_num, ids) == 1);
   BOOST_CHECK(ids.empty());
   BOOST_REQUIRE_NO_THROW(block_log::repair_log(blocks_dir));
}

BOOST_AUTO_TEST_CASE(test_trim_blocklog_front) {
   tester chain;
   chain.produce_blocks(10);
   chain.produce_blocks(20);
   chain.close();

   namespace bfs = boost::filesystem;

   auto  blocks_dir = chain.get_config().blocks_dir;

   scoped_temp_path temp1, temp2;
   boost::filesystem::create_directory(temp1.path);
   bfs::copy(blocks_dir / "blocks.log", temp1.path / "blocks.log");
   bfs::copy(blocks_dir / "blocks.index", temp1.path / "blocks.index");
   BOOST_REQUIRE_NO_THROW(block_log::trim_blocklog_front(temp1.path, temp2.path, 10));
   BOOST_REQUIRE_NO_THROW(block_log::smoke_test(temp1.path, 1));

   block_log old_log(blocks_dir);
   block_log new_log(temp1.path);
   BOOST_CHECK(new_log.first_block_num() == 10);
   BOOST_CHECK(new_log.head()->block_num() == old_log.head()->block_num());
}

BOOST_AUTO_TEST_SUITE_END()
