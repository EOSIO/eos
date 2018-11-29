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

BOOST_AUTO_TEST_SUITE(database_gmr_blklst_tests)

vector<name> parse_list_string(string items)
{
   vector<name> item_list;
   vector<string> itemlist;
   boost::split(itemlist, items, boost::is_any_of(","));
   for (string item : itemlist)
   {
      item_list.push_back(string_to_name(item.c_str()));
   }

   return item_list;
}

// Simple tests of undo infrastructure
BOOST_AUTO_TEST_CASE(list_config_parse_test)
{
   try
   {
      TESTER test;

      string str = "alice,bob,tom";
      vector<name> list = parse_list_string(str);
      BOOST_TEST(list.size() > 0);
      account_name n = N(a);
      if (list.size() > 0)
      {
         n = *(list.begin());
      }

      BOOST_TEST(n != N(a));
      BOOST_TEST(n == N(alice));
   }
   FC_LOG_AND_RETHROW()
}

// Simple tests of undo infrastructure
BOOST_AUTO_TEST_CASE(set_name_list_test)
{
   try
   {
      TESTER test;
      // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.
      chainbase::database &db = const_cast<chainbase::database &>(test.control->db());

      auto ses = db.start_undo_session(true);

      string str = "alice,bob,tom";
      vector<name> list = parse_list_string(str);

      flat_set<account_name> nameset(list.begin(), list.end());

      test.control->set_actor_blacklist(nameset);

      // Make sure we can retrieve that account by name
      const global_property2_object &ptr = db.get<global_property2_object>();

      // Create an account
      db.modify(ptr, [&](global_property2_object &a) {
         a.cfg.actor_blacklist = {N(a)};
         a.cfg.contract_blacklist = {N(a)};
         a.cfg.resource_greylist = {N(a)};
      });

      int64_t lt = static_cast<int64_t>(list_type::actor_blacklist_type);
      int64_t lat = static_cast<int64_t>(list_action_type::insert_type);
      test.control->set_name_list(lt, lat, list);

     


       const flat_set<account_name>&  ab =  test.control->get_actor_blacklist();
       const flat_set<account_name>&  cb =  test.control->get_contract_blacklist();
       const flat_set<account_name>&  rg =  test.control->get_resource_greylist();

     


      auto convert_names = [&](const shared_vector<name>& namevec, flat_set<account_name>& nameset) -> void {
        for(const auto& a :namevec)
        {
           nameset.insert(uint64_t(a));
        }
      };

      flat_set<account_name> aab;
      flat_set<account_name> acb;
      flat_set<account_name> arg;

      const global_property2_object &ptr1 = db.get<global_property2_object>();
      chain_config2 c = ptr1.cfg;

      BOOST_TEST(c.actor_blacklist.size() == 4);
      BOOST_TEST(ab.size() == 4);

      convert_names(c.actor_blacklist, aab);
      convert_names(c.contract_blacklist, acb);
      convert_names(c.resource_greylist, arg);

     
      if (c.actor_blacklist.size() == 4)
      {

         bool b = (aab.find(N(a)) != aab.end());
         BOOST_TEST(b);
      }

      bool d = ab.find(N(a)) != ab.end();
      BOOST_TEST(d);
      bool m = aab.find(N(alice)) != aab.end();
      BOOST_TEST(m);

      // Undo creation of the account
      ses.undo();

    
   }
   FC_LOG_AND_RETHROW()
}

// Simple tests of undo infrastructure
BOOST_AUTO_TEST_CASE(actor_blacklist_config_test)
{
   try
   {
      TESTER test;
      // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.
      chainbase::database &db = const_cast<chainbase::database &>(test.control->db());

      auto ses = db.start_undo_session(true);

      //   string str= "alice,bob,tom";
      //   vector<name> list = parse_list_string(str);

      // Make sure we can retrieve that account by name
      const global_property2_object &ptr = db.get<global_property2_object>();

      // Create an account
      db.modify(ptr, [&](global_property2_object &a) {
         a.cfg.actor_blacklist = {N(a)};
      });

      chain_config2 a = ptr.cfg;

      account_name v;
      if (a.actor_blacklist.size() > 0)
      {
         v = *(a.actor_blacklist.begin());
      }

      std::size_t s = a.actor_blacklist.size();

      BOOST_TEST(1 == s);

      BOOST_TEST(v == N(a));

      // Undo creation of the account
      ses.undo();


   }
   FC_LOG_AND_RETHROW()
}

// Simple tests of undo infrastructure
BOOST_AUTO_TEST_CASE(contract_blacklist_config_test)
{
   try
   {
      TESTER test;
      // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.
      chainbase::database &db = const_cast<chainbase::database &>(test.control->db());

      auto ses = db.start_undo_session(true);

      //   string str= "alice,bob,tom";
      //   vector<name> list = parse_list_string(str);

      // Make sure we can retrieve that account by name
      const global_property2_object &ptr = db.get<global_property2_object>();

      // Create an account
      db.modify(ptr, [&](global_property2_object &a) {
         a.cfg.contract_blacklist = {N(a)};
      });

      chain_config2 a = ptr.cfg;

       account_name v ;
      if (a.contract_blacklist.size() > 0)
      {
         v = *(a.contract_blacklist.begin());
      }

      std::size_t s = a.contract_blacklist.size();

      BOOST_TEST(1 == s);

      BOOST_TEST(v == N(a));

      // Undo creation of the account
      ses.undo();


   }
   FC_LOG_AND_RETHROW()
}

// Simple tests of undo infrastructure
BOOST_AUTO_TEST_CASE(resource_greylist_config_test)
{
   try
   {
      TESTER test;
      // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.
      chainbase::database &db = const_cast<chainbase::database &>(test.control->db());

      auto ses = db.start_undo_session(true);

      //   string str= "alice,bob,tom";
      //   vector<name> list = parse_list_string(str);

      // Make sure we can retrieve that account by name
      const global_property2_object &ptr = db.get<global_property2_object>();

      // Create an account
      db.modify(ptr, [&](global_property2_object &a) {
         a.cfg.resource_greylist = {N(a)};
      });

      chain_config2 a = ptr.cfg;

      account_name v ;
      if (a.resource_greylist.size() > 0)
      {
         v = *(a.resource_greylist.begin());
      }
      
    std::size_t s = a.resource_greylist.size();

      BOOST_TEST(1 == s);

      BOOST_TEST(v == N(a));

      // Undo creation of the account
      ses.undo();

   }
   FC_LOG_AND_RETHROW()
}

// Simple tests of undo infrastructure
BOOST_AUTO_TEST_CASE(gmrource_limit_config_test)
{
   try
   {
      TESTER test;
      // Bypass read-only restriction on state DB access for this unit test which really needs to mutate the DB to properly conduct its test.
      chainbase::database &db = const_cast<chainbase::database &>(test.control->db());

      auto ses = db.start_undo_session(true);

      // Make sure we can retrieve that account by name
      const global_property2_object &ptr = db.get<global_property2_object>();

      // Create an account
      db.modify(ptr, [&](global_property2_object &a) {
         a.gmr.cpu_us = 100;
         a.gmr.net_byte = 1024;
         a.gmr.ram_byte = 1;
      });

      BOOST_TEST(ptr.gmr.cpu_us == 100);
      BOOST_TEST(ptr.gmr.net_byte == 1024);
      BOOST_TEST(ptr.gmr.ram_byte == 1);

      // Undo creation of the account
      ses.undo();


   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
