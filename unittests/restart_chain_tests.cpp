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

struct dummy_action {
   static eosio::chain::name get_name() { return N(dummyaction); }
   static eosio::chain::name get_account() { return N(testapi); }

   char     a; // 1
   uint64_t b; // 8
   int32_t  c; // 4
};

struct cf_action {
   static eosio::chain::name get_name() { return N(cfaction); }
   static eosio::chain::name get_account() { return N(testapi); }

   uint32_t payload = 100;
   uint32_t cfd_idx = 0; // context free data index
};

FC_REFLECT(dummy_action, (a)(b)(c))
FC_REFLECT(cf_action, (payload)(cfd_idx))

#define DUMMY_ACTION_DEFAULT_A 0x45
#define DUMMY_ACTION_DEFAULT_B 0xab11cd1244556677
#define DUMMY_ACTION_DEFAULT_C 0x7451ae12

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
   fc::optional<chain_id_type> chain_id = genesis.compute_chain_id();
   BOOST_REQUIRE_EXCEPTION(other.open(chain_id), chain_id_type_exception,
                           fc_exception_message_starts_with("chain ID in state "));
}

BOOST_AUTO_TEST_CASE(test_restart_from_block_log) {
   tester chain;

   chain.create_account(N(replay1));
   chain.produce_blocks(1);
   chain.create_account(N(replay2));
   chain.produce_blocks(1);
   chain.create_account(N(replay3));
   chain.produce_blocks(1);

   BOOST_REQUIRE_NO_THROW(chain.control->get_account(N(replay1)));
   BOOST_REQUIRE_NO_THROW(chain.control->get_account(N(replay2)));
   BOOST_REQUIRE_NO_THROW(chain.control->get_account(N(replay3)));

   chain.close();

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

BOOST_AUTO_TEST_CASE(test_light_validation_restart_from_block_log) {
   tester chain(setup_policy::full);

   chain.create_account(N(testapi));
   chain.create_account(N(dummy));
   chain.produce_block();
   chain.set_code(N(testapi), contracts::test_api_wasm());
   chain.produce_block();

   cf_action          cfa;
   signed_transaction trx;
   action             act({}, cfa);
   trx.context_free_actions.push_back(act);
   trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(100)); // verify payload matches context free data
   trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(200));
   // add a normal action along with cfa
   dummy_action da = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   action       act1(vector<permission_level>{{N(testapi), config::active_name}}, da);
   trx.actions.push_back(act1);
   chain.set_transaction_headers(trx);
   // run normal passing case
   auto sigs  = trx.sign(chain.get_private_key(N(testapi), "active"), chain.control->get_chain_id());
   auto trace = chain.push_transaction(trx);
   chain.produce_block();

   BOOST_REQUIRE(trace->receipt);
   BOOST_CHECK_EQUAL(trace->receipt->status, transaction_receipt::executed);
   BOOST_CHECK_EQUAL(2, trace->action_traces.size());

   BOOST_CHECK(trace->action_traces.at(0).context_free); // cfa
   BOOST_CHECK_EQUAL("test\n", trace->action_traces.at(0).console); // cfa executed

   BOOST_CHECK(!trace->action_traces.at(1).context_free); // non-cfa
   BOOST_CHECK_EQUAL("", trace->action_traces.at(1).console);

   chain.close();

   controller::config copied_config = chain.get_config();
   auto               genesis       = chain::block_log::extract_genesis_state(chain.get_config().blocks_dir);
   BOOST_REQUIRE(genesis);

   // remove the state files to make sure we are starting from block log
   remove_existing_states(copied_config);
   transaction_trace_ptr other_trace;

   replay_tester from_block_log_chain(copied_config, *genesis,
                                      [&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> x) {
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

   BOOST_CHECK(other_trace->action_traces.at(0).context_free); // cfa
   BOOST_CHECK_EQUAL("", other_trace->action_traces.at(0).console); // cfa not executed for replay
   BOOST_CHECK_EQUAL(trace->action_traces.at(0).receipt->global_sequence, other_trace->action_traces.at(0).receipt->global_sequence);
   BOOST_CHECK_EQUAL(trace->action_traces.at(0).receipt->digest(), other_trace->action_traces.at(0).receipt->digest());

   BOOST_CHECK(!other_trace->action_traces.at(1).context_free); // non-cfa
   BOOST_CHECK_EQUAL("", other_trace->action_traces.at(1).console);
   BOOST_CHECK_EQUAL(trace->action_traces.at(1).receipt->global_sequence, other_trace->action_traces.at(1).receipt->global_sequence);
   BOOST_CHECK_EQUAL(trace->action_traces.at(1).receipt->digest(), other_trace->action_traces.at(1).receipt->digest());
}

BOOST_AUTO_TEST_SUITE_END()
