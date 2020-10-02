#include <eosio/chain/authorization_manager.hpp>
#include <boost/test/unit_test.hpp>
#include <contracts.hpp>
#include <eosio/state_history/create_deltas.hpp>
#include <eosio/state_history/log.hpp>
#include <eosio/state_history/trace_converter.hpp>
#include <eosio/testing/tester.hpp>
#include <fc/io/json.hpp>

#include "test_cfd_transaction.hpp"
#include <boost/filesystem.hpp>

#pragma push_macro("N")
#undef N
#include <eosio/stream.hpp>
#include <eosio/ship_protocol.hpp>
#pragma pop_macro("N")

using namespace eosio;
using namespace testing;
using namespace chain;
using namespace std::literals;
using prunable_data_type = eosio::chain::packed_transaction::prunable_data_type;

namespace bio = boost::iostreams;
extern const char* const state_history_plugin_abi;

prunable_data_type::prunable_data_t get_prunable_data_from_traces(std::vector<state_history::transaction_trace>& traces,
                                                                  const transaction_id_type&                     id) {
   auto cfd_trace_itr = std::find_if(traces.begin(), traces.end(), [id](const state_history::transaction_trace& v) {
      return std::get<state_history::transaction_trace_v0>(v).id == id;
   });

   // make sure the trace with cfd can be found
   BOOST_REQUIRE(cfd_trace_itr != traces.end());
   BOOST_REQUIRE(std::holds_alternative<state_history::transaction_trace_v0>(*cfd_trace_itr));
   auto trace_v0 = std::get<state_history::transaction_trace_v0>(*cfd_trace_itr);
   BOOST_REQUIRE(trace_v0.partial);
   BOOST_REQUIRE(std::holds_alternative<state_history::partial_transaction_v1>(*trace_v0.partial));
   return std::get<state_history::partial_transaction_v1>(*trace_v0.partial).prunable_data->prunable_data;
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
   BOOST_REQUIRE(!std::holds_alternative<prunable_data_type::none>(get_prunable_data_from_traces_bin(cfd_entry, cfd_trace->id)));

   // prune the cfd for the block
   std::vector<transaction_id_type> ids{cfd_trace->id};
   fc::datastream<char*>            rw_strm(cfd_entry.data(), cfd_entry.size());
   state_history::trace_converter::prune_traces(rw_strm, cfd_entry.size(), ids);
   BOOST_CHECK(ids.size() == 0);

   // read the pruned trace and make sure it's pruned
   BOOST_CHECK(std::holds_alternative<prunable_data_type::none>(get_prunable_data_from_traces_bin(cfd_entry, cfd_trace->id)));
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

   chain.control->accepted_block.connect([&](const block_state_ptr& bs) { log.store(chain.control->combined_db(), bs); });
   chain.control->block_start.connect([&](uint32_t block_num) { log.block_start(block_num); } );

   deploy_test_api(chain);
   auto cfd_trace = push_test_cfd_transaction(chain);
   chain.produce_blocks(1);

   auto traces = log.get_traces(cfd_trace->block_num);
   BOOST_REQUIRE(traces.size());

   BOOST_REQUIRE(!std::holds_alternative<prunable_data_type::none>(get_prunable_data_from_traces(traces, cfd_trace->id)));

   std::vector<transaction_id_type> ids{cfd_trace->id};
   log.prune_transactions(cfd_trace->block_num, ids);
   BOOST_REQUIRE(ids.empty());

   // we assume the nodeos has to be stopped while running, it can only be read
   // correctly with restart
   state_history_traces_log new_log({ .log_dir = state_history_dir.path });
   auto                     pruned_traces = new_log.get_traces(cfd_trace->block_num);
   BOOST_REQUIRE(pruned_traces.size());

   BOOST_CHECK(std::holds_alternative<prunable_data_type::none>(get_prunable_data_from_traces(pruned_traces, cfd_trace->id)));
}

