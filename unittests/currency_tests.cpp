#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/generated_transaction_object.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <contracts.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

class currency_tester : public TESTER {
   public:

      auto push_action(const account_name& signer, const action_name &name, const variant_object &data ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = N(eosio.token);
         act.name = name;
         act.authorization = vector<permission_level>{{signer, config::active_name}};
         act.data = abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

         signed_transaction trx;
         trx.actions.emplace_back(std::move(act));

         set_transaction_headers(trx);
         trx.sign(get_private_key(signer, "active"), control->get_chain_id());
         return push_transaction(trx);
      }

      asset get_balance(const account_name& account) const {
         return get_currency_balance(N(eosio.token), symbol(SY(4,CUR)), account);
      }

      auto transfer(const account_name& from, const account_name& to, const std::string& quantity, const std::string& memo = "") {
         auto trace = push_action(from, N(transfer), mutable_variant_object()
                                  ("from",     from)
                                  ("to",       to)
                                  ("quantity", quantity)
                                  ("memo",     memo)
                                  );
         produce_block();
         return trace;
      }

      auto issue(const account_name& to, const std::string& quantity, const std::string& memo = "") {
         auto trace = push_action(N(eosio.token), N(issue), mutable_variant_object()
                                  ("to",       to)
                                  ("quantity", quantity)
                                  ("memo",     memo)
                                  );
         produce_block();
         return trace;
      }

      currency_tester()
         :TESTER(),abi_ser(json::from_string(contracts::eosio_token_abi().data()).as<abi_def>(), abi_serializer_max_time)
      {
         create_account( N(eosio.token));
         set_code( N(eosio.token), contracts::eosio_token_wasm() );

         auto result = push_action(N(eosio.token), N(create), mutable_variant_object()
                 ("issuer",       eosio_token)
                 ("maximum_supply", "1000000000.0000 CUR")
                 ("can_freeze", 0)
                 ("can_recall", 0)
                 ("can_whitelist", 0)
         );
         wdump((result));

         result = push_action(N(eosio.token), N(issue), mutable_variant_object()
                 ("to",       eosio_token)
                 ("quantity", "1000000.0000 CUR")
                 ("memo", "gggggggggggg")
         );
         wdump((result));
         produce_block();
      }

      abi_serializer abi_ser;
      static const name eosio_token;
};

const name currency_tester::eosio_token = N(eosio.token);

BOOST_AUTO_TEST_SUITE(currency_tests)

BOOST_AUTO_TEST_CASE( bootstrap ) try {
   auto expected = asset::from_string( "1000000.0000 CUR" );
   currency_tester t;
   auto actual = t.get_currency_balance(N(eosio.token), expected.get_symbol(), N(eosio.token));
   BOOST_REQUIRE_EQUAL(expected, actual);
} FC_LOG_AND_RETHROW() /// test_api_bootstrap

BOOST_FIXTURE_TEST_CASE( test_transfer, currency_tester ) try {
   create_accounts( {N(alice)} );

   // make a transfer from the contract to a user
   {
      auto trace = push_action(N(eosio.token), N(transfer), mutable_variant_object()
         ("from", eosio_token)
         ("to",   "alice")
         ("quantity", "100.0000 CUR")
         ("memo", "fund Alice")
      );

      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace->id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ) );
   }
} FC_LOG_AND_RETHROW() /// test_transfer

BOOST_FIXTURE_TEST_CASE( test_duplicate_transfer, currency_tester ) {
   create_accounts( {N(alice)} );

   auto trace = push_action(N(eosio.token), N(transfer), mutable_variant_object()
      ("from", eosio_token)
      ("to",   "alice")
      ("quantity", "100.0000 CUR")
      ("memo", "fund Alice")
   );

   BOOST_REQUIRE_THROW(push_action(N(eosio.token), N(transfer), mutable_variant_object()
                                    ("from", eosio_token)
                                    ("to",   "alice")
                                    ("quantity", "100.0000 CUR")
                                    ("memo", "fund Alice")),
                       tx_duplicate);

   produce_block();

   BOOST_CHECK_EQUAL(true, chain_has_transaction(trace->id));
   BOOST_CHECK_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ) );
}

