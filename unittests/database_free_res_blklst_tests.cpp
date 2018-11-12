/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/testing/tester.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <fc/crypto/digest.hpp>
#include <boost/container/flat_set.hpp>
#include <eosio/chain/types.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif


using namespace eosio::chain;
using namespace eosio::testing;
namespace bfs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(database_free_res_blklst_tests)

// shared_vector<name> parse_list_string(string items)
// {
//   shared_vector<name> item_list;
// vector<string> itemlist;
//       boost::split(itemlist, items, boost::is_any_of(","));
//       for (string item: itemlist) {
//               item_list.push_back(string_to_name(item.c_str()));
//       }

//       return item_list;
// }


//     // Simple tests of undo infrastructure
//    BOOST_AUTO_TEST_CASE(list_config_parse_test) {
//       try {
//          TESTER test;
   
//       string str= "alice,bob,tom";
//       shared_vector<name> list = parse_list_string(str);
//          BOOST_TEST(list.size() > 0);
//          account_name n =N(a);
//          if(list.size()>0)
//          {
//             n = *(list.begin());
//          }

//          BOOST_TEST(n !=N(a));
//           BOOST_TEST(n ==N(alice));
        
//       } FC_LOG_AND_RETHROW()
//    }

//  // Simple tests of undo infrastructure
//    BOOST_AUTO_TEST_CASE(actor_blacklist_config_test) {
//       try {
//          TESTER test;
//        // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.
//          eosio::chain::database& db = const_cast<eosio::chain::database&>( test.control->db() );

//          auto ses = db.start_undo_session(true);

//       string str= "alice,bob,tom";
//       shared_vector<name> list = parse_list_string(str);

//          // Create an account
//          db.create<global_property2_object>([&](global_property2_object &a) {
//                 a.cfg.actor_blacklist = list;
   
//          });

        
//          // Make sure we can retrieve that account by name
//          auto  ptr = db.get<global_property2_object>();

//          BOOST_TEST(ptr != nullptr);
     
//          chain_config2 a = ptr.cfg;
       
//          uint64_t v = 0;
//          if(a.actor_blacklist.size() > 0)
//          {
//                v =  *(a.list.begin());
//          }
//          BOOST_TEST(v > 0);
     
//          // Undo creation of the account
//          ses.undo();

//         //  // Make sure we can no longer find the account
//         //  ptr = db.find<global_property_list_object, by_name, std::string>("billy");
//         //  BOOST_TEST(ptr == nullptr);
//       } FC_LOG_AND_RETHROW()
//    }
   




//     // Simple tests of undo infrastructure
//    BOOST_AUTO_TEST_CASE(contract_blacklist_config_test) {
//       try {
//          TESTER test;
//        // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.
//          eosio::chain::database& db = const_cast<eosio::chain::database&>( test.control->db() );

//          auto ses = db.start_undo_session(true);

//       string str= "alice,bob,tom";
//       shared_vector<name> list = parse_list_string(str);

//          // Create an account
//          db.create<global_property2_object>([&](global_property2_object &a) {
           
//    a.cfg.contract_blacklist= list;

//          });

        
//          // Make sure we can retrieve that account by name
//          auto  ptr = db.get<global_property2_object>();

//          BOOST_TEST(ptr != nullptr);
     
//          chain_config2 a = ptr.cfg;
       
//          uint64_t v = 0;
//          if(a.contract_blacklist.size() > 0)
//          {
//                v =  *(a.list.begin());
//          }
//          BOOST_TEST(v > 0);
     
//          // Undo creation of the account
//          ses.undo();

//         //  // Make sure we can no longer find the account
//         //  ptr = db.find<global_property_list_object, by_name, std::string>("billy");
//         //  BOOST_TEST(ptr == nullptr);
//       } FC_LOG_AND_RETHROW()
//    }

//     // Simple tests of undo infrastructure
//    BOOST_AUTO_TEST_CASE(resource_greylist_config_test) {
//       try {
//          TESTER test;
//        // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.
//          eosio::chain::database& db = const_cast<eosio::chain::database&>( test.control->db() );

//          auto ses = db.start_undo_session(true);

//       string str= "alice,bob,tom";
//       shared_vector<name> list = parse_list_string(str);

//          // Create an account
//          db.create<global_property2_object>([&](global_property2_object &a) {
             
//    a.cfg.resource_greylist= list;
//          });

//          // Make sure we can retrieve that account by name
//          auto  ptr = db.get<global_property2_object>();

//          BOOST_TEST(ptr != nullptr);
     
//          chain_config2 a = ptr.cfg;
       
//          uint64_t v = 0;
//          if(a.resource_greylist.size() > 0)
//          {
//                v =  *(a.list.begin());
//          }
//          BOOST_TEST(v > 0);
     
//          // Undo creation of the account
//          ses.undo();

//         //  // Make sure we can no longer find the account
//         //  ptr = db.find<global_property_list_object, by_name, std::string>("billy");
//         //  BOOST_TEST(ptr == nullptr);
//       } FC_LOG_AND_RETHROW()
//    }
   

//   // Simple tests of undo infrastructure
//    BOOST_AUTO_TEST_CASE(free_resource_limit_config_test) {
//       try {
//          TESTER test;
//        // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.
//          eosio::chain::database& db = const_cast<eosio::chain::database&>( test.control->db() );

//          auto ses = db.start_undo_session(true);

//          // Create an account
//          db.create<global_property2_object>([&](global_property2_object &a) {
//       a.rmg.cpu_us= 100;
//    a.rmg.net_byte=1024;
//    a.rmg.ram_byte=1;
//          });

//          // Make sure we can retrieve that account by name
//          auto  ptr = db.get<global_property2_object>();

//          BOOST_TEST(ptr != nullptr);
     
//          resouces_minimum_guarantee a = ptr.rmg;
       
        
//          BOOST_TEST( a.rmg.cpu_us== 100);
//          BOOST_TEST( a.rmg.net_byte==1024);
//          BOOST_TEST( a.rmg.ram_byte==1);
     
//          // Undo creation of the account
//          ses.undo();

//         //  // Make sure we can no longer find the account
//         //  ptr = db.find<global_property_list_object, by_name, std::string>("billy");
//         //  BOOST_TEST(ptr == nullptr);
//       } FC_LOG_AND_RETHROW()
//    }
   


BOOST_AUTO_TEST_SUITE_END()
