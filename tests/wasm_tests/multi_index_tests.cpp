#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <multi_index_test/multi_index_test.wast.hpp>
#include <multi_index_test/multi_index_test.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

using namespace eosio::chain::contracts;
using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(multi_index_tests)

BOOST_FIXTURE_TEST_CASE( multi_index_load, tester ) try {

   produce_blocks(2);
   create_accounts( {N(multitest)} );
   produce_blocks(2);

   set_code( N(multitest), multi_index_test_wast );
   set_abi( N(multitest), multi_index_test_abi );

   produce_blocks(1);

   abi_serializer abi_ser(json::from_string(multi_index_test_abi).as<abi_def>());

   signed_transaction trx1;
   {
      auto& trx = trx1;
      action trigger_act;
      trigger_act.account = N(multitest);
      trigger_act.name = N(trigger);
      trigger_act.authorization = vector<permission_level>{{N(multitest), config::active_name}};
      trigger_act.data = abi_ser.variant_to_binary("trigger", mutable_variant_object()
                                                   ("what", 0)
      );
      trx.actions.emplace_back(std::move(trigger_act));
      set_tapos(trx);
      trx.sign(get_private_key(N(multitest), "active"), chain_id_type());
      push_transaction(trx);
   }

   signed_transaction trx2;
   {
      auto& trx = trx2;

      action trigger_act;
      trigger_act.account = N(multitest);
      trigger_act.name = N(trigger);
      trigger_act.authorization = vector<permission_level>{{N(multitest), config::active_name}};
      trigger_act.data = abi_ser.variant_to_binary("trigger", mutable_variant_object()
                                                   ("what", 1)
      );
      trx.actions.emplace_back(std::move(trigger_act));
      set_tapos(trx);
      trx.sign(get_private_key(N(multitest), "active"), chain_id_type());
      push_transaction(trx);
   }

   produce_block();
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx1.id()));
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx2.id()));

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
