/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <boost/test/unit_test.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/block_summary_object.hpp>
#include <eos/chain/wasm_interface.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <currency/currency.wast.hpp>
#include <exchange/exchange.wast.hpp>
#include <infinite/infinite.wast.hpp>

using namespace eosio;
using namespace chain;


   struct order_id {
      account_name name;
      uint64_t     number  = 0;
   };

   struct __attribute((packed)) bid {
      order_id           buyer;
      unsigned __int128  price;
      uint64_t           quantity;
      types::time        expiration;
      uint8_t            fill_or_kill = false;
   };
   struct __attribute((packed)) ask {
      order_id           seller;
      unsigned __int128  price;
      uint64_t           quantity;
      types::time        expiration;
      uint8_t            fill_or_kill = false;
   };
FC_REFLECT( order_id, (name)(number) );
FC_REFLECT( bid, (buyer)(price)(quantity)(expiration)(fill_or_kill) );
FC_REFLECT( ask, (seller)(price)(quantity)(expiration)(fill_or_kill) );

   struct record {
      uint64_t a = 0;
      uint64_t b = 0;
      uint64_t c = 0;
      fc::uint128 d = 0;
      fc::uint128 e = 0;
   };

   struct by_abcde;
   struct by_abced;
   using test_index = boost::multi_index_container<
      record,
      indexed_by<
         ordered_unique<tag<by_abcde>, 
            composite_key< record,
               member<record, uint64_t, &record::a>,
               member<record, uint64_t, &record::b>,
               member<record, uint64_t, &record::c>,
               member<record, fc::uint128, &record::d>,
               member<record, fc::uint128, &record::e>
            >
         >,
         ordered_unique<tag<by_abced>, 
            composite_key< record,
               member<record, uint64_t, &record::a>,
               member<record, uint64_t, &record::b>,
               member<record, uint64_t, &record::c>,
               member<record, fc::uint128, &record::e>,
               member<record, fc::uint128, &record::d>
            >
         >
      >
   >;
//   FC_REFLECT( record, (a)(b)(c)(d)(e) );

BOOST_AUTO_TEST_SUITE(slow_tests)


BOOST_FIXTURE_TEST_CASE(multiindex, testing_fixture)
{
   test_index idx;
   idx.emplace( record{1,0,0,1,9} );
   idx.emplace( record{1,1,1,2,8} );
   idx.emplace( record{1,2,3,3,7} );
   idx.emplace( record{1,2,3,5,6} );
   idx.emplace( record{1,2,3,6,5} );
   idx.emplace( record{1,2,3,7,5} );
   idx.emplace( record{1,2,3,8,3} );

   auto& by_de = idx.get<by_abcde>();
   auto& by_ed = idx.get<by_abced>();

   auto itr = by_de.lower_bound( boost::make_tuple(1,2,3,0,0) );
   BOOST_REQUIRE( itr != by_de.end() );
   BOOST_REQUIRE( itr->d == 3 );

   auto itr2 = by_ed.lower_bound( boost::make_tuple(1,2,3,0,0) );
   BOOST_REQUIRE( itr2 != by_ed.end() );
   BOOST_REQUIRE( itr2->e == 3 );
   //BOOST_REQUIRE( itr2 == by_ed.begin() );
   Make_Blockchain(chain)
   auto& db = chain.get_mutable_database();
   db.create<key128x128_value_object>( [&]( auto& obj ) {
        obj.scope = 1;
        obj.code = 1;
        obj.table = 1;
        obj.primary_key = 1;
        obj.secondary_key = 2;
   });
   db.create<key128x128_value_object>( [&]( auto& obj ) {
        obj.scope = 1;
        obj.code = 1;
        obj.table = 1;
        obj.primary_key = 2;
        obj.secondary_key = 3;
   });
   db.create<key128x128_value_object>( [&]( auto& obj ) {
        obj.scope = 1;
        obj.code = 1;
        obj.table = 1;
        obj.primary_key = 3;
        obj.secondary_key = 2;
   });
   {
   auto& sidx = db.get_index< key128x128_value_index, by_scope_secondary> ();
   auto lower = sidx.lower_bound( boost::make_tuple( 1, 1, 1, 0, 0 ) );
   BOOST_REQUIRE( lower != sidx.end() );
   BOOST_REQUIRE( lower->primary_key == 1 );
   BOOST_REQUIRE( lower->secondary_key == 2 );
   }

}

