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

BOOST_AUTO_TEST_SUITE(block_tests)

// Simple test of block production and head_block_num tracking
BOOST_FIXTURE_TEST_CASE(produce_blocks, testing_fixture)
{ try {
      Make_Database(db)

      BOOST_CHECK_EQUAL(db.head_block_num(), 0);
      db.produce_blocks();
      BOOST_CHECK_EQUAL(db.head_block_num(), 1);
      db.produce_blocks(5);
      BOOST_CHECK_EQUAL(db.head_block_num(), 6);
      db.produce_blocks(db.get_global_properties().active_producers.size());
      BOOST_CHECK_EQUAL(db.head_block_num(), db.get_global_properties().active_producers.size() + 6);
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(order_dependent_transactions, testing_fixture)
{ try {
      Make_Database(db);
      db.produce_blocks(10);

      Make_Account(db, newguy);

      Transfer_Asset(db, newguy, init0, Asset(1));
      BOOST_CHECK_EQUAL(db.get_liquid_balance("newguy"), Asset(99));
      BOOST_CHECK_EQUAL(db.get_liquid_balance("init0"), Asset(100000-99));

      db.produce_blocks();
      BOOST_CHECK_EQUAL(db.head_block_num(), 11);
      BOOST_CHECK(db.fetch_block_by_number(11).valid());
      BOOST_CHECK(!db.fetch_block_by_number(11)->cycles.empty());
      BOOST_CHECK(!db.fetch_block_by_number(11)->cycles.front().empty());
      BOOST_CHECK_EQUAL(db.fetch_block_by_number(11)->cycles.front().front().user_input.size(), 2);
      BOOST_CHECK_EQUAL(db.get_liquid_balance("newguy"), Asset(99));
      BOOST_CHECK_EQUAL(db.get_liquid_balance("init0"), Asset(100000-99));
} FC_LOG_AND_RETHROW() }


vector<uint8_t> assemble_wast( const std::string& wast ) {
   std::cout << "\n" << wast << "\n";
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
      Make_Database(db);
      db.produce_blocks(10);
      Make_Account(db, simplecoin);
      db.produce_blocks(1);



/*
      auto c_apply = R"(
/// Start the EOS Built In Library HERE
typedef long long    uint64_t;
typedef unsigned int uint32_t;

void print( char* string, int length );
void printi( int );
void printi64( uint64_t );
void assert( int test, char* message );
void store( const char* key, int keylength, const char* value, int valuelen );
int load( const char* key, int keylength, char* value, int maxlen );
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

void onInit() {
  static Balance initial;
  static AccountName simplecoin;
  AccountName_initCString( &simplecoin, "simplecoin", 10 );
  initial.balance = 1000*1000;
  
  store( &simplecoin, sizeof(AccountName), &initial, sizeof(Balance));
}


void onApply_Transfer_simplecoin() {
   static char buffer[100];

   int read   = readMessage( buffer, 100  );
   static Transfer message;
   static DataStream ds;
   DataStream_init( &ds, buffer, read );
   Transfer_unpack( &ds, &message );

   static Balance from_balance;
   static Balance to_balance;
   to_balance.balance = 0;
   
   read = load( (char*)&message.from, sizeof(message.from), (char*)&from_balance.balance, sizeof(from_balance.balance) );
   assert( read == sizeof(Balance), "no existing balance" );
   assert( from_balance.balance >= message.amount, "insufficient funds" );
   read = load( (char*)&message.to, sizeof(message.to), (char*)&from_balance.balance, sizeof(from_balance.balance) );
   
   to_balance.balance   += message.amount;
   from_balance.balance -= message.amount;

   if( from_balance.balance )
      store( (char*)&message.from, sizeof(message.from), (char*)&from_balance.balance, sizeof(from_balance.balance) );
   else
      remove( (char*)&message.from, sizeof(message.from) );
 
   store( (char*)&message.to, sizeof(message.to), (char*)&to_balance.balance, sizeof(to_balance.balance) );
}
      )";
*/
std::string wast_apply = 
R"(
(module
  (type $FUNCSIG$vii (func (param i32 i32)))
  (type $FUNCSIG$viiii (func (param i32 i32 i32 i32)))
  (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
  (type $FUNCSIG$vj (func (param i64)))
  (type $FUNCSIG$iiiii (func (param i32 i32 i32 i32) (result i32)))
  (type $FUNCSIG$i (func (result i32)))
  (type $FUNCSIG$iiii (func (param i32 i32 i32) (result i32)))
  (import "env" "AccountName_unpack" (func $AccountName_unpack (param i32 i32)))
  (import "env" "Varint_unpack" (func $Varint_unpack (param i32 i32)))
  (import "env" "assert" (func $assert (param i32 i32)))
  (import "env" "load" (func $load (param i32 i32 i32 i32) (result i32)))
  (import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
  (import "env" "print" (func $print (param i32 i32)))
  (import "env" "printi64" (func $printi64 (param i64)))
  (import "env" "readMessage" (func $readMessage (param i32 i32) (result i32)))
  (import "env" "remove" (func $remove (param i32 i32) (result i32)))
  (import "env" "store" (func $store (param i32 i32 i32 i32)))
  (table 0 anyfunc)
  (memory $0 1)
  (data (i32.const 8224) "out of memory\00")
  (data (i32.const 8240) "String is longer than account name allows\00")
  (data (i32.const 8288) "read past end of stream\00")
  (data (i32.const 8368) "simplecoin\00")
  (data (i32.const 8384) "onInit simiplecoin\00")
  (data (i32.const 8416) "onApply_Transfer_simplecoin\00")
  (data (i32.const 8672) "no existing balance\00")
  (data (i32.const 8704) "insufficient funds\00")
  (export "memory" (memory $0))
  (export "malloc" (func $malloc))
  (export "DataStream_init" (func $DataStream_init))
  (export "AccountName_initString" (func $AccountName_initString))
  (export "AccountName_initCString" (func $AccountName_initCString))
  (export "uint64_unpack" (func $uint64_unpack))
  (export "String_unpack" (func $String_unpack))
  (export "Transfer_unpack" (func $Transfer_unpack))
  (export "onInit" (func $onInit))
  (export "onApply_Transfer_simplecoin" (func $onApply_Transfer_simplecoin))
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
  (func $onInit
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
    (call $print
      (i32.const 8384)
      (i32.const 15)
    )
  )
  (func $onApply_Transfer_simplecoin
    (local $0 i32)
    (local $1 i32)
    (local $2 i64)
    (call $print
      (i32.const 8416)
      (i32.const 20)
    )
    (set_local $0
      (call $readMessage
        (i32.const 8448)
        (i32.const 100)
      )
    )
    (i32.store offset=8632
      (i32.const 0)
      (i32.const 8448)
    )
    (i32.store offset=8636
      (i32.const 0)
      (i32.const 8448)
    )
    (i32.store offset=8640
      (i32.const 0)
      (i32.add
        (get_local $0)
        (i32.const 8448)
      )
    )
    (call $AccountName_unpack
      (i32.const 8632)
      (i32.const 8552)
    )
    (call $AccountName_unpack
      (i32.const 8632)
      (i32.const 8584)
    )
    (call $assert
      (i32.le_u
        (i32.add
          (i32.load offset=8636
            (i32.const 0)
          )
          (i32.const 8)
        )
        (i32.load offset=8640
          (i32.const 0)
        )
      )
      (i32.const 8288)
    )
    (i64.store offset=8616
      (i32.const 0)
      (i64.load align=1
        (tee_local $0
          (i32.load offset=8636
            (i32.const 0)
          )
        )
      )
    )
    (i32.store offset=8636
      (i32.const 0)
      (i32.add
        (get_local $0)
        (i32.const 8)
      )
    )
    (call $Varint_unpack
      (i32.const 8632)
      (i32.const 8312)
    )
    (call $assert
      (i32.le_u
        (i32.add
          (i32.load offset=8636
            (i32.const 0)
          )
          (i32.load offset=8312
            (i32.const 0)
          )
        )
        (i32.load offset=8640
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
        (i32.load offset=8636
          (i32.const 0)
        )
        (i32.load offset=8312
          (i32.const 0)
        )
      )
    )
    (i32.store offset=8624
      (i32.const 0)
      (i32.add
        (get_local $0)
        (i32.const 16)
      )
    )
    (call $print
      (i32.const 8552)
      (i32.const 32)
    )
    (call $print
      (i32.const 8584)
      (i32.const 32)
    )
    (call $printi64
      (i64.load offset=8616
        (i32.const 0)
      )
    )
    (i64.store offset=8656
      (i32.const 0)
      (i64.const 0)
    )
    (call $assert
      (i32.eq
        (call $load
          (i32.const 8552)
          (i32.const 32)
          (i32.const 8648)
          (i32.const 8)
        )
        (i32.const 8)
      )
      (i32.const 8672)
    )
    (call $assert
      (i64.ge_s
        (i64.load offset=8648
          (i32.const 0)
        )
        (i64.load offset=8616
          (i32.const 0)
        )
      )
      (i32.const 8704)
    )
    (drop
      (call $load
        (i32.const 8584)
        (i32.const 32)
        (i32.const 8648)
        (i32.const 8)
      )
    )
    (i64.store offset=8656
      (i32.const 0)
      (i64.add
        (i64.load offset=8656
          (i32.const 0)
        )
        (tee_local $2
          (i64.load offset=8616
            (i32.const 0)
          )
        )
      )
    )
    (i64.store offset=8648
      (i32.const 0)
      (tee_local $2
        (i64.sub
          (i64.load offset=8648
            (i32.const 0)
          )
          (get_local $2)
        )
      )
    )
    (block $label$0
      (block $label$1
        (br_if $label$1
          (i64.eqz
            (get_local $2)
          )
        )
        (call $store
          (i32.const 8552)
          (i32.const 32)
          (i32.const 8648)
          (i32.const 8)
        )
        (br $label$0)
      )
      (drop
        (call $remove
          (i32.const 8552)
          (i32.const 32)
        )
      )
    )
    (call $store
      (i32.const 8584)
      (i32.const 32)
      (i32.const 8656)
      (i32.const 8)
    )
  )
)

)";

      types::SetCode handler;
      handler.account = "simplecoin";
      types::DefineStruct interface;
      interface.scope = "simplecoin";
      interface.definition = types::GetStruct<types::Transfer>::type();

      auto wasm = assemble_wast( wast_apply );
      handler.code.resize(wasm.size());
      memcpy( handler.code.data(), wasm.data(), wasm.size() );

      {
				 eos::chain::SignedTransaction trx; 
         trx.messages.resize(2);
         trx.messages[0].sender = "simplecoin";
         trx.messages[0].recipient = "sys";
				 trx.setMessage(0, "DefineStruct",interface);
         trx.messages[1].sender = "simplecoin";
         trx.messages[1].recipient = "sys";
				 trx.setMessage(1, "SetCode", handler);
         trx.expiration = db.head_block_time() + 100; 
         trx.set_reference_block(db.head_block_id()); 
				 db.push_transaction(trx);
				 db.produce_blocks(1);
      }


     { 
        eos::chain::SignedTransaction trx; 
        trx.emplaceMessage("simplecoin", "simplecoin", vector<AccountName>{"init1"}, "Transfer", 
                           types::Transfer{"simplecoin", "init1", 101, "hello"} ); 
        trx.expiration = db.head_block_time() + 100; 
        trx.set_reference_block(db.head_block_id()); 
        db.push_transaction(trx); 
     }
     { 
        wlog( "transfer 102 from init1 to init2" );
        eos::chain::SignedTransaction trx; 
        trx.emplaceMessage("init1", "simplecoin", vector<AccountName>{"init2"}, "Transfer", 
                           types::Transfer{"init1", "init2", 102, "hello again"} ); 
        trx.expiration = db.head_block_time() + 100; 
        trx.set_reference_block(db.head_block_id()); 
        db.push_transaction(trx); 
     }



/*
			auto start = fc::time_point::now();
		  for( uint32_t i = 0; i < 200; ++i ) {
         Transfer_Asset(db, init3, init1, Asset(2+i), "transfer 100");
      }
			auto end = fc::time_point::now();
			idump((  200*1000000.0 / (end-start).count() ) );
*/
		  db.produce_blocks(1);
/*

      const auto& world = db_db.get<key_value_object,by_scope_key>(boost::make_tuple(AccountName("init1"),
                                                                                     String("hello")));
      BOOST_CHECK_EQUAL( string(world.value.c_str()), "world" );
*/

} FC_LOG_AND_RETHROW() }

// Simple test of block production when a block is missed
BOOST_FIXTURE_TEST_CASE(missed_blocks, testing_fixture)
{ try {
      Make_Database(db)

      db.produce_blocks();
      BOOST_CHECK_EQUAL(db.head_block_num(), 1);

      AccountName skipped_producers[3] = {db.get_scheduled_producer(1),
                                          db.get_scheduled_producer(2),
                                          db.get_scheduled_producer(3)};
      auto next_block_time = db.get_slot_time(4);
      auto next_producer = db.get_scheduled_producer(4);

      BOOST_CHECK_EQUAL(db.head_block_num(), 1);
      db.produce_blocks(1, 3);
      BOOST_CHECK_EQUAL(db.head_block_num(), 2);
      BOOST_CHECK_EQUAL(db.head_block_time().to_iso_string(), next_block_time.to_iso_string());
      BOOST_CHECK_EQUAL(db.head_block_producer(), next_producer);
      BOOST_CHECK_EQUAL(db.get_producer(next_producer).total_missed, 0);

      for (auto producer : skipped_producers) {
         BOOST_CHECK_EQUAL(db.get_producer(producer).total_missed, 1);
      }
} FC_LOG_AND_RETHROW() }

// Simple sanity test of test network: if databases aren't connected to the network, they don't sync to eachother
BOOST_FIXTURE_TEST_CASE(no_network, testing_fixture)
{ try {
      Make_Databases((db1)(db2))

      BOOST_CHECK_EQUAL(db1.head_block_num(), 0);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 0);
      db1.produce_blocks();
      BOOST_CHECK_EQUAL(db1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 0);
      db2.produce_blocks(5);
      BOOST_CHECK_EQUAL(db1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 5);
} FC_LOG_AND_RETHROW() }

// Test that two databases on the same network do sync to eachother
BOOST_FIXTURE_TEST_CASE(simple_network, testing_fixture)
{ try {
      Make_Databases((db1)(db2))
      Make_Network(net, (db1)(db2))

      BOOST_CHECK_EQUAL(db1.head_block_num(), 0);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 0);
      db1.produce_blocks();
      BOOST_CHECK_EQUAL(db1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 1);
      BOOST_CHECK_EQUAL(db1.head_block_id().str(), db2.head_block_id().str());
      db2.produce_blocks(5);
      BOOST_CHECK_EQUAL(db1.head_block_num(), 6);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 6);
      BOOST_CHECK_EQUAL(db1.head_block_id().str(), db2.head_block_id().str());
} FC_LOG_AND_RETHROW() }

