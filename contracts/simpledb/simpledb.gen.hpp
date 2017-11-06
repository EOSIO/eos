#include <eoslib/raw.hpp>
#include <eoslib/datastream.hpp>
#include <eoslib/memory.hpp>
#include <eoslib/print.hpp>

// These functions can be auto-generated using the ABI definition.
// The specialization for current_message must only be defined if the
// struct has at least one variable length type (String, bytes, etc),
// otherwise the default function will do ok.
//
// valueToBytes/bytesToValue must only be defined when we detected a
// KeyValue table since we need to pass the serialized version of the value
// to the underlying db functions.
//
// Then we can simplify the interface for the c++ helper VarTable (maybe reanaming to KeyValueTable?) 
// specifying the KeyType and the ValueType.
// 
// template<account_name scope, account_name code, table_name table, typename KeyType, typename ValueType>
// struct KeyValueTable
//
// and inside the store/load functions (table_impl_obj) we need to call valueToBytes/bytesToValue accordingly

namespace eos {
   template<typename T>
   T bytesToValue(const bytes& b) { return *reinterpret_cast<T*>(b.data); }

   template<>
   KeyValue1 current_message<KeyValue1>() {
      uint32_t msgsize = message_size();
      char* buffer = (char *)eos::malloc(msgsize);
      assert(read_message(buffer, msgsize) == msgsize, "error reading KeyValue1");
      datastream<char *> ds(buffer, msgsize);
      KeyValue1 value;
      raw::unpack(ds, value.key);
      raw::unpack(ds, value.value);
      eos::free(buffer);
      return value;
   }

   bytes valueToBytes(const KeyValue1& s) {
      uint32_t maxsize=0;
      maxsize += s.value.get_size() + 4;
      
      char* buffer = (char *)eos::malloc(maxsize);
      datastream<char *> ds(buffer, maxsize);     

      raw::pack(ds, s.value);
      
      bytes b;
      b.len = ds.tellp();
      b.data = (uint8_t*)buffer;

      return b;
   }

   template<>
   KeyValue1 bytesToValue<KeyValue1>(const bytes& b) {
      datastream<char *> ds((char*)b.data, b.len);
      KeyValue1 kv;
      raw::unpack(ds, kv.value);
      return kv;
   }

   template<>
   KeyValue2 current_message<KeyValue2>() {
      uint32_t msgsize = message_size();
      char* buffer = (char *)eos::malloc(msgsize);
      assert(read_message(buffer, msgsize) == msgsize, "error reading KeyValue2");
      datastream<char *> ds(buffer, msgsize);
      KeyValue2 value;
      raw::unpack(ds, value.key);
      raw::unpack(ds, value.value.name);
      raw::unpack(ds, value.value.age);
      eos::free(buffer);
      return value;
   }

   bytes valueToBytes(const KeyValue2& s) {
      uint32_t maxsize=0;
      maxsize += s.value.name.get_size() + 4;
      maxsize += sizeof(s.value.age);
      
      char* buffer = (char *)eos::malloc(maxsize);
      datastream<char *> ds(buffer, maxsize);     

      raw::pack(ds, s.value.name);
      raw::pack(ds, s.value.age);
      
      bytes b;
      b.len = ds.tellp();
      b.data = (uint8_t*)buffer;

      return b;
   }

   template<>
   KeyValue2 bytesToValue<KeyValue2>(const bytes& b) {
      datastream<char *> ds((char*)b.data, b.len);
      KeyValue2 kv;
      raw::unpack(ds, kv.value.name);
      raw::unpack(ds, kv.value.age);
      return kv;
   }


}