BOOST_FIXTURE_TEST_CASE( test_addtransfer, currency_tester ) try {
   create_accounts( {N(alice)} );

   // make a transfer from the contract to a user
   {
      auto trace = push_action(N(eosio.token), N(transfer), mutable_variant_object()
         ("from", eosio_token)
         ("to",   "alice")
         ("quantity", "100.0000 CUR")
         ("memo", "fund Alice")
      );

      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace->id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ));
   }

   // make a transfer from the contract to a user
   {
      auto trace = push_action(N(eosio.token), N(transfer), mutable_variant_object()
         ("from", eosio_token)
         ("to",   "alice")
         ("quantity", "10.0000 CUR")
         ("memo", "add Alice")
      );

      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace->id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "110.0000 CUR" ));
   }
} FC_LOG_AND_RETHROW() /// test_transfer


BOOST_FIXTURE_TEST_CASE( test_overspend, currency_tester ) try {
   create_accounts( {N(alice), N(bob)} );

   // make a transfer from the contract to a user
   {
      auto trace = push_action(N(eosio.token), N(transfer), mutable_variant_object()
         ("from", eosio_token)
         ("to",   "alice")
         ("quantity", "100.0000 CUR")
         ("memo", "fund Alice")
      );

      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace->id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ));
   }

   // Overspend!
   {
      variant_object data = mutable_variant_object()
         ("from", "alice")
         ("to",   "bob")
         ("quantity", "101.0000 CUR")
         ("memo", "overspend! Alice");

      BOOST_CHECK_EXCEPTION( push_action(N(alice), N(transfer), data),
                             eosio_assert_message_exception, eosio_assert_message_is("overdrawn balance") );
      produce_block();

      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "100.0000 CUR" ));
      BOOST_REQUIRE_EQUAL(get_balance(N(bob)), asset::from_string( "0.0000 CUR" ));
   }
} FC_LOG_AND_RETHROW() /// test_overspend

BOOST_FIXTURE_TEST_CASE( test_fullspend, currency_tester ) try {
   create_accounts( {N(alice), N(bob)} );

   // make a transfer from the contract to a user
   {
      auto trace = push_action(N(eosio.token), N(transfer), mutable_variant_object()
         ("from", eosio_token)
         ("to",   "alice")
         ("quantity", "100.0000 CUR")
         ("memo", "fund Alice")
      );

      produce_block();

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace->id));
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

      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trace->id));
      BOOST_REQUIRE_EQUAL(get_balance(N(alice)), asset::from_string( "0.0000 CUR" ));
      BOOST_REQUIRE_EQUAL(get_balance(N(bob)), asset::from_string( "100.0000 CUR" ));
   }

} FC_LOG_AND_RETHROW() /// test_fullspend



