#pragma once
#include <eoslib/eos.hpp>

namespace eos {

   inline void print_native( const char* ptr ) {
      prints(ptr);
   }

   inline void print_native( uint64_t num ) {
      printi(num);
   }
   inline void print_native( uint32_t num ) {
      printi(num);
   }
   inline void print_native( int num ) {
      printi(num);
   }

   inline void print_native( uint128 num ) {
      printi128((uint128_t*)&num);
   }
   inline void print_native( uint128_t num ) {
      printi128((uint128_t*)&num);
   }

   inline void print_native( Name name ) {
      printn(name.value);
   }

   template<typename T>
   inline void print_native( T&& t ) {
      t.print();
   }

   template<typename Arg>
   inline void print( Arg a ) {
      print_native(a);
   }
   template<typename Arg, typename... Args>
   void print( Arg a, Args... args ) {
      print(a);
      print(args...);
   }

}
