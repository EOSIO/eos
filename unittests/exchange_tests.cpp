#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/symbol.hpp>

#include <eosio.token/eosio.token.wast.hpp>
#include <eosio.token/eosio.token.abi.hpp>

#include <exchange/exchange.wast.hpp>
#include <exchange/exchange.abi.hpp>

#include <proxy/proxy.wast.hpp>
#include <proxy/proxy.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

static const auto msec_per_year = 60 * 60 * 24 * 365 * 1000ull;

#define A(X) asset::from_string( #X )

#define CHECK_ASSERT(S, M)                                                                                  \
   try {                                                                                                    \
      S;                                                                                                    \
      BOOST_ERROR("exception eosio_assert_message_exception is expected");                                  \
   } catch(eosio_assert_message_exception& e) {                                                             \
      if(e.top_message() != "assertion failure with message: " M)                                           \
         BOOST_ERROR("expected \"assertion failure with message: " M "\" got \"" + e.top_message() + "\""); \
   }

struct margin_state {
   extended_asset total_lendable;
   extended_asset total_lent;
   extended_asset collected_interest;
   double         least_collateralized = 0;
   double         interest_shares = 0;
};
FC_REFLECT( margin_state, (total_lendable)(total_lent)(collected_interest)(least_collateralized)(interest_shares) )

struct exchange_state {
   account_name      manager;
   extended_asset    supply;
   double            fee = 0;
   double            interest_rate = 0;
   bool              need_continuation = false;

   struct market_continuation {
      bool              active = false;
      account_name      seller = 0;
      extended_asset    sell;
      extended_symbol   receive_symbol;
   };

   market_continuation active_market_continuation;

   struct connector {
      extended_asset balance;
      uint32_t       weight = 500;
      margin_state   peer_margin;
   };

   connector base;
   connector quote;
};

FC_REFLECT( exchange_state::market_continuation, (active)(seller)(sell)(receive_symbol) );
FC_REFLECT( exchange_state::connector, (balance)(weight)(peer_margin) );
FC_REFLECT( exchange_state, (manager)(supply)(fee)(interest_rate)(need_continuation)(active_market_continuation)(base)(quote) );

class exchange_tester : public TESTER {
   public:
       auto push_action(account_name contract,
                       const account_name& signer,
                       const action_name &name, const variant_object &data ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = contract;
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
         return get_currency_balance(N(exchange), symbol(SY(4,CUR)), account);
      }

      void check_balance(account_name contract, account_name account, asset expected)
      {
         auto actual = get_currency_balance(contract, expected.get_symbol(), account);
         BOOST_REQUIRE_EQUAL(expected, actual);
      }