// Test that TaPoS still works after block 65535 (See Issue #55)
BOOST_FIXTURE_TEST_CASE(tapos_wrap, testing_fixture)
{ try {
      Make_Blockchain(chain)
      Make_Account(chain, system);
      Make_Account(chain, acct);
      chain.produce_blocks(1);
      Transfer_Asset(chain, inita, system, asset(1000) );
      Transfer_Asset(chain, system, acct, asset(5));
      Stake_Asset(chain, acct, asset(5).amount);
      wlog("Hang on, this will take a minute...");
      chain.produce_blocks(65536);
      Begin_Unstake_Asset(chain, acct, asset(1).amount);
} FC_LOG_AND_RETHROW() }

// Verify that staking and unstaking works
BOOST_FIXTURE_TEST_CASE(stake, testing_fixture)
{ try {
   // Create account sam with default balance of 100, and stake 55 of it
   Make_Blockchain(chain);
   Make_Account(chain, sam);

   chain.produce_blocks();

   Transfer_Asset(chain, inita, sam, asset(55) );

   // MakeAccount should start sam out with some staked balance
   BOOST_REQUIRE_EQUAL(chain.get_staked_balance("sam"), asset(100).amount);

   Stake_Asset(chain, sam, asset(55).amount);

   // Check balances
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), asset(155).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), asset(0).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), asset(0).amount);

   chain.produce_blocks();

   // Start unstaking 20, check balances
   BOOST_CHECK_THROW(Begin_Unstake_Asset(chain, sam, asset(156).amount), chain::message_precondition_exception);
   Begin_Unstake_Asset(chain, sam, asset(20).amount);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), asset(135).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), asset(20).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), asset(0).amount);

   // Make sure we can't liquidate early
   BOOST_CHECK_THROW(Finish_Unstake_Asset(chain, sam, asset(10).amount), chain::message_precondition_exception);

   // Fast forward to when we can liquidate
   wlog("Hang on, this will take a minute...");
   chain.produce_blocks(config::staked_balance_cooldown_seconds / config::block_interval_seconds + 1);

   BOOST_CHECK_THROW(Finish_Unstake_Asset(chain, sam, asset(21).amount), chain::message_precondition_exception);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), asset(135).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), asset(20).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), asset(0).amount);

   // Liquidate 10 of the 20 unstaking and check balances
   Finish_Unstake_Asset(chain, sam, asset(10).amount);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), asset(135).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), asset(10).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), asset(10).amount);

   // Liquidate 2 of the 10 left unstaking and check balances
   Finish_Unstake_Asset(chain, sam, asset(2).amount);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), asset(135).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), asset(8).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), asset(12).amount);

   // Ignore the 8 left in unstaking, and begin unstaking 5, which should restake the 8, and start over unstaking 5
   Begin_Unstake_Asset(chain, sam, asset(5).amount);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), asset(138).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), asset(5).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), asset(12).amount);

   // Begin unstaking 20, which should only deduct 15 from staked, since 5 was already in unstaking
   Begin_Unstake_Asset(chain, sam, asset(20).amount);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), asset(123).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), asset(20).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), asset(12).amount);
} FC_LOG_AND_RETHROW() }

vector<uint8_t> assemble_wast( const std::string& wast ) {
   //   std::cout << "\n" << wast << "\n";
   IR::Module module;
   std::vector<WAST::Error> parseErrors;
   WAST::parseModule(wast.c_str(),wast.size(),module,parseErrors);
   if(parseErrors.size())
   {
      // Print any parse errors;
      std::cerr << "Error parsing WebAssembly text file:" << std::endl;
      for(auto& error : parseErrors)
      {
         std::cerr << ":" << error.locus.describe() << ": " << error.message.c_str() << std::endl;
         std::cerr << error.locus.sourceLine << std::endl;
         std::cerr << std::setw(error.locus.column(8)) << "^" << std::endl;
      }
      FC_ASSERT( !"error parsing wast" );
   }

   try
   {
      // Serialize the WebAssembly module.
      Serialization::ArrayOutputStream stream;
      WASM::serialize(stream,module);
      return stream.getBytes();
   }
   catch(Serialization::FatalSerializationException exception)
   {
      std::cerr << "Error serializing WebAssembly binary file:" << std::endl;
      std::cerr << exception.message << std::endl;
      throw;
   }
}

