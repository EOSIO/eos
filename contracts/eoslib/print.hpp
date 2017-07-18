#pragma once
#include <eoslib/print.h>
#include <eoslib/types.hpp>
#include <eoslib/math.hpp>

namespace eos {


   inline void print( const char* ptr ) {
      prints(ptr);
   }

   inline void print( uint64_t num ) {
      printi(num);
   }
   inline void print( uint32_t num ) {
      printi(num);
   }
   inline void print( int num ) {
      printi(num);
   }

   inline void print( uint128 num ) {
      printi128((uint128_t*)&num);
   }
   inline void print( uint128_t num ) {
      printi128((uint128_t*)&num);
   }

   inline void print( Name name ) {
      printn(name.value);
   }

   template<typename T>
   inline void print( T&& t ) {
      t.print();
   }


   /**
    *  @defgroup consoleCppapi Console C++ API
    *  @ingroup consoleapi
    *  @brief C++ wrapper for Console C API
    *
    *  This API uses C++ varidic templates and type detection to
    *  make it easy to print any native type. You can even overload
    *  the `print()` method for your own custom types.
    *
    *  ### Example:
    *  ```
    *     print( "hello world, this is a number: ", 5 );
    *  ```
    *
    *  @section override Overriding Print for your Types
    *
    *  There are two ways to overload print:
    *  1. implement void print( const T& )
    *  2. implement T::print()const
    *
    *  @{
    */
   template<typename Arg, typename... Args>
   void print( Arg a, Args... args ) {
      print(a);
      print(args...);
   }

   /**
    * Simulate C++ style streams
    */
   class iostream {};

   template<typename T>
   inline iostream& operator<<( iostream& out, const T& v ) {
      print( v );
      return out;
   }

   static iostream cout;

   /// @} consoleCppapi


}