BOOST_AUTO_TEST_CASE(test_chain_state_log) {

   namespace bfs = boost::filesystem;
   tester chain;

   scoped_temp_path state_history_dir;
   fc::create_directories(state_history_dir.path);
   state_history_chain_state_log log({ .log_dir = state_history_dir.path });

   uint32_t last_accepted_block_num = 0;

   chain.control->accepted_block.connect([&](const block_state_ptr& block_state) {
      log.store(chain.control->combined_db(), block_state);
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
         traces_log.store(control.combined_db(), bs); 
         chain_state_log.store(control.combined_db(), bs); 
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

   BOOST_REQUIRE(!std::holds_alternative<prunable_data_type::none>(get_prunable_data_from_traces(traces, cfd_trace->id)));

   std::vector<transaction_id_type> ids{cfd_trace->id};
   chain.traces_log.prune_transactions(cfd_trace->block_num, ids);
   BOOST_REQUIRE(ids.empty());

   // we assume the nodeos has to be stopped while running, it can only be read
   // correctly with restart
   state_history_traces_log new_log(config);
   auto                     pruned_traces = new_log.get_traces(cfd_trace->block_num);
   BOOST_REQUIRE(pruned_traces.size());

   BOOST_CHECK(std::holds_alternative<prunable_data_type::none>(get_prunable_data_from_traces(pruned_traces, cfd_trace->id)));
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
   std::optional<block_position>    prev_block;

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

BOOST_AUTO_TEST_CASE(test_traces_present)
{
   namespace bfs = boost::filesystem;
   tester chain;
   deploy_test_api(chain);

   scoped_temp_path state_history_dir;
   fc::create_directories(state_history_dir.path);
   state_history_config config;
   config.log_dir = state_history_dir.path;
   state_history_traces_log log(config);

   bool onblock_test_executed = false;

   chain.control->applied_transaction.connect(
      [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
         const transaction_trace_ptr &trace_ptr = std::get<0>(t);
         const chain::packed_transaction_ptr &transaction = std::get<1>(t);
         log.add_transaction(trace_ptr, transaction);

         // see issue #9159
         if (!trace_ptr->action_traces.empty() && trace_ptr->action_traces[0].act.name == N(onblock)) {
            BOOST_CHECK(chain::is_onblock(*trace_ptr));
            trace_ptr->action_traces.clear();
            BOOST_CHECK(!chain::is_onblock(*trace_ptr));
            onblock_test_executed = true;
         }
   });
   chain.control->accepted_block.connect([&](const block_state_ptr& bs) { log.store(chain.control->combined_db(), bs); });

   auto tr_ptr = chain.create_account(N(newacc));

   chain.produce_block();

   BOOST_CHECK(onblock_test_executed);

   auto traces = log.get_traces(tr_ptr->block_num);
   BOOST_REQUIRE_EQUAL(traces.size(), 1);

   auto trace_itr = std::find_if(traces.begin(), traces.end(), [tr_ptr](const state_history::transaction_trace& v) {
      return std::get<state_history::transaction_trace_v0>(v).id == tr_ptr->id;
   });

   BOOST_REQUIRE(trace_itr != traces.end());

   auto &action_traces = std::get<state_history::transaction_trace_v0>(*trace_itr).action_traces;

   auto new_account_action_itr = std::find_if(action_traces.begin(), action_traces.end(), [](const state_history::action_trace& v) {
      return std::get<state_history::action_trace_v1>(v).act.name == N(newaccount).to_uint64_t();
   });

   BOOST_REQUIRE(new_account_action_itr != action_traces.end());
}


class table_deltas_tester : public tester {
public:
   using tester::tester;
   using deltas_vector = vector<eosio::state_history::table_delta>;

   pair<bool, deltas_vector::iterator> find_table_delta(const std::string &name) {
      v = eosio::state_history::create_deltas(control->db(), false);;

      auto find_by_name = [&name](const auto& x) {
         return x.name == name;
      };

      auto it = std::find_if(v.begin(), v.end(), find_by_name);

      return make_pair(it != v.end(), it);
   }

   template <typename A, typename B>
   vector<A> deserialize_data(deltas_vector::iterator &it) {
      vector<A> result;
      for(int i=0; i < it->rows.obj.size(); i++) {
         eosio::input_stream stream{it->rows.obj[i].second.data(), it->rows.obj[i].second.size()};
         result.push_back(std::get<A>(eosio::from_bin<B>(stream)));
      }
      return result;
   }

private:
   deltas_vector v;
};

BOOST_AUTO_TEST_CASE(test_deltas_account_creation) {
   table_deltas_tester chain;
   chain.produce_block();

   // Check that no account table deltas are present
   BOOST_REQUIRE_EQUAL(chain.find_table_delta("account").first, false);

   // Create new account
   chain.create_account(N(newacc));

   // Verify that a new record for the new account in the state delta of the block
   auto result = chain.find_table_delta("account");
   BOOST_REQUIRE(result.first);
   auto &it_account = result.second;
   BOOST_REQUIRE_EQUAL(it_account->rows.obj.size(), 1);
   auto accounts = chain.deserialize_data<eosio::ship_protocol::account_v0, eosio::ship_protocol::account>(it_account);
   BOOST_REQUIRE_EQUAL(accounts[0].name.to_string(), "newacc");
}

BOOST_AUTO_TEST_CASE(test_deltas_account_metadata) {
   table_deltas_tester chain;
   chain.produce_block();

   chain.create_account(N(newacc));

   // Spot onto account metadata
   auto result = chain.find_table_delta("account_metadata");
   BOOST_REQUIRE(result.first);
   auto &it_account_metadata = result.second;
   BOOST_REQUIRE_EQUAL(it_account_metadata->rows.obj.size(), 1);
   auto accounts_metadata = chain.deserialize_data<eosio::ship_protocol::account_metadata_v0, eosio::ship_protocol::account_metadata>(it_account_metadata);
   BOOST_REQUIRE_EQUAL(accounts_metadata[0].name.to_string(), "newacc");
   BOOST_REQUIRE_EQUAL(accounts_metadata[0].privileged, false);
}


BOOST_AUTO_TEST_CASE(test_deltas_account_permission) {
   table_deltas_tester chain;
   chain.produce_block();

   chain.create_account(N(newacc));

   // Check that the permissions of this new account are in the delta
   vector<string> expected_permission_names{ "owner", "active" };
   auto result = chain.find_table_delta("permission");
   BOOST_REQUIRE(result.first);
   auto &it_permission = result.second;
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 2);
   auto accounts_permissions = chain.deserialize_data<eosio::ship_protocol::permission_v0, eosio::ship_protocol::permission>(it_permission);
   for(int i = 0; i < accounts_permissions.size(); i++)
   {
      BOOST_REQUIRE_EQUAL(it_permission->rows.obj[i].first, true);
      BOOST_REQUIRE_EQUAL(accounts_permissions[i].owner.to_string(), "newacc");
      BOOST_REQUIRE_EQUAL(accounts_permissions[i].name.to_string(), expected_permission_names[i]);
   }
}

BOOST_AUTO_TEST_CASE(test_deltas_account_permission_creation_and_deletion) {
   table_deltas_tester chain;
   chain.produce_block();

   chain.create_account(N(newacc));

   auto& authorization_manager = chain.control->get_authorization_manager();
   const permission_object* ptr = authorization_manager.find_permission( {N(newacc), N(active)} );
   BOOST_REQUIRE(ptr != nullptr);

   // Create new permission
   chain.set_authority(N(newacc), N(mypermission), ptr->auth,  N(active));

   const permission_object* ptr_sub = authorization_manager.find_permission( {N(newacc), N(mypermission)} );
   BOOST_REQUIRE(ptr_sub != nullptr);

   // Verify that the new permission is present in the state delta
   std::vector<std::string> expected_permission_names{ "owner", "active", "mypermission" };
   auto result = chain.find_table_delta("permission");
   BOOST_REQUIRE(result.first);
   auto &it_permission = result.second;
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 3);
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj[2].first, true);
   auto accounts_permissions = chain.deserialize_data<eosio::ship_protocol::permission_v0, eosio::ship_protocol::permission>(it_permission);
   BOOST_REQUIRE_EQUAL(accounts_permissions[2].owner.to_string(), "newacc");
   BOOST_REQUIRE_EQUAL(accounts_permissions[2].name.to_string(), "mypermission");
   BOOST_REQUIRE_EQUAL(accounts_permissions[2].parent.to_string(), "active");

   chain.produce_block();

   // Delete the permission
   chain.delete_authority(N(newacc), N(mypermission));

   result = chain.find_table_delta("permission");
   BOOST_REQUIRE(result.first);
   auto &it_permission_del = result.second;
   BOOST_REQUIRE_EQUAL(it_permission_del->rows.obj.size(), 1);
   BOOST_REQUIRE_EQUAL(it_permission_del->rows.obj[0].first, false);
   accounts_permissions = chain.deserialize_data<eosio::ship_protocol::permission_v0, eosio::ship_protocol::permission>(it_permission_del);
   BOOST_REQUIRE_EQUAL(accounts_permissions[0].owner.to_string(), "newacc");
   BOOST_REQUIRE_EQUAL(accounts_permissions[0].name.to_string(), "mypermission");
   BOOST_REQUIRE_EQUAL(accounts_permissions[0].parent.to_string(), "active");
}

