#include <eosio/state_history/create_deltas.hpp>
#include <eosio/state_history/serialization.hpp>

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <boost/test/unit_test.hpp>
#include <contracts.hpp>
#include <eosio/state_history/log.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/stream.hpp>
#include <eosio/ship_protocol.hpp>
#include <fc/io/json.hpp>

#include "test_cfd_transaction.hpp"
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>

using namespace eosio;
using namespace testing;
using namespace chain;

BOOST_AUTO_TEST_SUITE(test_state_history)

BOOST_AUTO_TEST_CASE(test_deltas_account)
{
   tester chain;
   chain.produce_blocks(1);

   std::string name="account";
   auto find_by_name = [&name](const auto& x) {
      return x.name == name;
   };

   auto v = eosio::state_history::create_deltas(chain.control->db(), false);

   auto it_account = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_account==v.end());

   name="permission";
   auto it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission==v.end());

   chain.create_account(eosio::chain::string_to_name("newacc"));

   v = eosio::state_history::create_deltas(chain.control->db(), false);

   // Verify that a new record for that account in the state delta of the block
   name="account";
   it_account = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_account!=v.end());
   BOOST_REQUIRE_EQUAL(it_account->rows.obj.size(), 1);
   eosio::input_stream s{ it_account->rows.obj[0].second.data(), it_account->rows.obj[0].second.size() };
   auto account = std::get<0>(eosio::from_bin<eosio::ship_protocol::account>(s));
   BOOST_REQUIRE_EQUAL(account.name.to_string(),"newacc");

   // Check that the permissions of this new account are in the delta
   name="permission";
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission!=v.end());
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 2);
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj[0].first, true);

   std::vector<std::string> expected_permission_names{"owner","active"};
   for(int i=0;i<it_permission->rows.obj.size();i++)
   {
      eosio::input_stream ps{ it_permission->rows.obj[i].second.data(), it_permission->rows.obj[i].second.size() };
      auto permission = std::get<0>(eosio::from_bin<eosio::ship_protocol::permission>(ps));
      BOOST_REQUIRE_EQUAL(it_permission->rows.obj[i].first, true);
      BOOST_REQUIRE(permission.owner.to_string()=="newacc" && permission.name.to_string()==expected_permission_names[i]);
   }

   auto& authorization_manager = chain.control->get_authorization_manager();
   const permission_object* ptr = authorization_manager.find_permission({eosio::chain::string_to_name("newacc"), eosio::chain::string_to_name("active")});
   BOOST_REQUIRE(ptr!=nullptr);

   // Create new permission
   chain.set_authority(eosio::chain::string_to_name("newacc"), eosio::chain::string_to_name("mypermission"), ptr->auth,  eosio::chain::string_to_name("active"));

   const permission_object* ptr_sub = authorization_manager.find_permission({eosio::chain::string_to_name("newacc"), eosio::chain::string_to_name("mypermission")});
   BOOST_REQUIRE(ptr_sub!=nullptr);

   // Verify that the new permission is present in the state delta
   v = eosio::state_history::create_deltas(chain.control->db(), false);
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission!=v.end());
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 3);

   expected_permission_names.push_back("mypermission");
   for(int i=0;i<it_permission->rows.obj.size();i++)
   {
     eosio::input_stream ps{ it_permission->rows.obj[i].second.data(), it_permission->rows.obj[i].second.size() };
     auto permission = std::get<0>(eosio::from_bin<eosio::ship_protocol::permission>(ps));
     BOOST_REQUIRE_EQUAL(it_permission->rows.obj[i].first, true);
     BOOST_REQUIRE(permission.owner.to_string()=="newacc" && permission.name.to_string()==expected_permission_names[i]);
   }

   chain.produce_blocks(1);

   // Modify the permission authority
   auto empty_authority = authority(1, {}, {});
   chain.set_authority(eosio::chain::string_to_name("newacc"), eosio::chain::string_to_name("mypermission"), ptr->auth,  eosio::chain::string_to_name("active"));
   v = eosio::state_history::create_deltas(chain.control->db(), false);
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission!=v.end());
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 1);
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj[0].first, true);
   {
      eosio::input_stream ps{ it_permission->rows.obj[0].second.data(), it_permission->rows.obj[0].second.size() };
      auto permission = std::get<0>(eosio::from_bin<eosio::ship_protocol::permission>(ps));
      BOOST_REQUIRE(permission.owner.to_string()=="newacc" && permission.name.to_string()=="mypermission");
   }

   // Delete the permission
   chain.delete_authority(eosio::chain::string_to_name("newacc"),eosio::chain::string_to_name("mypermission"));
   v = eosio::state_history::create_deltas(chain.control->db(), false);
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 1);
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj[0].first, false);
   {
      eosio::input_stream ps{ it_permission->rows.obj[0].second.data(), it_permission->rows.obj[0].second.size() };
      auto permission = std::get<0>(eosio::from_bin<eosio::ship_protocol::permission>(ps));
      BOOST_REQUIRE(permission.owner.to_string()=="newacc" && permission.name.to_string()=="mypermission");
   }
}

BOOST_AUTO_TEST_CASE(test_traces_present)
{
   tester chain;

   chain.control->applied_transaction.connect(
      [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
         const transaction_trace_ptr& trx_trace_ptr = std::get<0>(t);
         BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trx_trace_ptr->receipt->status);
         BOOST_REQUIRE_EQUAL(trx_trace_ptr->action_traces.size(), 1);
         BOOST_REQUIRE_EQUAL(trx_trace_ptr->action_traces[0].act.name.to_string(), "newaccount");
   });

   chain.control->accepted_block.connect(
      [&](const block_state_ptr& bs) {
         std::cout<<"block accepted "<<std::endl;
   });

   chain.create_account(eosio::chain::string_to_name("newacc"));

   //chain.produce_blocks();
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