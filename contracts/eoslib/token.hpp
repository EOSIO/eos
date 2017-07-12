#pragma once
namespace eos {

   template<typename NumberType, uint64_t CurrencyType = N(eos) >
   struct token {
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
   };

   /**
    *  A price is written as  X  Base/Quote
    */
   template<typename BaseToken, typename QuoteToken>
   struct price
   {
      /**
       *  The largest base 10 integer that can be represented with 53 bits of
       *  a double. This number keeps the math in range of JavaScript. By
       *  being a power of 10 it makes it easier for developers to read and
       *  interpret the integer by simply shifting the decimal.
       */
      static const uint64_t precision = 1000ll*1000ll*1000ll*1000ll*1000ll;

      price():base_per_quote(1){}

      price( BaseToken base, QuoteToken quote ) {
         assert( base  >= BaseToken(1), "invalid price" );
         assert( quote >= QuoteToken(1), "invalid price" );

         base_per_quote = base.quantity;
         base_per_quote *= precision;
         base_per_quote /= quote.quantity;
      }

      friend QuoteToken operator / ( BaseToken b, const price& q ) {
         return QuoteToken( uint64_t((uint128_t(b.quantity) * precision)  / q.base_per_quote) );
      }

      friend BaseToken operator * ( QuoteToken b, const price& q ) {
         //return QuoteToken( uint64_t( mult_div_i128( b.quantity, q.base_per_quote, precision ) ) );
         return BaseToken( uint64_t((b.quantity * q.base_per_quote) / precision) );
      }

      friend bool operator <= ( const price& a, const price& b ) { return a.base_per_quote <= b.base_per_quote; }
      friend bool operator <  ( const price& a, const price& b ) { return a.base_per_quote <  b.base_per_quote; }
      friend bool operator >= ( const price& a, const price& b ) { return a.base_per_quote >= b.base_per_quote; }
      friend bool operator >  ( const price& a, const price& b ) { return a.base_per_quote >  b.base_per_quote; }
      friend bool operator == ( const price& a, const price& b ) { return a.base_per_quote == b.base_per_quote; }
      friend bool operator != ( const price& a, const price& b ) { return a.base_per_quote != b.base_per_quote; }

      private:
      /**
       * represented as number of base tokens to purchase 1 quote token 
       */
      uint128_t base_per_quote; 
   };

   typedef eos::token<uint64_t,N(eos)>   Tokens;

   struct Transfer {
     AccountName  from;
     AccountName  to;
     Tokens       quantity;
   };
} // namespace eos
