#include <eoslib/raw.hpp>
#include <eoslib/datastream.hpp>
#include <eoslib/memory.hpp>
#include <eoslib/print.hpp>

// These functions can be auto-generated using the ABI definition.
// The specialization for currentMessage must only be defined if the 
// struct has at least one variable length type (String, Bytes, etc), 
// otherwise the default function will do ok.
//
// valueToBytes/bytesToValue must only be defined when we detected a
// KeyValue table since we need to pass the serialized version of the value
// to the underlying db functions.
//
// Then we can simplify the interface for the c++ helper VarTable (maybe reanaming to KeyValueTable?) 
// specifying the KeyType and the ValueType.
// 
// template<AccountName scope, AccountName code, TableName table, typename KeyType, typename ValueType>
// struct KeyValueTable
//
// and inside the store/load functions (table_impl_obj) we need to call valueToBytes/bytesToValue accordingly

namespace eos {
   template<typename T>
   T bytesToValue(const Bytes& bytes) { return *reinterpret_cast<T*>(bytes.data); }

   template<>
   KeyValue1 currentMessage<KeyValue1>() {
      uint32_t msgsize = messageSize();
      char* buffer = (char *)eos::malloc(msgsize);
      assert(readMessage(buffer, msgsize) == msgsize, "error reading KeyValue1");
      datastream<char *> ds(buffer, msgsize);
      KeyValue1 value;
      raw::unpack(ds, value.key);
      raw::unpack(ds, value.value);
      eos::free(buffer);
      return value;
   }

   Bytes valueToBytes(const KeyValue1& s) {
      uint32_t maxsize=0;
      maxsize += s.value.get_size() + 4;
      
      char* buffer = (char *)eos::malloc(maxsize);
      datastream<char *> ds(buffer, maxsize);     

      raw::pack(ds, s.value);
      
      Bytes bytes;
      bytes.len = ds.tellp();
      bytes.data = (uint8_t*)buffer;

      return bytes;
   }

   template<>
   KeyValue1 bytesToValue<KeyValue1>(const Bytes& bytes) {
      datastream<char *> ds((char*)bytes.data, bytes.len);
      KeyValue1 kv;
      raw::unpack(ds, kv.value);
      return kv;
   }

   template<>
   KeyValue2 currentMessage<KeyValue2>() {
      uint32_t msgsize = messageSize();
      char* buffer = (char *)eos::malloc(msgsize);
      assert(readMessage(buffer, msgsize) == msgsize, "error reading KeyValue2");
      datastream<char *> ds(buffer, msgsize);
      KeyValue2 value;
      raw::unpack(ds, value.key);
      raw::unpack(ds, value.value.name);
      raw::unpack(ds, value.value.age);
      eos::free(buffer);
      return value;
   }

   Bytes valueToBytes(const KeyValue2& s) {
      uint32_t maxsize=0;
      maxsize += s.value.name.get_size() + 4;
      maxsize += sizeof(s.value.age);
      
      char* buffer = (char *)eos::malloc(maxsize);
      datastream<char *> ds(buffer, maxsize);     

      raw::pack(ds, s.value.name);
      raw::pack(ds, s.value.age);
      
      Bytes bytes;
      bytes.len = ds.tellp();
      bytes.data = (uint8_t*)buffer;

      return bytes;
   }

   template<>
   KeyValue2 bytesToValue<KeyValue2>(const Bytes& bytes) {
      datastream<char *> ds((char*)bytes.data, bytes.len);
      KeyValue2 kv;
      raw::unpack(ds, kv.value.name);
      raw::unpack(ds, kv.value.age);
      return kv;
   }


}
