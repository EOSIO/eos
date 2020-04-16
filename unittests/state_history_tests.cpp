#include <boost/test/unit_test.hpp>
#include <contracts.hpp>
#include <eosio/state_history/log.hpp>
#include <eosio/testing/tester.hpp>
#include <fc/io/json.hpp>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/filesystem.hpp>

using namespace eosio;
using namespace testing;
using namespace chain;

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

std::vector<chain::signed_block_ptr> deploy_test_api(eosio::testing::tester& chain) {
   std::vector<chain::signed_block_ptr> result;
   chain.create_account(N(testapi));
   chain.create_account(N(dummy));
   result.push_back(chain.produce_block());
   chain.set_code(N(testapi), contracts::test_api_wasm());
   result.push_back(chain.produce_block());
   return result;
}

eosio::chain::transaction_trace_ptr push_test_cfd_transaction(eosio::testing::tester& chain) {
   cf_action          cfa;
   signed_transaction trx;
   action             act({}, cfa);
   trx.context_free_actions.push_back(act);
   trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(100));
   trx.context_free_data.emplace_back(fc::raw::pack<uint32_t>(200));
   // add a normal action along with cfa
   dummy_action da = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   action       act1(vector<permission_level>{{N(testapi), config::active_name}}, da);
   trx.actions.push_back(act1);
   chain.set_transaction_headers(trx);
   // run normal passing case
   auto sigs = trx.sign(chain.get_private_key(N(testapi), "active"), chain.control->get_chain_id());
   return chain.push_transaction(trx);
}

state_history::partial_transaction_v0 get_partial_from_traces_bin(const bytes&               traces_bin,
                                                                  const transaction_id_type& id) {
   fc::datastream<const char*>                   strm(traces_bin.data(), traces_bin.size());
   std::vector<state_history::transaction_trace> traces;
   fc::raw::unpack(strm, traces);

   auto cfd_trace_itr = std::find_if(traces.begin(), traces.end(), [id](const state_history::transaction_trace& v) {
      return v.get<state_history::transaction_trace_v0>().id == id;
   });

   // make sure the trace with cfd can be found
   BOOST_REQUIRE(cfd_trace_itr != traces.end());
   BOOST_REQUIRE(cfd_trace_itr->contains<state_history::transaction_trace_v0>());
   auto trace_v0 = cfd_trace_itr->get<state_history::transaction_trace_v0>();
   BOOST_REQUIRE(trace_v0.partial);
   BOOST_REQUIRE(trace_v0.partial->contains<state_history::partial_transaction_v0>());
   return trace_v0.partial->get<state_history::partial_transaction_v0>();
}

struct scoped_temp_path {
   boost::filesystem::path path;
   scoped_temp_path() { path = boost::filesystem::unique_path(); }
   ~scoped_temp_path() {
      boost::filesystem::remove_all(path);
   }
};

struct trace_converter_test_fixture {
   packed_transaction::prunable_data_type::compression_type compression;