      exchange_state get_market_state( account_name exchange, symbol sym ) {

         uint64_t s = sym.value() >> 8;
         const auto& db  = control->db();
         const auto* tbl = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(exchange, s, N(markets)));
         if (tbl) {
            const auto *obj = db.find<key_value_object, by_scope_primary>(boost::make_tuple(tbl->id, s));
            if( obj ) {
               fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
               exchange_state result;
               fc::raw::unpack( ds, result );
               return result;
            }
         }
         FC_ASSERT( false, "unknown market state" );
      }

      extended_asset get_exchange_balance( account_name exchange, account_name currency,
                                           symbol sym, account_name owner ) {
         const auto& db  = control->db();
         const auto* tbl = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(exchange, owner, N(exaccounts)));

         if (tbl) {
            const auto *obj = db.find<key_value_object, by_scope_primary>(boost::make_tuple(tbl->id, owner));
            if( obj ) {
               fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
               account_name own;
               flat_map<pair<symbol,account_name>, int64_t> balances;

               fc::raw::unpack( ds, own );
               fc::raw::unpack( ds, balances);

          //     wdump((balances));
               auto b = balances[ make_pair( sym, currency ) ];
               return extended_asset( asset( b, sym ), currency );
            }
         }
         return extended_asset();
      }

      void check_exchange_balance(account_name exchange, account_name currency, account_name owner, asset expected)
      {
         auto actual = get_exchange_balance( exchange, currency, expected.get_symbol(), owner );
         BOOST_REQUIRE_EQUAL( expected, actual.quantity );
      }

      double get_lent_shares( account_name exchange, symbol market, account_name owner, bool base )
      {
         const auto& db  = control->db();

         auto scope = ((market.value() >> 8) << 4) + (base ? 1 : 2);

         const auto* tbl = db.find<table_id_object, by_code_scope_table>(boost::make_tuple(exchange, scope, N(loans)));

         if (tbl) {
            const auto *obj = db.find<key_value_object, by_scope_primary>(boost::make_tuple(tbl->id, owner));
            if( obj ) {
               fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
               account_name own;
               double       interest_shares;

               fc::raw::unpack( ds, own );
               fc::raw::unpack( ds, interest_shares);

               FC_ASSERT( interest_shares > 0, "interest_shares not positive" );
               return interest_shares;
            }
         }
         return 0;
      }

      void deploy_token( account_name ac ) {
         create_account( ac );
         set_code( ac, eosio_token_wast );
      }

      void deploy_exchange( account_name ac ) {
         create_account( ac );
         set_code( ac, exchange_wast );
      }

      void create_currency( name contract, name signer, asset maxsupply ) {
         push_action(contract, signer, N(create), mutable_variant_object()
                 ("issuer",       contract )
                 ("maximum_supply", maxsupply )
                 ("can_freeze", 0)
                 ("can_recall", 0)
                 ("can_whitelist", 0)
         );
      }

      void issue( name contract, name signer, name to, asset amount ) {
         push_action( contract, signer, N(issue), mutable_variant_object()
                 ("to",      to )
                 ("quantity", amount )
                 ("memo", "")
         );
      }

      auto marketorder( name ex_contract, name signer, symbol market,
                  extended_asset sell, extended_symbol receive )
      {
         wdump((market)(sell)(receive));
         wdump((market.to_string()));
         wdump((fc::variant(market).as_string()));
         wdump((fc::variant(market).as<symbol>()));
         return push_action( ex_contract, signer, N(marketorder), mutable_variant_object()
                 ("seller",  signer )
                 ("market",  market )
                 ("sell", sell)
                 ("receive", receive)
         );
      }

      auto deposit( name exchangecontract, name signer, extended_asset amount ) {
         return push_action( amount.contract, signer, N(transfer), mutable_variant_object()
            ("from", signer )
            ("to",  exchangecontract )
            ("quantity", amount.quantity )
            ("memo", "deposit")
         );
      }

      auto withdraw( name exchangecontract, name signer, extended_asset amount ) {
         return push_action( exchangecontract, signer, N(withdraw), mutable_variant_object()
            ("from", signer )
            ("quantity", amount )
         );
      }

      auto lend( name contract, name signer, extended_asset quantity, symbol market ) {
         return push_action( contract, signer, N(lend), mutable_variant_object()
             ("lender", signer )
             ("market", market )
             ("quantity", quantity )
         );
      }
      auto unlend( name contract, name signer, double interest_shares, extended_symbol interest_symbol, symbol market ) {
         return push_action( contract, signer, N(unlend), mutable_variant_object()
             ("lender", signer )
             ("market", market )
             ("interest_shares", interest_shares)
             ("interest_symbol", interest_symbol)
         );
      }

      auto upmargin( name contract, name borrower, const symbol& market, const extended_asset& delta_borrow, const extended_asset& delta_collateral ) {
         return push_action( contract, borrower, N(upmargin), mutable_variant_object()
            ("borrower", borrower)
            ("market", market)
            ("delta_borrow", delta_borrow)
            ("delta_collateral", delta_collateral)
         );
      }

      auto continuation( name contract, name signer, const symbol& market, int32_t max_ops ) {
         return push_action( contract, signer, N(continuation), mutable_variant_object()
            ("market", market)
            ("max_ops", max_ops)
         );
      }

      auto create_exchange( name contract, name signer,
                            extended_asset base_deposit,
                            extended_asset quote_deposit,
                            asset exchange_supply, 
                            double fee = 0, double interest_rate = 0 ) {
         return push_action( contract, signer, N(createx), mutable_variant_object()
                        ("creator", signer)
                        ("initial_supply", exchange_supply)
                        ("fee", fee)
                        ("interest_rate", interest_rate)
                        ("base_deposit", base_deposit)
                        ("quote_deposit", quote_deposit)
                    );
      }


      exchange_tester()
      :TESTER(),abi_ser(json::from_string(exchange_abi).as<abi_def>(), abi_serializer_max_time)
      {
         create_account( N(dan) );
         create_account( N(trader) );

         deploy_exchange( N(exchange) );

         create_currency( N(exchange), N(exchange), A(1000000.00 USD) );
         create_currency( N(exchange), N(exchange), A(1000000.00 BTC) );

         issue( N(exchange), N(exchange), N(dan), A(1000.00 USD) );
         issue( N(exchange), N(exchange), N(dan), A(1000.00 BTC) );

         deposit( N(exchange), N(dan), extended_asset( A(500.00 USD), N(exchange) ) );
         deposit( N(exchange), N(dan), extended_asset( A(500.00 BTC), N(exchange) ) );

         create_exchange( N(exchange), N(dan),
                            extended_asset( A(400.00 USD), N(exchange) ),
                            extended_asset( A(400.00 BTC), N(exchange) ),
                            A(10000000.00 EXC) );

         produce_block();
      }

      abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(exchange_tests)

