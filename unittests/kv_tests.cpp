#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include <contracts.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;
namespace bdata = boost::unit_test::data;

using mvo = fc::mutable_variant_object;

struct kv {
   eosio::chain::bytes k;
   eosio::chain::bytes v;
};
FC_REFLECT(kv, (k)(v))

struct itparam {
   name db;
   uint32_t count;
   bool erase;
};
FC_REFLECT(itparam, (db)(count)(erase))

class kv_tester : public tester {
 public:
   kv_tester(backing_store_type backing_store) : tester(setup_policy::full, db_read_mode::SPECULATIVE, std::optional<uint32_t>{}, std::optional<uint32_t>{}, backing_store) {
      produce_blocks(2);

      create_accounts({ "kvtest"_n, "kvtest1"_n, "kvtest2"_n, "kvtest3"_n, "kvtest4"_n });
      produce_blocks(2);

      for(auto account : {"kvtest"_n, "kvtest1"_n, "kvtest2"_n, "kvtest3"_n, "kvtest4"_n}) {
         set_code(account, contracts::kv_test_wasm());
         set_abi(account, contracts::kv_test_abi().data());
      }

      set_code(config::system_account_name, contracts::kv_bios_wasm());
      set_abi(config::system_account_name, contracts::kv_bios_abi().data());

      produce_blocks();

      {
         const auto& accnt = control->db().get<account_object, by_name>("kvtest"_n);
         abi_def     abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         abi_ser.set_abi(abi, abi_serializer::create_yield_function(abi_serializer_max_time));
      }
      {
         const auto& accnt = control->db().get<account_object, by_name>(config::system_account_name);
         abi_def     abi;
         BOOST_TEST_REQUIRE(abi_serializer::to_abi(accnt.abi, abi));
         sys_abi_ser.set_abi(abi, abi_serializer::create_yield_function(abi_serializer_max_time));
      }

      BOOST_TEST_REQUIRE(set_limit(-1) == "");
      BOOST_TEST_REQUIRE(set_limit(-1, "kvtest1"_n) == "");
      BOOST_TEST_REQUIRE(set_limit(-1, "kvtest2"_n) == "");
      BOOST_TEST_REQUIRE(set_limit(-1, "kvtest3"_n) == "");
      BOOST_TEST_REQUIRE(set_limit(-1, "kvtest4"_n) == "");
      BOOST_TEST_REQUIRE(set_kv_limits(1024, 1024*1024) == "");
      BOOST_TEST_REQUIRE(set_kv_limits(1024, 1024*1024) == "");
      produce_block();
   }

   action_result push_action(const action_name& name, const variant_object& data, chain::name account = "kvtest"_n, chain::name authorizer = "kvtest"_n) {
      string action_type_name = abi_ser.get_action_type(name);
      action act;
      act.account = account;
      act.name    = name;
      act.data    = abi_ser.variant_to_binary(action_type_name, data, abi_serializer::create_yield_function(abi_serializer_max_time));
      return base_tester::push_action(std::move(act), authorizer.to_uint64_t());
   }

   action_result set_limit(int64_t limit, name account = "kvtest"_n) {
      string action_type_name = sys_abi_ser.get_action_type("setramlimit"_n);
      action act;
      act.account = config::system_account_name;
      act.name    = "setramlimit"_n;
      act.data    = sys_abi_ser.variant_to_binary(action_type_name, mvo()("account", account)("limit", limit), abi_serializer::create_yield_function(abi_serializer_max_time));
      return base_tester::push_action(std::move(act), config::system_account_name.to_uint64_t());
   }
   action_result set_kv_limits(uint32_t klimit, uint32_t vlimit, uint32_t ilimit = 256) {
      string action_type_name = sys_abi_ser.get_action_type("ramkvlimits"_n);
      action act;
      act.account = config::system_account_name;
      act.name    = "ramkvlimits"_n;
      act.data    = sys_abi_ser.variant_to_binary(action_type_name, mvo()("k", klimit)("v", vlimit)("i", ilimit), abi_serializer::create_yield_function(abi_serializer_max_time));
      return base_tester::push_action(std::move(act), config::system_account_name.to_uint64_t());
   }

   void erase(const char* error, name contract, const char* k) {
      BOOST_REQUIRE_EQUAL(error, push_action("erase"_n, mvo()("contract", contract)("k", k)));
   }

   template <typename V>
   void get(const char* error, name contract, const char* k, V v) {
      BOOST_REQUIRE_EQUAL(error, push_action("get"_n, mvo()("contract", contract)("k", k)("v", v)));
   }

   template <typename V>
   action_result set(name contract, const char* k, V v, name payer, name authorizer) {
      return push_action("set"_n, mvo()("contract", contract)("k", k)("v", v)("payer", payer), contract, authorizer);
   }

   template <typename V>
   action_result set(name contract, const char* k, V v) {
      return push_action("set"_n, mvo()("contract", contract)("k", k)("v", v)("payer", contract));
   }

   action_result iterlimit(const std::vector<itparam>& params) {
      return push_action("itlimit"_n, mvo()("params", params));
   }

   action make_setmany_action(name contract, const std::vector<kv>& kvs) {
      action act;
      act.account = contract;
      act.name = "setmany"_n;
      act.data = abi_ser.variant_to_binary("setmany", mvo()("contract", contract)("kvs", kvs), abi_serializer::create_yield_function(abi_serializer_max_time));
      act.authorization = vector<permission_level>{{"kvtest"_n, config::active_name}};
      return act;
   }

