#include <eosio/testing/tester.hpp>
#include <boost/test/unit_test.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/testing/chainbase_fixture.hpp>

#include <algorithm>
#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio::chain::resource_limits;
using namespace eosio::testing;
using namespace eosio::chain;

class free_resource_limits_fixture : private chainbase_fixture<512 * 1024>, public resource_limits_manager
{
 public:
   free_resource_limits_fixture()
       : chainbase_fixture(), resource_limits_manager(*chainbase_fixture::_db)
   {
      add_indices();
      initialize_database();
   }

   ~free_resource_limits_fixture() {}

   chainbase::database::session start_session()
   {
      return chainbase_fixture::_db->start_undo_session(true);
   }
};

BOOST_AUTO_TEST_SUITE(free_resource_limits_test)

BOOST_FIXTURE_TEST_CASE(check_block_limits_cpu, free_resource_limits_fixture)
try
{
   const account_name account(1);
   const uint64_t increment = 1000;
   initialize_account(account);
   set_account_limits(account, 1000, 0, 0);
   process_account_limit_updates();

   uint16_t free_resource_limit_per_day = 100;
  
   // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.

   // test.control->startup();

   //  // Make sure we can no longer find

   const uint64_t expected_iterations = config::default_max_block_cpu_usage / increment;

   for (int idx = 0; idx < expected_iterations; idx++)
   {
      add_transaction_usage({account}, increment, 0, 0);
   }


   auto arl = get_account_cpu_limit_ex(account, true);

   BOOST_REQUIRE_EQUAL(arl.available, 0);
   BOOST_REQUIRE_THROW(add_transaction_usage({account}, increment, 0, 0), block_resource_exhausted);
}
FC_LOG_AND_RETHROW();

BOOST_FIXTURE_TEST_CASE(check_block_limits_cpu_lowerthan, free_resource_limits_fixture)
try
{
   const account_name account(1);
   const uint64_t increment = 1000;
   initialize_account(account);
   set_account_limits(account, increment, increment, increment);
   process_account_limit_updates();


   const uint64_t expected_iterations = config::default_max_block_cpu_usage / increment;

   for (int idx = 0; idx < expected_iterations; idx++)
   {
      add_transaction_usage({account}, increment, 0, 0);
   }

 
   auto arl = get_account_cpu_limit_ex(account, true);
   BOOST_TEST(arl.available > 0);

   BOOST_REQUIRE_THROW(add_transaction_usage({account}, increment, 0, 0), block_resource_exhausted);
   
}
FC_LOG_AND_RETHROW();

BOOST_FIXTURE_TEST_CASE(check_block_limits_net_lowerthan, free_resource_limits_fixture)
try
{
   const account_name account(1);
   const uint64_t increment = 1000;
   initialize_account(account);
   set_account_limits(account, increment, increment, increment);
   process_account_limit_updates();
   

   //   const uint64_t expected_iterations = config::default_max_block_cpu_usage / increment;

   //   for (int idx = 0; idx < expected_iterations; idx++) {
   add_transaction_usage({account}, increment, 0, 0);
   //   }

  
   auto arl = get_account_net_limit_ex(account, true);
   BOOST_TEST(arl.available > 0);

   BOOST_REQUIRE_THROW(add_transaction_usage({account}, increment, 0, 0), block_resource_exhausted);
  
}
FC_LOG_AND_RETHROW();

BOOST_AUTO_TEST_SUITE_END()
