#pragma once
#include <eoslib/types.hpp>
#include <eoslib/datastream.hpp>
#include <eoslib/raw_fwd.hpp>

namespace eos { namespace raw {
   template<typename Stream> inline void pack( Stream& s, const record1& value ) {
      raw::pack(s, value.key);
      raw::pack(s, value.u256);
      raw::pack(s, value.u128);
      raw::pack(s, value.u64);
      raw::pack(s, value.u32);
      raw::pack(s, value.u16);
      raw::pack(s, value.u8);
      raw::pack(s, value.i64);
      raw::pack(s, value.i32);
      raw::pack(s, value.i16);
      raw::pack(s, value.i8);
      raw::pack(s, value.price);
   }
   template<typename Stream> inline void unpack( Stream& s, record1& value ) {
      raw::unpack(s, value.key);
      raw::unpack(s, value.u256);
      raw::unpack(s, value.u128);
      raw::unpack(s, value.u64);
      raw::unpack(s, value.u32);
      raw::unpack(s, value.u16);
      raw::unpack(s, value.u8);
      raw::unpack(s, value.i64);
      raw::unpack(s, value.i32);
      raw::unpack(s, value.i16);
      raw::unpack(s, value.i8);
      raw::unpack(s, value.price);
   }
   template<typename Stream> inline void pack( Stream& s, const record2& value ) {
      raw::pack(s, value.key1);
      raw::pack(s, value.key2);
   }
   template<typename Stream> inline void unpack( Stream& s, record2& value ) {
      raw::unpack(s, value.key1);
      raw::unpack(s, value.key2);
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
   template<typename Stream> inline void pack( Stream& s, const KeyValue1& value ) {
      raw::pack(s, value.key);
      raw::pack(s, value.value);
   }
   template<typename Stream> inline void unpack( Stream& s, KeyValue1& value ) {
      raw::unpack(s, value.key);
      raw::unpack(s, value.value);
   }
   template<typename Stream> inline void pack( Stream& s, const ComplexType& value ) {
      raw::pack(s, value.name);
      raw::pack(s, value.age);
   }
   template<typename Stream> inline void unpack( Stream& s, ComplexType& value ) {
      raw::unpack(s, value.name);
      raw::unpack(s, value.age);
   }
   template<typename Stream> inline void pack( Stream& s, const KeyValue2& value ) {
      raw::pack(s, value.key);
      raw::pack(s, value.value);
   }
   template<typename Stream> inline void unpack( Stream& s, KeyValue2& value ) {
      raw::unpack(s, value.key);
      raw::unpack(s, value.value);
   }
} }

#include <eoslib/raw.hpp>
namespace eos {
   void print_ident(int n){while(n-->0){print("  ");}};
   template<typename Type>
   Type current_message_ex() {
      uint32_t size = message_size();
      char* data = (char *)eos::malloc(size);
      assert(data && read_message(data, size) == size, "error reading message");
      Type value;
      eos::raw::unpack(data, size, value);
      eos::free(data);
      return value;
   }
   void dump(const record1& value, int tab=0) {
      print_ident(tab);print("key:[");printi(uint64_t(value.key));print("]\n");
      print_ident(tab);print("u256:[");printhex((void*)&value.u256, sizeof(value.u256));print("]\n");
      print_ident(tab);print("u128:[");printi128(&value.u128);print("]\n");
      print_ident(tab);print("u64:[");printi(uint64_t(value.u64));print("]\n");
      print_ident(tab);print("u32:[");printi(uint64_t(value.u32));print("]\n");
      print_ident(tab);print("u16:[");printi(uint64_t(value.u16));print("]\n");
      print_ident(tab);print("u8:[");printi(uint64_t(value.u8));print("]\n");
      print_ident(tab);print("i64:[");printi(uint64_t(value.i64));print("]\n");
      print_ident(tab);print("i32:[");printi(uint64_t(value.i32));print("]\n");
      print_ident(tab);print("i16:[");printi(uint64_t(value.i16));print("]\n");
      print_ident(tab);print("i8:[");printi(uint64_t(value.i8));print("]\n");
      print_ident(tab);print("price:[");printi(value.price.base.amount);print(" ");printn(value.price.base.symbol);print("/");printi(value.price.quote.amount);print(" ");printn(value.price.quote.symbol);print("]\n");
   }
   template<>
   record1 current_message<record1>() {
      return current_message_ex<record1>();
   }
   void dump(const record2& value, int tab=0) {
      print_ident(tab);print("key1:[");printi128(&value.key1);print("]\n");
      print_ident(tab);print("key2:[");printi128(&value.key2);print("]\n");
   }
   template<>
   record2 current_message<record2>() {
      return current_message_ex<record2>();
   }
   void dump(const record3& value, int tab=0) {
      print_ident(tab);print("key1:[");printi(uint64_t(value.key1));print("]\n");
      print_ident(tab);print("key2:[");printi(uint64_t(value.key2));print("]\n");
      print_ident(tab);print("key3:[");printi(uint64_t(value.key3));print("]\n");
   }
   template<>
   record3 current_message<record3>() {
      return current_message_ex<record3>();
   }
   void dump(const KeyValue1& value, int tab=0) {
      print_ident(tab);print("key:[");prints_l(value.key.get_data(), value.key.get_size());print("]\n");
      print_ident(tab);print("value:[");prints_l(value.value.get_data(), value.value.get_size());print("]\n");
   }
   template<>
   KeyValue1 current_message<KeyValue1>() {
      return current_message_ex<KeyValue1>();
   }
   void dump(const ComplexType& value, int tab=0) {
      print_ident(tab);print("name:[");prints_l(value.name.get_data(), value.name.get_size());print("]\n");
      print_ident(tab);print("age:[");printi(uint64_t(value.age));print("]\n");
   }
   template<>
   ComplexType current_message<ComplexType>() {
      return current_message_ex<ComplexType>();
   }
   void dump(const KeyValue2& value, int tab=0) {
      print_ident(tab);print("key:[");prints_l(value.key.get_data(), value.key.get_size());print("]\n");
      print_ident(tab);print("value:[");print("\n"); eos::dump(value.value, tab+1);print_ident(tab);print("]\n");
   }
   template<>
   KeyValue2 current_message<KeyValue2>() {
      return current_message_ex<KeyValue2>();
   }
} //eos