   action make_erase_action(name contract, const char* k) {
      action act;
      act.account = contract;
      act.name = "erase"_n;
      act.data = abi_ser.variant_to_binary("erase", mvo()("contract", contract)("k", k), abi_serializer::create_yield_function(abi_serializer_max_time));
      act.authorization = vector<permission_level>{{"kvtest"_n, config::active_name}};
      return act;
   }

   action make_fail_action(name contract) {
      action act;
      act.account = contract;
      act.name = "fail"_n;
      act.authorization = vector<permission_level>{{"kvtest"_n, config::active_name}};
      return act;
   }

   void setmany(const char* error, name contract, const std::vector<kv>& kvs) {
      BOOST_REQUIRE_EQUAL(error, push_action("setmany"_n, mvo()("contract", contract)("kvs", kvs), contract));
   }

   template <typename T>
   void scan(const char* error, name contract, const char* prefix, T lower, const std::vector<kv>& expected) {
      BOOST_REQUIRE_EQUAL(error, push_action("scan"_n, mvo()("contract", contract)("prefix", prefix)(
                                                            "lower", lower)("expected", expected)));
      BOOST_REQUIRE_EQUAL(error, push_action("scan"_n, mvo()("contract", contract)("prefix", prefix)(
                                                            "lower", lower)("expected", expected), contract));
   }

   template <typename T>
   action_result scanrev(name contract, const char* prefix, T lower,
                const std::vector<kv>& expected) {
      auto result1 = push_action("scanrev"_n, mvo()("contract", contract)("prefix", prefix)(
                                           "lower", lower)("expected", expected));
      auto result2 = push_action("scanrev"_n, mvo()("contract", contract)("prefix", prefix)(
                                           "lower", lower)("expected", expected), contract);
      BOOST_TEST(result1 == result2);
      return result1;
   }

   void itstaterased(const char* error, name contract, const char* prefix, const char* k, const char* v,
                     int test_id, bool insert, bool reinsert) {
      BOOST_REQUIRE_EQUAL(error, push_action("itstaterased"_n, mvo()("contract", contract)("prefix", prefix)(
                                                                    "k", k)("v", v)("test_id", test_id)("insert", insert)("reinsert", reinsert)));
   }

   uint64_t get_usage(name account="kvtest"_n) {
      return control->get_resource_limits_manager().get_account_ram_usage(account);
   }

   void test_basic() {
      get("", "kvtest"_n, "", nullptr);
      BOOST_TEST("" == set("kvtest"_n, "", ""));
      get("", "kvtest"_n, "", "");
      BOOST_TEST("" == set("kvtest"_n, "", "1234"));
      get("", "kvtest"_n, "", "1234");
      get("", "kvtest"_n, "00", nullptr);
      BOOST_TEST("" == set("kvtest"_n, "00", "aabbccdd"));
      get("", "kvtest"_n, "00", "aabbccdd");
      get("", "kvtest"_n, "02", nullptr);
      erase("", "kvtest"_n, "02");
      get("", "kvtest"_n, "02", nullptr);
      BOOST_TEST("" == set("kvtest"_n, "02", "42"));
      get("", "kvtest"_n, "02", "42");
      get("", "kvtest"_n, "01020304", nullptr);
      BOOST_TEST("" == set("kvtest"_n, "01020304", "aabbccddee"));
      erase("", "kvtest"_n, "02");

      get("", "kvtest"_n, "01020304", "aabbccddee");
      get("", "kvtest"_n, "", "1234");
      get("", "kvtest"_n, "00", "aabbccdd");
      get("", "kvtest"_n, "02", nullptr);
      get("", "kvtest"_n, "01020304", "aabbccddee");
   }

   void test_other_contract() {
      get("", "missing"_n, "", nullptr);
      get("", "kvtest1"_n, "", nullptr);
      BOOST_TEST(set("missing"_n, "", "") == "Can not write to this key");
      BOOST_TEST(set("kvtest1"_n, "", "") == "Can not write to this key");
      erase("Can not write to this key", "missing"_n, "");
      erase("Can not write to this key", "kvtest1"_n, "");
   }

   void test_get_data() {
      BOOST_TEST(push_action("getdata"_n, mvo()) == "");
   }