// Test that two databases joining and leaving a network sync correctly after a fork
BOOST_FIXTURE_TEST_CASE(forked_network, testing_fixture)
{ try {
      Make_Databases((db1)(db2))
      Make_Network(net)

      BOOST_CHECK_EQUAL(db1.head_block_num(), 0);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 0);
      db1.produce_blocks();
      BOOST_CHECK_EQUAL(db1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 0);
      BOOST_CHECK_NE(db1.head_block_id().str(), db2.head_block_id().str());

      net.connect_database(db1);
      net.connect_database(db2);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 1);
      BOOST_CHECK_EQUAL(db1.head_block_id().str(), db2.head_block_id().str());

      db2.produce_blocks(5);
      BOOST_CHECK_EQUAL(db1.head_block_num(), 6);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 6);
      BOOST_CHECK_EQUAL(db1.head_block_id().str(), db2.head_block_id().str());

      net.disconnect_database(db1);
      db1.produce_blocks(1, 1);
      db2.produce_blocks();
      BOOST_CHECK_EQUAL(db1.head_block_num(), 7);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 7);
      BOOST_CHECK_NE(db1.head_block_id().str(), db2.head_block_id().str());

      db2.produce_blocks(1, 1);
      net.connect_database(db1);
      BOOST_CHECK_EQUAL(db1.head_block_num(), 8);
      BOOST_CHECK_EQUAL(db2.head_block_num(), 8);
      BOOST_CHECK_EQUAL(db1.head_block_id().str(), db2.head_block_id().str());
} FC_LOG_AND_RETHROW() }