BOOST_AUTO_TEST_CASE( bootstrap ) try {
   auto expected = asset::from_string( "1000000.0000 CUR" );
   exchange_tester t;
   t.create_currency( N(exchange), N(exchange), expected );
   t.issue( N(exchange), N(exchange), N(exchange), expected );
   auto actual = t.get_currency_balance(N(exchange), expected.get_symbol(), N(exchange));
   BOOST_REQUIRE_EQUAL(expected, actual);
} FC_LOG_AND_RETHROW() /// test_api_bootstrap


BOOST_AUTO_TEST_CASE( exchange_create ) try {
   auto expected = asset::from_string( "1000000.0000 CUR" );
   exchange_tester t;

   t.issue( N(exchange), N(exchange), N(trader), A(2000.00 BTC) );
   t.issue( N(exchange), N(exchange), N(trader), A(2000.00 USD) );

   t.deposit( N(exchange), N(trader), extended_asset( A(1500.00 USD), N(exchange) ) );
   t.deposit( N(exchange), N(trader), extended_asset( A(1500.00 BTC), N(exchange) ) );

   auto trader_ex_usd = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(trader) );
   auto trader_ex_btc = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"BTC"), N(trader) );
   auto dan_ex_usd    = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(dan) );
   auto dan_ex_btc    = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"BTC"), N(dan) );

   auto dan_ex_exc = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"EXC"), N(dan) );
   wdump((dan_ex_exc));

   auto result = t.marketorder( N(exchange), N(trader), symbol(2,"EXC"),
                          extended_asset( A(10.00 BTC), N(exchange) ),
                          extended_symbol{ symbol(2,"USD"), N(exchange) } );

   for( const auto& at : result->action_traces )
      ilog( "${s}", ("s",at.console) );

   trader_ex_usd = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(trader) );
   trader_ex_btc = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"BTC"), N(trader) );
   wdump((trader_ex_btc.quantity));
   wdump((trader_ex_usd.quantity));

   result = t.marketorder( N(exchange), N(trader), symbol(2,"EXC"),
                          extended_asset( A(9.75 USD), N(exchange) ),
                          extended_symbol{ symbol(2,"BTC"), N(exchange) } );

   trader_ex_usd = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(trader) );
   trader_ex_btc = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"BTC"), N(trader) );

   for( const auto& at : result->action_traces )
      ilog( "${s}", ("s",at.console) );

   wdump((trader_ex_btc.quantity));
   wdump((trader_ex_usd.quantity));

   BOOST_REQUIRE_EQUAL( trader_ex_usd.quantity, A(1499.99 USD) );
   BOOST_REQUIRE_EQUAL( trader_ex_btc.quantity, A(1499.98 BTC) );

   wdump((t.get_market_state( N(exchange), symbol(2,"EXC") ) ));

   //BOOST_REQUIRE_EQUAL(expected, actual);
} FC_LOG_AND_RETHROW() /// test_api_bootstrap


BOOST_AUTO_TEST_CASE( exchange_lend ) try {
   exchange_tester t;

   t.create_account( N(lender) );
   t.issue( N(exchange), N(exchange), N(lender), A(2000.00 BTC) );
   t.issue( N(exchange), N(exchange), N(lender), A(2000.00 USD) );

   t.deposit( N(exchange), N(lender), extended_asset( A(1500.00 USD), N(exchange) ) );
   t.deposit( N(exchange), N(lender), extended_asset( A(1500.00 BTC), N(exchange) ) );

   auto lender_ex_usd = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(lender) );
   auto lender_ex_btc = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"BTC"), N(lender) );

   t.lend( N(exchange), N(lender), extended_asset( A(1000.00 USD), N(exchange) ), symbol(2,"EXC") );

   lender_ex_usd = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(lender) );
   lender_ex_btc = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"BTC"), N(lender) );

   wdump((lender_ex_btc.quantity));
   wdump((lender_ex_usd.quantity));

   BOOST_REQUIRE_EQUAL( lender_ex_usd.quantity, A(500.00 USD) );

   auto lentshares = t.get_lent_shares( N(exchange), symbol(2,"EXC"), N(lender), true );
   wdump((lentshares));
   BOOST_REQUIRE_EQUAL( lentshares, 100000 );

   wdump((t.get_market_state( N(exchange), symbol(2,"EXC") ) ));

   t.unlend( N(exchange), N(lender), lentshares, extended_symbol{ symbol(2,"USD"), N(exchange)}, symbol(2,"EXC") );

   lentshares = t.get_lent_shares( N(exchange), symbol(2,"EXC"), N(lender), true );
   wdump((lentshares));

   wdump((t.get_market_state( N(exchange), symbol(2,"EXC") ) ));

   //BOOST_REQUIRE_EQUAL(expected, actual);
} FC_LOG_AND_RETHROW() /// test_api_bootstrap

