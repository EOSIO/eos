/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

using namespace eos;
using namespace chain;

BOOST_AUTO_TEST_SUITE(slow_tests)

// Test that TaPoS still works after block 65535 (See Issue #55)
BOOST_FIXTURE_TEST_CASE(tapos_wrap, testing_fixture)
{ try {
      Make_Blockchain(chain)
      Make_Account(chain, acct);
      Transfer_Asset(chain, system, acct, Asset(5));
      Stake_Asset(chain, acct, Asset(5).amount);
      wlog("Hang on, this will take a minute...");
      chain.produce_blocks(65536);
      Begin_Unstake_Asset(chain, acct, Asset(1).amount);
} FC_LOG_AND_RETHROW() }

// Verify that staking and unstaking works
BOOST_FIXTURE_TEST_CASE(stake, testing_fixture)
{ try {
   // Create account sam with default balance of 100, and stake 55 of it
   Make_Blockchain(chain);
   Make_Account(chain, sam);
   Transfer_Asset(chain, inita, sam, Asset(55) );

   // MakeAccount should start sam out with some staked balance
   BOOST_REQUIRE_EQUAL(chain.get_staked_balance("sam"), Asset(100).amount);

   Stake_Asset(chain, sam, Asset(55).amount);

   // Check balances
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), Asset(155).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), Asset(0).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), Asset(0).amount);

   chain.produce_blocks();

   // Start unstaking 20, check balances
   BOOST_CHECK_THROW(Begin_Unstake_Asset(chain, sam, Asset(156).amount), chain::message_precondition_exception);
   Begin_Unstake_Asset(chain, sam, Asset(20).amount);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), Asset(135).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), Asset(20).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), Asset(0).amount);

   // Make sure we can't liquidate early
   BOOST_CHECK_THROW(Finish_Unstake_Asset(chain, sam, Asset(10).amount), chain::message_precondition_exception);

   // Fast forward to when we can liquidate
   wlog("Hang on, this will take a minute...");
   chain.produce_blocks(config::StakedBalanceCooldownSeconds / config::BlockIntervalSeconds + 1);

   BOOST_CHECK_THROW(Finish_Unstake_Asset(chain, sam, Asset(21).amount), chain::message_precondition_exception);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), Asset(135).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), Asset(20).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), Asset(0).amount);

   // Liquidate 10 of the 20 unstaking and check balances
   Finish_Unstake_Asset(chain, sam, Asset(10).amount);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), Asset(135).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), Asset(10).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), Asset(10).amount);

   // Liquidate 2 of the 10 left unstaking and check balances
   Finish_Unstake_Asset(chain, sam, Asset(2).amount);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), Asset(135).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), Asset(8).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), Asset(12).amount);

   // Ignore the 8 left in unstaking, and begin unstaking 5, which should restake the 8, and start over unstaking 5
   Begin_Unstake_Asset(chain, sam, Asset(5).amount);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), Asset(138).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), Asset(5).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), Asset(12).amount);

   // Begin unstaking 20, which should only deduct 15 from staked, since 5 was already in unstaking
   Begin_Unstake_Asset(chain, sam, Asset(20).amount);
   BOOST_CHECK_EQUAL(chain.get_staked_balance("sam"), Asset(123).amount);
   BOOST_CHECK_EQUAL(chain.get_unstaking_balance("sam"), Asset(20).amount);
   BOOST_CHECK_EQUAL(chain.get_liquid_balance("sam"), Asset(12).amount);
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

//Test account script processing
BOOST_FIXTURE_TEST_CASE(create_script, testing_fixture)
{ try {
      Make_Blockchain(chain);
      chain.produce_blocks(10);
      Make_Account(chain, simplecoin);
      chain.produce_blocks(1);

#include "wast/simplecoin.wast"

      types::setcode handler;
      handler.account = "simplecoin";

      auto wasm = assemble_wast( simplecoin_wast );
      handler.code.resize(wasm.size());
      memcpy( handler.code.data(), wasm.data(), wasm.size() );

      {
         eos::chain::SignedTransaction trx;
         trx.messages.resize(1);
         trx.messages[0].code = config::SystemContractName;
         trx.messages[0].recipients = {"simplecoin"};
         trx.setMessage(0, "setcode", handler);
         trx.expiration = chain.head_block_time() + 100;
         trx.set_reference_block(chain.head_block_id());
         chain.push_transaction(trx);
         chain.produce_blocks(1);
      }


      auto start = fc::time_point::now();
      for (uint32_t i = 0; i < 100000; ++i)
      {
         eos::chain::SignedTransaction trx;
         trx.emplaceMessage("simplecoin", vector<AccountName>{"inita"},
                            vector<types::AccountPermission>{},
                            "transfer", types::transfer{"simplecoin", "inita", 1+i, "hello"});
         trx.expiration = chain.head_block_time() + 100;
         trx.set_reference_block(chain.head_block_id());
         chain.push_transaction(trx);
      }
      auto end = fc::time_point::now();
      idump((100000*1000000.0 / (end-start).count()));

      chain.produce_blocks(10);

} FC_LOG_AND_RETHROW() }

