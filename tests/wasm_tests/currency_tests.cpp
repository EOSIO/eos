#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <currency/currency.wast.hpp>
#include <currency/currency.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;
using namespace fc;

class currency_tester : tester {
   public:
      currency_tester()
      :tester()
      ,abi_ser(json::from_string(currency_abi).as<abi_def>())
      {
         const auto token_supply = asset::from_string("1000000.0000 CUR");

         create_account( N(currency));
         set_code( N(currency), currency_wast );

         push_action(N(currency), N(transfer), {{N(currency), config::active_name}}, mutable_variant_object()
            ("to",       "currency")
            ("quantity", "1000000.0000 CUR")
         );
         produce_block();
      }

      auto push_action(const account_name& signer, const action_name &name, const variant_object &data ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = N(currency);
         act.name = name;
         act.authorization = vector<permission_level>{{signer, config::active_name}};
         act.data = abi_ser.variant_to_binary(action_type_name, data);

         signed_transaction trx;
         trx.actions.emplace_back(std::move(act));
         set_tapos(trx);
         trx.sign(get_private_key(signer, "active"), chain_id_type());
         return push_transaction(trx);
      }

      auto get_balance(const account_name& account) const {
         return get_currency_balance(N(currency), SY(4,CUR), account);
      }

      abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(currency_tests)

BOOST_AUTO_TEST_CASE( bootstrap ) try {
   auto expected = asset::from_string( "1000000.0000 CUR" );
   currency_tester t;
   auto actual = t.get_currency_balance(N(currency), expected.get_symbol(), N(currency));
   BOOST_REQUIRE_EQUAL(expected, actual);
} FC_LOG_AND_RETHROW() /// test_api_bootstrap

BOOST_FIXTURE_TEST_CASE( test_transfer, currency_tester ) try {
   create_accounts( {N(alice)} );

   // make a transfer from the contract to a user
   {
      auto trace = push_action(N(currency), N(transfer), mutable_variant_object()
         ("from", "currency")
         ("to",   "alice")
         ("quantity", "100.0000 CUR")
         ("memo", "fund Alice")
      );

      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ));
   }
} FC_LOG_AND_RETHROW() /// test_transfer

BOOST_FIXTURE_TEST_CASE( test_addtransfer, currency_tester ) try {
   create_accounts( {N(alice)} );

   // make a transfer from the contract to a user
   {
      auto trace = push_action(N(currency), N(transfer), mutable_variant_object()
         ("from", "currency")
         ("to",   "alice")
         ("quantity", "100.0000 CUR")
         ("memo", "fund Alice")
      );

      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ));
   }

   // make a transfer from the contract to a user
   {
      auto trace = push_action(N(currency), N(transfer), mutable_variant_object()
         ("from", "currency")
         ("to",   "alice")
         ("quantity", "10.0000 CUR")
         ("memo", "add Alice")
      );

      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "110.0000 CUR" ));
   }
} FC_LOG_AND_RETHROW() /// test_transfer


BOOST_FIXTURE_TEST_CASE( test_overspend, currency_tester ) try {
   create_accounts( {N(alice), N(bob)} );

   // make a transfer from the contract to a user
   {
      auto trace = push_action(N(currency), N(transfer), mutable_variant_object()
         ("from", "currency")
         ("to",   "alice")
         ("quantity", "100.0000 CUR")
         ("memo", "fund Alice")
      );

      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ));
   }

   // Overspend!
   {
      variant_object data = mutable_variant_object()
         ("from", "alice")
         ("to",   "bob")
         ("quantity", "101.0000 CUR")
         ("memo", "overspend! Alice");

      BOOST_CHECK_EXCEPTION(push_action(N(alice), N(transfer), data), fc::assert_exception, assert_message_is("integer underflow subtracting token balance"));
      produce_block();

      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ));
      BOOST_REQUIRE_EQUAL(get_balance(N(bob)), asset::from_string( "0.0000 CUR" ));
   }
} FC_LOG_AND_RETHROW() /// test_overspend