BOOST_FIXTURE_TEST_CASE(test_symbol, TESTER) try {

   {
      symbol dollar(2, "DLLR");
      BOOST_REQUIRE_EQUAL(SY(2, DLLR), dollar.value());
      BOOST_REQUIRE_EQUAL(2, dollar.decimals());
      BOOST_REQUIRE_EQUAL(100, dollar.precision());
      BOOST_REQUIRE_EQUAL("DLLR", dollar.name());
      BOOST_REQUIRE_EQUAL(true, dollar.valid());
   }

   {
      symbol sys(4, "SYS");
      BOOST_REQUIRE_EQUAL(SY(4,SYS), sys.value());
      BOOST_REQUIRE_EQUAL("4,SYS", sys.to_string());
      BOOST_REQUIRE_EQUAL("SYS", sys.name());
      BOOST_REQUIRE_EQUAL(4, sys.decimals());
   }

   // default is "4,${CORE_SYMBOL_NAME}"
   {
      symbol def;
      BOOST_REQUIRE_EQUAL(4, def.decimals());
      BOOST_REQUIRE_EQUAL(CORE_SYMBOL_NAME, def.name());
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
                            symbol_type_exception, fc_exception_message_is("creating symbol from empty string"));
   }

   // precision part missing
   {
      BOOST_CHECK_EXCEPTION(symbol::from_string("RND"),
                            symbol_type_exception, fc_exception_message_is("missing comma in symbol"));
   }

   // 0 decimals part
   {
      symbol sym = symbol::from_string("0,EURO");
      BOOST_REQUIRE_EQUAL(0, sym.decimals());
      BOOST_REQUIRE_EQUAL("EURO", sym.name());
   }

   // invalid - contains lower case characters, no validation
   {
      BOOST_CHECK_EXCEPTION(symbol malformed(SY(6,EoS)),
                            symbol_type_exception, fc_exception_message_is("invalid symbol: EoS"));
   }

   // invalid - contains lower case characters, exception thrown
   {
      BOOST_CHECK_EXCEPTION(symbol(5,"EoS"),
                            symbol_type_exception, fc_exception_message_is("invalid character in symbol name"));
   }

   // Missing decimal point, should create asset with 0 decimals
   {
      asset a = asset::from_string("10 CUR");
      BOOST_REQUIRE_EQUAL(a.get_amount(), 10);
      BOOST_REQUIRE_EQUAL(a.precision(), 1);
      BOOST_REQUIRE_EQUAL(a.decimals(), 0);
      BOOST_REQUIRE_EQUAL(a.symbol_name(), "CUR");
   }

   // Missing space
   {
      BOOST_CHECK_EXCEPTION(asset::from_string("10CUR"),
                            asset_type_exception, fc_exception_message_is("Asset's amount and symbol should be separated with space"));
   }

   // Precision is not specified when decimal separator is introduced
   {
      BOOST_CHECK_EXCEPTION(asset::from_string("10. CUR"),
                            asset_type_exception, fc_exception_message_is("Missing decimal fraction after decimal point"));
   }

   // Missing symbol
   {
      BOOST_CHECK_EXCEPTION(asset::from_string("10"),
                            asset_type_exception, fc_exception_message_is("Asset's amount and symbol should be separated with space"));
   }

   // Multiple spaces
   {
      asset a = asset::from_string("1000000000.00000  CUR");
      BOOST_REQUIRE_EQUAL(a.get_amount(), 100000000000000);
      BOOST_REQUIRE_EQUAL(a.decimals(), 5);
      BOOST_REQUIRE_EQUAL(a.symbol_name(), "CUR");
      BOOST_REQUIRE_EQUAL(a.to_string(), "1000000000.00000 CUR");
   }

   // Valid asset
   {
      asset a = asset::from_string("1000000000.00000 CUR");
      BOOST_REQUIRE_EQUAL(a.get_amount(), 100000000000000);
      BOOST_REQUIRE_EQUAL(a.decimals(), 5);
      BOOST_REQUIRE_EQUAL(a.symbol_name(), "CUR");
      BOOST_REQUIRE_EQUAL(a.to_string(), "1000000000.00000 CUR");
   }

   // Negative asset
   {
      asset a = asset::from_string("-001000000.00010 CUR");
      BOOST_REQUIRE_EQUAL(a.get_amount(), -100000000010);
      BOOST_REQUIRE_EQUAL(a.decimals(), 5);
      BOOST_REQUIRE_EQUAL(a.symbol_name(), "CUR");
      BOOST_REQUIRE_EQUAL(a.to_string(), "-1000000.00010 CUR");
   }

   // Negative asset below 1
   {
      asset a = asset::from_string("-000000000.00100 CUR");
      BOOST_REQUIRE_EQUAL(a.get_amount(), -100);
      BOOST_REQUIRE_EQUAL(a.decimals(), 5);
      BOOST_REQUIRE_EQUAL(a.symbol_name(), "CUR");
      BOOST_REQUIRE_EQUAL(a.to_string(), "-0.00100 CUR");
   }

   // Negative asset below 1
   {
      asset a = asset::from_string("-0.0001 PPP");
      BOOST_REQUIRE_EQUAL(a.get_amount(), -1);
      BOOST_REQUIRE_EQUAL(a.decimals(), 4);
      BOOST_REQUIRE_EQUAL(a.symbol_name(), "PPP");
      BOOST_REQUIRE_EQUAL(a.to_string(), "-0.0001 PPP");
   }

} FC_LOG_AND_RETHROW() /// test_symbol

