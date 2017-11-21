#include <eoslib/raw.hpp>
#include <eoslib/datastream.hpp>
#include <eoslib/memory.hpp>
#include <eoslib/print.hpp>

// These functions can be auto-generated using the ABI definition.
// The specialization for current_action must only be defined if the
// struct has at least one variable length type (String, bytes, etc),
// otherwise the default function will do ok.
//
// value_to_bytes/bytes_to_value must only be defined when we detected a
// key_value table since we need to pass the serialized version of the value
// to the underlying db functions.
//
// Then we can simplify the interface for the c++ helper VarTable (maybe reanaming to key_value_table?)
// specifying the KeyType and the ValueType.
// 
// template<account_name scope, account_name code, table_name table, typename KeyType, typename ValueType>
// struct key_value_table
//
// and inside the store/load functions (table_impl_obj) we need to call value_to_bytes/bytes_to_value accordingly

namespace eosio {
   template<typename T>
   T bytes_to_value(const bytes& bytes_val) { return *reinterpret_cast<T*>(bytes_val.data); }

   template<>
   key_value1 current_action<key_value1>() {
      uint32_t msgsize = action_size();
      char* buffer = (char *)eosio::malloc(msgsize);
      assert(read_action(buffer, msgsize) == msgsize, "error reading key_value1");
      datastream<char *> ds(buffer, msgsize);
      key_value1 value;
      raw::unpack(ds, value.key);
      raw::unpack(ds, value.value);
      eosio::free(buffer);
      return value;
   }

   bytes value_to_bytes(const key_value1& s) {
      uint32_t maxsize=0;
      maxsize += s.value.get_size() + 4;
      
      char* buffer = (char *)eosio::malloc(maxsize);
      datastream<char *> ds(buffer, maxsize);     

      raw::pack(ds, s.value);
      
      bytes packed_bytes;
      packed_bytes.len = ds.tellp();
      packed_bytes.data = (uint8_t*)buffer;

      return packed_bytes;
   }

   template<>
   key_value1 bytes_to_value<key_value1>(const bytes& bytes_val) {
      datastream<char *> ds((char*)bytes_val.data, bytes_val.len);
      key_value1 kv;
      raw::unpack(ds, kv.value);
      return kv;
   }

   template<>
   key_value2 current_action<key_value2>() {
      uint32_t msgsize = action_size();
      char* buffer = (char *)eosio::malloc(msgsize);
      assert(read_action(buffer, msgsize) == msgsize, "error reading key_value2");
      datastream<char *> ds(buffer, msgsize);
      key_value2 value;
      raw::unpack(ds, value.key);
      raw::unpack(ds, value.value.name);
      raw::unpack(ds, value.value.age);
      eosio::free(buffer);
      return value;
   }

   bytes value_to_bytes(const key_value2& s) {
      uint32_t maxsize=0;
      maxsize += s.value.name.get_size() + 4;
      maxsize += sizeof(s.value.age);
      
      char* buffer = (char *)eosio::malloc(maxsize);
      datastream<char *> ds(buffer, maxsize);     

      raw::pack(ds, s.value.name);
      raw::pack(ds, s.value.age);
      
      bytes packed_bytes;
      packed_bytes.len = ds.tellp();
      packed_bytes.data = (uint8_t*)buffer;

      return packed_bytes;
   }

   template<>
   key_value2 bytes_to_value<key_value2>(const bytes& bytes_val) {
      datastream<char *> ds((char*)bytes_val.data, bytes_val.len);
      key_value2 kv;
      raw::unpack(ds, kv.value.name);
      raw::unpack(ds, kv.value.age);
      return kv;
   }


}
