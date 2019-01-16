/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/testing/tester.hpp>
#include <Runtime/Runtime.h>

#include <fc/io/json.hpp>
#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(multi_index_tests)

BOOST_FIXTURE_TEST_CASE( multi_index_load, TESTER ) try {

   produce_blocks(2);
   create_accounts( {N(multitest)} );
   produce_blocks(2);

   set_code( N(multitest), contracts::multi_index_test_wasm() );
   set_abi( N(multitest), contracts::multi_index_test_abi().data() );

   produce_blocks(1);

   auto abi_string = std::string(contracts::multi_index_test_abi().data());

   abi_serializer abi_ser(json::from_string(abi_string).as<abi_def>(), abi_serializer_max_time);

   signed_transaction trx1;
   {
      auto& trx = trx1;
      action trigger_act;
      trigger_act.account = N(multitest);
      trigger_act.name = N(multitest);
      trigger_act.authorization = vector<permission_level>{{N(multitest), config::active_name}};
      trigger_act.data = abi_ser.variant_to_binary("multitest", mutable_variant_object()
                                                   ("what", 0),
                                                   abi_serializer_max_time
      );
   
      trx.actions.emplace_back(std::move(trigger_act));
      set_transaction_headers(trx);
      trx.sign(get_private_key(N(multitest), "active"), control->get_chain_id());
      push_transaction(trx);
   }
    
   signed_transaction trx2;
   {
      auto& trx = trx2;

      action trigger_act;
      trigger_act.account = N(multitest);
      trigger_act.name = N(multitest);
      trigger_act.authorization = vector<permission_level>{{N(multitest), config::active_name}};
      trigger_act.data = abi_ser.variant_to_binary("multitest", mutable_variant_object()
                                                   ("what", 1),
                                                   abi_serializer_max_time
      );
      trx.actions.emplace_back(std::move(trigger_act));
      set_transaction_headers(trx);
      trx.sign(get_private_key(N(multitest), "active"), control->get_chain_id());
      push_transaction(trx);
   }

   produce_block();
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx1.id()));
   BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx2.id()));

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