BOOST_FIXTURE_TEST_CASE( test_proxy, currency_tester ) try {
   produce_blocks(2);

   create_accounts( {N(alice), N(proxy)} );
   produce_block();

   set_code(N(proxy), contracts::proxy_wasm());
   produce_blocks(1);

   abi_serializer proxy_abi_ser(json::from_string(contracts::proxy_abi().data()).as<abi_def>(), abi_serializer_max_time);

   // set up proxy owner
   {
      signed_transaction trx;
      action setowner_act;
      setowner_act.account = N(proxy);
      setowner_act.name = N(setowner);
      setowner_act.authorization = vector<permission_level>{{N(proxy), config::active_name}};
      setowner_act.data = proxy_abi_ser.variant_to_binary("setowner", mutable_variant_object()
         ("owner", "alice")
         ("delay", 10),
         abi_serializer_max_time
      );
      trx.actions.emplace_back(std::move(setowner_act));

      set_transaction_headers(trx);
      trx.sign(get_private_key(N(proxy), "active"), control->get_chain_id());
      push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

   // for now wasm "time" is in seconds, so we have to truncate off any parts of a second that may have applied
   fc::time_point expected_delivery(fc::seconds(control->head_block_time().sec_since_epoch()) + fc::seconds(10));
   {
      auto trace = push_action(N(eosio.token), N(transfer), mutable_variant_object()
         ("from", eosio_token)
         ("to",   "proxy")
         ("quantity", "5.0000 CUR")
         ("memo", "fund Proxy")
      );
   }

   while(control->head_block_time() < expected_delivery) {
      produce_block();
      BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("5.0000 CUR"));
      BOOST_REQUIRE_EQUAL(get_balance( N(alice)),   asset::from_string("0.0000 CUR"));
   }

   produce_block();
   BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("0.0000 CUR"));
   BOOST_REQUIRE_EQUAL(get_balance( N(alice)),   asset::from_string("5.0000 CUR"));

} FC_LOG_AND_RETHROW() /// test_currency

