/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "simpledb.hpp"
#include "simpledb.gen.hpp"
#include <eoslib/db.hpp>

extern "C" {
   void init()  {

   }
   
   void apply( uint64_t code, uint64_t action ) {
      if( code == N(simpledb) ) {
         if( action == N(insertkv1) ) {
            // eosc push message simpledb insertkv1 '{"key":"a", "value":"aa"}' -S simpledb
            // eosc get table simpledb simpledb keyvalue1
            auto kv1 = eos::currentMessage<KeyValue1>();
            //eos::print(kv1.key.len, "-", (const char*)kv1.key.str, "->" , kv1.value.len, "-", (const char*)kv1.value.str, "\n");

            //Use kv1 in some way
            Bytes bytes = eos::valueToBytes(kv1);

            uint32_t err = store_str( N(simpledb), N(keyvalue1), (char *)kv1.key.get_data(), kv1.key.get_size(), (char*)bytes.data, bytes.len);
         } else if( action == N(insertkv2) ) {
            // eosc push message simpledb insertkv2 '{"key":"a", "value":{"name":"aaa", "age":10}}' -S simpledb
            // eosc get table simpledb simpledb keyvalue2
            auto kv2 = eos::currentMessage<KeyValue2>();
            //eos::print(kv2.key.len, "-", (const char*)kv2.key.str, "->" , (const char*)kv2.value.name.str, "-", kv2.value.age, "\n");

            //Use kv2 in some way
            Bytes bytes = eos::valueToBytes(kv2);

            uint32_t err = store_str( N(simpledb), N(keyvalue2), (char *)kv2.key.get_data(), kv2.key.get_size(), (char*)bytes.data, bytes.len);

         } else if( action == N(insert1) ) {
            record1 tmp;
            readMessage(&tmp, sizeof(tmp));
            store_i64( N(simpledb), N(record1), &tmp, sizeof(tmp) );
         } else if(action == N(insert2)) {
            record2 tmp;
            readMessage(&tmp, sizeof(tmp));
            store_i128i128( N(simpledb), N(record2), &tmp, sizeof(tmp) );
         } else if(action == N(insert3)) {
            record3 tmp;
            readMessage(&tmp, sizeof(tmp));
            store_i64i64i64( N(simpledb), N(record3), &tmp, sizeof(tmp) );
         } else {
            assert(0, "unknown message");
         }
      }
   }
}