BOOST_AUTO_TEST_CASE(test_deltas_account_permission_modification) {
   table_deltas_tester chain;
   chain.produce_block();

   chain.create_account(N(newacc));
   chain.produce_block();
   public_key_type keys[] = {
         public_key_type("PUB_WA_WdCPfafVNxVMiW5ybdNs83oWjenQXvSt1F49fg9mv7qrCiRwHj5b38U3ponCFWxQTkDsMC"s), // Test for correct serialization of WA key, see issue #9087
         public_key_type("PUB_K1_12wkBET2rRgE8pahuaczxKbmv7ciehqsne57F9gtzf1PVb7Rf7o"s),
         public_key_type("PUB_R1_6FPFZqw5ahYrR9jD96yDbbDNTdKtNqRbze6oTDLntrsANgQKZu"s)};
   const int K1_storage_type_which_value = 0;

   for(auto &key: keys) {
      // Modify the permission authority
      auto wa_authority = authority(1, {key_weight{key, 1}}, {});
      chain.set_authority(N(newacc), N(active), wa_authority, N(owner));

      auto result = chain.find_table_delta("permission");
      BOOST_REQUIRE(result.first);

      auto &it_permission = result.second;
      BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 1);

      auto accounts_permissions = chain.deserialize_data<eosio::ship_protocol::permission_v0, eosio::ship_protocol::permission>(it_permission);
      BOOST_REQUIRE_EQUAL(accounts_permissions[0].owner.to_string(), "newacc");
      BOOST_REQUIRE_EQUAL(accounts_permissions[0].name.to_string(), "active");
      BOOST_REQUIRE_EQUAL(accounts_permissions[0].auth.keys.size(), 1);

      if(key.which() != K1_storage_type_which_value)
         BOOST_REQUIRE_EQUAL(public_key_to_string(accounts_permissions[0].auth.keys[0].key), key.to_string());
      else
         BOOST_REQUIRE_EQUAL(public_key_to_string(accounts_permissions[0].auth.keys[0].key), "PUB_K1_12wkBET2rRgE8pahuaczxKbmv7ciehqsne57F9gtzf1PVb7Rf7o");

      chain.produce_block();
   }
}

