#pragma once

namespace eosio {

   static constexpr uint64_t string_to_symbol( uint8_t precision, const char* str ) {
      uint32_t len = 0;
      while( str[len] ) ++len;

      uint64_t result = 0;
      for( uint32_t i = 0; i < len; ++i ) {
         if( str[i] < 'A' || str[i] > 'Z' ) {
            /// ERRORS?
         } else {
            result |= (uint64_t(str[i]) << (8*(1+i)));
         }
      }

      result |= uint64_t(precision);
      return result;
   }

   #define S(P,X) ::eosio::string_to_symbol(P,#X)

   typedef uint64_t symbol_name;

   struct asset {
      int64_t      amount = 0;
      symbol_name  symbol = S(4,EOS);

      explicit asset( int64_t a = 0, uint64_t s = S(4,EOS))
      :amount(a),symbol(s){}


      template<typename DataStream>
      friend DataStream& operator << ( DataStream& ds, const asset& t ){
         return ds << t.amount << t.symbol;
      }
      template<typename DataStream>
      friend DataStream& operator >> ( DataStream& ds, asset& t ){
         return ds >> t.amount >> t.symbol;
      }

      asset& operator-=( const asset& a ) {
         eosio_assert( a.symbol == symbol, "attempt to subtract asset with different symbol" );
         eosio_assert( amount >= a.amount, "integer underflow subtracting asset balance" );
         amount -= a.amount;
         return *this;
      }

      asset& operator+=( const asset& a ) {
         eosio_assert( a.symbol == symbol, "attempt to add asset with different symbol" );
         eosio_assert( amount + a.amount >= a.amount, "integer overflow adding asset balance" );
         amount += a.amount;
         return *this;
      }

      inline friend asset operator+( const asset& a, const asset& b ) {
         asset result = a;
         result += b;
         return result;
      }

      inline friend asset operator-( const asset& a, const asset& b ) {
         asset result = a;
         result -= b;
         return result;
      }

      asset& operator*=( int64_t a ) {
         eosio_assert( a == 0 || (amount * a) / a == amount, "integer overflow multiplying asset balance" );
         amount *= a;
         return *this;
      }

      inline friend asset operator*( const asset& a, int64_t b ) {
         asset result = a;
         result *= b;
         return result;
      }

      inline friend asset operator*( int64_t b, const asset& a ) {
         asset result = a;
         result *= b;
         return result;
      }

      asset& operator/=( int64_t a ) {
         amount /= a;
         return *this;
      }

      inline friend asset operator/( const asset& a, int64_t b ) {
         asset result = a;
         result /= b;
         return result;
      }

      inline friend int64_t operator/( const asset& a, const asset& b ) {
         return a.amount / b.amount;
      }

   };

} /// namespace eosio 