// Check that the recent_slots_filled bitmap is being updated correctly
BOOST_FIXTURE_TEST_CASE( rsf_missed_blocks, testing_fixture )
{ try {
      Make_Database(db)
      db.produce_blocks();

      auto rsf = [&]() -> string
      {
         auto rsf = db.get_dynamic_global_properties().recent_slots_filled;
         string result = "";
         result.reserve(64);
         for( int i=0; i<64; i++ )
         {
            result += ((rsf & 1) == 0) ? '0' : '1';
            rsf >>= 1;
         }
         return result;
      };

      auto pct = []( uint32_t x ) -> uint32_t
      {
         return uint64_t( config::Percent100 ) * x / 64;
      };

      BOOST_CHECK_EQUAL( rsf(),
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), config::Percent100 );

      db.produce_blocks(1, 1);
      BOOST_CHECK_EQUAL( rsf(),
         "0111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(127-64) );

      db.produce_blocks(1, 1);
      BOOST_CHECK_EQUAL( rsf(),
         "0101111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(126-64) );

      db.produce_blocks(1, 2);
      BOOST_CHECK_EQUAL( rsf(),
         "0010101111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(124-64) );

      db.produce_blocks(1, 3);
      BOOST_CHECK_EQUAL( rsf(),
         "0001001010111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(121-64) );

      db.produce_blocks(1, 5);
      BOOST_CHECK_EQUAL( rsf(),
         "0000010001001010111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(116-64) );

      db.produce_blocks(1, 8);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000010000010001001010111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(108-64) );

      db.produce_blocks(1, 13);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000100000000100000100010010101111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(95-64) );

      db.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1000000000000010000000010000010001001010111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(95-64) );

      db.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1100000000000001000000001000001000100101011111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(95-64) );

      db.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1110000000000000100000000100000100010010101111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(95-64) );

      db.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1111000000000000010000000010000010001001010111111111111111111111"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(95-64) );

      db.produce_blocks(1, 64);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000000000000000000000000000000000000"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(0) );

      db.produce_blocks(1, 63);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000000000000000000000000000000000001"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(1) );

      db.produce_blocks(1, 32);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000010000000000000000000000000000000"
      );
      BOOST_CHECK_EQUAL( db.producer_participation_rate(), pct(1) );
} FC_LOG_AND_RETHROW() }