BOOST_AUTO_TEST_CASE(test_deltas_permission_link) {
   table_deltas_tester chain;
   chain.produce_block();

   chain.create_account(N(newacc));

   // Spot onto permission_link
   const auto spending_priv_key = chain.get_private_key(N(newacc), "spending");
   const auto spending_pub_key = spending_priv_key.get_public_key();

   chain.set_authority(N(newacc), N(spending), spending_pub_key, N(active));
   chain.link_authority(N(newacc), N(eosio), N(spending), N(reqauth));
   chain.push_reqauth(N(newacc), { permission_level{N(newacc), N(spending)} }, { spending_priv_key });

   auto result = chain.find_table_delta("permission_link");
   BOOST_REQUIRE(result.first);
   auto &it_permission_link = result.second;
   BOOST_REQUIRE_EQUAL(it_permission_link->rows.obj.size(), 1);
   auto permission_links = chain.deserialize_data<eosio::ship_protocol::permission_link_v0, eosio::ship_protocol::permission_link>(it_permission_link);
   BOOST_REQUIRE_EQUAL(permission_links[0].account.to_string(), "newacc");
   BOOST_REQUIRE_EQUAL(permission_links[0].message_type.to_string(), "reqauth");
   BOOST_REQUIRE_EQUAL(permission_links[0].required_permission.to_string(), "spending");
}

BOOST_AUTO_TEST_CASE(test_deltas_global_property_history) {
   // Assuming max transaction delay is 45 days (default in config.hpp)
   table_deltas_tester chain;

   // Change max_transaction_delay to 60 sec
   auto params = chain.control->get_global_properties().configuration;
   params.max_transaction_delay = 60;
   chain.push_action( config::system_account_name, N(setparams), config::system_account_name,
                             mutable_variant_object()
                             ("params", params) );

   // Deserialize and spot onto some data
   auto result = chain.find_table_delta("global_property");
   BOOST_REQUIRE(result.first);
   auto &it_global_property = result.second;
   BOOST_REQUIRE_EQUAL(it_global_property->rows.obj.size(), 1);
   auto global_properties = chain.deserialize_data<eosio::ship_protocol::global_property_v1, eosio::ship_protocol::global_property>(it_global_property);
   auto configuration = std::get<eosio::ship_protocol::chain_config_v0>(global_properties[0].configuration);
   BOOST_REQUIRE_EQUAL(configuration.max_transaction_delay, 60);
}

