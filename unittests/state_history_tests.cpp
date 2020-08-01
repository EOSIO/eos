#include <boost/test/unit_test.hpp>
#include <contracts.hpp>
#include <eosio/state_history/create_deltas.hpp>
#include <eosio/state_history/log.hpp>
#include <eosio/state_history/trace_converter.hpp>
#include <eosio/testing/tester.hpp>
#include <fc/io/json.hpp>

#include "test_cfd_transaction.hpp"
#include <boost/filesystem.hpp>

#undef N

#include <eosio/ship_protocol.hpp>
#include <eosio/stream.hpp>

using namespace eosio;
using namespace testing;
using namespace chain;
using prunable_data_type = eosio::chain::packed_transaction::prunable_data_type;

namespace bio = boost::iostreams;
extern const char* const state_history_plugin_abi;

prunable_data_type::prunable_data_t get_prunable_data_from_traces(std::vector<state_history::transaction_trace>& traces,
                                                                  const transaction_id_type&                     id) {
   auto cfd_trace_itr = std::find_if(traces.begin(), traces.end(), [id](const state_history::transaction_trace& v) {
      return v.get<state_history::transaction_trace_v0>().id == id;
   });

   // make sure the trace with cfd can be found
   BOOST_REQUIRE(cfd_trace_itr != traces.end());
   BOOST_REQUIRE(cfd_trace_itr->contains<state_history::transaction_trace_v0>());
   auto trace_v0 = cfd_trace_itr->get<state_history::transaction_trace_v0>();
   BOOST_REQUIRE(trace_v0.partial);
   BOOST_REQUIRE(trace_v0.partial->contains<state_history::partial_transaction_v1>());
   return trace_v0.partial->get<state_history::partial_transaction_v1>().prunable_data->prunable_data;
}

prunable_data_type::prunable_data_t get_prunable_data_from_traces_bin(const std::vector<char>&   entry,
                                                                      const transaction_id_type& id) {
   fc::datastream<const char*>                   strm(entry.data(), entry.size());
   std::vector<state_history::transaction_trace> traces;
   state_history::trace_converter::unpack(strm, traces);
   return get_prunable_data_from_traces(traces, id);
}

struct state_history_abi_serializer {
   tester&        chain;
   abi_serializer sr;

   state_history_abi_serializer(tester& chn)
       : chain(chn)
       , sr(fc::json::from_string(state_history_plugin_abi).as<abi_def>(),
            abi_serializer::create_yield_function(chain.abi_serializer_max_time)) {}

   fc::variant deserialize(const chain::bytes& data, const char* type) {
      fc::datastream<const char*> strm(data.data(), data.size());
      auto                        result =
          sr.binary_to_variant(type, strm, abi_serializer::create_yield_function(chain.abi_serializer_max_time));
      BOOST_CHECK(data.size() == strm.tellp());
      return result;
   }
};

BOOST_AUTO_TEST_SUITE(test_state_history)

BOOST_AUTO_TEST_CASE(test_trace_converter) {

   tester chain;
   using namespace eosio::state_history;

   state_history::transaction_trace_cache cache;
   std::map<uint32_t, chain::bytes>       on_disk_log_entries;

   chain.control->applied_transaction.connect(
       [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
          cache.add_transaction(std::get<0>(t), std::get<1>(t));
       });

   chain.control->accepted_block.connect([&](const block_state_ptr& bs) {
      auto                              traces = cache.prepare_traces(bs);
      fc::datastream<std::vector<char>> strm;
      state_history::trace_converter::pack(strm, chain.control->db(), true, traces,
                                           state_history::compression_type::zlib);
      on_disk_log_entries[bs->block_num] = strm.storage();
   });

   chain.control->block_start.connect([&](uint32_t block_num) {
      cache.clear();
   });

   deploy_test_api(chain);
   auto cfd_trace = push_test_cfd_transaction(chain);
   chain.produce_blocks(1);

   BOOST_CHECK(on_disk_log_entries.size());

   // Now deserialize the on disk trace log and make sure that the cfd exists
   auto& cfd_entry = on_disk_log_entries.at(cfd_trace->block_num);
   BOOST_REQUIRE(!get_prunable_data_from_traces_bin(cfd_entry, cfd_trace->id).contains<prunable_data_type::none>());

   // prune the cfd for the block
   std::vector<transaction_id_type> ids{cfd_trace->id};
   fc::datastream<char*>            rw_strm(cfd_entry.data(), cfd_entry.size());
   state_history::trace_converter::prune_traces(rw_strm, cfd_entry.size(), ids);
   BOOST_CHECK(ids.size() == 0);

   // read the pruned trace and make sure it's pruned
   BOOST_CHECK(get_prunable_data_from_traces_bin(cfd_entry, cfd_trace->id).contains<prunable_data_type::none>());
}

