#include <boost/test/unit_test.hpp>
#include <contracts.hpp>
#include <eosio/state_history/log.hpp>
#include <eosio/testing/tester.hpp>
#include <fc/io/json.hpp>

#include "test_cfd_transaction.hpp"
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>

using namespace eosio;
using namespace testing;
using namespace chain;

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

BOOST_AUTO_TEST_SUITE(test_state_history)

BOOST_AUTO_TEST_CASE(test_trace_converter_test) {

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
      on_disk_log_entries_v0[bs->block_num] = converter_v0.pack(chain.control->db(), true, bs, 0);
      on_disk_log_entries_v1[bs->block_num] = converter_v1.pack(chain.control->db(), true, bs, 1);
   });

   chain.control->block_start.connect([&](uint32_t block_num) {
      converter_v0.clear_cache();
      converter_v1.clear_cache();
   });

   deploy_test_api(chain);
   auto cfd_trace = push_test_cfd_transaction(chain);
   chain.produce_blocks(1);

   BOOST_CHECK(on_disk_log_entries_v0.size());
   BOOST_CHECK(on_disk_log_entries_v1.size());

   // make sure v0 and v1 are identifical when transform to traces bin
   BOOST_CHECK(std::equal(on_disk_log_entries_v0.begin(), on_disk_log_entries_v0.end(), on_disk_log_entries_v1.begin(),
                          [&](const auto& entry_v0, const auto& entry_v1) {
                             return state_history::trace_converter::to_traces_bin_v0(entry_v0.second, 0) ==
                                    state_history::trace_converter::to_traces_bin_v0(entry_v1.second, 1);
                          }));

   // Now deserialize the on disk trace log and make sure that the cfd exists
   auto& cfd_entry_v1 = on_disk_log_entries_v1.at(cfd_trace->block_num);
   auto  partial =
       get_partial_from_traces_bin(state_history::trace_converter::to_traces_bin_v0(cfd_entry_v1, 1), cfd_trace->id);
   BOOST_REQUIRE(partial.context_free_data.size());
   BOOST_REQUIRE(partial.signatures.size());

   // prune the cfd for the block
   std::vector<transaction_id_type> ids{cfd_trace->id};
   auto                             offsets = state_history::trace_converter::prune_traces(cfd_entry_v1, 1, ids);
   BOOST_CHECK(offsets.first > 0 && offsets.second > 0);
   BOOST_CHECK(ids.size() == 0);

   // read the pruned trace and make sure the signature/cfd are empty
   auto pruned_partial =
       get_partial_from_traces_bin(state_history::trace_converter::to_traces_bin_v0(cfd_entry_v1, 1), cfd_trace->id);
   BOOST_CHECK(pruned_partial.context_free_data.size() == 0);
   BOOST_CHECK(pruned_partial.signatures.size() == 0);
}

BOOST_AUTO_TEST_CASE(test_trace_log) {
   namespace bfs = boost::filesystem;
   tester chain;

   scoped_temp_path state_history_dir;
   fc::create_directories(state_history_dir.path);
   state_history_traces_log log(state_history_dir.path);

   chain.control->applied_transaction.connect(
       [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
          log.add_transaction(std::get<0>(t), std::get<1>(t));
       });

   chain.control->accepted_block.connect([&](const block_state_ptr& bs) { log.store(chain.control->db(), bs); });
   chain.control->block_start.connect([&](uint32_t block_num) { log.block_start(block_num); } );

   deploy_test_api(chain);
   auto cfd_trace = push_test_cfd_transaction(chain);
   chain.produce_blocks(10);

   packed_transaction::prunable_data_type prunable;
   auto                                   x = prunable.prune_all();

   auto traces_bin = log.get_log_entry(cfd_trace->block_num);
   BOOST_REQUIRE(traces_bin);

   auto partial = get_partial_from_traces_bin(*traces_bin, cfd_trace->id);
   BOOST_REQUIRE(partial.context_free_data.size());
   BOOST_REQUIRE(partial.signatures.size());

   std::vector<transaction_id_type> ids{cfd_trace->id};
   log.prune_transactions(cfd_trace->block_num, ids);
   BOOST_REQUIRE(ids.empty());

   // we assume the nodeos has to be stopped while running, it can only be read
   // correctly with restart
   state_history_traces_log new_log(state_history_dir.path);
   auto                     pruned_traces_bin = new_log.get_log_entry(cfd_trace->block_num);
   BOOST_REQUIRE(pruned_traces_bin);

   auto pruned_partial = get_partial_from_traces_bin(*pruned_traces_bin, cfd_trace->id);
   BOOST_CHECK(pruned_partial.context_free_data.empty());
   BOOST_CHECK(pruned_partial.signatures.empty());
}

BOOST_AUTO_TEST_SUITE_END()