   void test_scan(name account = "kvtest"_n) {
      setmany("", account,
              {
                    kv{ {}, { 0x12 } },
                    kv{ { 0x22 }, {} },
                    kv{ { 0x22, 0x11 }, { 0x34 } },
                    kv{ { 0x22, 0x33 }, { 0x18 } },
                    kv{ { 0x44 }, { 0x76 } },
                    kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                    kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                    kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
              });

      // no prefix, no lower bound
      scan("", account, "", nullptr,
           {
                 kv{ {}, { 0x12 } },
                 kv{ { 0x22 }, {} },
                 kv{ { 0x22, 0x11 }, { 0x34 } },
                 kv{ { 0x22, 0x33 }, { 0x18 } },
                 kv{ { 0x44 }, { 0x76 } },
                 kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                 kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                 kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
           });

      // no prefix, lower bound = ""
      scan("", account, "", "",
           {
                 kv{ {}, { 0x12 } },
                 kv{ { 0x22 }, {} },
                 kv{ { 0x22, 0x11 }, { 0x34 } },
                 kv{ { 0x22, 0x33 }, { 0x18 } },
                 kv{ { 0x44 }, { 0x76 } },
                 kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                 kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                 kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
           });

      // no prefix, lower bound = "221100"
      scan("", account, "", "221100",
           {
                 kv{ { 0x22, 0x33 }, { 0x18 } },
                 kv{ { 0x44 }, { 0x76 } },
                 kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                 kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                 kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
           });

      // no prefix, lower bound = "2233"
      scan("", account, "", "2233",
           {
                 kv{ { 0x22, 0x33 }, { 0x18 } },
                 kv{ { 0x44 }, { 0x76 } },
                 kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                 kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                 kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
           });

      // prefix = "22", no lower bound
      scan("", account, "22", nullptr,
           {
                 kv{ { 0x22 }, {} },
                 kv{ { 0x22, 0x11 }, { 0x34 } },
                 kv{ { 0x22, 0x33 }, { 0x18 } },
           });

      // prefix = "22", lower bound = "2211"
      scan("", account, "22", "2211",
           {
                 kv{ { 0x22, 0x11 }, { 0x34 } },
                 kv{ { 0x22, 0x33 }, { 0x18 } },
           });

      // prefix = "22", lower bound = "2234"
      scan("", account, "22", "2234", {});

      // prefix = "33", no lower bound
      scan("", account, "33", nullptr, {});

      // prefix = "44", lower bound = "223300"
      // kv_it_lower_bound finds "44", which is in the prefix range
      scan("", account, "44", "223300",
           {
                 kv{ { 0x44 }, { 0x76 } },
                 kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                 kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                 kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
           });

      // prefix = "44", lower bound = "2233"
      // lower_bound over the whole table would find "2233", which is out of the prefix range,
      // and should not affect the result.
      scan("", account, "44", "2233",
           {
                 kv{ { 0x44 }, { 0x76 } },
                 kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                 kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                 kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
           });
   } // test_scan

   void test_scanrev(name account) {
      setmany("", account,
              {
                    kv{ {}, { 0x12 } },
                    kv{ { 0x22 }, {} },
                    kv{ { 0x22, 0x11 }, { 0x34 } },
                    kv{ { 0x22, 0x33 }, { 0x18 } },
                    kv{ { 0x44 }, { 0x76 } },
                    kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                    kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                    kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
              });

      // no prefix, no lower bound
      BOOST_TEST("" == scanrev(account, "", nullptr,
              {
                    kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
                    kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                    kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                    kv{ { 0x44 }, { 0x76 } },
                    kv{ { 0x22, 0x33 }, { 0x18 } },
                    kv{ { 0x22, 0x11 }, { 0x34 } },
                    kv{ { 0x22 }, {} },
                    kv{ {}, { 0x12 } },
              }));

      // no prefix, lower bound = "4402"
      BOOST_TEST("" == scanrev(account, "", "4402",
              {
                    kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
                    kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                    kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                    kv{ { 0x44 }, { 0x76 } },
                    kv{ { 0x22, 0x33 }, { 0x18 } },
                    kv{ { 0x22, 0x11 }, { 0x34 } },
                    kv{ { 0x22 }, {} },
                    kv{ {}, { 0x12 } },
              }));

      // no prefix, lower bound = "44"
      BOOST_TEST("" == scanrev(account, "", "44",
              {
                    kv{ { 0x44 }, { 0x76 } },
                    kv{ { 0x22, 0x33 }, { 0x18 } },
                    kv{ { 0x22, 0x11 }, { 0x34 } },
                    kv{ { 0x22 }, {} },
                    kv{ {}, { 0x12 } },
              }));

      // prefix = "22", no lower bound
      BOOST_TEST("" == scanrev(account, "22", nullptr,
              {
                    kv{ { 0x22, 0x33 }, { 0x18 } },
                    kv{ { 0x22, 0x11 }, { 0x34 } },
                    kv{ { 0x22 }, {} },
              }));

      // prefix = "22", lower bound = "2233"
      BOOST_TEST("" == scanrev(account, "22", "2233",
              {
                    kv{ { 0x22, 0x33 }, { 0x18 } },
                    kv{ { 0x22, 0x11 }, { 0x34 } },
                    kv{ { 0x22 }, {} },
              }));

      // prefix = "22", lower bound = "2234"
      BOOST_TEST("" == scanrev(account, "22", "2234", {}));

      // prefix = "33", no lower bound
      BOOST_TEST("" == scanrev(account, "33", nullptr, {}));
   } // test_scanrev

   void test_scanrev2() {
      setmany("", "kvtest"_n,
              {
                    kv{ { 0x00, char(0xFF), char(0xFF) }, { 0x10 } },
                    kv{ { 0x01 }, { 0x20 } },
                    kv{ { 0x01, 0x00 }, { 0x30 } },
              });

      // prefix = "00FFFF", no lower bound
      BOOST_TEST("" == scanrev("kvtest"_n, "00FFFF", nullptr,
              {
                    kv{ { 0x00, char(0xFF), char(0xFF) }, { 0x10 } },
              }));
   } // test_scanrev2