// Check that a db rewinds to the LIB after being closed and reopened
BOOST_FIXTURE_TEST_CASE(restart_db, testing_fixture)
{ try {
      auto lag = EOS_PERCENT(config::BlocksPerRound, config::IrreversibleThresholdPercent);
      {
         Make_Database(db, x);

         db.produce_blocks(20);

         BOOST_CHECK_EQUAL(db.head_block_num(), 20);
         BOOST_CHECK_EQUAL(db.last_irreversible_block_num(), 20 - lag);
      }

      {
         Make_Database(db, x);

         // After restarting, we should have rewound to the last irreversible block.
         BOOST_CHECK_EQUAL(db.head_block_num(), 20 - lag);
         db.produce_blocks(5);
         BOOST_CHECK_EQUAL(db.head_block_num(), 25 - lag);
      }
} FC_LOG_AND_RETHROW() }

// Check that a db which is closed and reopened successfully syncs back with the network, including retrieving blocks
// that it missed while it was down
BOOST_FIXTURE_TEST_CASE(sleepy_db, testing_fixture)
{ try {
      Make_Database(producer)
      Make_Network(net, (producer))

      auto lag = EOS_PERCENT(config::BlocksPerRound, config::IrreversibleThresholdPercent);
      producer.produce_blocks(20);

      {
         // The new node, sleepy, joins, syncs, disconnects
         Make_Database(sleepy, sleepy)
         net.connect_database(sleepy);
         BOOST_CHECK_EQUAL(producer.head_block_num(), 20);
         BOOST_CHECK_EQUAL(sleepy.head_block_num(), 20);

         net.disconnect_database(sleepy);
      }

      // 5 new blocks are produced
      producer.produce_blocks(5);
      BOOST_CHECK_EQUAL(producer.head_block_num(), 25);

      // Sleepy is reborn! Check that it is now rewound to the LIB...
      Make_Database(sleepy, sleepy)
      BOOST_CHECK_EQUAL(sleepy.head_block_num(), 20 - lag);

      // Reconnect sleepy to the network and check that it syncs up to the present
      net.connect_database(sleepy);
      BOOST_CHECK_EQUAL(sleepy.head_block_num(), 25);
      BOOST_CHECK_EQUAL(sleepy.head_block_id().str(), producer.head_block_id().str());
} FC_LOG_AND_RETHROW() }

