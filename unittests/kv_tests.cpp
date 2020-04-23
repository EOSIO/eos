#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>

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
   kv_tester() {
      produce_blocks(2);

      create_accounts({ N(kvtest), N(kvtest1), N(kvtest2), N(kvtest3), N(kvtest4) });
      produce_blocks(2);

      for(auto account : {N(kvtest), N(kvtest1), N(kvtest2), N(kvtest3), N(kvtest4)}) {
         set_code(account, contracts::kv_test_wasm());
         set_abi(account, contracts::kv_test_abi().data());
      }

      set_code(config::system_account_name, contracts::kv_bios_wasm());
      set_abi(config::system_account_name, contracts::kv_bios_abi().data());

      produce_blocks();

      {
         const auto& accnt = control->db().get<account_object, by_name>(N(kvtest));
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

      BOOST_TEST_REQUIRE(set_limit(N(eosio.kvdisk), -1) == "");
      BOOST_TEST_REQUIRE(set_limit(N(eosio.kvdisk), -1, N(kvtest1)) == "");
      BOOST_TEST_REQUIRE(set_limit(N(eosio.kvdisk), -1, N(kvtest2)) == "");
      BOOST_TEST_REQUIRE(set_limit(N(eosio.kvdisk), -1, N(kvtest3)) == "");
      BOOST_TEST_REQUIRE(set_limit(N(eosio.kvdisk), -1, N(kvtest4)) == "");
      BOOST_TEST_REQUIRE(set_kv_limits(N(eosio.kvram), 1024, 1024*1024) == "");
      BOOST_TEST_REQUIRE(set_kv_limits(N(eosio.kvdisk), 1024, 1024*1024) == "");
      produce_block();
   }

   action_result push_action(const action_name& name, const variant_object& data, chain::name account = N(kvtest)) {
      string action_type_name = abi_ser.get_action_type(name);
      action act;
      act.account = account;
      act.name    = name;
      act.data    = abi_ser.variant_to_binary(action_type_name, data, abi_serializer::create_yield_function(abi_serializer_max_time));
      return base_tester::push_action(std::move(act), N(kvtest).to_uint64_t());
   }

   action_result set_limit(name db, int64_t limit, name account = N(kvtest)) {
      action_name name;
      if(db == N(eosio.kvdisk)) {
         name = N(setdisklimit);
      } else if(db == N(eosio.kvram)) {
         name = N(setramlimit);
      } else {
         BOOST_FAIL("Wrong database id");
      }
      string action_type_name = sys_abi_ser.get_action_type(name);
      action act;
      act.account = config::system_account_name;
      act.name    = name;
      act.data    = sys_abi_ser.variant_to_binary(action_type_name, mvo()("account", account)("limit", limit), abi_serializer::create_yield_function(abi_serializer_max_time));
      return base_tester::push_action(std::move(act), config::system_account_name.to_uint64_t());
   }

   action_result set_kv_limits(name db, uint32_t klimit, uint32_t vlimit, uint32_t ilimit = 256) {
      action_name name;
      if(db == N(eosio.kvdisk)) {
         name = N(diskkvlimits);
      } else if(db == N(eosio.kvram)) {
         name = N(ramkvlimits);
      } else {
         BOOST_FAIL("Wrong database id");
      }
      string action_type_name = sys_abi_ser.get_action_type(name);
      action act;
      act.account = config::system_account_name;
      act.name    = name;
      act.data    = sys_abi_ser.variant_to_binary(action_type_name, mvo()("k", klimit)("v", vlimit)("i", ilimit), abi_serializer::create_yield_function(abi_serializer_max_time));
      return base_tester::push_action(std::move(act), config::system_account_name.to_uint64_t());
   }

   void erase(const char* error, name db, name contract, const char* k) {
      BOOST_REQUIRE_EQUAL(error, push_action(N(erase), mvo()("db", db)("contract", contract)("k", k)));
   }

   template <typename V>
   void get(const char* error, name db, name contract, const char* k, V v) {
      BOOST_REQUIRE_EQUAL(error, push_action(N(get), mvo()("db", db)("contract", contract)("k", k)("v", v)));
   }

   template <typename V>
   action_result set(name db, name contract, const char* k, V v) {
      return push_action(N(set), mvo()("db", db)("contract", contract)("k", k)("v", v));
   }

   action_result iterlimit(const std::vector<itparam>& params) {
      return push_action(N(itlimit), mvo()("params", params));
   }

   void setmany(const char* error, name db, name contract, const std::vector<kv>& kvs) {
      BOOST_REQUIRE_EQUAL(error, push_action(N(setmany), mvo()("db", db)("contract", contract)("kvs", kvs), contract));
   }

   template <typename T>
   void scan(const char* error, name db, name contract, const char* prefix, T lower, const std::vector<kv>& expected) {
      BOOST_REQUIRE_EQUAL(error, push_action(N(scan), mvo()("db", db)("contract", contract)("prefix", prefix)(
                                                            "lower", lower)("expected", expected)));
      BOOST_REQUIRE_EQUAL(error, push_action(N(scan), mvo()("db", db)("contract", contract)("prefix", prefix)(
                                                            "lower", lower)("expected", expected), contract));
   }

   template <typename T>
   action_result scanrev(name db, name contract, const char* prefix, T lower,
                const std::vector<kv>& expected) {
      auto result1 = push_action(N(scanrev), mvo()("db", db)("contract", contract)("prefix", prefix)(
                                           "lower", lower)("expected", expected));
      auto result2 = push_action(N(scanrev), mvo()("db", db)("contract", contract)("prefix", prefix)(
                                           "lower", lower)("expected", expected), contract);
      BOOST_TEST(result1 == result2);
      return result1;
   }

   void itstaterased(const char* error, name db, name contract, const char* prefix, const char* k, const char* v,
                     int test_id, bool insert, bool reinsert) {
      BOOST_REQUIRE_EQUAL(error, push_action(N(itstaterased), mvo()("db", db)("contract", contract)("prefix", prefix)(
                                                                    "k", k)("v", v)("test_id", test_id)("insert", insert)("reinsert", reinsert)));
   }

   uint64_t get_usage(name db) {
      if (db == N(eosio.kvram)) {
         return control->get_resource_limits_manager().get_account_ram_usage(N(kvtest));
      } else if (db == N(eosio.kvdisk)) {
         return control->get_resource_limits_manager().get_account_disk_usage(N(kvtest));
      }
      BOOST_FAIL("Wrong db");
      return 0;
   }

   void test_basic(name db) {
      get("", db, N(kvtest), "", nullptr);
      BOOST_TEST("" == set(db, N(kvtest), "", ""));
      get("", db, N(kvtest), "", "");
      BOOST_TEST("" == set(db, N(kvtest), "", "1234"));
      get("", db, N(kvtest), "", "1234");
      get("", db, N(kvtest), "00", nullptr);
      BOOST_TEST("" == set(db, N(kvtest), "00", "aabbccdd"));
      get("", db, N(kvtest), "00", "aabbccdd");
      get("", db, N(kvtest), "02", nullptr);
      erase("", db, N(kvtest), "02");
      get("", db, N(kvtest), "02", nullptr);
      BOOST_TEST("" == set(db, N(kvtest), "02", "42"));
      get("", db, N(kvtest), "02", "42");
      get("", db, N(kvtest), "01020304", nullptr);
      BOOST_TEST("" == set(db, N(kvtest), "01020304", "aabbccddee"));
      erase("", db, N(kvtest), "02");

      get("", db, N(kvtest), "01020304", "aabbccddee");
      get("", db, N(kvtest), "", "1234");
      get("", db, N(kvtest), "00", "aabbccdd");
      get("", db, N(kvtest), "02", nullptr);
      get("", db, N(kvtest), "01020304", "aabbccddee");
   }

   void test_other_contract(name db) {
      get("", db, N(missing), "", nullptr);
      get("", db, N(kvtest1), "", nullptr);
      BOOST_TEST(set(db, N(missing), "", "") == "Can not write to this key");
      BOOST_TEST(set(db, N(kvtest1), "", "") == "Can not write to this key");
      erase("Can not write to this key", db, N(missing), "");
      erase("Can not write to this key", db, N(kvtest1), "");
   }

   void test_get_data(name db) {
      BOOST_TEST(push_action(N(getdata), mvo()("db", db)) == "");
   }

   void test_scan(name db, name account = N(kvtest)) {
      setmany("", db, account,
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
      scan("", db, account, "", nullptr,
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
      scan("", db, account, "", "",
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
      scan("", db, account, "", "221100",
           {
                 kv{ { 0x22, 0x33 }, { 0x18 } },
                 kv{ { 0x44 }, { 0x76 } },
                 kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                 kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                 kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
           });

      // no prefix, lower bound = "2233"
      scan("", db, account, "", "2233",
           {
                 kv{ { 0x22, 0x33 }, { 0x18 } },
                 kv{ { 0x44 }, { 0x76 } },
                 kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                 kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                 kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
           });

      // prefix = "22", no lower bound
      scan("", db, account, "22", nullptr,
           {
                 kv{ { 0x22 }, {} },
                 kv{ { 0x22, 0x11 }, { 0x34 } },
                 kv{ { 0x22, 0x33 }, { 0x18 } },
           });

      // prefix = "22", lower bound = "2211"
      scan("", db, account, "22", "2211",
           {
                 kv{ { 0x22, 0x11 }, { 0x34 } },
                 kv{ { 0x22, 0x33 }, { 0x18 } },
           });

      // prefix = "22", lower bound = "2234"
      scan("", db, account, "22", "2234", {});

      // prefix = "33", no lower bound
      scan("", db, account, "33", nullptr, {});

      // prefix = "44", lower bound = "223300"
      // kv_it_lower_bound finds "44", which is in the prefix range
      scan("", db, account, "44", "223300",
           {
                 kv{ { 0x44 }, { 0x76 } },
                 kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                 kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                 kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
           });

      // prefix = "44", lower bound = "2233"
      // lower_bound over the whole table would find "2233", which is out of the prefix range,
      // and should not affect the result.
      scan("", db, account, "44", "2233",
           {
                 kv{ { 0x44 }, { 0x76 } },
                 kv{ { 0x44, 0x00 }, { 0x11, 0x22, 0x33 } },
                 kv{ { 0x44, 0x01 }, { 0x33, 0x22, 0x11 } },
                 kv{ { 0x44, 0x02 }, { 0x78, 0x04 } },
           });
   } // test_scan

   void test_scanrev(name db, name account) {
      setmany("", db, account,
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
      BOOST_TEST("" == scanrev(db, account, "", nullptr,
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
      BOOST_TEST("" == scanrev(db, account, "", "4402",
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
      BOOST_TEST("" == scanrev(db, account, "", "44",
              {
                    kv{ { 0x44 }, { 0x76 } },
                    kv{ { 0x22, 0x33 }, { 0x18 } },
                    kv{ { 0x22, 0x11 }, { 0x34 } },
                    kv{ { 0x22 }, {} },
                    kv{ {}, { 0x12 } },
              }));

      // prefix = "22", no lower bound
      BOOST_TEST("" == scanrev(db, account, "22", nullptr,
              {
                    kv{ { 0x22, 0x33 }, { 0x18 } },
                    kv{ { 0x22, 0x11 }, { 0x34 } },
                    kv{ { 0x22 }, {} },
              }));

      // prefix = "22", lower bound = "2233"
      BOOST_TEST("" == scanrev(db, account, "22", "2233",
              {
                    kv{ { 0x22, 0x33 }, { 0x18 } },
                    kv{ { 0x22, 0x11 }, { 0x34 } },
                    kv{ { 0x22 }, {} },
              }));

      // prefix = "22", lower bound = "2234"
      BOOST_TEST("" == scanrev(db, account, "22", "2234", {}));

      // prefix = "33", no lower bound
      BOOST_TEST("" == scanrev(db, account, "33", nullptr, {}));
   } // test_scanrev

   void test_scanrev2(name db) {
      setmany("", db, N(kvtest),
              {
                    kv{ { 0x00, char(0xFF), char(0xFF) }, { 0x10 } },
                    kv{ { 0x01 }, { 0x20 } },
                    kv{ { 0x01, 0x00 }, { 0x30 } },
              });

      // prefix = "00FFFF", no lower bound
      BOOST_TEST("" == scanrev(db, N(kvtest), "00FFFF", nullptr,
              {
                    kv{ { 0x00, char(0xFF), char(0xFF) }, { 0x10 } },
              }));
   } // test_scanrev2

   void test_iterase(name db) {
      for(bool reinsert : {false, true}) {
         // pre-inserted
         for(bool insert : {false, true}) {
            for(int i = 0; i < 8; ++i) {
               setmany("", db, N(kvtest), { kv{ { 0x22 }, { 0x12 } } });
               produce_block();
               itstaterased("Iterator to erased element", db, N(kvtest), "", "22", "12", i, insert, reinsert );
            }
            setmany("", db, N(kvtest), { kv{ { 0x22 }, { 0x12 } } });
            produce_block();
            itstaterased("", db, N(kvtest), "", "22", "12", 8, insert, reinsert );
            if(!reinsert) {
               setmany("", db, N(kvtest), { kv{ { 0x22 }, { 0x12 } } });
               produce_block();
               itstaterased("", db, N(kvtest), "", "22", "12", 9, insert, reinsert );
            }
            setmany("", db, N(kvtest), { kv{ { 0x22 }, { 0x12 } }, kv{ { 0x23 }, { 0x13 } } });
            produce_block();
            itstaterased("", db, N(kvtest), "", "22", "12", 10, insert, reinsert );
            erase("", db, N(kvtest), "23");
            setmany("", db, N(kvtest), { kv{ { 0x22 }, { 0x12 } } });
            produce_block();
            itstaterased("", db, N(kvtest), "", "22", "12", 11, insert, reinsert );
         }
         // inserted inside contract
         for(int i = 0; i < 8; ++i) {
            erase("", db, N(kvtest), "22");
            produce_block();
            itstaterased("Iterator to erased element", db, N(kvtest), "", "22", "12", i, true, reinsert );
         }
         erase("", db, N(kvtest), "22");
         produce_block();
         itstaterased("", db, N(kvtest), "", "22", "12", 8, true, reinsert );
         if(!reinsert) {
            erase("", db, N(kvtest), "22");
            produce_block();
            itstaterased("", db, N(kvtest), "", "22", "12", 9, true, reinsert );
         }
         erase("", db, N(kvtest), "22");
         setmany("", db, N(kvtest), { kv{ { 0x23 }, { 0x13 } } });
         produce_block();
         itstaterased("", db, N(kvtest), "", "22", "12", 10, true, reinsert );
         erase("", db, N(kvtest), "23");
         erase("", db, N(kvtest), "22");
         produce_block();
         itstaterased("", db, N(kvtest), "", "22", "12", 11, true, reinsert );
      }
   }

   void test_ram_usage(name db) {
      uint64_t base_usage = get_usage(db);
      // DISK should start at 0, because it's only used by the kv store.
      if(db == N(eosio.kvdisk)) BOOST_TEST(base_usage == 0);

      get("", db, N(kvtest), "11", nullptr);
      BOOST_TEST(get_usage(db) == base_usage);
      BOOST_TEST("" == set(db, N(kvtest), "11", ""));
      BOOST_TEST(get_usage(db) == base_usage + 112 + 1);
      BOOST_TEST("" == set(db, N(kvtest), "11", "1234"));
      BOOST_TEST(get_usage(db) == base_usage + 112 + 1 + 2);
      BOOST_TEST("" == set(db, N(kvtest), "11", "12"));
      BOOST_TEST(get_usage(db) == base_usage + 112 + 1 + 1);
      erase("", db, N(kvtest), "11");
      BOOST_TEST(get_usage(db) == base_usage);
   }

   void test_resource_limit(name db) {
      uint64_t base_usage = get_usage(db);
      // insert a new element
      BOOST_TEST_REQUIRE(set_limit(db, base_usage + 112) == "");
      BOOST_TEST(set(db, N(kvtest), "11", "").find("account kvtest has insufficient") == 0);
      BOOST_TEST_REQUIRE(set_limit(db, base_usage + 112 + 1) == "");
      BOOST_TEST("" == set(db, N(kvtest), "11", ""));
      // increase the size of a value
      BOOST_TEST_REQUIRE(set_limit(db, base_usage + 112 + 1 + 2 - 1) == "");
      BOOST_TEST(set(db, N(kvtest), "11", "1234").find("account kvtest has insufficient") == 0);
      BOOST_TEST_REQUIRE(set_limit(db, base_usage + 112 + 1 + 2) == "");
      BOOST_TEST("" == set(db, N(kvtest), "11", "1234"));
      // decrease the size of a value
      BOOST_TEST("" == set(db, N(kvtest), "11", ""));
      // decrease limits
      BOOST_TEST(set_limit(db, base_usage + 112).find("account kvtest has insufficient") == 0);
      BOOST_TEST(set_limit(db, base_usage + 112 + 1) == "");
      // erase an element
      erase("", db, N(kvtest), "11");
   }

   void test_key_value_limit(name db) {
      BOOST_TEST_REQUIRE(set_kv_limits(db, 4, 4) == "");
      setmany("", db, N(kvtest), {{bytes(4, 'a'), bytes(4, 'a')}});
      setmany("Key too large", db, N(kvtest), {{bytes(4 + 1, 'a'), bytes()}});
      setmany("Value too large", db, N(kvtest), {{bytes(), bytes(4 + 1, 'a')}});

      // The check happens at the point of calling set.  Changing the bad value later doesn't bypass errors.
      setmany("Value too large", db, N(kvtest), {{bytes(4, 'b'), bytes(5, 'b')}, {bytes(4, 'b'), {}}});

      // The key is checked even if it already exists
      setmany("Key too large", db, N(kvtest), {{bytes(1024, 'a'), {}}});

      scan("", db, N(kvtest), "00000000", "", {});
      scan("Prefix too large", db, N(kvtest), "0000000000", "", {});
   }

   action make_set_action(name db, std::size_t size) {
      std::vector<char> k;
      std::vector<char> v(size, 'a');
      action act;
      act.account = N(kvtest);
      act.name    = N(set);
      act.data    = abi_ser.variant_to_binary("set", mvo()("db", db)("contract", N(kvtest))("k", k)("v", v), abi_serializer::create_yield_function(abi_serializer_max_time));
      act.authorization = vector<permission_level>{{N(kvtest), config::active_name}};
      return act;
   }

   action make_set_limit_action(name db, int64_t limit) {
      action act;
      act.account = N(eosio);
      act.name    = db == N(eosio.kvram)?N(setramlimit):N(setdisklimit);
      act.data    = sys_abi_ser.variant_to_binary(act.name.to_string(), mvo()("account", N(kvtest))("limit", limit), abi_serializer::create_yield_function(abi_serializer_max_time));
      act.authorization = vector<permission_level>{{N(kvtest), config::active_name}};
      return act;
   }

   // Go over the limit and back in one transaction, with two separate actions
   void test_kv_inc_dec_usage(name db) {
      BOOST_TEST_REQUIRE(set_limit(db, get_usage(db) + 256) == "");
      produce_block();
      signed_transaction trx;
      trx.actions.push_back(make_set_action(db, 512));
      trx.actions.push_back(make_set_action(db, 0));
      set_transaction_headers(trx);
      trx.sign(get_private_key(N(kvtest), "active"), control->get_chain_id());
      push_transaction(trx);
      produce_block();
   }

   // Increase usage and then the limit in two actions in the same transaction.
   void test_kv_inc_usage_and_limit(name db) {
      auto base_usage = get_usage(db);
      BOOST_TEST_REQUIRE(set_limit(db, base_usage + 256) == "");
      produce_block();
      signed_transaction trx;
      {
         trx.actions.push_back(make_set_action(db, 512));
         trx.actions.push_back(make_set_limit_action(db, base_usage + 624));
      }
      set_transaction_headers(trx);
      trx.sign(get_private_key(N(kvtest), "active"), control->get_chain_id());
      push_transaction(trx);
      produce_block();
   }

   // Decrease limit and then usage in two separate actions in the same transaction
   void test_kv_dec_limit_and_usage(name db) {
      auto base_usage = get_usage(db);
      BOOST_TEST_REQUIRE(set_limit(db, base_usage + 1024) == "");
      BOOST_TEST_REQUIRE(set(db, N(kvtest), "", std::vector<char>(512, 'a')) == "");
      produce_block();
      signed_transaction trx;
      {
         trx.actions.push_back(make_set_limit_action(db, base_usage + 256));
         trx.actions.push_back(make_set_action(db, 0));
      }
      set_transaction_headers(trx);
      trx.sign(get_private_key(N(kvtest), "active"), control->get_chain_id());
      push_transaction(trx);
      produce_block();
   }

   void test_max_iterators() {
      BOOST_TEST_REQUIRE(set_kv_limits(N(eosio.kvram), 1024, 1024, 5) == "");
      BOOST_TEST_REQUIRE(set_kv_limits(N(eosio.kvdisk), 1024, 1024, 7) == "");
      // individual limits
      BOOST_TEST(iterlimit({{N(eosio.kvram), 5, false}}) == "");
      BOOST_TEST(iterlimit({{N(eosio.kvram), 6, false}}) == "Too many iterators");
      BOOST_TEST(iterlimit({{N(eosio.kvdisk), 7, false}}) == "");
      BOOST_TEST(iterlimit({{N(eosio.kvdisk), 2000, false}}) == "Too many iterators");
      // both limits together
      BOOST_TEST(iterlimit({{N(eosio.kvram), 5, false}, {N(eosio.kvdisk), 7, false}}) == "");
      BOOST_TEST(iterlimit({{N(eosio.kvram), 6, false}, {N(eosio.kvdisk), 7, false}}) == "Too many iterators");
      BOOST_TEST(iterlimit({{N(eosio.kvram), 5, false}, {N(eosio.kvdisk), 8, false}}) == "Too many iterators");
      // erase iterators and create more
      BOOST_TEST(iterlimit({{N(eosio.kvram), 4, false}, {N(eosio.kvdisk), 6, false},
                            {N(eosio.kvram), 2, true}, {N(eosio.kvdisk), 2, true},
                            {N(eosio.kvram), 3, false}, {N(eosio.kvdisk), 3, false}}) == "");
      BOOST_TEST(iterlimit({{N(eosio.kvram), 4, false}, {N(eosio.kvdisk), 6, false},
                            {N(eosio.kvram), 2, true}, {N(eosio.kvdisk), 2, true},
                            {N(eosio.kvram), 3, false}, {N(eosio.kvdisk), 4, false}}) == "Too many iterators");
      BOOST_TEST(iterlimit({{N(eosio.kvram), 4, false}, {N(eosio.kvdisk), 6, false},
                            {N(eosio.kvram), 2, true}, {N(eosio.kvdisk), 2, true},
                            {N(eosio.kvram), 4, false}, {N(eosio.kvdisk), 3, false}}) == "Too many iterators");
      // fallback limit - testing this is impractical because it uses too much memory
      // This many iterators would consume at least 400 GiB.
      // BOOST_TEST_REQUIRE(set_kv_limits(N(eosio.kvram), 1024, 1024, 0xFFFFFFFF) == "");
      // BOOST_TEST_REQUIRE(set_kv_limits(N(eosio.kvdisk), 1024, 1024, 0xFFFFFFFF) == "");
      // BOOST_TEST(iterlimit({{N(eosio.kvram), 0xFFFFFFFF, false}, {N(eosio.kvdisk), 1, false}}) == "Too many iterators");
   }

   abi_serializer abi_ser;
   abi_serializer sys_abi_ser;
};

BOOST_AUTO_TEST_SUITE(kv_tests)

BOOST_FIXTURE_TEST_CASE(kv_basic, kv_tester) try {
   BOOST_REQUIRE_EQUAL("Bad key-value database ID", push_action(N(itlifetime), mvo()("db", N(oops))));
   BOOST_REQUIRE_EQUAL("", push_action(N(itlifetime), mvo()("db", N(eosio.kvram))));
   BOOST_REQUIRE_EQUAL("", push_action(N(itlifetime), mvo()("db", N(eosio.kvdisk))));
   test_basic(N(eosio.kvram));
   test_basic(N(eosio.kvdisk));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scan, kv_tester) try { //
   // four possibilities depending on whether the next or previous contract table has elements
   test_scan(N(eosio.kvram), N(kvtest2));
   test_scan(N(eosio.kvram), N(kvtest4));
   test_scan(N(eosio.kvram), N(kvtest3));
   test_scan(N(eosio.kvram), N(kvtest1));
   test_scan(N(eosio.kvdisk), N(kvtest2));
   test_scan(N(eosio.kvdisk), N(kvtest4));
   test_scan(N(eosio.kvdisk), N(kvtest3));
   test_scan(N(eosio.kvdisk), N(kvtest1));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scan_ram_after_disk, kv_tester) try { //
   // Make sure that the behavior of one database is not affected by having the other database populated.
   test_scan(N(eosio.kvdisk), N(kvtest2));
   test_scan(N(eosio.kvdisk), N(kvtest4));
   test_scan(N(eosio.kvdisk), N(kvtest3));
   test_scan(N(eosio.kvdisk), N(kvtest1));
   test_scan(N(eosio.kvram), N(kvtest2));
   test_scan(N(eosio.kvram), N(kvtest4));
   test_scan(N(eosio.kvram), N(kvtest3));
   test_scan(N(eosio.kvram), N(kvtest1));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scanrev, kv_tester) try { //
   test_scanrev(N(eosio.kvram), N(kvtest2));
   test_scanrev(N(eosio.kvram), N(kvtest4));
   test_scanrev(N(eosio.kvram), N(kvtest3));
   test_scanrev(N(eosio.kvram), N(kvtest1));
   test_scanrev(N(eosio.kvdisk), N(kvtest2));
   test_scanrev(N(eosio.kvdisk), N(kvtest4));
   test_scanrev(N(eosio.kvdisk), N(kvtest3));
   test_scanrev(N(eosio.kvdisk), N(kvtest1));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scanrev_ram_after_disk, kv_tester) try { //
   test_scanrev(N(eosio.kvdisk), N(kvtest2));
   test_scanrev(N(eosio.kvdisk), N(kvtest4));
   test_scanrev(N(eosio.kvdisk), N(kvtest3));
   test_scanrev(N(eosio.kvdisk), N(kvtest1));
   test_scanrev(N(eosio.kvram), N(kvtest2));
   test_scanrev(N(eosio.kvram), N(kvtest4));
   test_scanrev(N(eosio.kvram), N(kvtest3));
   test_scanrev(N(eosio.kvram), N(kvtest1));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_scanrev2, kv_tester) try { //
   test_scanrev2(N(eosio.kvram));
   test_scanrev2(N(eosio.kvdisk));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_iterase, kv_tester) try { //
   test_iterase(N(eosio.kvram));
   test_iterase(N(eosio.kvdisk));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_ram_usage, kv_tester) try { //
   test_ram_usage(N(eosio.kvram));
   test_ram_usage(N(eosio.kvdisk));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_resource_limit, kv_tester) try { //
   test_resource_limit(N(eosio.kvram));
   test_resource_limit(N(eosio.kvdisk));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(kv_key_value_limit, kv_tester) try { //
   test_key_value_limit(N(eosio.kvram));
   test_key_value_limit(N(eosio.kvdisk));
}
FC_LOG_AND_RETHROW()

constexpr name databases[] = { N(eosio.kvram), N(eosio.kvdisk) };

BOOST_DATA_TEST_CASE_F(kv_tester, kv_inc_dec_usage, bdata::make(databases), db) try { //
   test_kv_inc_dec_usage(db);
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_tester, kv_inc_usage_and_limit, bdata::make(databases), db) try { //
   test_kv_inc_usage_and_limit(db);
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_tester, kv_dec_limit_and_usage, bdata::make(databases), db) try { //
   test_kv_dec_limit_and_usage(db);
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_tester, get_data, bdata::make(databases), db) try { //
   test_get_data(db);
}
FC_LOG_AND_RETHROW()

BOOST_DATA_TEST_CASE_F(kv_tester, other_contract, bdata::make(databases), db) try { //
   test_other_contract(db);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(max_iterators, kv_tester) try { //
   test_max_iterators();
}
FC_LOG_AND_RETHROW()

// Initializes a kv_database configuration to usable values.
// Must be set on a privileged account
static const char kv_setup_wast[] =  R"=====(
(module
 (func $kv_set_parameters_packed (import "env" "set_kv_parameters_packed") (param i64 i32 i32))
 (func $set_resource_limit (import "env" "set_resource_limit") (param i64 i64 i64))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
   (call $kv_set_parameters_packed (get_local 2) (i32.const 0) (i32.const 16))
   (call $set_resource_limit (i64.const 11327368866104868864) (i64.const 5454140623722381312) (i64.const -1))
   (call $set_resource_limit (i64.const 11327368596746665984) (i64.const 5454140623722381312) (i64.const -1))
 )
 (data (i32.const 4) "\00\04\00\00")
 (data (i32.const 8) "\00\00\10\00")
 (data (i32.const 12) "\80\00\00\00")
)
)=====";

// Call iterator intrinsics that create native state
// and then notify another contract.  The kv context should
// not affect recipient.
// State tested:
// - iterator id (including re-use of destroyed iterators)
// - temporary buffer for kv_get
// - access checking for write
static const char kv_notify_wast[] = R"=====(
(module
 (func $kv_it_create (import "env" "kv_it_create") (param i64 i64 i32 i32) (result i32))
 (func $kv_it_destroy (import "env" "kv_it_destroy") (param i32))
 (func $kv_get (import "env" "kv_get") (param i64 i64 i32 i32 i32) (result i32))
 (func $kv_set (import "env" "kv_set") (param i64 i64 i32 i32 i32 i32) (result i64))
 (func $require_recipient (import "env" "require_recipient") (param i64))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (drop (call $kv_it_create (get_local 2) (get_local 0) (i32.const 0) (i32.const 0)))
  (drop (call $kv_it_create (get_local 2) (get_local 0) (i32.const 0) (i32.const 0)))
  (call $kv_it_destroy (i32.const 2))
  (drop (call $kv_set (get_local 2) (get_local 0) (i32.const 0) (i32.const 0) (i32.const 1) (i32.const 1)))
  (drop (call $kv_get (get_local 2) (get_local 0) (i32.const 0) (i32.const 0) (i32.const 8)))
  (call $require_recipient (i64.const 11327368596746665984))
 )
)
)=====";

static const char kv_notified_wast[] = R"=====(
(module
 (func $kv_it_create (import "env" "kv_it_create")(param i64 i64 i32 i32) (result i32))
 (func $kv_get_data (import "env" "kv_get_data") (param i64 i32 i32 i32) (result i32))
 (func $kv_set (import "env" "kv_set") (param i64 i64 i32 i32 i32 i32) (result i64))
 (func $eosio_assert (import "env" "eosio_assert") (param i32 i32))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
  (call $eosio_assert (i32.eq (call $kv_it_create (get_local 2) (get_local 0) (i32.const 0) (i32.const 0)) (i32.const 1)) (i32.const 80))
  (call $eosio_assert (i32.eq (call $kv_get_data (get_local 2) (i32.const 0) (i32.const 0) (i32.const 0)) (i32.const 0)) (i32.const 160))
  (drop (call $kv_set (get_local 2) (get_local 0) (i32.const 0) (i32.const 0) (i32.const 1) (i32.const 1)))
 )
 (data (i32.const 80) "Wrong iterator value")
 (data (i32.const 160) "Temporary data buffer not empty")
)
)=====";

BOOST_DATA_TEST_CASE_F(tester, notify, bdata::make(databases), db) {
   create_accounts({ N(setup), N(notified), N(notify) });
   set_code( N(setup), kv_setup_wast );
   push_action( N(eosio), N(setpriv), N(eosio), mutable_variant_object()("account", N(setup))("is_priv", 1));
   BOOST_TEST_REQUIRE(push_action( action({}, N(setup), db, {}), N(setup).to_uint64_t() ) == "");

   set_code( N(notify), kv_notify_wast );
   set_code( N(notified), kv_notified_wast );

   BOOST_TEST_REQUIRE(push_action( action({}, N(notify), db, {}), N(notify).to_uint64_t() ) == "");
}

BOOST_AUTO_TEST_SUITE_END()
