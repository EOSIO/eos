#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <currency/currency.wast.hpp>
#include <currency/currency.abi.hpp>

#include <proxy/proxy.wast.hpp>
#include <proxy/proxy.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;
using namespace fc;

class currency_tester : public tester {
   public:

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

      asset get_balance(const account_name& account) const {
         return get_currency_balance(N(currency), symbol(SY(4,CUR)), account);
      }


      currency_tester()
      :tester(),abi_ser(json::from_string(currency_abi).as<abi_def>())
      {
         create_account( N(currency));
         set_code( N(currency), currency_wast );

         push_action(N(currency), N(issue), mutable_variant_object()
                 ("to",       "currency")
                 ("quantity", "1000000.0000 CUR")
         );
         produce_block();
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
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ) );
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

      auto trace = push_action(N(alice), N(transfer), data);
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

BOOST_FIXTURE_TEST_CASE( test_proxy, currency_tester ) try {
   produce_blocks(2);

   create_accounts( {N(alice), N(proxy)} );
   produce_block();

   set_code(N(proxy), proxy_wast);
   produce_blocks(1);

   abi_serializer proxy_abi_ser(json::from_string(proxy_abi).as<abi_def>());

   // set up proxy owner
   {
      signed_transaction trx;
      action setowner_act;
      setowner_act.account = N(proxy);
      setowner_act.name = N(setowner);
      setowner_act.authorization = vector<permission_level>{{N(proxy), config::active_name}};
      setowner_act.data = proxy_abi_ser.variant_to_binary("setowner", mutable_variant_object()
         ("owner", "alice")
         ("delay", 10)
      );
      trx.actions.emplace_back(std::move(setowner_act));

      set_tapos(trx);
      trx.sign(get_private_key(N(proxy), "active"), chain_id_type());
      push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

   // for now wasm "time" is in seconds, so we have to truncate off any parts of a second that may have applied
   fc::time_point expected_delivery(fc::seconds(control->head_block_time().sec_since_epoch()) + fc::seconds(10));
   {
      auto trace = push_action(N(currency), N(transfer), mutable_variant_object()
         ("from", "currency")
         ("to",   "proxy")
         ("quantity", "5.0000 CUR")
         ("memo", "fund Proxy")
      );
   }

   while(control->head_block_time() < expected_delivery) {
      control->push_deferred_transactions(true);
      produce_block();
      BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("5.0000 CUR"));
      BOOST_REQUIRE_EQUAL(get_balance( N(alice)),   asset::from_string("0.0000 CUR"));
   }

   control->push_deferred_transactions(true);
   BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("0.0000 CUR"));
   BOOST_REQUIRE_EQUAL(get_balance( N(alice)),   asset::from_string("5.0000 CUR"));

} FC_LOG_AND_RETHROW() /// test_currency

BOOST_FIXTURE_TEST_CASE( test_deferred_failure, currency_tester ) try {
   produce_blocks(2);

   create_accounts( {N(alice), N(bob), N(proxy)} );
   produce_block();

   set_code(N(proxy), proxy_wast);
   set_code(N(bob), proxy_wast);
   produce_blocks(1);

   abi_serializer proxy_abi_ser(json::from_string(proxy_abi).as<abi_def>());

   // set up proxy owner
   {
      signed_transaction trx;
      action setowner_act;
      setowner_act.account = N(proxy);
      setowner_act.name = N(setowner);
      setowner_act.authorization = vector<permission_level>{{N(proxy), config::active_name}};
      setowner_act.data = proxy_abi_ser.variant_to_binary("setowner", mutable_variant_object()
         ("owner", "bob")
         ("delay", 10)
      );
      trx.actions.emplace_back(std::move(setowner_act));

      set_tapos(trx);
      trx.sign(get_private_key(N(proxy), "active"), chain_id_type());
      push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

   // for now wasm "time" is in seconds, so we have to truncate off any parts of a second that may have applied
   fc::time_point expected_delivery(fc::seconds(control->head_block_time().sec_since_epoch()) + fc::seconds(10));
   auto trace = push_action(N(currency), N(transfer), mutable_variant_object()
      ("from", "currency")
      ("to",   "proxy")
      ("quantity", "5.0000 CUR")
      ("memo", "fund Proxy")
   );

   BOOST_REQUIRE_EQUAL(trace.deferred_transactions.size(), 1);
   auto deferred_id = trace.deferred_transactions.back().id();

   while(control->head_block_time() < expected_delivery) {
      control->push_deferred_transactions(true);
      produce_block();
      BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("5.0000 CUR"));
      BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 CUR"));
      BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 CUR"));
      BOOST_REQUIRE_EQUAL(chain_has_transaction(deferred_id), false);
   }

   fc::time_point expected_redelivery(fc::seconds(control->head_block_time().sec_since_epoch()) + fc::seconds(10));
   control->push_deferred_transactions(true);
   produce_block();
   BOOST_REQUIRE_EQUAL(chain_has_transaction(deferred_id), true);
   BOOST_REQUIRE_EQUAL(get_transaction_receipt(deferred_id).status, transaction_receipt::soft_fail);

   // set up alice owner
   {
      signed_transaction trx;
      action setowner_act;
      setowner_act.account = N(bob);
      setowner_act.name = N(setowner);
      setowner_act.authorization = vector<permission_level>{{N(bob), config::active_name}};
      setowner_act.data = proxy_abi_ser.variant_to_binary("setowner", mutable_variant_object()
         ("owner", "alice")
         ("delay", 0)
      );
      trx.actions.emplace_back(std::move(setowner_act));

      set_tapos(trx);
      trx.sign(get_private_key(N(bob), "active"), chain_id_type());
      push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

   while(control->head_block_time() < expected_redelivery) {
      control->push_deferred_transactions(true);
      produce_block();
      BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("5.0000 CUR"));
      BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 CUR"));
      BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 CUR"));
   }

   control->push_deferred_transactions(true);
   BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("0.0000 CUR"));
   BOOST_REQUIRE_EQUAL(get_balance( N(alice)), asset::from_string("0.0000 CUR"));
   BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("5.0000 CUR"));

   control->push_deferred_transactions(true);

   BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("0.0000 CUR"));
   BOOST_REQUIRE_EQUAL(get_balance( N(alice)), asset::from_string("5.0000 CUR"));
   BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 CUR"));

} FC_LOG_AND_RETHROW() /// test_currency


BOOST_AUTO_TEST_SUITE_END()
