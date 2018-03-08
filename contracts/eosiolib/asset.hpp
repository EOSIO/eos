#pragma once
#include <eosiolib/serialize.hpp>

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

   static constexpr bool is_valid_symbol( symbol_name sym ) {
      sym >>= 8;
      for( int i = 0; i < 7; ++i ) {
         unsigned char c = sym & 0xff;
         if( !('A' <= c && c <= 'Z')  ) return false;
         sym >>= 8;
         if( !(c & 0xff) ) {
            do { 
              sym >>= 8;
              if( (sym & 0xff) ) return false;
              ++i;
            } while( i < 7 );
         }
      }
      return true;
   }

   static constexpr uint32_t symbol_name_length( symbol_name tmp ) {
      tmp >>= 8; /// skip precision
      uint32_t length = 0;
      while( tmp & 0xff && length <= 7) {
         ++length;
         tmp >>= 8;
      }

      return length;
   }

   struct symbol_type {
      symbol_type( symbol_name v = S(4,EOS) ):value(v){}
      symbol_name value;

      bool     is_valid()const  { return is_valid_symbol( value ); }
      uint8_t  precision()const { return value & 0xff; }
      uint64_t name()const      { return value & (~uint64_t(0xff)); }
      uint32_t name_length()const { return symbol_name_length( value ); }

      operator symbol_name()const { return value; }

      EOSLIB_SERIALIZE( symbol_type, (value) )
   };

   struct asset {
      int64_t      amount = 0;
      symbol_type  symbol = S(4,EOS);

      explicit asset( int64_t a = 0, symbol_name s = S(4,EOS))
      :amount(a),symbol(s){}

      friend asset operator + ( const asset& a, const asset& b ) {
         eosio_assert( a.symbol == b.symbol, "type mismatch" );
         asset result( a.amount + b.amount, a.symbol );
         eosio_assert( result.amount > a.amount && result.amount > b.amount, "overflow" );
         return result;
      }
      friend asset operator - ( const asset& a, const asset& b ) {
         eosio_assert( a.symbol == b.symbol, "type mismatch" );
         asset result( a.amount - b.amount, a.symbol );

         if( b.amount > 0 )
            eosio_assert( a.amount > result.amount, "underflow" );
         if( b.amount < 0 )
            eosio_assert( a.amount < result.amount, "overflow" );

         return result;
      }

      asset& operator+=( const asset& a ) {
         eosio_assert( symbol == a.symbol, "type missmatch" );
         return *this = (*this + a);
      }

      EOSLIB_SERIALIZE( asset, (amount)(symbol) )
   };

   struct extended_asset : public asset {
      account_name contract;

      EOSLIB_SERIALIZE( extended_asset, (amount)(symbol)(contract) )
   };

} /// namespace eosio 
