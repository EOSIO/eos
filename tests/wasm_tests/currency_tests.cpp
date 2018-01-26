#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <currency/currency.wast.hpp>
#include <currency/currency.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;
using namespace fc;

struct issue {
   static uint64_t get_account(){ return N(currency); }
   static uint64_t get_name(){ return N(issue); }

   account_name to;
   asset        quantity;
};
FC_REFLECT( issue, (to)(quantity) )

BOOST_AUTO_TEST_SUITE(currency_tests)

BOOST_FIXTURE_TEST_CASE( test_generic_currency, tester ) try {
   produce_blocks(2000);
   create_accounts( {N(currency), N(usera), N(userb)}, asset::from_string("1000.0000 EOS") );
   produce_blocks(2);
   set_code( N(currency), currency_wast );
   produce_blocks(2);

   auto expected = asset::from_string( "10.0000 CUR" );


   {
      signed_transaction trx;
      trx.actions.emplace_back(vector<permission_level>{{N(currency), config::active_name}},
                               issue{ .to = N(usera),
                                  .quantity = expected
                               });

      set_tapos(trx);
      trx.sign(get_private_key(N(currency), "active"), chain_id_type());
      auto result = push_transaction(trx);
      for( const auto& act : result.action_traces )
         std::cerr << act.console << "\n";
      produce_block();

      auto actual = get_currency_balance(N(currency), expected.symbol, N(usera));
      BOOST_REQUIRE_EQUAL(expected, actual);
   }

} FC_LOG_AND_RETHROW() /// test_api_bootstrap

BOOST_FIXTURE_TEST_CASE( test_currency, tester ) try {
   produce_blocks(2000);

   create_accounts( {N(currency), N(alice), N(bob)}, asset::from_string("1000.0000 EOS") );
   transfer( N(inita), N(currency), "10.0000 EOS", "memo" );
   produce_block();

   set_code(N(currency), currency_wast);
   set_abi(N(currency), currency_abi);
   produce_blocks(1);

   const auto& accnt  = control->get_database().get<account_object,by_name>( N(currency) );
   abi_def abi;
   BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
   abi_serializer abi_ser(abi);
   const auto token_supply = asset::from_string("1000000.0000 CUR");

   // issue tokens
   {
      signed_transaction trx;
      action issue_act;
      issue_act.account = N(currency);
      issue_act.name = N(issue);
      issue_act.authorization = vector<permission_level>{{N(currency), config::active_name}};
      issue_act.data = abi_ser.variant_to_binary("issue", mutable_variant_object()
         ("to",       "currency")
         ("quantity", "1000000.0000 CUR")
      );
      trx.actions.emplace_back(std::move(issue_act));

      set_tapos(trx);
      trx.sign(get_private_key(N(currency), "active"), chain_id_type());
      control->push_transaction(trx);
      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      BOOST_REQUIRE_EQUAL(get_currency_balance(N(currency), token_supply.symbol, N(currency)), asset::from_string( "1000000.0000 CUR" ));
   }

   // make a transfer from the contract to a user
   {
      signed_transaction trx;
      action transfer_act;
      transfer_act.account = N(currency);
      transfer_act.name = N(transfer);
      transfer_act.authorization = vector<permission_level>{{N(currency), config::active_name}};
      transfer_act.data = abi_ser.variant_to_binary("transfer", mutable_variant_object()
         ("from", "currency")
         ("to",   "alice")
         ("quantity", "100.0000 CUR")
         ("memo", "fund Alice")
      );
      trx.actions.emplace_back(std::move(transfer_act));

      set_tapos(trx);
      trx.sign(get_private_key(N(currency), "active"), chain_id_type());
      control->push_transaction(trx);
      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      BOOST_REQUIRE_EQUAL(get_currency_balance(N(currency), token_supply.symbol, N(alice)), asset::from_string( "100.0000 CUR" ));
   }

   // Overspend!
   {
      signed_transaction trx;
      action transfer_act;
      transfer_act.account = N(currency);
      transfer_act.name = N(transfer);
      transfer_act.authorization = vector<permission_level>{{N(alice), config::active_name}};
      transfer_act.data = abi_ser.variant_to_binary("transfer", mutable_variant_object()
         ("from", "alice")
         ("to",   "bob")
         ("quantity", "101.0000 CUR")
         ("memo", "overspend! Alice")
      );
      trx.actions.emplace_back(std::move(transfer_act));

      set_tapos(trx);
      trx.sign(get_private_key(N(alice), "active"), chain_id_type());
      BOOST_CHECK_EXCEPTION(control->push_transaction(trx), fc::assert_exception, assert_message_is("integer underflow subtracting token balance"));
      produce_block();

      BOOST_REQUIRE_EQUAL(false, chain_has_transaction(trx.id()));
      BOOST_REQUIRE_EQUAL(get_currency_balance(N(currency), token_supply.symbol, N(alice)), asset::from_string( "100.0000 CUR" ));
      BOOST_REQUIRE_EQUAL(get_currency_balance(N(currency), token_supply.symbol, N(bob)), asset::from_string( "0.0000 CUR" ));
   }

   // Full spend
   {
      signed_transaction trx;
      action transfer_act;
      transfer_act.account = N(currency);
      transfer_act.name = N(transfer);
      transfer_act.authorization = vector<permission_level>{{N(alice), config::active_name}};
      transfer_act.data = abi_ser.variant_to_binary("transfer", mutable_variant_object()
         ("from", "alice")
         ("to",   "bob")
         ("quantity", "100.0000 CUR")
         ("memo", "all in! Alice")
      );
      trx.actions.emplace_back(std::move(transfer_act));

      set_tapos(trx);
      trx.sign(get_private_key(N(alice), "active"), chain_id_type());
      control->push_transaction(trx);
      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      BOOST_REQUIRE_EQUAL(get_currency_balance(N(currency), token_supply.symbol, N(alice)), asset::from_string( "0.0000 CUR" ));
      BOOST_REQUIRE_EQUAL(get_currency_balance(N(currency), token_supply.symbol, N(bob)), asset::from_string( "100.0000 CUR" ));
   }

} FC_LOG_AND_RETHROW() /// test_currency

BOOST_AUTO_TEST_SUITE_END()
