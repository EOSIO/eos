/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "simpledb.hpp"
#include "simpledb.gen.hpp"

#include <eoslib/db.hpp>
#include <eoslib/print.hpp>
#include <eoslib/types.hpp>
#include <eoslib/raw.hpp>

extern "C" {
   void init()  {

   }
   
   void apply( uint64_t code, uint64_t action ) {
      if( code == N(simpledb) ) {
         if( action == N(insertkv1) ) {
            // eosc push message simpledb insertkv1 '{"key":"a", "value":"aa"}' -S simpledb
            // eosc get table simpledb simpledb keyvalue1
            auto kv1 = eos::current_message_ex<KeyValue1>();
            eos::print("Inserting KeyValue1\n");
            eos::dump(kv1);
            Bytes bytes = eos::raw::to_bytes(kv1.value);
            uint32_t err = store_str( N(simpledb), N(keyvalue1), (char *)kv1.key.get_data(), kv1.key.get_size(), (char*)bytes.data, bytes.len);
         } else if( action == N(insertkv2) ) {
            // eosc push message simpledb insertkv2 '{"key":"a", "value":{"name":"aaa", "age":10}}' -S simpledb
            // eosc get table simpledb simpledb keyvalue2
            auto kv2 = eos::current_message_ex<KeyValue2>();
            eos::print("Inserting KeyValue2\n");
            eos::dump(kv2);
            Bytes bytes = eos::raw::to_bytes(kv2.value);
            uint32_t err = store_str( N(simpledb), N(keyvalue2), (char *)kv2.key.get_data(), kv2.key.get_size(), (char*)bytes.data, bytes.len);
         } else if( action == N(insert1) ) {
            // eosc push message simpledb insert1 '{"key":75}' -S simpledb
            // eosc get table simpledb simpledb record1
            auto tmp = eos::current_message_ex<record1>();
            eos::print("Inserting record1\n");
            eos::dump(tmp);
            auto bytes = eos::raw::to_bytes(tmp);
            store_i64( N(simpledb), N(record1), bytes.data, bytes.len);
         } else if(action == N(insert2)) {
            // eosc push message simpledb insert2 '{"key1":"75", "key2":"77"}' -S simpledb
            // eosc get table simpledb simpledb record2
            auto tmp = eos::current_message_ex<record2>();
            eos::print("Inserting record2\n");
            eos::dump(tmp);
            auto bytes = eos::raw::to_bytes(tmp);
            store_i128i128( N(simpledb), N(record2), bytes.data, bytes.len);
         } else if(action == N(insert3)) {
            // eosc push message simpledb insert3 '{"key1":75, "key2":77, "key3":79}' -S simpledb
            // eosc get table simpledb simpledb record3
            auto tmp = eos::current_message_ex<record3>();
            eos::print("Inserting record3\n");
            eos::dump(tmp);
            auto bytes = eos::raw::to_bytes(tmp);
            store_i64i64i64( N(simpledb), N(record3), bytes.data, bytes.len);
         } else {
            assert(0, "unknown message");
         }
      }
   }
}
