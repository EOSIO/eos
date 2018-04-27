#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::chain_apis;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

class eosio_token_tester : public tester {
public:

   eosio_token_tester() {
      produce_blocks( 2 );

      create_accounts( { N(alice), N(bob), N(carol), N(eosio.token) } );
      produce_blocks( 2 );

      set_code( N(eosio.token), eosio_token_wast );
      set_abi( N(eosio.token), eosio_token_abi );

      produce_blocks();

      const auto& accnt = control->get_database().get<account_object,by_name>( N(eosio.token) );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi);
   }

   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data ) {
      string action_type_name = abi_ser.get_action_type(name);

      action act;
      act.account = N(eosio.token);
      act.name    = name;
      act.data    = abi_ser.variant_to_binary( action_type_name, data );

      return base_tester::push_action( std::move(act), uint64_t(signer));
   }
   
   fc::variant get_stats( const string& symbolname )
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), symbol_code, N(stat), symbol_code );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "currency_stats", data );
   }

   fc::variant get_account( account_name acc, const string& symbolname)
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), acc, N(accounts), symbol_code );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "account", data );
   }

   action_result create( account_name issuer,
                asset        maximum_supply,
                uint8_t      issuer_can_freeze,
                uint8_t      issuer_can_recall,  
                uint8_t      issuer_can_whitelist ) {

      return push_action( N(eosio.token), N(create), mvo()
           ( "issuer", issuer)
           ( "maximum_supply", maximum_supply)
           ( "can_freeze", issuer_can_freeze)
           ( "can_recall", issuer_can_recall)
           ( "can_whitelist", issuer_can_whitelist)
      );
   }

   action_result issue( account_name issuer, account_name to, asset quantity, string memo ) {
      return push_action( issuer, N(issue), mvo()
           ( "to", to)
           ( "quantity", quantity)
           ( "memo", memo)
      );
   }

   action_result transfer( account_name from, 
                  account_name to,
                  asset        quantity,
                  string       memo ) {
      return push_action( from, N(transfer), mvo()
           ( "from", from)
           ( "to", to)
           ( "quantity", quantity)
           ( "memo", memo)
      );
   }

   abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(eosio_token_tests)

BOOST_FIXTURE_TEST_CASE( create_tests, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1000.000 TKN"), false, false, false);
   auto stats = get_stats("3,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0.000 TKN")
      ("max_supply", "1000.000 TKN")
      ("issuer", "alice")
      ("can_freeze",0)
      ("can_recall",0)
      ("can_whitelist",0)
      ("is_frozen",false)
      ("enforce_whitelist",false)
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( create_negative_max_supply, eosio_token_tester ) try {

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: max-supply must be positive" ), 
      create( N(alice), asset::from_string("-1000.000 TKN"), false, false, false)
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( create_max_supply, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("4611686018427387903 TKN"), true, false, false);
   auto stats = get_stats("0,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0 TKN")
      ("max_supply", "4611686018427387903 TKN")
      ("issuer", "alice")
      ("can_freeze",1)
      ("can_recall",0)
      ("can_whitelist",0)
      ("is_frozen",false)
      ("enforce_whitelist",false)
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: invalid supply" ), 
      create( N(alice), asset::from_string("4611686018427387904 TKN"), true, false, false)
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( create_max_decimals, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1.000000000000000000 TKN"), true, false, false);
   auto stats = get_stats("18,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0.000000000000000000 TKN")
      ("max_supply", "1.000000000000000000 TKN")
      ("issuer", "alice")
      ("can_freeze",1)
      ("can_recall",0)
      ("can_whitelist",0)
      ("is_frozen",false)
      ("enforce_whitelist",false)
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: invalid supply" ), 
      create( N(alice), asset::from_string("4.611686018427387904 TKN"), true, false, false)
   );

   //1.0000000000000000000 => 0x8ac7230489e80000L
   //TODO: Better error message
   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: max-supply must be positive" ), 
      create( N(alice), asset::from_string("1.0000000000000000000 TKN"), true, false, false)
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( issue_tests, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1000.000 TKN"), false, false, false);
   produce_blocks(1);

   issue( N(alice), N(alice), asset::from_string("500.000 TKN"), "hola" );

   auto stats = get_stats("3,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "500.000 TKN")
      ("max_supply", "1000.000 TKN")
      ("issuer", "alice")
      ("can_freeze",0)
      ("can_recall",0)
      ("can_whitelist",0)
      ("is_frozen",false)
      ("enforce_whitelist",false)
   );

   auto alice_balance = get_account(N(alice), "3,TKN");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "500.000 TKN")
      ("frozen", 0)
      ("whitelist", 1)
   );

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: quantity exceeds available supply" ), 
      issue( N(alice), N(alice), asset::from_string("500.001 TKN"), "hola" )
   );

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: invalid quantity" ), 
      issue( N(alice), N(alice), asset::from_string("4611686018427387.904 TKN"), "hola" )
   );

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: must issue positive quantity" ), 
      issue( N(alice), N(alice), asset::from_string("-1.000 TKN"), "hola" )
   );

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: comparison of assets with different symbols is not allowed" ),
      issue( N(alice), N(alice), asset::from_string("1.00 TKN"), "hola" )
   );

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: comparison of assets with different symbols is not allowed" ),
      issue( N(alice), N(alice), asset::from_string("1.0000 TKN"), "hola" )
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( transfer_tests, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1000 CERO"), false, false, false);
   produce_blocks(1);

   issue( N(alice), N(alice), asset::from_string("1000 CERO"), "hola" );

   auto stats = get_stats("0,CERO");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "1000 CERO")
      ("max_supply", "1000 CERO")
      ("issuer", "alice")
      ("can_freeze",0)
      ("can_recall",0)
      ("can_whitelist",0)
      ("is_frozen",false)
      ("enforce_whitelist",false)
   );

   auto alice_balance = get_account(N(alice), "0,CERO");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "1000 CERO")
      ("frozen", 0)
      ("whitelist", 1)
   );

   transfer( N(alice), N(bob), asset::from_string("300 CERO"), "hola" );
   
   alice_balance = get_account(N(alice), "0,CERO");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "700 CERO")
      ("frozen", 0)
      ("whitelist", 1)
   );

   auto bob_balance = get_account(N(bob), "0,CERO");
   REQUIRE_MATCHING_OBJECT( bob_balance, mvo()
      ("balance", "300 CERO")
      ("frozen", 0)
      ("whitelist", 1)
   );

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: overdrawn balance" ), 
      transfer( N(alice), N(bob), asset::from_string("701 CERO"), "hola" )
   );

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: invalid quantity" ), 
      transfer( N(alice), N(alice), asset::from_string("4611686018427387904 CERO"), "hola" )
   );

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: must transfer positive quantity" ), 
      transfer( N(alice), N(alice), asset::from_string("-1000 CERO"), "hola" )
   );

   BOOST_REQUIRE_EQUAL( error( "condition: assertion failed: attempt to subtract asset with different symbol" ),
      transfer( N(alice), N(alice), asset::from_string("1.0 CERO"), "hola" )
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
