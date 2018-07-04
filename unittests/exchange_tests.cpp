#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/symbol.hpp>

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

#define A(X) asset::from_string( #X )

struct margin_state {
   extended_asset total_lendable;
   extended_asset total_lent;
   double         least_collateralized = 0;
   double         interest_shares = 0;
};
FC_REFLECT( margin_state, (total_lendable)(total_lent)(least_collateralized)(interest_shares) )

struct exchange_state {
   account_name      manager;
   extended_asset    supply;
   uint32_t          fee = 0;

   struct connector {
      extended_asset balance;
      uint32_t       weight = 500;
      margin_state   peer_margin;
   };

   connector base;
   connector quote;
};

FC_REFLECT( exchange_state::connector, (balance)(weight)(peer_margin) );
FC_REFLECT( exchange_state, (manager)(supply)(fee)(base)(quote) );

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
         act.data = abi_ser.variant_to_binary(action_type_name, data);

         signed_transaction trx;
         trx.actions.emplace_back(std::move(act));
         set_transaction_headers(trx);
         trx.sign(get_private_key(signer, "active"), control->get_chain_id());
         return push_transaction(trx);
      }

      asset get_balance(const account_name& account) const {
         return get_currency_balance(N(exchange), symbol(SY(4,CUR)), account);
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

               return interest_shares;
            }
         }
         FC_ASSERT( false, "unable to find loan balance" );
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

      auto trade( name ex_contract, name signer, symbol market,
                  extended_asset sell, extended_asset min_receive )
      {
         wdump((market)(sell)(min_receive));
         wdump((market.to_string()));
         wdump((fc::variant(market).as_string()));
         wdump((fc::variant(market).as<symbol>()));
         return push_action( ex_contract, signer, N(trade), mutable_variant_object()
                 ("seller",  signer )
                 ("market",  market )
                 ("sell", sell)
                 ("min_receive", min_receive)
                 ("expire", 0)
                 ("fill_or_kill", 1)
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

      auto create_exchange( name contract, name signer,
                            extended_asset base_deposit,
                            extended_asset quote_deposit,
                            asset exchange_supply ) {
         return push_action( contract, signer, N(createx), mutable_variant_object()
                        ("creator", signer)
                        ("initial_supply", exchange_supply)
                        ("fee", 0)
                        ("base_deposit", base_deposit)
                        ("quote_deposit", quote_deposit)
                    );
      }


      exchange_tester()
      :TESTER(),abi_ser(json::from_string(exchange_abi).as<abi_def>())
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

   auto result = t.trade( N(exchange), N(trader), symbol(2,"EXC"),
                          extended_asset( A(10.00 BTC), N(exchange) ),
                          extended_asset( A(0.01 USD), N(exchange) ) );

   for( const auto& at : result->action_traces )
      ilog( "${s}", ("s",at.console) );

   trader_ex_usd = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(trader) );
   trader_ex_btc = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"BTC"), N(trader) );
   wdump((trader_ex_btc.quantity));
   wdump((trader_ex_usd.quantity));

   result = t.trade( N(exchange), N(trader), symbol(2,"EXC"),
                          extended_asset( A(9.75 USD), N(exchange) ),
                          extended_asset( A(0.01 BTC), N(exchange) ) );

   trader_ex_usd = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"USD"), N(trader) );
   trader_ex_btc = t.get_exchange_balance( N(exchange), N(exchange), symbol(2,"BTC"), N(trader) );

   for( const auto& at : result->action_traces )
      ilog( "${s}", ("s",at.console) );

   wdump((trader_ex_btc.quantity));
   wdump((trader_ex_usd.quantity));

   BOOST_REQUIRE_EQUAL( trader_ex_usd.quantity, A(1500.00 USD) );
   BOOST_REQUIRE_EQUAL( trader_ex_btc.quantity, A(1499.99 BTC) );

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




BOOST_AUTO_TEST_SUITE_END()