BOOST_AUTO_TEST_CASE(test_deltas_protocol_feature_history) {
   table_deltas_tester chain(setup_policy::none);
   const auto &pfm = chain.control->get_protocol_feature_manager();

   chain.produce_block();

   auto d = pfm.get_builtin_digest(builtin_protocol_feature_t::preactivate_feature);
   BOOST_REQUIRE(d);

   // Activate PREACTIVATE_FEATURE.
   chain.schedule_protocol_features_wo_preactivation({*d});

   chain.produce_block();

   // Now the latest bios contract can be set.
   chain.set_before_producer_authority_bios_contract();

   // Spot onto some data of the protocol state table delta
   auto result = chain.find_table_delta("protocol_state");
   BOOST_REQUIRE(result.first);
   auto &it_protocol_state = result.second;
   BOOST_REQUIRE_EQUAL(it_protocol_state->rows.obj.size(), 1);
   auto protocol_states = chain.deserialize_data<eosio::ship_protocol::protocol_state_v0, eosio::ship_protocol::protocol_state>(it_protocol_state);
   auto protocol_feature = std::get<eosio::ship_protocol::activated_protocol_feature_v0>(protocol_states[0].activated_protocol_features[0]);

   auto digest_byte_array = protocol_feature.feature_digest.extract_as_byte_array();
   char digest_array[digest_byte_array.size()];
   for(int i=0; i < digest_byte_array.size(); i++) digest_array[i] = digest_byte_array[i];
   eosio::chain::digest_type digest_in_delta(digest_array, digest_byte_array.size());

   BOOST_REQUIRE_EQUAL(digest_in_delta, *d);
}

BOOST_AUTO_TEST_CASE(test_deltas_contract) {
   table_deltas_tester chain;
   chain.produce_block();

   chain.create_account(N(tester));

   chain.set_code(N(tester), contracts::get_table_test_wasm());
   chain.set_abi(N(tester), contracts::get_table_test_abi().data());

   chain.produce_block();

   auto trace = chain.push_action(N(tester), N(addhashobj), N(tester), mutable_variant_object()("hashinput", "hello" ));

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   trace = chain.push_action(N(tester), N(addnumobj), N(tester), mutable_variant_object()("input", 2));

   BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace->receipt->status);

   // Spot onto contract_table
   auto result = chain.find_table_delta("contract_table");
   BOOST_REQUIRE(result.first);
   auto &it_contract_table = result.second;
   BOOST_REQUIRE_EQUAL(it_contract_table->rows.obj.size(), 6);
   auto contract_tables = chain.deserialize_data<eosio::ship_protocol::contract_table_v0, eosio::ship_protocol::contract_table>(it_contract_table);
   BOOST_REQUIRE_EQUAL(contract_tables[0].table.to_string(), "hashobjs");
   BOOST_REQUIRE_EQUAL(contract_tables[1].table.to_string(), "hashobjs....1");
   BOOST_REQUIRE_EQUAL(contract_tables[2].table.to_string(), "numobjs");
   BOOST_REQUIRE_EQUAL(contract_tables[3].table.to_string(), "numobjs.....1");
   BOOST_REQUIRE_EQUAL(contract_tables[4].table.to_string(), "numobjs.....2");
   BOOST_REQUIRE_EQUAL(contract_tables[5].table.to_string(), "numobjs.....3");

   // Spot onto contract_row
   result = chain.find_table_delta("contract_row");
   BOOST_REQUIRE(result.first);
   auto &it_contract_row = result.second;
   BOOST_REQUIRE_EQUAL(it_contract_row->rows.obj.size(), 2);
   auto contract_rows = chain.deserialize_data<eosio::ship_protocol::contract_row_v0, eosio::ship_protocol::contract_row>(it_contract_row);
   BOOST_REQUIRE_EQUAL(contract_rows[0].table.to_string(), "hashobjs");
   BOOST_REQUIRE_EQUAL(contract_rows[1].table.to_string(), "numobjs");

   // Spot onto contract_index256
   result = chain.find_table_delta("contract_index256");
   BOOST_REQUIRE(result.first);
   auto &it_contract_index256 = result.second;
   BOOST_REQUIRE_EQUAL(it_contract_index256->rows.obj.size(), 2);
   auto contract_indices = chain.deserialize_data<eosio::ship_protocol::contract_index256_v0, eosio::ship_protocol::contract_index256>(it_contract_index256);
   BOOST_REQUIRE_EQUAL(contract_indices[0].table.to_string(), "hashobjs");
   BOOST_REQUIRE_EQUAL(contract_indices[1].table.to_string(), "hashobjs....1");
}