   void test_iterase() {
      for(bool reinsert : {false, true}) {
         // pre-inserted
         for(bool insert : {false, true}) {
            for(int i = 0; i < 8; ++i) {
               setmany("", "kvtest"_n, { kv{ { 0x22 }, { 0x12 } } });
               produce_block();
               itstaterased("Iterator to erased element", "kvtest"_n, "", "22", "12", i, insert, reinsert );
            }
            setmany("", "kvtest"_n, { kv{ { 0x22 }, { 0x12 } } });
            produce_block();
            itstaterased("", "kvtest"_n, "", "22", "12", 8, insert, reinsert );
            if(!reinsert) {
               setmany("", "kvtest"_n, { kv{ { 0x22 }, { 0x12 } } });
               produce_block();
               itstaterased("", "kvtest"_n, "", "22", "12", 9, insert, reinsert );
            }
            setmany("", "kvtest"_n, { kv{ { 0x22 }, { 0x12 } }, kv{ { 0x23 }, { 0x13 } } });
            produce_block();
            itstaterased("", "kvtest"_n, "", "22", "12", 10, insert, reinsert );
            erase("", "kvtest"_n, "23");
            setmany("", "kvtest"_n, { kv{ { 0x22 }, { 0x12 } } });
            produce_block();
            itstaterased("", "kvtest"_n, "", "22", "12", 11, insert, reinsert );
         }
         // inserted inside contract
         for(int i = 0; i < 8; ++i) {
            erase("", "kvtest"_n, "22");
            produce_block();
            itstaterased("Iterator to erased element", "kvtest"_n, "", "22", "12", i, true, reinsert );
         }
         erase("", "kvtest"_n, "22");
         produce_block();
         itstaterased("", "kvtest"_n, "", "22", "12", 8, true, reinsert );
         if(!reinsert) {
            erase("", "kvtest"_n, "22");
            produce_block();
            itstaterased("", "kvtest"_n, "", "22", "12", 9, true, reinsert );
         }
         erase("", "kvtest"_n, "22");
         setmany("", "kvtest"_n, { kv{ { 0x23 }, { 0x13 } } });
         produce_block();
         itstaterased("", "kvtest"_n, "", "22", "12", 10, true, reinsert );
         erase("", "kvtest"_n, "23");
         erase("", "kvtest"_n, "22");
         produce_block();
         itstaterased("", "kvtest"_n, "", "22", "12", 11, true, reinsert );
      }
   }

   void test_ram_usage() {
      uint64_t base_usage = get_usage();
      get("", "kvtest"_n, "11", nullptr);
      BOOST_TEST(get_usage("kvtest"_n) == base_usage);

      BOOST_TEST("" == set("kvtest"_n, "11", "", "kvtest"_n, "kvtest"_n));

      const int base_billable = config::billable_size_v<kv_object>;
      BOOST_TEST(get_usage("kvtest"_n) == base_usage + base_billable + 1);
      BOOST_TEST("" == set("kvtest"_n, "11", "1234", "kvtest"_n, "kvtest"_n));
      BOOST_TEST(get_usage("kvtest"_n) == base_usage + base_billable + 1 + 2);
      BOOST_TEST("" == set("kvtest"_n, "11", "12", "kvtest"_n, "kvtest"_n));
      BOOST_TEST(get_usage("kvtest"_n) == base_usage + base_billable + 1 + 1);
      erase("", "kvtest"_n, "11");
      BOOST_TEST(get_usage("kvtest"_n) == base_usage);

      // test payer changes
      BOOST_TEST("" == set("kvtest"_n, "11", "", "kvtest"_n, "kvtest"_n));
      BOOST_TEST(get_usage("kvtest"_n) == base_usage + base_billable + 1);

      uint64_t base_usage1 = get_usage("kvtest1"_n);
      BOOST_TEST("" == set("kvtest"_n, "11", "", "kvtest1"_n, "kvtest1"_n));

      BOOST_TEST(get_usage("kvtest"_n) == base_usage);
      BOOST_TEST(get_usage("kvtest1"_n) == base_usage1 + base_billable + 1);

      // test unauthorized payer
      BOOST_TEST("unprivileged contract cannot increase RAM usage of another account that has not authorized the action: kvtest1" == 
                  set("kvtest"_n, "11", "12", "kvtest1"_n, "kvtest2"_n));

      BOOST_TEST("unprivileged contract cannot increase RAM usage of another account that has not authorized the action: kvtest2" == 
                  set("kvtest"_n, "11", "12", "kvtest2"_n, "kvtest1"_n));
   }

   void test_resource_limit() {
      uint64_t base_usage = get_usage();
      // insert a new element
      const int base_billable = config::billable_size_v<kv_object>;
      BOOST_TEST_REQUIRE(set_limit(base_usage + base_billable) == "");
      BOOST_TEST(set("kvtest"_n, "11", "").find("account kvtest has insufficient") == 0);
      BOOST_TEST_REQUIRE(set_limit(base_usage + base_billable + 1) == "");
      BOOST_TEST("" == set("kvtest"_n, "11", ""));
      // increase the size of a value
      BOOST_TEST_REQUIRE(set_limit(base_usage + base_billable + 1 + 2 - 1) == "");
      BOOST_TEST(set("kvtest"_n, "11", "1234").find("account kvtest has insufficient") == 0);
      BOOST_TEST_REQUIRE(set_limit(base_usage + base_billable + 1 + 2) == "");
      BOOST_TEST("" == set("kvtest"_n, "11", "1234"));
      // decrease the size of a value
      BOOST_TEST("" == set("kvtest"_n, "11", ""));
      // decrease limits
      BOOST_TEST(set_limit(base_usage + base_billable).find("account kvtest has insufficient") == 0);
      BOOST_TEST(set_limit(base_usage + base_billable + 1) == "");
      // erase an element
      erase("", "kvtest"_n, "11");
   }