BOOST_FIXTURE_TEST_CASE( test_fullspend, currency_tester ) try {
   create_accounts( {N(alice), N(bob)} );

   // make a transfer from the contract to a user
   {
      auto trace = push_action(N(currency), N(transfer), mutable_variant_object()
         ("from", "currency")
         ("to",   "alice")
         ("quantity", "100.0000 CUR")
         ("memo", "fund Alice")
      );

      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ));
   }

   // Full spend
   {
      variant_object data = mutable_variant_object()
         ("from", "alice")
         ("to",   "bob")
         ("quantity", "100.0000 CUR")
         ("memo", "all in! Alice");

      auto trace = push_action(push_action(N(alice), N(transfer), data);
      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace.id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "0.0000 CUR" ));
      BOOST_REQUIRE_EQUAL(get_balance(N(bob)), asset::from_string( "100.0000 CUR" ));
   }

} FC_LOG_AND_RETHROW() /// test_fullspend



BOOST_FIXTURE_TEST_CASE(test_symbol, tester) try {

   {
      symbol dollar(2, "DLLR");
      BOOST_REQUIRE_EQUAL(SY(2, DLLR), dollar.value());
      BOOST_REQUIRE_EQUAL(2, dollar.decimals());
      BOOST_REQUIRE_EQUAL(100, dollar.precision());
      BOOST_REQUIRE_EQUAL("DLLR", dollar.name());
      BOOST_REQUIRE_EQUAL(true, dollar.valid());
   }

   {
      symbol eos(4, "EOS");
      BOOST_REQUIRE_EQUAL(EOS_SYMBOL_VALUE, eos.value());
      BOOST_REQUIRE_EQUAL("4,EOS", eos.to_string());
      BOOST_REQUIRE_EQUAL("EOS", eos.name());
      BOOST_REQUIRE_EQUAL(4, eos.decimals());
   }

   // default is "4,EOS"
   {
      symbol def;
      BOOST_REQUIRE_EQUAL(4, def.decimals());
      BOOST_REQUIRE_EQUAL("EOS", def.name());
   }
   // from string
   {
      symbol y = symbol::from_string("3,YEN");
      BOOST_REQUIRE_EQUAL(3, y.decimals());
      BOOST_REQUIRE_EQUAL("YEN", y.name());
   }

   // from empty string
   {
      BOOST_CHECK_EXCEPTION(symbol::from_string(""),
                            fc::assert_exception, assert_message_is("creating symbol from empty string"));
   }

   
   // precision part missing
   {
      BOOST_CHECK_EXCEPTION(symbol::from_string("RND"),
                            fc::assert_exception, assert_message_is("missing comma in symbol"));
   }


   // precision part missing
   {
      BOOST_CHECK_EXCEPTION(symbol::from_string("0,EURO"),
                            fc::assert_exception, assert_message_is("zero decimals in symbol"));
   }

   // invalid - contains lower case characters, no validation
   {
      symbol malformed(SY(6,EoS));
      BOOST_REQUIRE_EQUAL(false, malformed.valid());
      BOOST_REQUIRE_EQUAL("EoS", malformed.name());
      BOOST_REQUIRE_EQUAL(6, malformed.decimals());
   }

   // invalid - contains lower case characters, exception thrown 
   {
      BOOST_CHECK_EXCEPTION(symbol(5,"EoS"),
                            fc::assert_exception, assert_message_is("invalid character in symbol name"));
   }

   // invalid - missing decimal point
   {
      BOOST_CHECK_EXCEPTION(asset::from_string("10 CUR"),
                            fc::assert_exception, assert_message_is("dot missing in asset from string"));
   }

} FC_LOG_AND_RETHROW() /// test_symbol

BOOST_AUTO_TEST_SUITE_END()