//Test account script float rejection
BOOST_FIXTURE_TEST_CASE(create_script_w_float, testing_fixture)
{ try {
      Make_Blockchain(chain);
      chain.produce_blocks(10);
      Make_Account(chain, simplecoin);
      chain.produce_blocks(1);



/*
      auto c_apply = R"(
typedef long long    uint64_t;
typedef unsigned int uint32_t;

void print( char* string, int length );
void printi( int );
void printi64( uint64_t );
void assert( int test, char* message );
void store( const char* key, int keylength, const char* value, int valuelen );
int load( const char* key, int keylength, char* value, int maxlen );
int remove( const char* key, int keyLength );
void* memcpy( void* dest, const void* src, uint32_t size );
int readMessage( char* dest, int destsize );

void* malloc( unsigned int size ) {
    static char dynamic_memory[1024*8];
    static int  start = 0;
    int old_start = start;
    start +=  8*((size+7)/8);
    assert( start < sizeof(dynamic_memory), "out of memory" );
    return &dynamic_memory[old_start];
}


typedef struct {
  uint64_t   name[4];
} AccountName;

typedef struct {
  uint32_t  length;
  char      data[];
} String;


typedef struct {
   char* start;
   char* pos;
   char* end;
} DataStream;

void DataStream_init( DataStream* ds, char* start, int length ) {
  ds->start = start;
  ds->end   = start + length;
  ds->pos   = start;
}
void AccountName_initString( AccountName* a, String* s ) {
  assert( s->length <= sizeof(AccountName), "String is longer than account name allows" );
  memcpy( a, s->data, s->length );
}
void AccountName_initCString( AccountName* a, const char* s, uint32_t len ) {
  assert( len <= sizeof(AccountName), "String is longer than account name allows" );
  memcpy( a, s, len );
}

void AccountName_unpack( DataStream* ds, AccountName* account );
void uint64_unpack( DataStream* ds, uint64_t* value ) {
  assert( ds->pos + sizeof(uint64_t) <= ds->end, "read past end of stream" );
  memcpy( (char*)value, ds->pos, 8 );
  ds->pos += sizeof(uint64_t);
}
void Varint_unpack( DataStream* ds, uint32_t* value );
void String_unpack( DataStream* ds, String** value ) {
   static uint32_t size;
   Varint_unpack( ds, &size );
   assert( ds->pos + size <= ds->end, "read past end of stream");
   String* str = (String*)malloc( size + sizeof(String) );
   memcpy( str->data, ds->pos, size );
   *value = str;
}

/// END BUILT IN LIBRARY.... everything below this is "user contract"


typedef struct  {
  AccountName from;
  AccountName to;
  uint64_t    amount;
  String*     memo;
} Transfer;

void Transfer_unpack( DataStream* ds, Transfer* transfer )
{
   AccountName_unpack( ds, &transfer->from );
   AccountName_unpack( ds, &transfer->to   );
   uint64_unpack( ds, &transfer->amount );
   String_unpack( ds, &transfer->memo );
}

typedef struct {
  uint64_t    balance;
} Balance;

void init() {
  static Balance initial;
  static AccountName simplecoin;
  AccountName_initCString( &simplecoin, "simplecoin", 10 );
  initial.balance = 1000*1000;

  store( (const char*)&simplecoin, sizeof(AccountName), (const char*)&initial, sizeof(Balance));
}


void apply_simplecoin_transfer() {
   static char buffer[100];

   int read   = readMessage( buffer, 100  );
   static Transfer message;
   static DataStream ds;
   DataStream_init( &ds, buffer, read );
   Transfer_unpack( &ds, &message );

   static Balance from_balance;
   static Balance to_balance;
   to_balance.balance = 0;

   read = load( (const char*)&message.from, sizeof(message.from), (char*)&from_balance.balance,
sizeof(from_balance.balance) );
   assert( read == sizeof(Balance), "no existing balance" );
   assert( from_balance.balance >= message.amount, "insufficient funds" );
   read = load( (const char*)&message.to, sizeof(message.to), (char*)&to_balance.balance, sizeof(to_balance.balance) );

   to_balance.balance   += message.amount;
   from_balance.balance -= message.amount;

   double bal = to_balance.balance;
   if( bal + 0.5 < 50.5 )
     return;

   if( from_balance.balance )
      store( (const char*)&message.from, sizeof(AccountName), (const char*)&from_balance.balance,
sizeof(from_balance.balance) );
   else
      remove( (const char*)&message.from, sizeof(AccountName) );

   store( (const char*)&message.to, sizeof(message.to), (const char*)&to_balance.balance, sizeof(to_balance.balance) );
}

");
*/
std::string wast_apply =
R"(
(module
  (type $FUNCSIG$vii (func (param i32 i32)))
  (type $FUNCSIG$viiii (func (param i32 i32 i32 i32)))
  (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
  (type $FUNCSIG$iiiii (func (param i32 i32 i32 i32) (result i32)))
  (type $FUNCSIG$iiii (func (param i32 i32 i32) (result i32)))
  (import "env" "AccountName_unpack" (func $AccountName_unpack (param i32 i32)))
  (import "env" "Varint_unpack" (func $Varint_unpack (param i32 i32)))
  (import "env" "assert" (func $assert (param i32 i32)))
  (import "env" "load" (func $load (param i32 i32 i32 i32) (result i32)))
  (import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
  (import "env" "readMessage" (func $readMessage (param i32 i32) (result i32)))
  (import "env" "remove" (func $remove (param i32 i32) (result i32)))
  (import "env" "store" (func $store (param i32 i32 i32 i32)))
  (table 0 anyfunc)
  (memory $0 1)
  (data (i32.const 8224) "out of memory\00")
  (data (i32.const 8240) "String is longer than account name allows\00")
  (data (i32.const 8288) "read past end of stream\00")
  (data (i32.const 8368) "simplecoin\00")
  (data (i32.const 8608) "no existing balance\00")
  (data (i32.const 8640) "insufficient funds\00")
  (export "memory" (memory $0))
  (export "malloc" (func $malloc))
  (export "DataStream_init" (func $DataStream_init))
  (export "AccountName_initString" (func $AccountName_initString))
  (export "AccountName_initCString" (func $AccountName_initCString))
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
  (func $AccountName_initString (param $0 i32) (param $1 i32)
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
  (func $AccountName_initCString (param $0 i32) (param $1 i32) (param $2 i32)
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
    (call $AccountName_unpack
      (get_local $0)
      (get_local $1)
    )
    (call $AccountName_unpack
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
      (call $readMessage
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
    (call $AccountName_unpack
      (i32.const 8568)
      (i32.const 8488)
    )
    (call $AccountName_unpack
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

      eos::chain::SignedTransaction trx;
      trx.messages.resize(1);
      trx.messages[0].code = config::SystemContractName;
      trx.messages[0].recipients = {"simplecoin"};
      trx.setMessage(0, "setcode", handler);
      trx.expiration = chain.head_block_time() + 100;
      trx.set_reference_block(chain.head_block_id());
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
      Make_Account(chain, simplecoin);
      chain.produce_blocks(1);

#include "wast/loop.wast"

   types::setcode handler;
   handler.account = "simplecoin";

   auto wasm = assemble_wast( wast_apply );
   handler.code.resize(wasm.size());
   memcpy( handler.code.data(), wasm.data(), wasm.size() );

   {
      eos::chain::SignedTransaction trx;
      trx.messages.resize(1);
      trx.messages[0].code = config::SystemContractName;
      trx.messages[0].recipients = {"simplecoin"};
      trx.setMessage(0, "setcode", handler);
      trx.expiration = chain.head_block_time() + 100;
      trx.set_reference_block(chain.head_block_id());
      wlog("push_transaction 1");
      chain.push_transaction(trx);
      wlog("produce_blocks 1");
      chain.produce_blocks(1);
      wlog("produce_blocks 1");
   }


   auto start = fc::time_point::now();
   {
      eos::chain::SignedTransaction trx;
      trx.emplaceMessage("simplecoin", vector<AccountName>{"inita"},
                         vector<types::AccountPermission>{},
                         "transfer", types::transfer{"simplecoin", "inita", 1, "hello"});
      trx.expiration = chain.head_block_time() + 100;
      trx.set_reference_block(chain.head_block_id());
      try
      {
         wlog("starting long transaction");
         chain.push_transaction(trx);
         BOOST_FAIL("transaction should have failed with checktime_exceeded");
      }
      catch (const eos::chain::checktime_exceeded& check)
      {
         wlog("checktime_exceeded caught");
      }
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