BOOST_AUTO_TEST_CASE( withdraw ) try {
   exchange_tester t;
   t.create_account( N(loki) );
   t.deploy_token( N(loki.token) );
   t.create_currency( N(loki.token), N(loki.token), A(500.00 BTC) );
   t.issue( N(loki.token), N(loki.token), N(loki), A(500.00 BTC) );
   t.issue( N(exchange), N(exchange), N(loki), A(50.00 BTC) );
   t.check_balance(N(loki.token), N(loki), A(500.00 BTC));
   t.check_balance(N(exchange), N(loki), A(50.00 BTC));
   t.deposit( N(exchange), N(loki), extended_asset( A(500.00 BTC), N(loki.token) ) );
   t.deposit( N(exchange), N(loki), extended_asset( A(50.00 BTC), N(exchange) ) );
   CHECK_ASSERT( t.withdraw( N(exchange), N(loki), extended_asset( A(501.00 BTC), N(loki.token) ) ), "overdrawn balance 2" );
   CHECK_ASSERT( t.withdraw( N(exchange), N(loki), extended_asset( A(51.00 BTC), N(exchange) ) ), "overdrawn balance 2" );
   t.withdraw( N(exchange), N(loki), extended_asset( A(500.00 BTC), N(loki.token) ) );
   t.withdraw( N(exchange), N(loki), extended_asset( A(50.00 BTC), N(exchange) ) );
   t.check_balance(N(loki.token), N(loki), A(500.00 BTC));
   t.check_balance(N(exchange), N(loki), A(50.00 BTC));
} FC_LOG_AND_RETHROW() /// test_api_withdraw

BOOST_AUTO_TEST_CASE( exchange_lend2 ) try {
   exchange_tester t;

   auto lender1_amount = asset{ 1, symbol(2,"USD") };
   auto lender2_amount = asset{ (1ll << 53) - 2, symbol(2,"USD") };

   t.create_account( N(lender1) );
   t.issue( N(exchange), N(exchange), N(lender1), lender1_amount );
   t.check_balance(N(exchange), N(lender1), lender1_amount);
   t.deposit( N(exchange), N(lender1), extended_asset{ lender1_amount, N(exchange) } );
   BOOST_REQUIRE_EQUAL(lender1_amount, t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(lender1)).quantity);
   t.lend( N(exchange), N(lender1), extended_asset{ lender1_amount, N(exchange) }, symbol(2,"EXC") );

   t.create_account( N(lender2) );
   t.issue( N(exchange), N(exchange), N(lender2), lender2_amount );
   t.deposit( N(exchange), N(lender2), extended_asset{ lender2_amount, N(exchange) } );

   // lend max amount. Make sure no more is possible
   CHECK_ASSERT( t.lend( N(exchange), N(lender2), extended_asset{ lender2_amount + asset{ 1, symbol(2,"USD") }, N(exchange) }, symbol(2,"EXC") ), "overdrawn balance 2" );
   t.lend( N(exchange), N(lender2), extended_asset{ lender2_amount, N(exchange) }, symbol(2,"EXC") );

   // lender1: unlend everything
   auto lentshares1 = t.get_lent_shares( N(exchange), symbol(2,"EXC"), N(lender1), true );
   t.unlend( N(exchange), N(lender1), lentshares1, extended_symbol{ symbol(2,"USD"), N(exchange)}, symbol(2,"EXC") );
   BOOST_REQUIRE_EQUAL(lender1_amount, t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(lender1)).quantity);

   // lender2: unlend everything
   auto lentshares2 = t.get_lent_shares( N(exchange), symbol(2,"EXC"), N(lender2), true );
   t.unlend( N(exchange), N(lender2), lentshares2, extended_symbol{ symbol(2,"USD"), N(exchange)}, symbol(2,"EXC") );
   BOOST_REQUIRE_EQUAL(lender2_amount, t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(lender2)).quantity);

   // lender3: lend 100 USD and 100 BTC
   t.create_account( N(lender3) );
   t.issue( N(exchange), N(exchange), N(lender3), A(1000.00 USD) );
   t.issue( N(exchange), N(exchange), N(lender3), A(1000.00 BTC) );
   t.check_balance(N(exchange), N(lender3), A(1000.00 USD) );
   t.check_balance(N(exchange), N(lender3), A(1000.00 BTC) );
   t.deposit( N(exchange), N(lender3), extended_asset{ A(1000.00 USD), N(exchange) } );
   t.deposit( N(exchange), N(lender3), extended_asset{ A(1000.00 BTC), N(exchange) } );
   t.check_balance(N(exchange), N(lender3), A(0.00 USD) );
   t.check_balance(N(exchange), N(lender3), A(0.00 BTC) );
   t.lend( N(exchange), N(lender3), extended_asset{ A(100.00 USD), N(exchange) }, symbol(2,"EXC") );
   t.lend( N(exchange), N(lender3), extended_asset{ A(100.00 BTC), N(exchange) }, symbol(2,"EXC") );
   t.check_exchange_balance(N(exchange), N(exchange), N(lender3), A(900.00 USD) );
   t.check_exchange_balance(N(exchange), N(exchange), N(lender3), A(900.00 BTC) );

   // borrower1: borrow without and with enough collateral
   t.create_account( N(borrower1) );
   t.issue( N(exchange), N(exchange), N(borrower1), A(100.00 USD) );
   t.deposit( N(exchange), N(borrower1), extended_asset{ A(100.00 USD), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(0.00 BTC));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(100.00 USD));
   CHECK_ASSERT( t.upmargin( N(exchange), N(borrower1), symbol(2,"EXC"), extended_asset{ A(50.00 BTC), N(exchange) }, extended_asset{ A(50.00 USD), N(exchange) } ), "this update would trigger a margin call" );
   t.upmargin( N(exchange), N(borrower1), symbol(2,"EXC"), extended_asset{ A(50.00 BTC), N(exchange) }, extended_asset{ A(60.00 USD), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(50.00 BTC));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(40.00 USD));

   // lender3: trigger a margin call by issuing a market order, then unlend everything
   t.marketorder( N(exchange), N(lender3), symbol(2,"EXC"), extended_asset{ A(100.00 USD), N(exchange) }, extended_symbol{ symbol(2,"BTC"), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(50.00 BTC));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(42.83 USD));
   auto lentshares3_usd = t.get_lent_shares( N(exchange), symbol(2,"EXC"), N(lender3), true );
   auto lentshares3_btc = t.get_lent_shares( N(exchange), symbol(2,"EXC"), N(lender3), false );
   t.unlend( N(exchange), N(lender3), lentshares3_usd, extended_symbol{ symbol(2,"USD"), N(exchange)}, symbol(2,"EXC") );
   t.unlend( N(exchange), N(lender3), lentshares3_btc, extended_symbol{ symbol(2,"BTC"), N(exchange)}, symbol(2,"EXC") );
} FC_LOG_AND_RETHROW() /// exchange_lend2

