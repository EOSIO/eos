#include <eosio/state_history/create_deltas.hpp>
#include <eosio/state_history/serialization.hpp>

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/authorization_manager.hpp>

using namespace eosio;
using namespace testing;
using boost::signals2::scoped_connection;

BOOST_AUTO_TEST_SUITE(state_history_plugin_tests)

std::pair<std::string, std::string> get_owner_and_name_from_permission_stream(chain::bytes &bytes)
{
   fc::datastream<const char *> ds(bytes.data(), bytes.size());
   unsigned_int size;
   fc::raw::unpack(ds, size);
   uint64_t owner;
   fc::raw::unpack(ds, owner);
   uint64_t name;
   fc::raw::unpack(ds, name);
   return std::make_pair(chain::name(owner).to_string(), chain::name(name).to_string());
}

std::string get_name_from_account_datastream(chain::bytes &bytes)
{
   fc::datastream<const char *> ds(bytes.data(), bytes.size());
   unsigned_int size;
   fc::raw::unpack(ds, size);
   uint64_t account_name;
   fc::raw::unpack( ds, account_name );
   return chain::name(account_name).to_string();
}

BOOST_AUTO_TEST_CASE(test_deltas_account)
{
   tester chain;
   std::string name="account";
   auto find_by_name = [&name](const auto& x) {
      return x.name == name;
   };

   auto v = eosio::state_history::create_deltas(chain.control->db(), false);

   auto it_account = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_account!=v.end());
   BOOST_REQUIRE_EQUAL(it_account->rows.obj.size(), 1);
   BOOST_REQUIRE_EQUAL(get_name_from_account_datastream(it_account->rows.obj[0].second), "eosio");

   name="permission";
   auto it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission == v .end());

   chain.create_account(N(newacc));

   v = eosio::state_history::create_deltas(chain.control->db(), false);

   // Verify that a new record for that account in the state delta of the block
   name="account";
   it_account = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_account!=v.end());
   BOOST_REQUIRE_EQUAL(it_account->rows.obj.size(), 2);
   BOOST_REQUIRE_EQUAL(get_name_from_account_datastream(
                     it_account->rows.obj[0].second), "eosio");
   BOOST_REQUIRE_EQUAL(get_name_from_account_datastream(
                     it_account->rows.obj[1].second), "newacc");

   // Check that the permissions of this new account are in the delta
   name="permission";
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission!=v.end());
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 2);
   BOOST_REQUIRE(get_owner_and_name_from_permission_stream(
                     it_permission->rows.obj[0].second)==std::make_pair(std::string("newacc"), std::string("owner")));
   BOOST_REQUIRE(get_owner_and_name_from_permission_stream(
                     it_permission->rows.obj[1].second)==std::make_pair(std::string("newacc"), std::string("active")));

   auto& authorization = chain.control->get_mutable_authorization_manager();
   const permission_object* ptr = authorization.find_permission({N(newacc), N(active)});
   BOOST_REQUIRE(ptr!=nullptr);

   // Create new permission
   chain.set_authority(N(newacc), N(mypermission), ptr->auth,  N(active));

   const permission_object* ptr_sub = authorization.find_permission({N(newacc), N(mypermission)});
   BOOST_REQUIRE(ptr_sub!=nullptr);

   // Verify that the new permission is present in the state delta
   v = eosio::state_history::create_deltas(chain.control->db(), false);
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission!=v.end());
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 3);
   BOOST_REQUIRE(get_owner_and_name_from_permission_stream(
                     it_permission->rows.obj[0].second)==std::make_pair(std::string("newacc"), std::string("owner")));
   BOOST_REQUIRE(get_owner_and_name_from_permission_stream(
                     it_permission->rows.obj[1].second)==std::make_pair(std::string("newacc"), std::string("active")));
   BOOST_REQUIRE(
       get_owner_and_name_from_permission_stream(
           it_permission->rows.obj[2].second)==std::make_pair(std::string("newacc"), std::string("mypermission")));

   chain.produce_blocks();

   // Modify the permission authority
   auto empty_authority = authority(1, {}, {});
   chain.set_authority(N(newacc), N(mypermission), ptr->auth,  N(active));
   v = eosio::state_history::create_deltas(chain.control->db(), false);
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission!=v.end());
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 1);
   BOOST_REQUIRE(
       get_owner_and_name_from_permission_stream(
           it_permission->rows.obj[0].second)==std::make_pair(std::string("newacc"), std::string("mypermission")));

   // Delete the permission
   chain.delete_authority(N(newacc),N(mypermission));
   v = eosio::state_history::create_deltas(chain.control->db(), false);
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE_EQUAL(it_permission->rows.obj.size(), 1);
   BOOST_REQUIRE(
       get_owner_and_name_from_permission_stream(
           it_permission->rows.obj[0].second)==std::make_pair(std::string("newacc"), std::string("mypermission")));
}

BOOST_AUTO_TEST_CASE(test_traces_present) {
   tester chain;

   chain.control->applied_transaction.connect(
      [&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> t) {
         const transaction_trace_ptr& trx_trace_ptr = std::get<0>(t);
         BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trx_trace_ptr->receipt->status);
         BOOST_REQUIRE_EQUAL(trx_trace_ptr->action_traces.size(), 1);
         BOOST_REQUIRE_EQUAL(trx_trace_ptr->action_traces[0].act.name.to_string(), "newaccount");
   });

   chain.control->accepted_block.connect(
      [&](const block_state_ptr& bs) {
         std::cout<<"block accepted "<<std::endl;
   });

   chain.create_account(N(newacc));

   //chain.produce_blocks();
}

BOOST_AUTO_TEST_SUITE_END()