BOOST_FIXTURE_TEST_CASE( test_deferred_failure, currency_tester ) try {
   produce_blocks(2);

   create_accounts( {N(alice), N(bob), N(proxy)} );
   produce_block();

   set_code(N(proxy), contracts::proxy_wasm());
   set_code(N(bob), contracts::proxy_wasm());
   produce_blocks(1);

   abi_serializer proxy_abi_ser(json::from_string(contracts::proxy_abi().data()).as<abi_def>(), abi_serializer_max_time);

   // set up proxy owner
   {
      signed_transaction trx;
      action setowner_act;
      setowner_act.account = N(proxy);
      setowner_act.name = N(setowner);
      setowner_act.authorization = vector<permission_level>{{N(proxy), config::active_name}};
      setowner_act.data = proxy_abi_ser.variant_to_binary("setowner", mutable_variant_object()
         ("owner", "bob")
         ("delay", 10),
         abi_serializer_max_time
      );
      trx.actions.emplace_back(std::move(setowner_act));

      set_transaction_headers(trx);
      trx.sign(get_private_key(N(proxy), "active"), control->get_chain_id());
      push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }
   const auto& index = control->db().get_index<generated_transaction_multi_index,by_trx_id>();
   BOOST_REQUIRE_EQUAL(0, index.size());

   auto trace = push_action(N(eosio.token), N(transfer), mutable_variant_object()
      ("from", eosio_token)
      ("to",   "proxy")
      ("quantity", "5.0000 CUR")
      ("memo", "fund Proxy")
   );
   fc::time_point expected_delivery = control->pending_block_time() + fc::seconds(10);

   BOOST_REQUIRE_EQUAL(1, index.size());
   auto deferred_id = index.begin()->trx_id;
   BOOST_REQUIRE_EQUAL(false, chain_has_transaction(deferred_id));

   while( control->pending_block_time() < expected_delivery ) {
      produce_block();
      BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("5.0000 CUR"));
      BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 CUR"));
      BOOST_REQUIRE_EQUAL(1, index.size());
      BOOST_REQUIRE_EQUAL(false, chain_has_transaction(deferred_id));
   }

   fc::time_point expected_redelivery = control->pending_block_time() + fc::seconds(10);
   // First deferred transaction should be retired in this block.
   // It will fail, and its onerror handler will reschedule the transaction for 10 seconds later.
   produce_block();
   BOOST_REQUIRE_EQUAL(1, index.size()); // Still one because the first deferred transaction retires but the second is created at the same time.
   BOOST_REQUIRE_EQUAL(get_transaction_receipt(deferred_id).status, transaction_receipt::soft_fail);
   auto deferred2_id = index.begin()->trx_id;

   // set up alice owner
   {
      signed_transaction trx;
      action setowner_act;
      setowner_act.account = N(bob);
      setowner_act.name = N(setowner);
      setowner_act.authorization = vector<permission_level>{{N(bob), config::active_name}};
      setowner_act.data = proxy_abi_ser.variant_to_binary("setowner", mutable_variant_object()
         ("owner", "alice")
         ("delay", 0),
         abi_serializer_max_time
      );
      trx.actions.emplace_back(std::move(setowner_act));

      set_transaction_headers(trx);
      trx.sign(get_private_key(N(bob), "active"), control->get_chain_id());
      push_transaction(trx);
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
   }

   while( control->pending_block_time() < expected_redelivery ) {
      produce_block();
      BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("5.0000 CUR"));
      BOOST_REQUIRE_EQUAL(get_balance( N(alice)),   asset::from_string("0.0000 CUR"));
      BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 CUR"));
      BOOST_REQUIRE_EQUAL(1, index.size());
      BOOST_REQUIRE_EQUAL(false, chain_has_transaction(deferred2_id));
   }

   BOOST_REQUIRE_EQUAL(1, index.size());

   // Second deferred transaction should be retired in this block and should succeed,
   // which should move tokens from the proxy contract to the bob contract, thereby trigger the bob contract to
   // schedule a third deferred transaction with no delay.
   // That third deferred transaction (which moves tokens from the bob contract to account alice) should be executed immediately
   // after in the same block (note that this is the current deferred transaction scheduling policy in tester and it may change).
   produce_block();
   BOOST_REQUIRE_EQUAL(0, index.size());
   BOOST_REQUIRE_EQUAL(get_transaction_receipt(deferred2_id).status, transaction_receipt::executed);

   BOOST_REQUIRE_EQUAL(get_balance( N(proxy)), asset::from_string("0.0000 CUR"));
   BOOST_REQUIRE_EQUAL(get_balance( N(alice)), asset::from_string("5.0000 CUR"));
   BOOST_REQUIRE_EQUAL(get_balance( N(bob)),   asset::from_string("0.0000 CUR"));

} FC_LOG_AND_RETHROW() /// test_currency

BOOST_FIXTURE_TEST_CASE( test_input_quantity, currency_tester ) try {

   produce_blocks(2);

   create_accounts( {N(alice), N(bob), N(carl)} );

   // transfer to alice using right precision
   {
      auto trace = transfer(eosio_token, N(alice), "100.0000 CUR");

      BOOST_CHECK_EQUAL(true, chain_has_transaction(trace->id));
      BOOST_CHECK_EQUAL(asset::from_string( "100.0000 CUR"), get_balance(N(alice)));
      BOOST_CHECK_EQUAL(1000000, get_balance(N(alice)).get_amount());
   }

   // transfer using different symbol name fails
   {
      BOOST_REQUIRE_THROW(transfer(N(alice), N(carl), "20.50 USD"), eosio_assert_message_exception);
   }

   // issue to alice using right precision
   {
      auto trace = issue(N(alice), "25.0256 CUR");

      BOOST_CHECK_EQUAL(true, chain_has_transaction(trace->id));
      BOOST_CHECK_EQUAL(asset::from_string("125.0256 CUR"), get_balance(N(alice)));
   }


} FC_LOG_AND_RETHROW() /// test_currency

BOOST_AUTO_TEST_SUITE_END()