BOOST_AUTO_TEST_CASE( exchange_fees ) try {
   exchange_tester t;

   // HIFEE: USD/BTC, 10% fee
   t.create_account( N(maker) );
   t.issue( N(exchange), N(exchange), N(maker), A(1000.00 USD) );
   t.issue( N(exchange), N(exchange), N(maker), A(1000.00 BTC) );
   t.deposit( N(exchange), N(maker), extended_asset{ A(1000.00 USD), N(exchange) } );
   t.deposit( N(exchange), N(maker), extended_asset{ A(1000.00 BTC), N(exchange) } );
   t.create_exchange(
      N(exchange),
      N(maker),
      extended_asset{ A(1000.00 USD), N(exchange) },
      extended_asset{ A(1000.00 BTC), N(exchange) },
      A(1000.00 HIFEE),
      0.10);

   // holder1: 10.00 USD
   t.create_account( N(holder1) );
   t.issue( N(exchange), N(exchange), N(holder1), A(10.00 USD) );
   t.deposit( N(exchange), N(holder1), extended_asset{ A(10.00 USD), N(exchange) } );
   t.marketorder( N(exchange), N(holder1), symbol(2,"HIFEE"), extended_asset{ A(10.00 USD), N(exchange) }, extended_symbol{ symbol(2,"HIFEE"), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(holder1), A(4.43 HIFEE));

   // trader1: round trip 10.00 USD
   t.create_account( N(trader1) );
   t.issue( N(exchange), N(exchange), N(trader1), A(10.00 USD) );
   t.deposit( N(exchange), N(trader1), extended_asset{ A(10.00 USD), N(exchange) } );
   t.marketorder( N(exchange), N(trader1), symbol(2,"HIFEE"), extended_asset{ A(10.00 USD), N(exchange) }, extended_symbol{ symbol(2,"HIFEE"), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(trader1), A(4.41 HIFEE));
   t.marketorder( N(exchange), N(trader1), symbol(2,"HIFEE"), extended_asset{ A(4.41 HIFEE), N(exchange) }, extended_symbol{ symbol(2,"BTC"), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(trader1), A(7.91 BTC));
   t.marketorder( N(exchange), N(trader1), symbol(2,"HIFEE"), extended_asset{ A(7.91 BTC), N(exchange) }, extended_symbol{ symbol(2,"HIFEE"), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(trader1), A(3.56 HIFEE));
   t.marketorder( N(exchange), N(trader1), symbol(2,"HIFEE"), extended_asset{ A(3.56 HIFEE), N(exchange) }, extended_symbol{ symbol(2,"USD"), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(trader1), A(6.51 USD)); // approx 10.00 * (0.90)^4
   t.check_exchange_balance(N(exchange), N(exchange), N(trader1), A(0.00 BTC));
   t.check_exchange_balance(N(exchange), N(exchange), N(trader1), A(0.00 HIFEE));

   // trader2: round trip 1000.00 USD
   t.create_account( N(trader2) );
   t.issue( N(exchange), N(exchange), N(trader2), A(1000.00 USD) );
   t.deposit( N(exchange), N(trader2), extended_asset{ A(1000.00 USD), N(exchange) } );
   t.marketorder( N(exchange), N(trader2), symbol(2,"HIFEE"), extended_asset{ A(1000.00 USD), N(exchange) }, extended_symbol{ symbol(2,"BTC"), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(trader2), A(398.22 BTC));
   t.marketorder( N(exchange), N(trader2), symbol(2,"HIFEE"), extended_asset{ A(398.22 BTC), N(exchange) }, extended_symbol{ symbol(2,"USD"), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(trader2), A(644.01 USD)); // slightly worse than 1000.00 * (0.90)^4
   t.check_exchange_balance(N(exchange), N(exchange), N(trader2), A(0.00 BTC));
   t.check_exchange_balance(N(exchange), N(exchange), N(trader2), A(0.00 HIFEE));

   // holder1: profit
   t.marketorder( N(exchange), N(holder1), symbol(2,"HIFEE"), extended_asset{ A(4.43 HIFEE), N(exchange) }, extended_symbol{ symbol(2,"USD"), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(holder1), A(10.94 USD));
} FC_LOG_AND_RETHROW() /// exchange_fees

BOOST_AUTO_TEST_CASE( interest ) try {
   exchange_tester t;

   // HIINT: USD/BTC, 20% interest
   t.create_account( N(maker) );
   t.issue( N(exchange), N(exchange), N(maker), A(1000.00 USD) );
   t.issue( N(exchange), N(exchange), N(maker), A(1000.00 BTC) );
   t.deposit( N(exchange), N(maker), extended_asset{ A(1000.00 USD), N(exchange) } );
   t.deposit( N(exchange), N(maker), extended_asset{ A(1000.00 BTC), N(exchange) } );
   t.create_exchange(
      N(exchange),
      N(maker),
      extended_asset{ A(1000.00 USD), N(exchange) },
      extended_asset{ A(1000.00 BTC), N(exchange) },
      A(1000.00 HIINT),
      0, 0.20);
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.collected_interest.quantity);

   // lender1: lend 100 BTC
   t.create_account( N(lender1) );
   t.issue( N(exchange), N(exchange), N(lender1), A(100.00 BTC) );
   t.check_balance(N(exchange), N(lender1), A(100.00 BTC) );
   t.deposit( N(exchange), N(lender1), extended_asset{ A(100.00 BTC), N(exchange) } );
   t.check_balance(N(exchange), N(lender1), A(0.00 BTC) );
   t.lend( N(exchange), N(lender1), extended_asset{ A(100.00 BTC), N(exchange) }, symbol(2,"HIINT") );
   t.check_exchange_balance(N(exchange), N(exchange), N(lender1), A(0.00 BTC) );

   // lender2: lend 50 USD and 50 BTC
   t.create_account( N(lender2) );
   t.issue( N(exchange), N(exchange), N(lender2), A(50.00 USD) );
   t.issue( N(exchange), N(exchange), N(lender2), A(50.00 BTC) );
   t.check_balance(N(exchange), N(lender2), A(50.00 USD) );
   t.check_balance(N(exchange), N(lender2), A(50.00 BTC) );
   t.deposit( N(exchange), N(lender2), extended_asset{ A(50.00 USD), N(exchange) } );
   t.deposit( N(exchange), N(lender2), extended_asset{ A(50.00 BTC), N(exchange) } );
   t.check_balance(N(exchange), N(lender2), A(0.00 USD) );
   t.check_balance(N(exchange), N(lender2), A(0.00 BTC) );
   t.lend( N(exchange), N(lender2), extended_asset{ A(50.00 USD), N(exchange) }, symbol(2,"HIINT") );
   t.lend( N(exchange), N(lender2), extended_asset{ A(50.00 BTC), N(exchange) }, symbol(2,"HIINT") );
   t.check_exchange_balance(N(exchange), N(exchange), N(lender2), A(0.00 USD) );
   t.check_exchange_balance(N(exchange), N(exchange), N(lender2), A(0.00 BTC) );
   BOOST_REQUIRE_EQUAL(A(150.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lendable.quantity);
   BOOST_REQUIRE_EQUAL(A(50.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lendable.quantity);

   // borrower1: borrow 50.00 BTC for .5 year
   t.create_account( N(borrower1) );
   t.issue( N(exchange), N(exchange), N(borrower1), A(100.00 USD) );
   t.deposit( N(exchange), N(borrower1), extended_asset{ A(100.00 USD), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(0.00 BTC));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(100.00 USD));
   t.upmargin( N(exchange), N(borrower1), symbol(2,"HIINT"), extended_asset{ A(50.00 BTC), N(exchange) }, extended_asset{ A(70.00 USD), N(exchange) } );
   BOOST_REQUIRE_EQUAL(A(50.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lent.quantity);
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lent.quantity);
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(50.00 BTC));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(30.00 USD));
   t.produce_block();
   t.produce_block( fc::milliseconds(msec_per_year / 2) );
   t.upmargin( N(exchange), N(borrower1), symbol(2,"HIINT"), extended_asset{ A(-50.00 BTC), N(exchange) }, extended_asset{ A(0.00 USD), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(0.00 BTC));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower1), A(92.64 USD)); // 7.36 USD interest
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lent.quantity);
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lent.quantity);

   // collect interest (deferred transaction)
   BOOST_REQUIRE_EQUAL(A(7.36 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(150.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lendable.quantity);
   BOOST_REQUIRE_EQUAL(A(50.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lendable.quantity);
   t.produce_block();
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(157.26 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lendable.quantity);
   BOOST_REQUIRE_EQUAL(A(50.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lendable.quantity);

   // borrower2: borrow 50.00 USD
   t.create_account( N(borrower2) );
   t.issue( N(exchange), N(exchange), N(borrower2), A(100.00 BTC) );
   t.deposit( N(exchange), N(borrower2), extended_asset{ A(100.00 BTC), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower2), A(0.00 USD));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower2), A(100.00 BTC));
   t.upmargin( N(exchange), N(borrower2), symbol(2,"HIINT"), extended_asset{ A(50.00 USD), N(exchange) }, extended_asset{ A(70.00 BTC), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower2), A(50.00 USD));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower2), A(30.00 BTC));
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lent.quantity);
   BOOST_REQUIRE_EQUAL(A(50.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lent.quantity);

   // lender2: can't unlend funds in margins
   auto lentshares2_usd = t.get_lent_shares( N(exchange), symbol(2,"HIINT"), N(lender2), true );
   CHECK_ASSERT( t.unlend( N(exchange), N(lender2), lentshares2_usd, extended_symbol{ symbol(2,"USD"), N(exchange)}, symbol(2,"HIINT") ), "funds are in margins" );

   // borrower2: remove margin after .5 year
   t.produce_block();
   t.produce_block( fc::milliseconds(msec_per_year / 2) );
   t.upmargin( N(exchange), N(borrower2), symbol(2,"HIINT"), extended_asset{ A(-50.00 USD), N(exchange) }, extended_asset{ A(0.00 BTC), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower2), A(0.00 USD));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower2), A(92.64 BTC)); // 7.36 BTC interest
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lent.quantity);
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lent.quantity);

   // collect interest (deferred transaction)
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(7.36 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(157.26 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lendable.quantity);
   BOOST_REQUIRE_EQUAL(A(50.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lendable.quantity);
   t.produce_block();
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(157.26 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lendable.quantity);
   BOOST_REQUIRE_EQUAL(A(57.37 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lendable.quantity);

   // borrower3: borrow 50.00 USD
   t.create_account( N(borrower3) );
   t.issue( N(exchange), N(exchange), N(borrower3), A(100.00 BTC) );
   t.deposit( N(exchange), N(borrower3), extended_asset{ A(100.00 BTC), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower3), A(0.00 USD));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower3), A(100.00 BTC));
   t.upmargin( N(exchange), N(borrower3), symbol(2,"HIINT"), extended_asset{ A(50.00 USD), N(exchange) }, extended_asset{ A(70.00 BTC), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower3), A(50.00 USD));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower3), A(30.00 BTC));
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lent.quantity);
   BOOST_REQUIRE_EQUAL(A(50.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lent.quantity);

   // trader1: trigger margin call after .5 year
   t.produce_block();
   t.produce_block( fc::milliseconds(msec_per_year / 2) );
   t.create_account( N(trader1) );
   t.issue( N(exchange), N(exchange), N(trader1), A(100.00 BTC) );
   t.deposit( N(exchange), N(trader1), extended_asset{ A(100.00 BTC), N(exchange) } );
   t.marketorder( N(exchange), N(trader1), symbol(2,"HIINT"), extended_asset{ A(100.00 BTC), N(exchange) }, extended_symbol{ symbol(2,"USD"), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower3), A(50.00 USD));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower3), A(39.92 BTC));
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lent.quantity);
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lent.quantity);

   // borrower3: try to borrow while market busy
   CHECK_ASSERT( t.upmargin( N(exchange), N(borrower3), symbol(2,"HIINT"), extended_asset{ A(50.00 USD), N(exchange) }, extended_asset{ A(70.00 BTC), N(exchange) } ), "market is busy" );

   // collect interest (deferred transaction)
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(7.36 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(157.26 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lendable.quantity);
   BOOST_REQUIRE_EQUAL(A(57.37 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lendable.quantity);
   t.produce_block();
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.collected_interest.quantity);
   BOOST_REQUIRE_EQUAL(A(157.26 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lendable.quantity);
   BOOST_REQUIRE_EQUAL(A(62.83 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lendable.quantity);

   // borrower4: borrow 50.00 USD
   t.create_account( N(borrower4) );
   t.issue( N(exchange), N(exchange), N(borrower4), A(100.00 BTC) );
   t.deposit( N(exchange), N(borrower4), extended_asset{ A(100.00 BTC), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower4), A(0.00 USD));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower4), A(100.00 BTC));
   t.upmargin( N(exchange), N(borrower4), symbol(2,"HIINT"), extended_asset{ A(50.00 USD), N(exchange) }, extended_asset{ A(93.00 BTC), N(exchange) } );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower4), A(50.00 USD));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower4), A(7.00 BTC));
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lent.quantity);
   BOOST_REQUIRE_EQUAL(A(50.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lent.quantity);

   // borrower4: yearly interest triggers margin call
   t.produce_block();
   t.produce_block( fc::milliseconds(msec_per_year) );
   t.produce_block();
   t.continuation( N(exchange), N(trader1), symbol(2,"HIINT"), 20 );
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower4), A(50.00 USD));
   t.check_exchange_balance(N(exchange), N(exchange), N(borrower4), A(7.91 BTC));
   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIINT")).quote.peer_margin.total_lent.quantity);
   BOOST_REQUIRE_EQUAL(A(0.00 USD), t.get_market_state(N(exchange), symbol(2,"HIINT")).base.peer_margin.total_lent.quantity);

   // todo: charge interest during margin update
   // todo: lender1 interest
   // todo: lender2 interest
} FC_LOG_AND_RETHROW() /// interest