// Test reindexing the blockchain
BOOST_FIXTURE_TEST_CASE(reindex, testing_fixture)
{ try {
      auto lag = EOS_PERCENT(config::BlocksPerRound, config::IrreversibleThresholdPercent);
      {
         chainbase::database db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE);
         block_log log(get_temp_dir("log"));
         fork_database fdb;
         native_contract::native_contract_chain_initializer initr(genesis_state());
         testing_database chain(db, fdb, log, initr, *this);

         chain.produce_blocks(100);

         BOOST_CHECK_EQUAL(chain.last_irreversible_block_num(), 100 - lag);
      }

      {
         chainbase::database db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE);
         block_log log(get_temp_dir("log"));
         fork_database fdb;
         native_contract::native_contract_chain_initializer initr(genesis_state());
         testing_database chain(db, fdb, log, initr, *this);

         BOOST_CHECK_EQUAL(chain.head_block_num(), 100 - lag);
         chain.produce_blocks(20);
         BOOST_CHECK_EQUAL(chain.head_block_num(), 120 - lag);
      }
} FC_LOG_AND_RETHROW() }

// Test wiping a database and resyncing with an ongoing network
BOOST_FIXTURE_TEST_CASE(wipe, testing_fixture)
{ try {
      Make_Databases((db1)(db2))
      Make_Network(net, (db1)(db2))
      {
         // Create db3 with a temporary data dir
         Make_Database(db3)
         net.connect_database(db3);

         db1.produce_blocks(3);
         db2.produce_blocks(3);
         BOOST_CHECK_EQUAL(db1.head_block_num(), 6);
         BOOST_CHECK_EQUAL(db2.head_block_num(), 6);
         BOOST_CHECK_EQUAL(db3.head_block_num(), 6);
         BOOST_CHECK_EQUAL(db1.head_block_id().str(), db2.head_block_id().str());
         BOOST_CHECK_EQUAL(db1.head_block_id().str(), db3.head_block_id().str());

         net.disconnect_database(db3);
      }

      {
         // Create new db3 with a new temporary data dir
         Make_Database(db3)
         BOOST_CHECK_EQUAL(db3.head_block_num(), 0);

         net.connect_database(db3);
         BOOST_CHECK_EQUAL(db3.head_block_num(), 6);

         db1.produce_blocks(3);
         db2.produce_blocks(3);
         BOOST_CHECK_EQUAL(db1.head_block_num(), 12);
         BOOST_CHECK_EQUAL(db2.head_block_num(), 12);
         BOOST_CHECK_EQUAL(db3.head_block_num(), 12);
         BOOST_CHECK_EQUAL(db1.head_block_id().str(), db2.head_block_id().str());
         BOOST_CHECK_EQUAL(db1.head_block_id().str(), db3.head_block_id().str());
      }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
