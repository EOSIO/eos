#pragma once
#include <eosiolib/serialize.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/system.h>
#include <tuple>
#include <limits>

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
         char c = (char)(sym & 0xff);
         if( !('A' <= c && c <= 'Z')  ) return false;
         sym >>= 8;
         if( !(sym & 0xff) ) {
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
      uint64_t precision()const { return value & 0xff; }
      uint64_t name()const      { return value >> 8;   }
      uint32_t name_length()const { return symbol_name_length( value ); }

      operator symbol_name()const { return value; }

      void print(bool show_precision=true)const {
         if( show_precision ){
            ::eosio::print(precision());
            prints(",");
         }

         auto sym = value;
         sym >>= 8;
         for( int i = 0; i < 7; ++i ) {
            char c = (char)(sym & 0xff);
            if( !c ) return;
            prints_l(&c, 1 );
            sym >>= 8;
         }
      }

      EOSLIB_SERIALIZE( symbol_type, (value) )
   };

   struct extended_symbol : public symbol_type
   {
      extended_symbol( symbol_name s = 0, account_name c = 0 ):symbol_type(s),contract(c){}

      account_name contract;

      void print()const {
         symbol_type::print();
         prints("@");
         printn( contract );
      }

      friend bool operator == ( const extended_symbol& a, const extended_symbol& b ) {
        return std::tie( a.value, a.contract ) == std::tie( b.value, b.contract );
      }
      friend bool operator != ( const extended_symbol& a, const extended_symbol& b ) {
        return std::tie( a.value, a.contract ) != std::tie( b.value, b.contract );
      }
      EOSLIB_SERIALIZE( extended_symbol, (value)(contract) )
   };

   struct asset {
      int64_t      amount = 0;
      symbol_type  symbol = S(4,EOS);

      static constexpr int64_t max_amount    = (1LL << 62) - 1;

      explicit asset( int64_t a = 0, symbol_name s = S(4,EOS))
      :amount(a),symbol(s)
      {
         eosio_assert( is_amount_within_range(), "magnitude of asset amount must be less than 2^62" );
         eosio_assert( symbol.is_valid(),        "invalid symbol name" );
      }

      bool is_amount_within_range()const { return -max_amount <= amount && amount <= max_amount; }
      bool is_valid()const               { return is_amount_within_range() && symbol.is_valid(); }

      void set_amount( int64_t a ) {
         amount = a;
         eosio_assert( is_amount_within_range(), "magnitude of asset amount must be less than 2^62" );
      }

      asset operator-()const {
         asset r = *this;
         r.amount = -r.amount;
         return r;
      }

      friend asset operator + ( const asset& a, const asset& b ) {
         eosio_assert( a.symbol == b.symbol, "type mismatch" );
         int64_t sum = a.amount + b.amount;
         eosio_assert( -max_amount <= sum, "underflow" );
         eosio_assert( sum <= max_amount,  "overflow" );
         return asset{sum, a.symbol};
      }

      friend asset operator - ( const asset& a, const asset& b ) {
         eosio_assert( a.symbol == b.symbol, "type mismatch" );
         int64_t difference = a.amount - b.amount;
         eosio_assert( -max_amount <= difference, "underflow" );
         eosio_assert( difference <= max_amount,  "overflow" );
         return asset{difference, a.symbol};
      }

      void print()const {
         int64_t p = (int64_t)symbol.precision();
         int64_t p10 = 1;
         while( p > 0  ) {
            p10 *= 10; --p;
         }
         p = (int64_t)symbol.precision();

         char fraction[p+1];
         fraction[p] = '\0';
         auto change = amount % p10;

         for( int64_t i = p -1; i >= 0; --i ) {
            fraction[i] = (change % 10) + '0';
            change /= 10;
         }
         printi( amount / p10 );
         prints(".");
         prints_l( fraction, uint32_t(p) );
         prints(" ");
         symbol.print(false);
      }

      asset& operator+=( const asset& a ) {
         return *this = (*this + a);
      }

      EOSLIB_SERIALIZE( asset, (amount)(symbol) )
   };

   struct extended_asset : public asset {
      account_name contract = N(eosio.token);

      extended_symbol get_extended_symbol()const { return extended_symbol( symbol, contract ); }
      extended_asset(){}
      extended_asset( int64_t v, extended_symbol s ):asset(v,s),contract(s.contract){}
      extended_asset( asset a, account_name c ):asset(a),contract(c){}

      void print()const {
         asset::print();
         prints("@");
         printn(contract);
      }
      extended_asset operator-()const {
         asset r = this->asset::operator-();
         return {r, contract};
      }

      friend extended_asset operator - ( const extended_asset& a, const extended_asset& b ) {
         eosio_assert( a.contract == b.contract, "type mismatch" );
         asset r = static_cast<const asset&>(a) - static_cast<const asset&>(b);
         return {r, a.contract};
      }

      friend extended_asset operator + ( const extended_asset& a, const extended_asset& b ) {
         eosio_assert( a.contract == b.contract, "type mismatch" );
         asset r = static_cast<const asset&>(a) + static_cast<const asset&>(b);
         return {r, a.contract};
      }

      EOSLIB_SERIALIZE( extended_asset, (amount)(symbol)(contract) )
   };


} /// namespace eosio
