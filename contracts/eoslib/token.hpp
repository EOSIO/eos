/**
 * @file token.hpp
 * @brief Defines types and ABI for standard token messages and database tables
 *
 */
#pragma once
#include <eoslib/math.hpp>
#include <eoslib/print.hpp>

/**
 *  @defgroup tokens Token API
 *  @brief Defines the ABI for interfacing with standard-compatible token messages and database tables.
 *  @ingroup contractdev
 *
 *
 */



namespace eos {

   /**
    *  @brief a uint64_t wrapper with checks for proper types and over/underflows.
    *
    *  @ingroup tokens
    */
   template<typename NumberType, uint64_t CurrencyType = N(eos) >
   struct token {
       static const uint64_t currency_type = CurrencyType;

       token(){}
       explicit token( NumberType v ):quantity(v){};
       NumberType quantity = 0;

       token& operator-=( const token& a ) {
          assert( quantity >= a.quantity, "integer underflow subtracting token balance" );
          quantity -= a.quantity;
          return *this;
       }

       token& operator+=( const token& a ) {
          assert( quantity + a.quantity >= a.quantity, "integer overflow adding token balance" );
          quantity += a.quantity;
          return *this;
       }

       inline friend token operator+( const token& a, const token& b ) {
          token result = a;
          result += b;
          return result;
       }

       inline friend token operator-( const token& a, const token& b ) {
          token result = a;
          result -= b;
          return result;
       }

       friend bool operator <= ( const token& a, const token& b ) { return a.quantity <= b.quantity; }
       friend bool operator <  ( const token& a, const token& b ) { return a.quantity <  b.quantity; }
       friend bool operator >= ( const token& a, const token& b ) { return a.quantity >= b.quantity; }
       friend bool operator >  ( const token& a, const token& b ) { return a.quantity >  b.quantity; }
       friend bool operator == ( const token& a, const token& b ) { return a.quantity == b.quantity; }
       friend bool operator != ( const token& a, const token& b ) { return a.quantity != b.quantity; }

       explicit operator bool()const { return quantity != 0; }

        inline void print() {
           eos::print( quantity, " ", Name(CurrencyType) );
        }
   };

   /**
    *  @brief defines a fixed precision price between two tokens.
    *
    *  A price is written as  X  Base/Quote
    *
    *  @ingroup tokens
    */
   template<typename BaseToken, typename QuoteToken>
   struct price
   {
      typedef BaseToken  base_token_type;
      typedef QuoteToken quote_token_type;
      /**
       *  The largest base 10 integer that can be represented with 53 bits of
       *  a double. This number keeps the math in range of JavaScript. By
       *  being a power of 10 it makes it easier for developers to read and
       *  interpret the integer by simply shifting the decimal.
       */
      static const uint64_t precision = 1000ll*1000ll*1000ll*1000ll*1000ll;

      price():base_per_quote(1ul){}

      price( BaseToken base, QuoteToken quote ) {
         assert( base  >= BaseToken(1ul), "invalid price" );
         assert( quote >= QuoteToken(1ul), "invalid price" );

         base_per_quote = base.quantity;
         base_per_quote *= precision;
         base_per_quote /= quote.quantity;
      }

      friend QuoteToken operator / ( BaseToken b, const price& q ) {
         eos::print( "operator/ ", uint128(b.quantity), " * ", uint128( precision ), " / ", q.base_per_quote, "\n" );
         return QuoteToken( uint64_t((uint128(b.quantity) * uint128(precision)   / q.base_per_quote)) );
      }

      friend BaseToken operator * ( const QuoteToken& b, const price& q ) {
         eos::print( "b: ", b, " \n" );
         eos::print( "operator* ", uint128(b.quantity), " * ", uint128( q.base_per_quote ), " / ", precision, "\n" );
         //return QuoteToken( uint64_t( mult_div_i128( b.quantity, q.base_per_quote, precision ) ) );
         return BaseToken( uint64_t((b.quantity * q.base_per_quote) / precision) );
      }

      friend bool operator <= ( const price& a, const price& b ) { return a.base_per_quote <= b.base_per_quote; }
      friend bool operator <  ( const price& a, const price& b ) { return a.base_per_quote <  b.base_per_quote; }
      friend bool operator >= ( const price& a, const price& b ) { return a.base_per_quote >= b.base_per_quote; }
      friend bool operator >  ( const price& a, const price& b ) { return a.base_per_quote >  b.base_per_quote; }
      friend bool operator == ( const price& a, const price& b ) { return a.base_per_quote == b.base_per_quote; }
      friend bool operator != ( const price& a, const price& b ) { return a.base_per_quote != b.base_per_quote; }

      inline void print() {
         eos::print( base_per_quote, ".", " ", Name(base_token_type::currency_type), "/", Name(quote_token_type::currency_type)  );
      }
      private:
      /**
       * represented as number of base tokens to purchase 1 quote token 
       */
      eos::uint128 base_per_quote; 
   };

   typedef eos::token<uint64_t,N(eos)>   Tokens;


   /**
    *  @brief the binary structure of the `transfer` message type for the `eos` contract.
    *
    *  @ingroup tokens
    */
   struct Transfer {
     static const uint64_t action_type = N(transfer);

     AccountName  from;
     AccountName  to;
     Tokens       quantity;
   };
} // namespace eos