BOOST_AUTO_TEST_CASE(test_trace_log) {
   namespace bfs = boost::filesystem;
   tester chain;

   scoped_temp_path state_history_dir;
   fc::create_directories(state_history_dir.path);
   state_history_traces_log log({ .log_dir = state_history_dir.path });

   chain.control->applied_transaction.connect(
       [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
          log.add_transaction(std::get<0>(t), std::get<1>(t));
       });

   chain.control->accepted_block.connect([&](const block_state_ptr& bs) { log.store(chain.control->db(), bs); });
   chain.control->block_start.connect([&](uint32_t block_num) { log.block_start(block_num); } );

   deploy_test_api(chain);
   auto cfd_trace = push_test_cfd_transaction(chain);
   chain.produce_blocks(1);

   auto traces = log.get_traces(cfd_trace->block_num);
   BOOST_REQUIRE(traces.size());

   BOOST_REQUIRE(!get_prunable_data_from_traces(traces, cfd_trace->id).contains<prunable_data_type::none>());

   std::vector<transaction_id_type> ids{cfd_trace->id};
   log.prune_transactions(cfd_trace->block_num, ids);
   BOOST_REQUIRE(ids.empty());

   // we assume the nodeos has to be stopped while running, it can only be read
   // correctly with restart
   state_history_traces_log new_log({ .log_dir = state_history_dir.path });
   auto                     pruned_traces = new_log.get_traces(cfd_trace->block_num);
   BOOST_REQUIRE(pruned_traces.size());

   BOOST_CHECK(get_prunable_data_from_traces(pruned_traces, cfd_trace->id).contains<prunable_data_type::none>());
}

BOOST_AUTO_TEST_CASE(test_chain_state_log) {

   namespace bfs = boost::filesystem;
   tester chain;

   scoped_temp_path state_history_dir;
   fc::create_directories(state_history_dir.path);
   state_history_chain_state_log log({ .log_dir = state_history_dir.path });

   uint32_t last_accepted_block_num = 0;

   chain.control->accepted_block.connect([&](const block_state_ptr& block_state) {
      log.store(chain.control->db(), block_state);
      last_accepted_block_num = block_state->block_num;
   });

   chain.produce_blocks(10);

   chain::bytes                                   entry = log.get_log_entry(last_accepted_block_num);
   std::vector<eosio::ship_protocol::table_delta> deltas;
   eosio::input_stream                            deltas_bin{entry.data(), entry.data() + entry.size()};
   BOOST_CHECK_NO_THROW(from_bin(deltas, deltas_bin));
}


struct state_history_tester_logs  {
   state_history_tester_logs(const state_history_config& config) 
      : traces_log(config) , chain_state_log(config) {}

   state_history_traces_log traces_log;
   state_history_chain_state_log chain_state_log;
};

struct state_history_tester : state_history_tester_logs, tester {
   state_history_tester(const state_history_config& config) 
   : state_history_tester_logs(config), tester ([&](eosio::chain::controller& control) {
      control.applied_transaction.connect(
       [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
          traces_log.add_transaction(std::get<0>(t), std::get<1>(t));
       });

      control.accepted_block.connect([&](const block_state_ptr& bs) { 
         traces_log.store(control.db(), bs); 
         chain_state_log.store(control.db(), bs); 
      });
      control.block_start.connect([&](uint32_t block_num) { traces_log.block_start(block_num); } );
   }) {}
};

