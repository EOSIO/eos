#include <eosio/state_history/create_deltas.hpp>

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>

using namespace eosio;
using namespace testing;
using boost::signals2::scoped_connection;

BOOST_AUTO_TEST_SUITE(state_history_plugin_tests)

BOOST_AUTO_TEST_CASE(test_deltas) {
   tester main;

   auto v = eosio::state_history::create_deltas(main.control->db(), false);

   std::string name="permission";
   auto find_by_name = [&name](const auto& x) {
      return x.name == name;
   };

   auto it = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it==v.end());

   name="resource_limits";
   it = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it==v.end());

   main.create_account(N(newacc));

   v = eosio::state_history::create_deltas(main.control->db(), false);

   name="permission";
   it = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it!=v.end());

   name="resource_limits";
   it = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it!=v.end());

   main.produce_block();

   v = eosio::state_history::create_deltas(main.control->db(), false);

   name="permission";
   it = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it==v.end());

   name="resource_limits";
   it = std::find_if(v.begin(), v.end(), find_by_name);
   BOOST_REQUIRE(it==v.end());
}

BOOST_AUTO_TEST_SUITE_END()