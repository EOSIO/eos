#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include "eosio.system_tester.hpp"

#include "Runtime/Runtime.h"

#include <fc/variant_object.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
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

      set_code( N(eosio.token), contracts::token_wasm() );
      set_abi( N(eosio.token), contracts::token_abi().data() );

      produce_blocks();

      const auto& accnt = control->db().get<account_object,by_name>( N(eosio.token) );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi, abi_serializer_max_time);
   }

   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data ) {
      string action_type_name = abi_ser.get_action_type(name);

      action act;
      act.account = N(eosio.token);
      act.name    = name;
      act.data    = abi_ser.variant_to_binary( action_type_name, data,abi_serializer_max_time );

      return base_tester::push_action( std::move(act), uint64_t(signer));
   }

   fc::variant get_stats( const string& symbolname )
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), symbol_code, N(stat), symbol_code );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "currency_stats", data, abi_serializer_max_time );
   }

   fc::variant get_account( account_name acc, const string& symbolname)
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), acc, N(accounts), symbol_code );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "account", data, abi_serializer_max_time );
   }

   action_result create( account_name issuer,
                         asset        maximum_supply ) {

      return push_action( N(eosio.token), N(create), mvo()
           ( "issuer", issuer)
           ( "maximum_supply", maximum_supply)
      );
   }

   action_result issue( account_name issuer, asset quantity, string memo ) {
      return push_action( issuer, N(issue), mvo()
           ( "to", issuer)
           ( "quantity", quantity)
           ( "memo", memo)
      );
   }

   action_result retire( account_name issuer, asset quantity, string memo ) {
      return push_action( issuer, N(retire), mvo()
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

   action_result open( account_name owner,
                       const string& symbolname,
                       account_name ram_payer    ) {
      return push_action( ram_payer, N(open), mvo()
           ( "owner", owner )
           ( "symbol", symbolname )
           ( "ram_payer", ram_payer )
      );
   }

   action_result close( account_name owner,
                        const string& symbolname ) {
      return push_action( owner, N(close), mvo()
           ( "owner", owner )
           ( "symbol", "0,CERO" )
      );
   }

   abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(eosio_token_tests)

BOOST_FIXTURE_TEST_CASE( create_tests, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1000.000 TKN"));
   auto stats = get_stats("3,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0.000 TKN")
      ("max_supply", "1000.000 TKN")
      ("issuer", "alice")
   );
   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( create_negative_max_supply, eosio_token_tester ) try {

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "max-supply must be positive" ),
      create( N(alice), asset::from_string("-1000.000 TKN"))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( symbol_already_exists, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("100 TKN"));
   auto stats = get_stats("0,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0 TKN")
      ("max_supply", "100 TKN")
      ("issuer", "alice")
   );
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "token with symbol already exists" ),
                        create( N(alice), asset::from_string("100 TKN"))
   );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( create_max_supply, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("4611686018427387903 TKN"));
   auto stats = get_stats("0,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0 TKN")
      ("max_supply", "4611686018427387903 TKN")
      ("issuer", "alice")
   );
   produce_blocks(1);

   asset max(10, symbol(SY(0, NKT)));
   share_type amount = 4611686018427387904;
   static_assert(sizeof(share_type) <= sizeof(asset), "asset changed so test is no longer valid");
   static_assert(std::is_trivially_copyable<asset>::value, "asset is not trivially copyable");
   memcpy(&max, &amount, sizeof(share_type)); // hack in an invalid amount

   BOOST_CHECK_EXCEPTION( create( N(alice), max) , asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });


} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( create_max_decimals, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1.000000000000000000 TKN"));
   auto stats = get_stats("18,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0.000000000000000000 TKN")
      ("max_supply", "1.000000000000000000 TKN")
      ("issuer", "alice")
   );
   produce_blocks(1);

   asset max(10, symbol(SY(0, NKT)));
   //1.0000000000000000000 => 0x8ac7230489e80000L
   share_type amount = 0x8ac7230489e80000L;
   static_assert(sizeof(share_type) <= sizeof(asset), "asset changed so test is no longer valid");
   static_assert(std::is_trivially_copyable<asset>::value, "asset is not trivially copyable");
   memcpy(&max, &amount, sizeof(share_type)); // hack in an invalid amount

   BOOST_CHECK_EXCEPTION( create( N(alice), max) , asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( issue_tests, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1000.000 TKN"));
   produce_blocks(1);

   issue( N(alice), asset::from_string("500.000 TKN"), "hola" );

   auto stats = get_stats("3,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "500.000 TKN")
      ("max_supply", "1000.000 TKN")
      ("issuer", "alice")
   );

   auto alice_balance = get_account(N(alice), "3,TKN");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "500.000 TKN")
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "quantity exceeds available supply" ),
                        issue( N(alice), asset::from_string("500.001 TKN"), "hola" )
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "must issue positive quantity" ),
                        issue( N(alice), asset::from_string("-1.000 TKN"), "hola" )
   );

   BOOST_REQUIRE_EQUAL( success(),
                        issue( N(alice), asset::from_string("1.000 TKN"), "hola" )
   );


} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( retire_tests, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1000.000 TKN"));
   produce_blocks(1);

   BOOST_REQUIRE_EQUAL( success(), issue( N(alice), asset::from_string("500.000 TKN"), "hola" ) );

   auto stats = get_stats("3,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "500.000 TKN")
      ("max_supply", "1000.000 TKN")
      ("issuer", "alice")
   );

   auto alice_balance = get_account(N(alice), "3,TKN");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "500.000 TKN")
   );

   BOOST_REQUIRE_EQUAL( success(), retire( N(alice), asset::from_string("200.000 TKN"), "hola" ) );
   stats = get_stats("3,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "300.000 TKN")
      ("max_supply", "1000.000 TKN")
      ("issuer", "alice")
   );
   alice_balance = get_account(N(alice), "3,TKN");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "300.000 TKN")
   );

   //should fail to retire more than current supply
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("overdrawn balance"), retire( N(alice), asset::from_string("500.000 TKN"), "hola" ) );

   BOOST_REQUIRE_EQUAL( success(), transfer( N(alice), N(bob), asset::from_string("200.000 TKN"), "hola" ) );
   //should fail to retire since tokens are not on the issuer's balance
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("overdrawn balance"), retire( N(alice), asset::from_string("300.000 TKN"), "hola" ) );
   //transfer tokens back
   BOOST_REQUIRE_EQUAL( success(), transfer( N(bob), N(alice), asset::from_string("200.000 TKN"), "hola" ) );

   BOOST_REQUIRE_EQUAL( success(), retire( N(alice), asset::from_string("300.000 TKN"), "hola" ) );
   stats = get_stats("3,TKN");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "0.000 TKN")
      ("max_supply", "1000.000 TKN")
      ("issuer", "alice")
   );
   alice_balance = get_account(N(alice), "3,TKN");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "0.000 TKN")
   );

   //trying to retire tokens with zero supply
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("overdrawn balance"), retire( N(alice), asset::from_string("1.000 TKN"), "hola" ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( transfer_tests, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1000 CERO"));
   produce_blocks(1);

   issue( N(alice), asset::from_string("1000 CERO"), "hola" );

   auto stats = get_stats("0,CERO");
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "1000 CERO")
      ("max_supply", "1000 CERO")
      ("issuer", "alice")
   );

   auto alice_balance = get_account(N(alice), "0,CERO");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "1000 CERO")
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

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "overdrawn balance" ),
      transfer( N(alice), N(bob), asset::from_string("701 CERO"), "hola" )
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "must transfer positive quantity" ),
      transfer( N(alice), N(bob), asset::from_string("-1000 CERO"), "hola" )
   );


} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( open_tests, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1000 CERO"));

   auto alice_balance = get_account(N(alice), "0,CERO");
   BOOST_REQUIRE_EQUAL(true, alice_balance.is_null() );
   BOOST_REQUIRE_EQUAL( wasm_assert_msg("tokens can only be issued to issuer account"),
                        push_action( N(alice), N(issue), mvo()
                                     ( "to",       "bob")
                                     ( "quantity", asset::from_string("1000 CERO") )
                                     ( "memo",     "") ) );
   BOOST_REQUIRE_EQUAL( success(), issue( N(alice), asset::from_string("1000 CERO"), "issue" ) );

   alice_balance = get_account(N(alice), "0,CERO");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "1000 CERO")
   );

   auto bob_balance = get_account(N(bob), "0,CERO");
   BOOST_REQUIRE_EQUAL(true, bob_balance.is_null() );

   BOOST_REQUIRE_EQUAL( success(), open( N(bob), "0,CERO", N(alice) ) );

   bob_balance = get_account(N(bob), "0,CERO");
   REQUIRE_MATCHING_OBJECT( bob_balance, mvo()
      ("balance", "0 CERO")
   );

   BOOST_REQUIRE_EQUAL( success(), transfer( N(alice), N(bob), asset::from_string("200 CERO"), "hola" ) );

   bob_balance = get_account(N(bob), "0,CERO");
   REQUIRE_MATCHING_OBJECT( bob_balance, mvo()
      ("balance", "200 CERO")
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "symbol does not exist" ),
                        open( N(carol), "0,INVALID", N(alice) ) );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "symbol precision mismatch" ),
                        open( N(carol), "1,CERO", N(alice) ) );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( close_tests, eosio_token_tester ) try {

   auto token = create( N(alice), asset::from_string("1000 CERO"));

   auto alice_balance = get_account(N(alice), "0,CERO");
   BOOST_REQUIRE_EQUAL(true, alice_balance.is_null() );

   BOOST_REQUIRE_EQUAL( success(), issue( N(alice), asset::from_string("1000 CERO"), "hola" ) );

   alice_balance = get_account(N(alice), "0,CERO");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "1000 CERO")
   );

   BOOST_REQUIRE_EQUAL( success(), transfer( N(alice), N(bob), asset::from_string("1000 CERO"), "hola" ) );

   alice_balance = get_account(N(alice), "0,CERO");
   REQUIRE_MATCHING_OBJECT( alice_balance, mvo()
      ("balance", "0 CERO")
   );

   BOOST_REQUIRE_EQUAL( success(), close( N(alice), "0,CERO" ) );
   alice_balance = get_account(N(alice), "0,CERO");
   BOOST_REQUIRE_EQUAL(true, alice_balance.is_null() );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
