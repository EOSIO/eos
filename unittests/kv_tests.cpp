#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

class kv_tester : public tester {
 public:
   kv_tester() {
      produce_blocks(2);

      create_accounts({ N(kvtest) });
      produce_blocks(2);

      set_code(N(kvtest), contracts::kv_test_wasm());
      set_abi(N(kvtest), contracts::kv_test_abi().data());

      produce_blocks();

      const auto& accnt = control->db().get<account_object, by_name>(N(kvtest));
      abi_def     abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi, abi_serializer_max_time);
   }

   action_result push_action(const action_name& name, const variant_object& data) {
      string action_type_name = abi_ser.get_action_type(name);
      action act;
      act.account = N(kvtest);
      act.name    = name;
      act.data    = abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);
      return base_tester::push_action(std::move(act), N(kvtest).to_uint64_t());
   }

   void erase(const char* error, uint64_t db, name contract, const char* k) {
      BOOST_REQUIRE_EQUAL(error, push_action(N(erase), mvo()("db", db)("contract", contract)("k", k)));
   }

   template <typename V>
   void get(const char* error, uint64_t db, name contract, const char* k, V v) {
      BOOST_REQUIRE_EQUAL(error, push_action(N(get), mvo()("db", db)("contract", contract)("k", k)("v", v)));
   }

   template <typename V>
   void set(const char* error, uint64_t db, name contract, const char* k, V v) {
      BOOST_REQUIRE_EQUAL(error, push_action(N(set), mvo()("db", db)("contract", contract)("k", k)("v", v)));
   }

   abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(kv_tests)

BOOST_FIXTURE_TEST_CASE(it_basic, kv_tester) try {
   BOOST_REQUIRE_EQUAL("Bad key-value database ID", push_action(N(itlifetime), mvo()("db", 2)));
   BOOST_REQUIRE_EQUAL("Bad key-value database ID", push_action(N(itlifetime), mvo()("db", 3)));
   BOOST_REQUIRE_EQUAL("", push_action(N(itlifetime), mvo()("db", 0)));

   get("", 0, N(kvtest), "", nullptr);
   set("", 0, N(kvtest), "", "");
   get("", 0, N(kvtest), "", "");
   set("", 0, N(kvtest), "", "1234");
   get("", 0, N(kvtest), "", "1234");
   get("", 0, N(kvtest), "00", nullptr);
   set("", 0, N(kvtest), "00", "aabbccdd");
   get("", 0, N(kvtest), "00", "aabbccdd");
   get("", 0, N(kvtest), "02", nullptr);
   erase("", 0, N(kvtest), "02");
   get("", 0, N(kvtest), "02", nullptr);
   set("", 0, N(kvtest), "02", "42");
   get("", 0, N(kvtest), "02", "42");
   get("", 0, N(kvtest), "01020304", nullptr);
   set("", 0, N(kvtest), "01020304", "aabbccddee");
   erase("", 0, N(kvtest), "02");

   get("", 0, N(kvtest), "01020304", "aabbccddee");
   get("", 0, N(kvtest), "", "1234");
   get("", 0, N(kvtest), "00", "aabbccdd");
   get("", 0, N(kvtest), "02", nullptr);
   get("", 0, N(kvtest), "01020304", "aabbccddee");
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
