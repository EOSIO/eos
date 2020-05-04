#include <eosio/state_history/create_deltas.hpp>

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/authorization_manager.hpp>

using namespace eosio;
using namespace testing;
using boost::signals2::scoped_connection;

BOOST_AUTO_TEST_SUITE(state_history_plugin_tests)

BOOST_AUTO_TEST_CASE(test_deltas_account) {
   tester main;
   std::string name="account";
   auto find_by_name = [&name](const auto& x) {
      return x.name == name;
   };

   auto v = eosio::state_history::create_deltas(main.control->db(), false);

   auto it_account = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_account!=v.end());
   BOOST_REQUIRE(it_account->rows.obj.size()==1);

   name="permission";
   auto it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission==v.end());

   main.create_account(N(newacc));

   v = eosio::state_history::create_deltas(main.control->db(), false);

   name="account";
   it_account = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_account!=v.end());
   BOOST_REQUIRE(it_account->rows.obj.size()==2);

   name="permission";
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission!=v.end());
   BOOST_REQUIRE(it_permission->rows.obj.size()==2);

   auto& authorization = main.control->get_mutable_authorization_manager();
   const permission_object* ptr = authorization.find_permission({N(newacc), N(active)});
   BOOST_REQUIRE(ptr!=nullptr);

   authorization.create_permission(N(newacc), N(mypermission), ptr->id, ptr->auth, fc::time_point::now() );

   permission_level pl2{N(newacc), N(mypermission)};
   const permission_object* ptr_sub = authorization.find_permission(pl2);

   BOOST_REQUIRE(ptr_sub!=nullptr);

   v = eosio::state_history::create_deltas(main.control->db(), false);
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission!=v.end());
   BOOST_REQUIRE(it_permission->rows.obj.size()==3);

   auto empty_authority = authority(1, {}, {});
   authorization.modify_permission(*ptr_sub, empty_authority);

   v = eosio::state_history::create_deltas(main.control->db(), false);
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission!=v.end());
   BOOST_REQUIRE(it_permission->rows.obj.size()==3);

   authorization.remove_permission(*ptr_sub);
   v = eosio::state_history::create_deltas(main.control->db(), false);
   it_permission = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it_permission->rows.obj.size()==2);
}

BOOST_AUTO_TEST_SUITE_END()