void SetCode( testing_blockchain& chain, account_name account, const char* wast ) {
   try {
      types::setcode handler;
      handler.account = account;

      auto wasm = assemble_wast( wast );
      handler.code.resize(wasm.size());
      memcpy( handler.code.data(), wasm.data(), wasm.size() );

      {
         eosio::chain::signed_transaction trx;
         trx.scope = {account};
         trx.messages.resize(1);
         trx.messages[0].code = config::eos_contract_name;
         trx.messages[0].authorization.emplace_back(types::account_permission{account,"active"});
         transaction_set_message(trx, 0, "setcode", handler);
         trx.expiration = chain.head_block_time() + 100;
         transaction_set_reference_block(trx, chain.head_block_id());
         chain.push_transaction(trx);
         chain.produce_blocks(1);
      }
} FC_LOG_AND_RETHROW( ) }

void TransferCurrency( testing_blockchain& chain, account_name from, account_name to, uint64_t amount ) {
   eosio::chain::signed_transaction trx;
   trx.scope = sort_names({from,to});
   transaction_emplace_message(trx, "currency", 
                      vector<types::account_permission>{ {from,"active"} },
                      "transfer", types::transfer{from, to, amount,""});

   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());
   idump((trx));
   chain.push_transaction(trx);
}

void WithdrawCurrency( testing_blockchain& chain, account_name from, account_name to, uint64_t amount ) {
   eosio::chain::signed_transaction trx;
   trx.scope = sort_names({from,to});
   transaction_emplace_message(trx, "currency", 
                      vector<types::account_permission>{ {from,"active"},{to,"active"} },
                      "transfer", types::transfer{from, to, amount,""});
   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());
   chain.push_transaction(trx);
}

//Test account script processing
BOOST_FIXTURE_TEST_CASE(create_script, testing_fixture)
{ try {
      chain_controller::txn_msg_limits rate_limit = { fc::time_point_sec(10), 100000, fc::time_point_sec(10), 100000 };
      Make_Blockchain(chain, ::eosio::chain_plugin::default_transaction_execution_time,
            ::eosio::chain_plugin::default_received_block_transaction_execution_time,
            ::eosio::chain_plugin::default_create_block_transaction_execution_time, rate_limit);
      chain.produce_blocks(10);
      Make_Account(chain, currency);
      chain.produce_blocks(1);


      types::setcode handler;
      handler.account = "currency";

      auto wasm = assemble_wast( currency_wast );
      handler.code.resize(wasm.size());
      memcpy( handler.code.data(), wasm.data(), wasm.size() );

      {
         eosio::chain::signed_transaction trx;
         trx.scope = {"currency"};
         trx.messages.resize(1);
         trx.messages[0].code = config::eos_contract_name;
         trx.messages[0].authorization.emplace_back(types::account_permission{"currency","active"});
         transaction_set_message(trx, 0, "setcode", handler);
         trx.expiration = chain.head_block_time() + 100;
         transaction_set_reference_block(trx, chain.head_block_id());
         chain.push_transaction(trx);
         chain.produce_blocks(1);
      }


      auto start = fc::time_point::now();
      for (uint32_t i = 0; i < 10000; ++i)
      {
         eosio::chain::signed_transaction trx;
         trx.scope = sort_names({"currency","inita"});
         transaction_emplace_message(trx, "currency", 
                            vector<types::account_permission>{ {"currency","active"} },
                            "transfer", types::transfer{"currency", "inita", 1+i,""});
         trx.expiration = chain.head_block_time() + 100;
         transaction_set_reference_block(trx, chain.head_block_id());
         //idump((trx));
         chain.push_transaction(trx);
      }
      auto end = fc::time_point::now();
      idump((10000*1000000.0 / (end-start).count()));

      chain.produce_blocks(10);

} FC_LOG_AND_RETHROW() }


