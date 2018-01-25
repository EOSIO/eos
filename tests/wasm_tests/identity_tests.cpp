#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <identity/identity.wast.hpp>
#include <identity/identity.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include "test_wasts.hpp"

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;
using namespace fc;

BOOST_AUTO_TEST_SUITE(identity_tests)

/**
 * Prove that action reading and assertions are working
 */
BOOST_FIXTURE_TEST_CASE( identity_test, tester ) try {
   produce_blocks(2);

   create_accounts( {N(owner), N(alice)}, asset::from_string("100000.0000 EOS") );
   produce_blocks(1000);

   set_code(N(owner), identity_wast);
   set_abi(N(owner), identity_abi);
   produce_blocks(1);

   const auto& accnt = control->get_database().get<account_object,by_name>( N(owner) );
   abi_def abi;
   BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
   abi_serializer abi_ser(abi);

   {
      signed_transaction trx;
      action create_act;
      create_act.account = N(owner);
      create_act.name = N(alice);
      create_act.authorization = vector<permission_level>{{N(alice), config::active_name}};
      create_act.data = abi_ser.variant_to_binary("create", mutable_variant_object()
                                                    ("creator", "alice")
                                                    ("identity", 1)
                                                   );
      trx.actions.emplace_back(std::move(create_act));
      set_tapos(trx);
      trx.sign(get_private_key(N(alice), "active"), chain_id_type());
      control->push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

} FC_LOG_AND_RETHROW() /// identity_test

BOOST_AUTO_TEST_SUITE_END()