   void test_key_value_limit() {
      BOOST_TEST_REQUIRE(set_kv_limits(4, 4) == "");
      setmany("", "kvtest"_n, {{bytes(4, 'a'), bytes(4, 'a')}});
      setmany("Key too large", "kvtest"_n, {{bytes(4 + 1, 'a'), bytes()}});
      setmany("Value too large", "kvtest"_n, {{bytes(), bytes(4 + 1, 'a')}});

      // The check happens at the point of calling set.  Changing the bad value later doesn't bypass errors.
      setmany("Value too large", "kvtest"_n, {{bytes(4, 'b'), bytes(5, 'b')}, {bytes(4, 'b'), {}}});

      // The key is checked even if it already exists
      setmany("Key too large", "kvtest"_n, {{bytes(1024, 'a'), {}}});

      scan("", "kvtest"_n, "00000000", "", {});
      scan("Prefix too large", "kvtest"_n, "0000000000", "", {});
   }

   action make_set_action(std::size_t size) {
      std::vector<char> k;
      std::vector<char> v(size, 'a');
      action act;
      act.account = "kvtest"_n;
      act.name    = "set"_n;
      act.data    = abi_ser.variant_to_binary("set", mvo()("contract", "kvtest"_n)("k", k)("v", v)("payer", "kvtest"_n), abi_serializer::create_yield_function(abi_serializer_max_time));
      act.authorization = vector<permission_level>{{"kvtest"_n, config::active_name}};
      return act;
   }

   action make_set_limit_action(int64_t limit) {
      action act;
      act.account = "eosio"_n;
      act.name    = "setramlimit"_n;
      act.data    = sys_abi_ser.variant_to_binary(act.name.to_string(), mvo()("account", "kvtest"_n)("limit", limit), abi_serializer::create_yield_function(abi_serializer_max_time));
      act.authorization = vector<permission_level>{{"kvtest"_n, config::active_name}};
      return act;
   }

   // Go over the limit and back in one transaction, with two separate actions
   void test_kv_inc_dec_usage() {
      BOOST_TEST_REQUIRE(set_limit(get_usage() + 256) == "");
      produce_block();
      signed_transaction trx;
      trx.actions.push_back(make_set_action(512));
      trx.actions.push_back(make_set_action(0));
      set_transaction_headers(trx);
      trx.sign(get_private_key("kvtest"_n, "active"), control->get_chain_id());
      push_transaction(trx);
      produce_block();
   }

   // Increase usage and then the limit in two actions in the same transaction.
   void test_kv_inc_usage_and_limit() {
      auto base_usage = get_usage();
      BOOST_TEST_REQUIRE(set_limit(base_usage + 256) == "");
      produce_block();
      signed_transaction trx;
      {
         trx.actions.push_back(make_set_action(512));
         trx.actions.push_back(make_set_limit_action(base_usage + 640));
      }
      set_transaction_headers(trx);
      trx.sign(get_private_key("kvtest"_n, "active"), control->get_chain_id());
      push_transaction(trx);
      produce_block();
   }

   // Decrease limit and then usage in two separate actions in the same transaction
   void test_kv_dec_limit_and_usage() {
      auto base_usage = get_usage();
      BOOST_TEST_REQUIRE(set_limit(base_usage + 1024) == "");
      BOOST_TEST_REQUIRE(set("kvtest"_n, "", std::vector<char>(512, 'a')) == "");
      produce_block();
      signed_transaction trx;
      {
         trx.actions.push_back(make_set_limit_action(base_usage + 256));
         trx.actions.push_back(make_set_action(0));
      }
      set_transaction_headers(trx);
      trx.sign(get_private_key("kvtest"_n, "active"), control->get_chain_id());
      push_transaction(trx);
      produce_block();
   }

   void test_max_iterators() {
      BOOST_TEST_REQUIRE(set_kv_limits(1024, 1024, 7) == "");
      // individual limits
      BOOST_TEST(iterlimit({{"eosio.kvram"_n, 7, false}}) == "");
      BOOST_TEST(iterlimit({{"eosio.kvram"_n, 2000, false}}) == "Too many iterators");
      // erase iterators and create more
      BOOST_TEST(iterlimit({{"eosio.kvram"_n, 6, false},
                            {"eosio.kvram"_n, 2, true},
                            {"eosio.kvram"_n, 3, false}}) == "");
      BOOST_TEST(iterlimit({{"eosio.kvram"_n, 6, false},
                            {"eosio.kvram"_n, 2, true},
                            {"eosio.kvram"_n, 4, false}}) == "Too many iterators");
      // fallback limit - testing this is impractical because it uses too much memory
      // This many iterators would consume at least 400 GiB.
      // BOOST_TEST_REQUIRE(set_kv_limits("eosio.kvram"_n, 1024, 1024, 0xFFFFFFFF) == "");
      // BOOST_TEST_REQUIRE(set_kv_limits("eosio.kvram"_n, 1024, 1024, 0xFFFFFFFF) == "");
      // BOOST_TEST(iterlimit({{"eosio.kvram"_n, 0xFFFFFFFF, false}, {"eosio.kvram"_n, 1, false}}) == "Too many iterators");
   }