static const uint64_t precision = 1000ll*1000ll*1000ll*1000ll*1000ll;
unsigned __int128 to_price( double p ) {
   uint64_t  pi(p);
   unsigned __int128 result(pi);
   result *= precision;

   double fract = p - pi;
   result += uint64_t( fract * precision );
   return result;
}


void SellCurrency( testing_blockchain& chain, account_name seller, account_name exchange, uint64_t ordernum, uint64_t cur_quantity, double price ) {

   ask b {  order_id{seller,ordernum},
            to_price(price),
            cur_quantity, 
            chain.head_block_time()+fc::days(3) 
         };

   eosio::chain::signed_transaction trx;
   trx.scope = sort_names({"exchange"});
   transaction_emplace_message(trx, "exchange", 
                      vector<types::account_permission>{ {seller,"active"} },
                      "sell", b );
   //trx.messages.back().set_packed( "sell", b);
   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());
   chain.push_transaction(trx);

}
void BuyCurrency( testing_blockchain& chain, account_name buyer, account_name exchange, uint64_t ordernum, uint64_t cur_quantity, double price ) {
   bid b {  order_id{buyer,ordernum},
            to_price(price),
            cur_quantity, 
            chain.head_block_time()+fc::days(3) 
         };

   eosio::chain::signed_transaction trx;
   trx.scope = sort_names({"exchange"});
   transaction_emplace_message(trx, "exchange", 
                      vector<types::account_permission>{ {buyer,"active"} },
                      "buy", b );
   //trx.messages.back().set_packed( "buy", b);
   trx.expiration = chain.head_block_time() + 100;
   transaction_set_reference_block(trx, chain.head_block_id());
   chain.push_transaction(trx);
}

BOOST_FIXTURE_TEST_CASE(create_exchange, testing_fixture) {
   try {
      Make_Blockchain(chain);
      chain.produce_blocks(2);
      Make_Account(chain, system);
      Make_Account(chain, currency);
      Make_Account(chain, exchange);
      chain.produce_blocks(1);

      SetCode(chain, "currency", currency_wast);
      SetCode(chain, "exchange", exchange_wast);

      chain.produce_blocks(1);

      ilog( "transfering currency to the users" );
      TransferCurrency( chain, "currency", "inita", 1000 );
      TransferCurrency( chain, "currency", "initb", 2000 );

      chain.produce_blocks(1);
      ilog( "transfering funds to the exchange" );
      TransferCurrency( chain, "inita", "exchange", 1000 );
      TransferCurrency( chain, "initb", "exchange", 2000 );

      Transfer_Asset(chain, inita, exchange, asset(500));
      Transfer_Asset(chain, initb, exchange, asset(500));

      BOOST_REQUIRE_THROW( TransferCurrency( chain, "initb", "exchange", 2000 ), fc::exception ); // insufficient funds


      BOOST_REQUIRE_THROW( WithdrawCurrency( chain, "exchange", "initb", 2001 ), fc::exception ); // insufficient funds

      ilog( "withdrawing from exchange" );

      WithdrawCurrency( chain, "exchange", "initb", 2000 );
      chain.produce_blocks(1);

      ilog( "send back to exchange" );
      TransferCurrency( chain, "initb", "exchange", 2000 );
      chain.produce_blocks(1);

      wlog( "start buy and sell" );
      uint64_t order_num = 1;
      SellCurrency( chain, "initb", "exchange", order_num++, 100, .5 );
      SellCurrency( chain, "initb", "exchange", order_num++, 100, .75 );
      SellCurrency( chain, "initb", "exchange", order_num++, 100, .85 );
      //BOOST_REQUIRE_THROW( SellCurrency( chain, "initb", "exchange", 1, 100, .5 ), fc::exception ); // order id already exists
      //SellCurrency( chain, "initb", "exchange", 2, 100, .75 );

//      BuyCurrency( chain, "initb", "exchange", 1, 50, .25 ); 
      BuyCurrency( chain, "initb", "exchange", order_num++, 50, .5 );
      //BOOST_REQUIRE_THROW( BuyCurrency( chain, "initb", "exchange", 1, 50, .25 ), fc::exception );  // order id already exists

      /// this should buy 5 from initb order 2 at a price of .75
      //BuyCurrency( chain, "initb", "exchange", 2, 50, .8 ); 

   } FC_LOG_AND_RETHROW() 
}