   void start() {
      tester chain;

      state_history::trace_converter converter_v0, converter_v1;
      std::map<uint32_t, bytes>      on_disk_log_entries_v0;
      std::map<uint32_t, bytes>      on_disk_log_entries_v1;

      chain.control->applied_transaction.connect(
          [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
             converter_v0.add_transaction(std::get<0>(t), std::get<1>(t));
             converter_v1.add_transaction(std::get<0>(t), std::get<1>(t));
          });

      chain.control->accepted_block.connect([&](const block_state_ptr& bs) {
         on_disk_log_entries_v0[bs->block_num] = converter_v0.pack(chain.control->db(), true, bs, 0, compression);
         on_disk_log_entries_v1[bs->block_num] = converter_v1.pack(chain.control->db(), true, bs, 1, compression);
      });

      deploy_test_api(chain);
      auto cfd_trace    = push_test_cfd_transaction(chain);
      chain.produce_blocks(10);

      BOOST_CHECK(on_disk_log_entries_v0.size());
      BOOST_CHECK(on_disk_log_entries_v1.size());

      // make sure v0 and v1 are identifical when transform to traces bin
      BOOST_CHECK(std::equal(on_disk_log_entries_v0.begin(), on_disk_log_entries_v0.end(),
                             on_disk_log_entries_v1.begin(), [&](const auto& entry_v0, const auto& entry_v1) {
                                return state_history::trace_converter::to_traces_bin_v0(entry_v0.second, 0) ==
                                       state_history::trace_converter::to_traces_bin_v0(entry_v1.second, 1);
                             }));

      // Now deserialize the on disk trace log and make sure that the cfd exists
      auto& cfd_entry_v1 = on_disk_log_entries_v1[cfd_trace->block_num];
      auto  partial =
          get_partial_from_traces_bin(state_history::trace_converter::to_traces_bin_v0(cfd_entry_v1, 1), cfd_trace->id);
      BOOST_REQUIRE(partial.context_free_data.size());
      BOOST_REQUIRE(partial.signatures.size());

      // prune the cfd for the block
      std::vector<transaction_id_type> ids{cfd_trace->id};
      auto                             prunable_cfd_entry_v1 = cfd_entry_v1;
      int                              offset = state_history::trace_converter::prune_traces(prunable_cfd_entry_v1, 1, ids);
      BOOST_CHECK(cfd_entry_v1.size() == prunable_cfd_entry_v1.size());
      BOOST_CHECK(offset != cfd_entry_v1.size());
      BOOST_CHECK(ids.size() == 0);

      auto [it1, it2] =
          std::mismatch(cfd_entry_v1.begin(), cfd_entry_v1.end(), prunable_cfd_entry_v1.begin(), prunable_cfd_entry_v1.end());
      BOOST_CHECK(it1 != cfd_entry_v1.end());
      BOOST_CHECK(it2 != prunable_cfd_entry_v1.end());

      // read the pruned trace and make sure the signature/cfd are empty
      auto pruned_partial = get_partial_from_traces_bin(state_history::trace_converter::to_traces_bin_v0(prunable_cfd_entry_v1, 1),
                                                  cfd_trace->id);
      BOOST_CHECK(pruned_partial.context_free_data.size() == 0);
      BOOST_CHECK(pruned_partial.signatures.size() == 0);
   }
};

BOOST_AUTO_TEST_SUITE(test_state_history)

BOOST_FIXTURE_TEST_CASE(test_trace_converter_test_no_compression, trace_converter_test_fixture) {
   compression = packed_transaction::prunable_data_type::compression_type::none;
   start();
}

BOOST_FIXTURE_TEST_CASE(test_trace_converter_test_zlib_compression, trace_converter_test_fixture) {
   compression = packed_transaction::prunable_data_type::compression_type::zlib;
   start();
}

BOOST_AUTO_TEST_CASE(test_trace_log) {
      tester chain;

      scoped_temp_path state_history_dir;
      fc::create_directories(state_history_dir.path);
      state_history_traces_log log(state_history_dir.path);

      chain.control->applied_transaction.connect(
          [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
             log.add_transaction(std::get<0>(t), std::get<1>(t));
          });

      chain.control->accepted_block.connect(
          [&](const block_state_ptr& bs) { log.store_traces(chain.control->db(), bs); });


      deploy_test_api(chain);
      auto cfd_trace    = push_test_cfd_transaction(chain);
      chain.produce_blocks(10);

      auto traces_bin = log.get_log_entry(cfd_trace->block_num);
      BOOST_REQUIRE(traces_bin);

      auto partial = get_partial_from_traces_bin(*traces_bin, cfd_trace->id);
      BOOST_REQUIRE(partial.context_free_data.size());
      BOOST_REQUIRE(partial.signatures.size());

      std::vector<transaction_id_type> ids{cfd_trace->id};
      log.prune_traces(cfd_trace->block_num, ids);
      BOOST_REQUIRE(ids.empty());

      auto pruned_traces_bin = log.get_log_entry(cfd_trace->block_num);
      BOOST_REQUIRE(pruned_traces_bin);

      auto pruned_partial = get_partial_from_traces_bin(*pruned_traces_bin, cfd_trace->id);
      BOOST_CHECK(pruned_partial.context_free_data.empty());
      BOOST_CHECK(pruned_partial.signatures.empty());
}

BOOST_AUTO_TEST_SUITE_END()
