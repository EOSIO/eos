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

   abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(kv_tests)

BOOST_FIXTURE_TEST_CASE(it_lifetime, kv_tester) try {
   BOOST_REQUIRE_EQUAL("Bad key-value database ID", push_action(N(itlifetime), mvo()("db", 2)));
   BOOST_REQUIRE_EQUAL("Bad key-value database ID", push_action(N(itlifetime), mvo()("db", 3)));
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("itlifetime passed"), push_action(N(itlifetime), mvo()("db", 0)));
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