   // Make sure that a failed transaction correctly rolls back changes to the database,
   // for both rocksdb and chainbase.
   void test_undo() {
      setmany("", "kvtest"_n,
              {
                    kv{ { 0x11 }, { 0x11 } },
                    kv{ { 0x22 }, { 0x11, 0x11 } }
              });

      signed_transaction trx;
      {
         trx.actions.push_back(make_setmany_action("kvtest"_n, { kv{ { 0x11 }, { 0x33 } }, kv{ { 0x44 }, { 0x55 } } } ));
         trx.actions.push_back(make_erase_action("kvtest"_n, "22"));
         trx.actions.push_back(make_fail_action("kvtest"_n));
      }
      set_transaction_headers(trx);
      trx.sign(get_private_key("kvtest"_n, "active"), control->get_chain_id());
      BOOST_CHECK_THROW(push_transaction(trx), eosio_assert_code_exception);

      scan("", "kvtest"_n, "", nullptr,
              {
                    kv{ { 0x11 }, { 0x11 } },
                    kv{ { 0x22 }, { 0x11, 0x11 } }
              });
   }

   void test_kv_basic_common() {
      BOOST_REQUIRE_EQUAL("", push_action("itlifetime"_n, mvo()));
      test_basic();
   }

   void test_kv_scan_common() { //
      // four possibilities depending on whether the next or previous contract table has elements
      test_scan("kvtest2"_n);
      test_scan("kvtest4"_n);
      test_scan("kvtest3"_n);
      test_scan("kvtest1"_n);
   }

   void test_kv_scanrev_common() {
      test_scanrev("kvtest2"_n);
      test_scanrev("kvtest4"_n);
      test_scanrev("kvtest3"_n);
      test_scanrev("kvtest1"_n);
   }
   
   void test_kv_scanrev2_common() {
      test_scanrev2();
   }
   
   void test_kv_iterase_common() {
      test_iterase();
   }
   
   void test_kv_ram_usage_common() {
      test_ram_usage();
   }
   
   void test_kv_resource_limit_common() { //
      test_resource_limit();
   }
   
   void test_kv_key_value_limit_common() { //
      test_key_value_limit();
   }

   abi_serializer abi_ser;
   abi_serializer sys_abi_ser;
};

class kv_chainbase_tester : public kv_tester {
 public:
   kv_chainbase_tester() : kv_tester(backing_store_type::CHAINBASE) { }
};

class kv_rocksdb_tester : public kv_tester {
 public:
   kv_rocksdb_tester() : kv_tester(backing_store_type::ROCKSDB) { }
};

BOOST_AUTO_TEST_SUITE(kv_tests)

BOOST_FIXTURE_TEST_CASE(kv_basic, kv_chainbase_tester) try {
   test_kv_basic_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scan, kv_chainbase_tester) try {
   test_kv_scan_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scanrev, kv_chainbase_tester) try {
   test_kv_scanrev_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scanrev2, kv_chainbase_tester) try { //
   test_kv_scanrev2_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_iterase, kv_chainbase_tester) try { //
   test_kv_iterase_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_ram_usage, kv_chainbase_tester) try { //
   test_kv_ram_usage_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_resource_limit, kv_chainbase_tester) try { //
   test_kv_resource_limit_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_key_value_limit, kv_chainbase_tester) try { //
   test_kv_key_value_limit_common();
}
FC_LOG_AND_RETHROW()

constexpr name databases[] = { "eosio.kvram"_n };

BOOST_DATA_TEST_CASE_F(kv_chainbase_tester, kv_inc_dec_usage, bdata::make(databases), db) try { //
   test_kv_inc_dec_usage();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_chainbase_tester, kv_inc_usage_and_limit, bdata::make(databases), db) try { //
   test_kv_inc_usage_and_limit();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_chainbase_tester, kv_dec_limit_and_usage, bdata::make(databases), db) try { //
   test_kv_dec_limit_and_usage();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_chainbase_tester, get_data, bdata::make(databases), db) try { //
   test_get_data();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_chainbase_tester, other_contract, bdata::make(databases), db) try { //
   test_other_contract();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(max_iterators, kv_chainbase_tester) try { //
   test_max_iterators();
}
FC_LOG_AND_RETHROW()

// RocksDB related test cases

BOOST_FIXTURE_TEST_CASE(kv_basic_rocksdb, kv_rocksdb_tester) try {
   test_kv_basic_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scan_rocksdb, kv_rocksdb_tester) try {
   test_kv_scan_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scanrev_rocksdb, kv_rocksdb_tester) try {
   test_kv_scanrev_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scanrev2_rocksdb, kv_rocksdb_tester) try { //
   test_kv_scanrev2_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_iterase_rocksdb, kv_rocksdb_tester) try { //
   test_kv_iterase_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_ram_usage_rocksdb, kv_rocksdb_tester) try { //
   test_kv_ram_usage_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_resource_limit_rocksdb, kv_rocksdb_tester) try { //
   test_kv_resource_limit_common();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_key_value_limit_rocksdb, kv_rocksdb_tester) try { //
   test_kv_key_value_limit_common();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_rocksdb_tester, kv_inc_dec_usage_rocksdb, bdata::make(databases), db) try { //
   test_kv_inc_dec_usage();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_rocksdb_tester, kv_inc_usage_and_limit_rocksdb, bdata::make(databases), db) try { //
   test_kv_inc_usage_and_limit();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_rocksdb_tester, kv_dec_limit_and_usage_rocksdb, bdata::make(databases), db) try { //
   test_kv_dec_limit_and_usage();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_rocksdb_tester, get_data_rocksdb, bdata::make(databases), db) try { //
   test_get_data();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_rocksdb_tester, other_contract_rocksdb, bdata::make(databases), db) try { //
   test_other_contract();
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(max_iterators_rocksdb, kv_rocksdb_tester) try { //
   test_max_iterators();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_chainbase_tester, undo, bdata::make(databases) * bdata::make({false, true}), db, rocks) try { //
   test_undo();
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_rocksdb_tester, rocksdb_undo, bdata::make(databases) * bdata::make({false, true}), db, rocks) try { //
   test_undo();
}
FC_LOG_AND_RETHROW()

// Initializes a kv_database configuration to usable values.
// Must be set on a privileged account
static const char kv_setup_wast[] =  R"=====(
(module
 (func $action_data_size (import "env" "action_data_size") (result i32))
 (func $read_action_data (import "env" "read_action_data") (param i32 i32) (result i32))
 (func $kv_set_parameters_packed (import "env" "set_kv_parameters_packed") (param i32 i32))
 (func $set_resource_limit (import "env" "set_resource_limit") (param i64 i64 i64))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
   (local $next_name_address i32)
   (local $bytes_remaining i32)
   (call $kv_set_parameters_packed (i32.const 0) (i32.const 16))
   (set_local $next_name_address (i32.const 16))
   (set_local $bytes_remaining (call $action_data_size))
   (if
      (i32.ge_s (get_local $bytes_remaining) (i32.const 8))
      (then
         (drop (call $read_action_data (get_local $next_name_address) (get_local $bytes_remaining)))
         (loop
            (call $set_resource_limit (i64.load (get_local $next_name_address)) (i64.const 13376816793197215744) (i64.const -1))            
            (set_local $bytes_remaining (i32.sub (get_local $bytes_remaining) (i32.const 8)))
            (set_local $next_name_address (i32.add (get_local $next_name_address) (i32.const 8)))
            (br_if 0 (i32.ge_s (get_local $bytes_remaining) (i32.const 8)))
         )
      )
   )
 )
 (data (i32.const 4) "\00\04\00\00")
 (data (i32.const 8) "\00\00\10\00")
 (data (i32.const 12) "\80\00\00\00")
)
)=====";

