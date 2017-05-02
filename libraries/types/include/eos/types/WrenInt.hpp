#pragma once
#include <eos/types/native.hpp>

namespace eos { namespace types {
   class InputDatastream;
   class OutputDatastream;

   /**
    * This class provides a wren wrapper for intergers up to 256 bits and
    * supports seralizing from 8 to 256 bits based upon the arguments passed to
    * unpack and pack.
    *
    * The WREN code generator will construct a UInt for all integer fields and then
    * implement pack/unpack to the proper width. This minimizies the number of 
    * unique classes that we need to define for WREN and comes with minimal overhead
    * due to boost::multiprecision's internal optimizations
    */
   class UInt {
      public:
         UInt256 value;

         UInt256( double value = 0 )
         :value( int64_t(value) ){}

         UInt operator+( const UInt& other )const {
            return value + other.value;
         }
         void unpack( InputDatastream&, int size = 256 );
         void pack( OutputDatastream&, int size = 256 )const;

#define BIND_OP(X) .bindMethod<decltype(&UInt::operator X), &UInt64::operator X>( false, ##X"(_)" )
         static void registerWrenClass() {
            wrenpp::beginModule( "main" )
               .bindClass<Wren::UInt64, double>( "UInt" )
                  .bindMethod<decltype(&UInt::unpack), &UInt::unpack>( false, "unpack(_,_)" )
                  .bindMethod<decltype(&UInt::pack), &UInt::pack>( false, "pack(_,_)" )
                   BIND_OP(+)
/*
                  .bindMethod<decltype(&UInt::sub), &Wren::UInt64::sub>( false, "-(_)" )
                  .bindMethod<decltype(&UInt::mul), &Wren::UInt64::mul>( false, "*(_)" )
                  .bindMethod<decltype(&UInt::div), &Wren::UInt64::div>( false, "/(_)" )
                  .bindMethod<decltype(&UInt::mod), &Wren::UInt64::mod>( false, "%(_)" )
                  .bindMethod<decltype(&UInt::fromString), &Wren::UInt64::fromString>( true, "fromString(_)" )
                  .bindMethod<decltype(&UInt::toString), &Wren::UInt64::toString>( false, "toString()" )
*/
               .endClass()
            .endModule();
         }
         static string wrenScript() {
            static const auto script = R"(
                 foreign class UInt64 {
                   construct new(v) {}
                   foreign unpack( datastream, size )
                   foreign pack( datastream, size )
                   foreign +(v)
                   foreign -(v)
                   foreign *(v)
                   foreign /(v)
                   foreign %(v)
                   foreign toString()
                   foreign static fromString(s)
                 }
            )";
            return script;
         }

   };

}} /// namespace eos::types