BOOST_AUTO_TEST_CASE(test_deltas_resources_history) {
   table_deltas_tester chain;
   chain.produce_block();

   chain.create_accounts({ N(eosio.token), N(eosio.ram), N(eosio.ramfee), N(eosio.stake)});

   chain.produce_blocks( 100 );

   chain.set_code( N(eosio.token), contracts::eosio_token_wasm() );
   chain.set_abi( N(eosio.token), contracts::eosio_token_abi().data() );

   chain.produce_block();

   chain.push_action(N(eosio.token), N(create), N(eosio.token), mutable_variant_object()
      ("issuer", "eosio.token" )
      ("maximum_supply", core_from_string("1000000000.0000") )
   );

   chain.push_action(N(eosio.token), N(issue), N(eosio.token), fc::mutable_variant_object()
      ("to",       "eosio")
      ("quantity", core_from_string("90.0000"))
      ("memo", "for stuff")
   );

   chain.produce_blocks(10);

   chain.set_code( config::system_account_name, contracts::eosio_system_wasm() );
   chain.set_abi( config::system_account_name, contracts::eosio_system_abi().data() );

   chain.push_action(config::system_account_name, N(init), config::system_account_name,
                        mutable_variant_object()
                        ("version", 0)
                        ("core", CORE_SYM_STR));

   signed_transaction trx;
   chain.set_transaction_headers(trx);

   authority owner_auth;
   owner_auth =  authority( chain.get_public_key( N(alice), "owner" ) );

   trx.actions.emplace_back( vector<permission_level>{{config::system_account_name,config::active_name}},
                                newaccount{
                                    .creator  = config::system_account_name,
                                    .name     =  N(alice),
                                    .owner    = owner_auth,
                                    .active   = authority( chain.get_public_key( N(alice), "active" ) )});

   trx.actions.emplace_back( chain.get_action( config::system_account_name, N(buyram), vector<permission_level>{{config::system_account_name,config::active_name}},
                                                  mutable_variant_object()
                                                      ("payer", config::system_account_name)
                                                      ("receiver",  N(alice))
                                                      ("quant", core_from_string("1.0000"))));

   trx.actions.emplace_back( chain.get_action( config::system_account_name, N(delegatebw), vector<permission_level>{{config::system_account_name,config::active_name}},
                                                  mutable_variant_object()
                                                      ("from", config::system_account_name)
                                                      ("receiver",  N(alice))
                                                      ("stake_net_quantity", core_from_string("10.0000") )
                                                      ("stake_cpu_quantity", core_from_string("10.0000") )
                                                      ("transfer", 0 )));

   chain.set_transaction_headers(trx);
   trx.sign( chain.get_private_key( config::system_account_name, "active" ), chain.control->get_chain_id()  );
   chain.push_transaction( trx );

   /*
   auto result = chain.find_table_delta("resource_limits");
   BOOST_REQUIRE(result.first);
   auto &it_resource_limits = result.second;
   BOOST_REQUIRE_EQUAL(it_resource_limits->rows.obj.size(), 2);
   auto resource_limits = chain.deserialize_data<eosio::ship_protocol::resource_limits_v0, eosio::ship_protocol::resource_limits>(it_resource_limits);
   BOOST_REQUIRE(resource_limits[0].owner.to_string() == "alice");
   BOOST_REQUIRE(resource_limits[0].ram_bytes != -1);

   result = chain.find_table_delta("resource_usage");
   BOOST_REQUIRE(result.first);
   auto &it_resource_usage = result.second;
   BOOST_REQUIRE_EQUAL(it_resource_usage->rows.obj.size(), 4);
   auto resource_usages = chain.deserialize_data<eosio::ship_protocol::resource_usage_v0, eosio::ship_protocol::resource_usage>(it_resource_usage);
   BOOST_REQUIRE(resource_usages[0].owner.to_string() == "alice");
   BOOST_REQUIRE(resource_usages[0].ram_usage > 0);
   */
}

BOOST_AUTO_TEST_SUITE_END()