std::vector<char> construct_names_payload( std::vector<name> names ) {
   std::vector<char> result;
   result.resize(names.size() * sizeof(name));
   fc::datastream<char*> ds(result.data(), result.size());
   for( auto n : names ) {
      fc::raw::pack(ds, n);
   }
   return result;
}

// Call iterator intrinsics that create native state
// and then notify another contract.  The kv context should
// not affect recipient.
// State tested:
// - iterator id (including re-use of destroyed iterators)
// - temporary buffer for kv_get
// - access checking for write
static const char kv_notify_wast[] = R"=====(
(module
 (func $kv_it_create (import "env" "kv_it_create") (param i64 i32 i32) (result i32))
 (func $kv_it_destroy (import "env" "kv_it_destroy") (param i32))
 (func $kv_get (import "env" "kv_get") (param i64 i32 i32 i32) (result i32))
 (func $kv_set (import "env" "kv_set") (param i64 i32 i32 i32 i32 i64) (result i64))
 (func $require_recipient (import "env" "require_recipient") (param i64))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (drop (call $kv_it_create (get_local 0) (i32.const 0) (i32.const 0)))
  (drop (call $kv_it_create (get_local 0) (i32.const 0) (i32.const 0)))
  (call $kv_it_destroy (i32.const 2))
  (drop (call $kv_set (get_local 0) (i32.const 0) (i32.const 0) (i32.const 1) (i32.const 1)  (get_local 0)))
  (drop (call $kv_get (get_local 0) (i32.const 0) (i32.const 0) (i32.const 8)))
  (call $require_recipient (i64.const 11327368596746665984))
 )
)
)=====";

static const char kv_notified_wast[] = R"=====(
(module
 (func $kv_it_create (import "env" "kv_it_create")(param i64 i32 i32) (result i32))
 (func $kv_get_data (import "env" "kv_get_data") (param i32 i32 i32) (result i32))
 (func $kv_set (import "env" "kv_set") (param i64 i32 i32 i32 i32 i64) (result i64))
 (func $eosio_assert (import "env" "eosio_assert") (param i32 i32))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (call $eosio_assert (i32.eq (call $kv_it_create (get_local 0) (i32.const 0) (i32.const 0)) (i32.const 1)) (i32.const 80))
  (call $eosio_assert (i32.eq (call $kv_get_data (i32.const 0) (i32.const 0) (i32.const 0)) (i32.const 0)) (i32.const 160))
  (drop (call $kv_set (get_local 0) (i32.const 0) (i32.const 0) (i32.const 1) (i32.const 1) (get_local 0)))
 )
 (data (i32.const 80) "Wrong iterator value")
 (data (i32.const 160) "Temporary data buffer not empty")
)
)=====";

BOOST_DATA_TEST_CASE_F(tester, notify, bdata::make(databases), db) {
   create_accounts({ "setup"_n, "notified"_n, "notify"_n });
   set_code( "setup"_n, kv_setup_wast );
   push_action( "eosio"_n, "setpriv"_n, "eosio"_n, mutable_variant_object()("account", "setup"_n)("is_priv", 1));
   BOOST_TEST_REQUIRE(push_action( action({}, "setup"_n, db, construct_names_payload({"notified"_n, "notify"_n})), "setup"_n.to_uint64_t() ) == "");

   set_code( "notify"_n, kv_notify_wast );
   set_code( "notified"_n, kv_notified_wast );

   BOOST_TEST_REQUIRE(push_action( action({}, "notify"_n, db, {}), "notify"_n.to_uint64_t() ) == "");
}