BOOST_AUTO_TEST_CASE(test_splitted_log) {
   namespace bfs = boost::filesystem;

   scoped_temp_path state_history_dir;
   fc::create_directories(state_history_dir.path);

   state_history_config config{
      .log_dir = state_history_dir.path,
      .retained_dir = "retained",
      .archive_dir = "archive",
      .stride  = 20,
      .max_retained_files = 5 
   };

   state_history_tester chain(config);
   chain.produce_blocks(50);

   deploy_test_api(chain);
   auto cfd_trace = push_test_cfd_transaction(chain);

   chain.produce_blocks(100);


   auto log_dir = state_history_dir.path;
   auto archive_dir  = log_dir / "archive";
   auto retained_dir = log_dir / "retained";

   BOOST_CHECK(bfs::exists( archive_dir / "trace_history-2-20.log" ));
   BOOST_CHECK(bfs::exists( archive_dir / "trace_history-2-20.index" ));
   BOOST_CHECK(bfs::exists( archive_dir / "trace_history-21-40.log" ));
   BOOST_CHECK(bfs::exists( archive_dir / "trace_history-21-40.index" ));

   BOOST_CHECK(bfs::exists( archive_dir / "chain_state_history-2-20.log" ));
   BOOST_CHECK(bfs::exists( archive_dir / "chain_state_history-2-20.index" ));
   BOOST_CHECK(bfs::exists( archive_dir / "chain_state_history-21-40.log" ));
   BOOST_CHECK(bfs::exists( archive_dir / "chain_state_history-21-40.index" ));

   BOOST_CHECK(bfs::exists( retained_dir / "trace_history-41-60.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "trace_history-41-60.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "trace_history-61-80.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "trace_history-61-80.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "trace_history-81-100.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "trace_history-81-100.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "trace_history-101-120.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "trace_history-101-120.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "trace_history-121-140.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "trace_history-121-140.index" ));

   BOOST_CHECK(bfs::exists( retained_dir / "chain_state_history-41-60.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "chain_state_history-41-60.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "chain_state_history-61-80.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "chain_state_history-61-80.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "chain_state_history-81-100.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "chain_state_history-81-100.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "chain_state_history-101-120.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "chain_state_history-101-120.index" ));
   BOOST_CHECK(bfs::exists( retained_dir / "chain_state_history-121-140.log" ));
   BOOST_CHECK(bfs::exists( retained_dir / "chain_state_history-121-140.index" ));

   BOOST_CHECK(chain.traces_log.get_traces(10).empty());
   BOOST_CHECK(chain.traces_log.get_traces(100).size());
   BOOST_CHECK(chain.traces_log.get_traces(140).size());
   BOOST_CHECK(chain.traces_log.get_traces(150).size());
   BOOST_CHECK(chain.traces_log.get_traces(160).empty());

   BOOST_CHECK(chain.chain_state_log.get_log_entry(10).empty());
   BOOST_CHECK(chain.chain_state_log.get_log_entry(100).size());
   BOOST_CHECK(chain.chain_state_log.get_log_entry(140).size());
   BOOST_CHECK(chain.chain_state_log.get_log_entry(150).size());
   BOOST_CHECK(chain.chain_state_log.get_log_entry(160).empty());

   auto traces = chain.traces_log.get_traces(cfd_trace->block_num);
   BOOST_REQUIRE(traces.size());

   BOOST_REQUIRE(!get_prunable_data_from_traces(traces, cfd_trace->id).contains<prunable_data_type::none>());

   std::vector<transaction_id_type> ids{cfd_trace->id};
   chain.traces_log.prune_transactions(cfd_trace->block_num, ids);
   BOOST_REQUIRE(ids.empty());

   // we assume the nodeos has to be stopped while running, it can only be read
   // correctly with restart
   state_history_traces_log new_log(config);
   auto                     pruned_traces = new_log.get_traces(cfd_trace->block_num);
   BOOST_REQUIRE(pruned_traces.size());

   BOOST_CHECK(get_prunable_data_from_traces(pruned_traces, cfd_trace->id).contains<prunable_data_type::none>());
}

BOOST_AUTO_TEST_CASE(test_corrupted_log_recovery) {
  namespace bfs = boost::filesystem;

   scoped_temp_path state_history_dir;
   fc::create_directories(state_history_dir.path);

   state_history_config config{
      .log_dir = state_history_dir.path,
      .archive_dir = "archive",
      .stride  = 100,
      .max_retained_files = 5 
   };

   state_history_tester chain(config);
   chain.produce_blocks(50);
   chain.close();

   // write a few random bytes to block log indicating the last block entry is incomplete
   fc::cfile logfile;
   logfile.set_file_path(state_history_dir.path / "trace_history.log");
   logfile.open("ab");
   const char random_data[] = "12345678901231876983271649837";
   logfile.write(random_data, sizeof(random_data));

   bfs::remove_all(chain.get_config().blog.log_dir/"reversible");

   state_history_tester new_chain(config);
   new_chain.produce_blocks(50);

   BOOST_CHECK(new_chain.traces_log.get_traces(10).size());
   BOOST_CHECK(new_chain.chain_state_log.get_log_entry(10).size());
}


BOOST_AUTO_TEST_CASE(test_state_result_abi) {
   using namespace state_history;

   tester chain;

   transaction_trace_cache          trace_cache;
   std::map<uint32_t, chain::bytes> history;
   fc::optional<block_position>     prev_block;

   chain.control->applied_transaction.connect(
       [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
          trace_cache.add_transaction(std::get<0>(t), std::get<1>(t));
       });

   chain.control->accepted_block.connect([&](const block_state_ptr& block_state) {
      auto& control = chain.control;

      fc::datastream<std::vector<char>> strm;
      trace_converter::pack(strm, control->db(), false, trace_cache.prepare_traces(block_state),
                            compression_type::none);
      strm.seekp(0);

      get_blocks_result_v1 message;
      message.head = block_position{control->head_block_num(), control->head_block_id()};
      message.last_irreversible =
          state_history::block_position{control->last_irreversible_block_num(), control->last_irreversible_block_id()};
      message.this_block = state_history::block_position{block_state->block->block_num(), block_state->id};
      message.prev_block = prev_block;
      message.block      = block_state->block;
      std::vector<state_history::transaction_trace> traces;
      state_history::trace_converter::unpack(strm, traces);
      message.traces = traces;
      message.deltas = fc::raw::pack(state_history::create_deltas(control->db(), !prev_block));

      prev_block                         = message.this_block;
      history[control->head_block_num()] = fc::raw::pack(state_history::state_result{message});
   });

   deploy_test_api(chain);
   auto cfd_trace = push_test_cfd_transaction(chain);
   chain.produce_blocks(1);

   state_history_abi_serializer serializer(chain);

   for (auto& [key, value] : history) {
      {
         // check the validity of the abi string
         auto result = serializer.deserialize(value, "result");
        
         auto& result_variant = result.get_array();
         BOOST_CHECK(result_variant[0].as_string() == "get_blocks_result_v1");
         auto& get_blocks_result_v1 = result_variant[1].get_object();

         chain::bytes traces_bin;
         fc::from_variant(get_blocks_result_v1["traces"], traces_bin);
         BOOST_CHECK_NO_THROW( serializer.deserialize(traces_bin, "transaction_trace[]") );
      
         chain::bytes deltas_bin;
         fc::from_variant(get_blocks_result_v1["deltas"], deltas_bin);
         BOOST_CHECK_NO_THROW( serializer.deserialize(deltas_bin, "table_delta[]"));
      }
      {
         // check the validity of abieos ship_protocol type definitions
         eosio::input_stream          bin{value.data(), value.data() + value.size()};
         eosio::ship_protocol::result result;
         BOOST_CHECK_NO_THROW(from_bin(result, bin));
         BOOST_CHECK(bin.remaining() == 0);

         auto& blocks_result_v1 = std::get<eosio::ship_protocol::get_blocks_result_v1>(result);

         std::vector<eosio::ship_protocol::transaction_trace> traces;
         BOOST_CHECK_NO_THROW(blocks_result_v1.traces.unpack(traces));

         std::vector<eosio::ship_protocol::table_delta> deltas;
         BOOST_CHECK_NO_THROW(blocks_result_v1.deltas.unpack(deltas));
      }
   }
}

BOOST_AUTO_TEST_SUITE_END()