//Test account script float rejection
BOOST_FIXTURE_TEST_CASE(create_script_w_float, testing_fixture)
{ try {
      Make_Blockchain(chain);
      chain.produce_blocks(10);
      Make_Account(chain, simplecoin);
      chain.produce_blocks(1);


std::string wast_apply =
R"(
(module
  (type $FUNCSIG$vii (func (param i32 i32)))
  (type $FUNCSIG$viiii (func (param i32 i32 i32 i32)))
  (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
  (type $FUNCSIG$iiiii (func (param i32 i32 i32 i32) (result i32)))
  (type $FUNCSIG$iiii (func (param i32 i32 i32) (result i32)))
  (import "env" "account_name_unpack" (func $account_name_unpack (param i32 i32)))
  (import "env" "Varint_unpack" (func $Varint_unpack (param i32 i32)))
  (import "env" "assert" (func $assert (param i32 i32)))
  (import "env" "load" (func $load (param i32 i32 i32 i32) (result i32)))
  (import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
  (import "env" "read_message" (func $read_message (param i32 i32) (result i32)))
  (import "env" "remove" (func $remove (param i32 i32) (result i32)))
  (import "env" "store" (func $store (param i32 i32 i32 i32)))
  (table 0 anyfunc)
  (memory $0 1)
  (data (i32.const 8224) "out of memory\00")
  (data (i32.const 8240) "string is longer than account name allows\00")
  (data (i32.const 8288) "read past end of stream\00")
  (data (i32.const 8368) "simplecoin\00")
  (data (i32.const 8608) "no existing balance\00")
  (data (i32.const 8640) "insufficient funds\00")
  (export "memory" (memory $0))
  (export "malloc" (func $malloc))
  (export "DataStream_init" (func $DataStream_init))
  (export "account_name_initString" (func $account_name_initString))
  (export "account_name_initCString" (func $account_name_initCString))
  (export "uint64_unpack" (func $uint64_unpack))
  (export "String_unpack" (func $String_unpack))
  (export "Transfer_unpack" (func $Transfer_unpack))
  (export "init" (func $init))
  (export "apply_simplecoin_transfer" (func $apply_simplecoin_transfer))
  (func $malloc (param $0 i32) (result i32)
    (local $1 i32)
    (i32.store offset=8208
      (i32.const 0)
      (tee_local $0
        (i32.add
          (tee_local $1
            (i32.load offset=8208
              (i32.const 0)
            )
          )
          (i32.and
            (i32.add
              (get_local $0)
              (i32.const 7)
            )
            (i32.const -8)
          )
        )
      )
    )
    (call $assert
      (i32.lt_u
        (get_local $0)
        (i32.const 8192)
      )
      (i32.const 8224)
    )
    (i32.add
      (get_local $1)
      (i32.const 16)
    )
  )
  (func $DataStream_init (param $0 i32) (param $1 i32) (param $2 i32)
    (i32.store
      (get_local $0)
      (get_local $1)
    )
    (i32.store offset=4
      (get_local $0)
      (get_local $1)
    )
    (i32.store offset=8
      (get_local $0)
      (i32.add
        (get_local $1)
        (get_local $2)
      )
    )
  )
  (func $account_name_initString (param $0 i32) (param $1 i32)
    (call $assert
      (i32.lt_u
        (i32.load
          (get_local $1)
        )
        (i32.const 33)
      )
      (i32.const 8240)
    )
    (drop
      (call $memcpy
        (get_local $0)
        (i32.add
          (get_local $1)
          (i32.const 4)
        )
        (i32.load
          (get_local $1)
        )
      )
    )
  )
  (func $account_name_initCString (param $0 i32) (param $1 i32) (param $2 i32)
    (call $assert
      (i32.lt_u
        (get_local $2)
        (i32.const 33)
      )
      (i32.const 8240)
    )
    (drop
      (call $memcpy
        (get_local $0)
        (get_local $1)
        (get_local $2)
      )
    )
  )
  (func $uint64_unpack (param $0 i32) (param $1 i32)
    (call $assert
      (i32.le_u
        (i32.add
          (i32.load offset=4
            (get_local $0)
          )
          (i32.const 8)
        )
        (i32.load offset=8
          (get_local $0)
        )
      )
      (i32.const 8288)
    )
    (i64.store align=1
      (get_local $1)
      (i64.load align=1
        (i32.load offset=4
          (get_local $0)
        )
      )
    )
    (i32.store offset=4
      (get_local $0)
      (i32.add
        (i32.load offset=4
          (get_local $0)
        )
        (i32.const 8)
      )
    )
  )
  (func $String_unpack (param $0 i32) (param $1 i32)
    (local $2 i32)
    (local $3 i32)
    (call $Varint_unpack
      (get_local $0)
      (i32.const 8312)
    )
    (call $assert
      (i32.le_u
        (i32.add
          (i32.load offset=4
            (get_local $0)
          )
          (i32.load offset=8312
            (i32.const 0)
          )
        )
        (i32.load offset=8
          (get_local $0)
        )
      )
      (i32.const 8288)
    )
    (i32.store offset=8208
      (i32.const 0)
      (tee_local $3
        (i32.add
          (i32.and
            (i32.add
              (i32.load offset=8312
                (i32.const 0)
              )
              (i32.const 11)
            )
            (i32.const -8)
          )
          (tee_local $2
            (i32.load offset=8208
              (i32.const 0)
            )
          )
        )
      )
    )
    (call $assert
      (i32.lt_u
        (get_local $3)
        (i32.const 8192)
      )
      (i32.const 8224)
    )
    (drop
      (call $memcpy
        (i32.add
          (get_local $2)
          (i32.const 20)
        )
        (i32.load offset=4
          (get_local $0)
        )
        (i32.load offset=8312
          (i32.const 0)
        )
      )
    )
    (i32.store
      (get_local $1)
      (i32.add
        (get_local $2)
        (i32.const 16)
      )
    )
  )
  (func $Transfer_unpack (param $0 i32) (param $1 i32)
    (local $2 i32)
    (local $3 i32)
    (call $account_name_unpack
      (get_local $0)
      (get_local $1)
    )
    (call $account_name_unpack
      (get_local $0)
      (i32.add
        (get_local $1)
        (i32.const 32)
      )
    )
    (call $assert
      (i32.le_u
        (i32.add
          (i32.load offset=4
            (get_local $0)
          )
          (i32.const 8)
        )
        (i32.load offset=8
          (get_local $0)
        )
      )
      (i32.const 8288)
    )
    (i64.store offset=64 align=1
      (get_local $1)
      (i64.load align=1
        (i32.load offset=4
          (get_local $0)
        )
      )
    )
    (i32.store offset=4
      (get_local $0)
      (i32.add
        (i32.load offset=4
          (get_local $0)
        )
        (i32.const 8)
      )
    )
    (call $Varint_unpack
      (get_local $0)
      (i32.const 8312)
    )
    (call $assert
      (i32.le_u
        (i32.add
          (i32.load offset=4
            (get_local $0)
          )
          (i32.load offset=8312
            (i32.const 0)
          )
        )
        (i32.load offset=8
          (get_local $0)
        )
      )
      (i32.const 8288)
    )
    (i32.store offset=8208
      (i32.const 0)
      (tee_local $3
        (i32.add
          (i32.and
            (i32.add
              (i32.load offset=8312
                (i32.const 0)
              )
              (i32.const 11)
            )
            (i32.const -8)
          )
          (tee_local $2
            (i32.load offset=8208
              (i32.const 0)
            )
          )
        )
      )
    )
    (call $assert
      (i32.lt_u
        (get_local $3)
        (i32.const 8192)
      )
      (i32.const 8224)
    )
    (drop
      (call $memcpy
        (i32.add
          (get_local $2)
          (i32.const 20)
        )
        (i32.load offset=4
          (get_local $0)
        )
        (i32.load offset=8312
          (i32.const 0)
        )
      )
    )
    (i32.store offset=72
      (get_local $1)
      (i32.add
        (get_local $2)
        (i32.const 16)
      )
    )
  )
  (func $init
    (call $assert
      (i32.const 1)
      (i32.const 8240)
    )
    (i64.store offset=8320
      (i32.const 0)
      (i64.const 1000000)
    )
    (i32.store16 offset=8336
      (i32.const 0)
      (i32.load16_u offset=8376 align=1
        (i32.const 0)
      )
    )
    (i64.store offset=8328
      (i32.const 0)
      (i64.load offset=8368 align=1
        (i32.const 0)
      )
    )
    (call $store
      (i32.const 8328)
      (i32.const 32)
      (i32.const 8320)
      (i32.const 8)
    )
  )
  (func $apply_simplecoin_transfer
    (local $0 i32)
    (local $1 i32)
    (local $2 i64)
    (set_local $0
      (call $read_message
        (i32.const 8384)
        (i32.const 100)
      )
    )
    (i32.store offset=8568
      (i32.const 0)
      (i32.const 8384)
    )
    (i32.store offset=8572
      (i32.const 0)
      (i32.const 8384)
    )
    (i32.store offset=8576
      (i32.const 0)
      (i32.add
        (get_local $0)
        (i32.const 8384)
      )
    )
    (call $account_name_unpack
      (i32.const 8568)
      (i32.const 8488)
    )
    (call $account_name_unpack
      (i32.const 8568)
      (i32.const 8520)
    )
    (call $assert
      (i32.le_u
        (i32.add
          (i32.load offset=8572
            (i32.const 0)
          )
          (i32.const 8)
        )
        (i32.load offset=8576
          (i32.const 0)
        )
      )
      (i32.const 8288)
    )
    (i64.store offset=8552
      (i32.const 0)
      (i64.load align=1
        (tee_local $0
          (i32.load offset=8572
            (i32.const 0)
          )
        )
      )
    )
    (i32.store offset=8572
      (i32.const 0)
      (i32.add
        (get_local $0)
        (i32.const 8)
      )
    )
    (call $Varint_unpack
      (i32.const 8568)
      (i32.const 8312)
    )
    (call $assert
      (i32.le_u
        (i32.add
          (i32.load offset=8572
            (i32.const 0)
          )
          (i32.load offset=8312
            (i32.const 0)
          )
        )
        (i32.load offset=8576
          (i32.const 0)
        )
      )
      (i32.const 8288)
    )
    (i32.store offset=8208
      (i32.const 0)
      (tee_local $1
        (i32.add
          (i32.and
            (i32.add
              (i32.load offset=8312
                (i32.const 0)
              )
              (i32.const 11)
            )
            (i32.const -8)
          )
          (tee_local $0
            (i32.load offset=8208
              (i32.const 0)
            )
          )
        )
      )
    )
    (call $assert
      (i32.lt_u
        (get_local $1)
        (i32.const 8192)
      )
      (i32.const 8224)
    )
    (drop
      (call $memcpy
        (i32.add
          (get_local $0)
          (i32.const 20)
        )
        (i32.load offset=8572
          (i32.const 0)
        )
        (i32.load offset=8312
          (i32.const 0)
        )
      )
    )
    (i64.store offset=8592
      (i32.const 0)
      (i64.const 0)
    )
    (i32.store offset=8560
      (i32.const 0)
      (i32.add
        (get_local $0)
        (i32.const 16)
      )
    )
    (call $assert
      (i32.eq
        (call $load
          (i32.const 8488)
          (i32.const 32)
          (i32.const 8584)
          (i32.const 8)
        )
        (i32.const 8)
      )
      (i32.const 8608)
    )
    (call $assert
      (i64.ge_s
        (i64.load offset=8584
          (i32.const 0)
        )
        (i64.load offset=8552
          (i32.const 0)
        )
      )
      (i32.const 8640)
    )
    (drop
      (call $load
        (i32.const 8520)
        (i32.const 32)
        (i32.const 8592)
        (i32.const 8)
      )
    )
    (i64.store offset=8592
      (i32.const 0)
      (i64.add
        (i64.load offset=8592
          (i32.const 0)
        )
        (tee_local $2
          (i64.load offset=8552
            (i32.const 0)
          )
        )
      )
    )
    (i64.store offset=8584
      (i32.const 0)
      (tee_local $2
        (i64.sub
          (i64.load offset=8584
            (i32.const 0)
          )
          (get_local $2)
        )
      )
    )
    (block $label$0
      (br_if $label$0
        (f64.lt
          (f64.add
            (f64.convert_s/i64
              (get_local $2)
            )
            (f64.const 0.5)
          )
          (f64.const 50.5)
        )
      )
      (block $label$1
        (br_if $label$1
          (i64.eqz
            (get_local $2)
          )
        )
        (call $store
          (i32.const 8488)
          (i32.const 32)
          (i32.const 8584)
          (i32.const 8)
        )
        (br $label$0)
      )
      (drop
        (call $remove
          (i32.const 8488)
          (i32.const 32)
        )
      )
    )
    (call $store
      (i32.const 8520)
      (i32.const 32)
      (i32.const 8592)
      (i32.const 8)
    )
  )
)

)";

      types::setcode handler;
      handler.account = "simplecoin";

      auto wasm = assemble_wast( wast_apply );
      handler.code.resize(wasm.size());
      memcpy( handler.code.data(), wasm.data(), wasm.size() );

      eosio::chain::signed_transaction trx;
      trx.scope = {"simplecoin"};
      trx.messages.resize(1);
      trx.messages[0].code = config::eos_contract_name;
      trx.messages[0].authorization.emplace_back(types::account_permission{"simplecoin","active"});
      transaction_set_message(trx, 0, "setcode", handler);
      trx.expiration = chain.head_block_time() + 100;
      transaction_set_reference_block(trx, chain.head_block_id());
      try {
         chain.push_transaction(trx);
         BOOST_FAIL("floating point instructions should be rejected");
/*      } catch (const Serialization::FatalSerializationException& fse) {
         BOOST_CHECK_EQUAL("float instructions not allowed", fse.message);
*/    } catch (const std::exception& exp) {
        BOOST_FAIL("Serialization::FatalSerializationException does not inherit from std::exception");
      } catch (...) {
        // empty throw expected, since
      }
} FC_LOG_AND_RETHROW() }

//Test account script float rejection
BOOST_FIXTURE_TEST_CASE(create_script_w_loop, testing_fixture)
{ try {
      Make_Blockchain(chain);
      chain.produce_blocks(10);
      Make_Account(chain, currency);
      chain.produce_blocks(1);


      types::setcode handler;
      handler.account = "currency";

      auto wasm = assemble_wast( infinite_wast );
      handler.code.resize(wasm.size());
      memcpy( handler.code.data(), wasm.data(), wasm.size() );

      {
         eosio::chain::signed_transaction trx;
         trx.scope = {"currency"};
         trx.messages.resize(1);
         trx.messages[0].code = config::eos_contract_name;
         trx.messages[0].authorization.emplace_back(types::account_permission{"currency","active"});
         transaction_set_message(trx, 0, "setcode", handler);
         trx.expiration = chain.head_block_time() + 100;
         transaction_set_reference_block(trx, chain.head_block_id());
         chain.push_transaction(trx);
         chain.produce_blocks(1);
      }


      {
         eosio::chain::signed_transaction trx;
         trx.scope = sort_names({"currency","inita"});
         transaction_emplace_message(trx, "currency",
                            vector<types::account_permission>{ {"currency","active"} },
                            "transfer", types::transfer{"currency", "inita", 1,""});
         trx.expiration = chain.head_block_time() + 100;
         transaction_set_reference_block(trx, chain.head_block_id());
         try
         {
            wlog("starting long transaction");
            chain.push_transaction(trx);
            BOOST_FAIL("transaction should have failed with checktime_exceeded");
         }
         catch (const eosio::chain::checktime_exceeded& check)
         {
            wlog("checktime_exceeded caught");
         }
      }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