// Check corner cases of alias checks for the kv_set and kv_get intrinsics
static const char kv_alias_pass_wast[] = R"=====(
(module
 (func $kv_get (import "env" "kv_get") (param i64 i32 i32 i32) (result i32))
 (func $kv_set (import "env" "kv_set") (param i64 i32 i32 i32 i32 i64) (result i64))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (drop (call $kv_set (get_local 0) (i32.const 0) (i32.const 0) (i32.const 0) (i32.const 1) (get_local 0)))
  (drop (call $kv_set (get_local 0) (i32.const 0) (i32.const 1) (i32.const 0) (i32.const 0) (get_local 0)))
  (drop (call $kv_set (get_local 0) (i32.const 0) (i32.const 1) (i32.const 1) (i32.const 0) (get_local 0)))
  (drop (call $kv_set (get_local 0) (i32.const 1) (i32.const 0) (i32.const 0) (i32.const 1) (get_local 0)))
  (drop (call $kv_set (get_local 0) (i32.const 0) (i32.const 2) (i32.const 1) (i32.const 0) (get_local 0)))
  (drop (call $kv_set (get_local 0) (i32.const 1) (i32.const 0) (i32.const 0) (i32.const 2) (get_local 0)))
  (drop (call $kv_get (get_local 0) (i32.const 1) (i32.const 1) (i32.const 2)))
  (drop (call $kv_get (get_local 0) (i32.const 2) (i32.const 0) (i32.const 2)))
  (drop (call $kv_get (get_local 0) (i32.const 6) (i32.const 1) (i32.const 2)))
  (drop (call $kv_get (get_local 0) (i32.const 6) (i32.const 0) (i32.const 2)))
  (drop (call $kv_get (get_local 0) (i32.const 4) (i32.const 0) (i32.const 2)))
 )
)
)=====";

static const char kv_alias_general_wast[] = R"=====(
(module
 (func $read_action_data (import "env" "read_action_data") (param i32 i32) (result i32))
 (func $kv_get (import "env" "kv_get") (param i64 i32 i32 i32) (result i32))
 (func $kv_set (import "env" "kv_set") (param i64 i32 i32 i32 i32 i64) (result i64))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (local $span_start i32)
  (local $span_size i32)
  (drop (call $read_action_data (i32.const 0) (i32.const 8)))
  (set_local $span_start (i32.load (i32.const 0)))
  (set_local $span_size  (i32.load (i32.const 4)))
  (drop (call $kv_get (get_local 0) (get_local $span_start) (get_local $span_size) (i32.const 32)))
  (drop (call $kv_set (get_local 0) (i32.const 64) (i32.const 4) (get_local $span_start) (get_local $span_size) (get_local 0)))
  (drop (call $kv_set (get_local 0) (get_local $span_start) (get_local $span_size) (i32.const 128) (i32.const 4) (get_local 0)))
 )
)
)=====";

BOOST_DATA_TEST_CASE_F(tester, alias, bdata::make(databases), db) {
   const name alias_pass_account{"alias.pass"_n};
   const name alias_general_account{"alias.gen"_n};

   create_accounts({ "setup"_n, alias_pass_account, alias_general_account });
   set_code( "setup"_n, kv_setup_wast );
   push_action( "eosio"_n, "setpriv"_n, "eosio"_n, mutable_variant_object()("account", "setup"_n)("is_priv", 1));
   BOOST_TEST_REQUIRE(push_action( action({}, "setup"_n, db, construct_names_payload({alias_pass_account, alias_general_account})), "setup"_n.to_uint64_t() ) == "");

   set_code( alias_pass_account, kv_alias_pass_wast );
   set_code( alias_general_account, kv_alias_general_wast );

   BOOST_TEST_CHECK(push_action( action({}, alias_pass_account, db, {}), alias_pass_account.to_uint64_t() ) == "");

   auto construct_span_payload = [](uint32_t span_start, uint32_t span_size) -> std::vector<char> {
      std::vector<char> result;
      result.resize(8);
      fc::datastream<char*> ds(result.data(), result.size());
      fc::raw::pack(ds, span_start);
      fc::raw::pack(ds, span_size);
      return result;
   };

   // The kv_alias_general_wast code effectively creates three reserved intervals in linear memory: [32, 36), [64, 68), and [128, 132).
   // The constructed span sent as an argument to the code can be used to test various aliasing scenarios.

   static const char* const alias_error_msg = "pointers not allowed to alias";

   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(31, 1)), alias_general_account.to_uint64_t() ) == "");
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(36, 28)), alias_general_account.to_uint64_t() ) == "");
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(68, 60)), alias_general_account.to_uint64_t() ) == "");
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(132, 1)), alias_general_account.to_uint64_t() ) == "");

   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(31, 2)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(32, 1)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(33, 1)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(35, 1)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(35, 2)), alias_general_account.to_uint64_t() ) == alias_error_msg);

   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(63, 2)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(64, 1)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(65, 1)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(67, 1)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(67, 2)), alias_general_account.to_uint64_t() ) == alias_error_msg);

   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(127, 2)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(128, 1)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(129, 1)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(131, 1)), alias_general_account.to_uint64_t() ) == alias_error_msg);
   BOOST_TEST_CHECK(push_action( action({}, alias_general_account, db, construct_span_payload(131, 2)), alias_general_account.to_uint64_t() ) == alias_error_msg);
}

BOOST_AUTO_TEST_SUITE_END()