BOOST_AUTO_TEST_CASE( cascade_loss ) try {
   exchange_tester t;

   // HIALL: USD/BTC, 10% fee, 20% interest
   t.create_account( N(maker) );
   t.issue( N(exchange), N(exchange), N(maker), A(1000.00 USD) );
   t.issue( N(exchange), N(exchange), N(maker), A(1000.00 BTC) );
   t.deposit( N(exchange), N(maker), extended_asset{ A(1000.00 USD), N(exchange) } );
   t.deposit( N(exchange), N(maker), extended_asset{ A(1000.00 BTC), N(exchange) } );
   t.create_exchange(
      N(exchange),
      N(maker),
      extended_asset{ A(1000.00 USD), N(exchange) },
      extended_asset{ A(1000.00 BTC), N(exchange) },
      A(1000.00 HIALL),
      0, 0.20);

   // lender1: lend 1000 BTC
   t.create_account( N(lender1) );
   t.issue( N(exchange), N(exchange), N(lender1), A(1000.00 BTC) );
   t.check_balance(N(exchange), N(lender1), A(1000.00 BTC) );
   t.deposit( N(exchange), N(lender1), extended_asset{ A(1000.00 BTC), N(exchange) } );
   t.check_balance(N(exchange), N(lender1), A(0.00 BTC) );
   t.lend( N(exchange), N(lender1), extended_asset{ A(1000.00 BTC), N(exchange) }, symbol(2,"HIALL") );
   t.check_exchange_balance(N(exchange), N(exchange), N(lender1), A(0.00 BTC) );
   BOOST_REQUIRE_EQUAL(A(1000.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIALL")).quote.peer_margin.total_lendable.quantity);

   // many weak margins
   auto borrower = N(borrower1111);
   for ( int i = 0; i < 189; ++i ) {
      // printf("\n\n<<<<<< %d\n\n", i);
      t.create_account( borrower );
      t.issue( N(exchange), N(exchange), borrower, A(100.00 USD) );
      t.deposit( N(exchange), borrower, extended_asset{ A(100.00 USD), N(exchange) } );
      t.upmargin( N(exchange), borrower, symbol(2,"HIALL"), extended_asset{ A(1.00 BTC), N(exchange) }, extended_asset{ A(1.60 USD), N(exchange) } );
      borrower += 16;
      if (!(i & 8))
         t.produce_block();
   }
   t.produce_block();

   // trader1: trigger margin call on entire set
   t.create_account( N(trader1) );
   t.issue( N(exchange), N(exchange), N(trader1), A(0.90 USD) );
   t.deposit( N(exchange), N(trader1), extended_asset{ A(0.90 USD), N(exchange) } );
   t.marketorder( N(exchange), N(trader1), symbol(2,"HIALL"), extended_asset{ A(0.90 USD), N(exchange) }, extended_symbol{ symbol(2,"BTC"), N(exchange) } );

   BOOST_REQUIRE_EQUAL(A(0.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIALL")).quote.peer_margin.total_lent.quantity);
   BOOST_REQUIRE_EQUAL(A(1000.00 BTC), t.get_market_state(N(exchange), symbol(2,"HIALL")).quote.peer_margin.total_lendable.quantity);
} FC_LOG_AND_RETHROW() /// cascade_loss

BOOST_AUTO_TEST_SUITE_END()
