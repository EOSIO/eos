#include <eoslib/types.hpp>
#include <eoslib/datastream.hpp>
#include <eoslib/raw.hpp>

namespace eos { namespace raw {
   inline uint32_t packed_size(const record1& c) {
      uint32_t size = 0;
      size += packed_size(c.key);
      return size;
   }
   template<typename Stream> inline void pack( Stream& s, const record1& value ) {
      raw::pack(s, value.key);
   }
   template<typename Stream> inline void unpack( Stream& s, record1& value ) {
      raw::unpack(s, value.key);
   }
   inline uint32_t packed_size(const record2& c) {
      uint32_t size = 0;
      size += packed_size(c.key1);
      size += packed_size(c.key2);
      return size;
   }
   template<typename Stream> inline void pack( Stream& s, const record2& value ) {
      raw::pack(s, value.key1);
      raw::pack(s, value.key2);
   }
   template<typename Stream> inline void unpack( Stream& s, record2& value ) {
      raw::unpack(s, value.key1);
      raw::unpack(s, value.key2);
   }
   inline uint32_t packed_size(const record3& c) {
      uint32_t size = 0;
      size += packed_size(c.key1);
      size += packed_size(c.key2);
      size += packed_size(c.key3);
      return size;
   }
   template<typename Stream> inline void pack( Stream& s, const record3& value ) {
      raw::pack(s, value.key1);
      raw::pack(s, value.key2);
      raw::pack(s, value.key3);
   }
   template<typename Stream> inline void unpack( Stream& s, record3& value ) {
      raw::unpack(s, value.key1);
      raw::unpack(s, value.key2);
      raw::unpack(s, value.key3);
   }
   inline uint32_t packed_size(const KeyValue1& c) {
      uint32_t size = 0;
      size += packed_size(c.key);
      size += packed_size(c.value);
      return size;
   }
   template<typename Stream> inline void pack( Stream& s, const KeyValue1& value ) {
      raw::pack(s, value.key);
      raw::pack(s, value.value);
   }
   template<typename Stream> inline void unpack( Stream& s, KeyValue1& value ) {
      raw::unpack(s, value.key);
      raw::unpack(s, value.value);
   }
   inline uint32_t packed_size(const ComplexType& c) {
      uint32_t size = 0;
      size += packed_size(c.name);
      size += packed_size(c.age);
      return size;
   }
   template<typename Stream> inline void pack( Stream& s, const ComplexType& value ) {
      raw::pack(s, value.name);
      raw::pack(s, value.age);
   }
   template<typename Stream> inline void unpack( Stream& s, ComplexType& value ) {
      raw::unpack(s, value.name);
      raw::unpack(s, value.age);
   }
   inline uint32_t packed_size(const KeyValue2& c) {
      uint32_t size = 0;
      size += packed_size(c.key);
      size += packed_size(c.value);
      return size;
   }
   template<typename Stream> inline void pack( Stream& s, const KeyValue2& value ) {
      raw::pack(s, value.key);
      raw::pack(s, value.value);
   }
   template<typename Stream> inline void unpack( Stream& s, KeyValue2& value ) {
      raw::unpack(s, value.key);
      raw::unpack(s, value.value);
   }
   template<typename Type> void from_bytes(const Bytes& bytes, Type& value) {
      datastream<char *> ds((char*)bytes.data, bytes.len);
      raw::unpack(ds, value);
   }
   template<typename Type> Bytes to_bytes(const Type& value) {
      uint32_t maxsize = packed_size(value);
      char* buffer = (char *)eos::malloc(maxsize);
      datastream<char *> ds(buffer, maxsize);
      pack(ds, value);
      Bytes bytes;
      bytes.len = ds.tellp();
      bytes.data = (uint8_t*)buffer;
      return bytes;
   }
} }

namespace eos {
   inline void print_ident(int n){while(n-->0){print("  ");}};
   void dump(const record1& value, int tab=0) {
      print_ident(tab);print("key:[");printi(uint64_t(value.key));print("]\n");
   }
   void dump(const record2& value, int tab=0) {
      print_ident(tab);print("key1:[");printi128(&value.key1);print("]\n");
      print_ident(tab);print("key2:[");printi128(&value.key2);print("]\n");
   }
   void dump(const record3& value, int tab=0) {
      print_ident(tab);print("key1:[");printi(uint64_t(value.key1));print("]\n");
      print_ident(tab);print("key2:[");printi(uint64_t(value.key2));print("]\n");
      print_ident(tab);print("key3:[");printi(uint64_t(value.key3));print("]\n");
   }
   void dump(const KeyValue1& value, int tab=0) {
      print_ident(tab);print("key:[");prints_l(value.key.get_data(), value.key.get_size());print("]\n");
      print_ident(tab);print("value:[");prints_l(value.value.get_data(), value.value.get_size());print("]\n");
   }
   void dump(const ComplexType& value, int tab=0) {
      print_ident(tab);print("name:[");prints_l(value.name.get_data(), value.name.get_size());print("]\n");
      print_ident(tab);print("age:[");printi(uint64_t(value.age));print("]\n");
   }
   void dump(const KeyValue2& value, int tab=0) {
      print_ident(tab);print("key:[");prints_l(value.key.get_data(), value.key.get_size());print("]\n");
      print_ident(tab);print("value:[");print("\n"); eos::dump(value.value, tab+1);print_ident(tab);print("]\n");
   }
   template<typename Type>
   Type current_message_ex() {
      uint32_t msgsize = messageSize();
      Bytes bytes;
      bytes.data = (uint8_t *)eos::malloc(msgsize);
      bytes.len  = msgsize;
      assert(bytes.data && readMessage(bytes.data, bytes.len) == msgsize, "error reading message");
      Type value;
      eos::raw::from_bytes(bytes, value);
      eos::free(bytes.data);
      return value;
   }
} //eos

