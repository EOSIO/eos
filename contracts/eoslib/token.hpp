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
 * @{
 */



namespace eos {

   /**
    *  Base token structure with checks for proper types and over/underflows.
    *  It supports the following operator: +=, -=, +, -, <=, <, ==, !=, >=, >, bool and also print functionality
    *  @brief a uint64_t wrapper with checks for proper types and over/underflows.
    *  @tparam NumberType - numeric type of the token
    *  @tparam CurrencyType - type of the currency (e.g. eos) represented as an unsigned 64 bit integer
    *  @ingroup tokens
    */
   template<typename NumberType, uint64_t CurrencyType = N(eos) >
   struct token {
       /**
        * Type of the currency (e.g. eos) represented as an unsigned 64 bit integer
        * @brief  Type of the currency
        */
       static const uint64_t currency_type = CurrencyType;

       /**
        * Default constructor
        * @brief Default constructor
        */
       token(){}

       /**
        * Constructor for token given quantity of tokens available
        * @brief Constructor for token given quantity of tokens available
        * @param v - quantity of tokens available
        */
       explicit token( NumberType v ):quantity(v){};

       /**
        * Quantity of tokens available
        * @brief Quantity of tokens available
        */
       NumberType quantity = 0;

       /**
        * Subtracts quantity of token from this object
        * Throws an exception if underflow
        * @brief Subtracts quantity of token from this object
        * @param a token to be subtracted
        * @return this token after subtraction
        */
       token& operator-=( const token& a ) {
          assert( quantity >= a.quantity, "integer underflow subtracting token balance" );
          quantity -= a.quantity;
          return *this;
       }

       /**
         * Adds quantity of token to this object
         * Throws an exception if overflow
         * @brief Adds quantity of token to this object
         * @param a token to be added
         * @return this token after addition
         */
       token& operator+=( const token& a ) {
          assert( quantity + a.quantity >= a.quantity, "integer overflow adding token balance" );
          quantity += a.quantity;
          return *this;
       }

       /**
         * Adds quantity of two tokens and return a new token
         * Throws an exception if overflow
         * @brief Adds quantity of two tokens and return a new token
         * @param a token to be added
         * @param b token to be added
         * @return result of addition as a new token
         */
       inline friend token operator+( const token& a, const token& b ) {
          token result = a;
          result += b;
          return result;
       }

       /**
         * Subtracts quantity of two tokens and return a new token
         * Throws an exception if underflow
         * @brief Subtracts quantity of two tokens and return a new token
         * @param a token to be subtracted
         * @param b token to be subtracted
         * @return result of subtraction as a new token
         */
       inline friend token operator-( const token& a, const token& b ) {
          token result = a;
          result -= b;
          return result;
       }

       /**
        * Less than or equal to comparison operator
        * @brief Less than or equal to comparison operator
        * @param a token to be compared
        * @param b token to be compared
        * @return true if quantity of a is less than or equal to quantity of b
        */
       friend bool operator <= ( const token& a, const token& b ) { return a.quantity <= b.quantity; }
       
       /**
        * Less than comparison operator
        * @brief Less than comparison operator
        * @param a token to be compared
        * @param b token to be compared
        * @return true if quantity of a is less than quantity of b
        */
       friend bool operator <  ( const token& a, const token& b ) { return a.quantity <  b.quantity; }

       /**
        * Greater than or equal to comparison operator
        * @brief Greater than or equal to comparison operator
        * @param a token to be compared
        * @param b token to be compared
        * @return true if quantity of a is greater than or equal to quantity of b
        */
       friend bool operator >= ( const token& a, const token& b ) { return a.quantity >= b.quantity; }

       /**
        * Greater than comparison operator
        * @brief Greater than comparison operator
        * @param a token to be compared
        * @param b token to be compared
        * @return true if quantity of a is greater than quantity of b
        */
       friend bool operator >  ( const token& a, const token& b ) { return a.quantity >  b.quantity; }

       /**
        * Equality comparison operator
        * @brief Equality comparison operator
        * @param a token to be compared
        * @param b token to be compared
        * @return true if quantity of a is equal to quantity of b
        */
       friend bool operator == ( const token& a, const token& b ) { return a.quantity == b.quantity; }
       /**
        * Inequality comparison operator
        * @brief Inequality comparison operator
        * @param a token to be compared
        * @param b token to be compared
        * @return true if quantity of a is not equal to quantity of b
        */
       friend bool operator != ( const token& a, const token& b ) { return a.quantity != b.quantity; }

       /**
        * Boolean conversion operator
        * @brief Boolean conversion operator
        * @return true if quantity is not zero
        */
       explicit operator bool()const { return quantity != 0; }

       /**
        * Print as string representation of the token (e.g. 1 EOS)
        * @brief Print as string
        */
       inline void print() {
           eos::print( quantity, " ", Name(CurrencyType) );
       }
   };

   /**
    *  Defines a fixed precision price between two tokens.
    *  A price is written as  X  Base/Quote.
    *  It supports the following operator: /, \, <=, <, ==, !=, >=, > and also print functionality
    *  @brief Defines a fixed precision price between two tokens.
    *  @tparam BaseToken - represents the type of the base token
    *  @tparam QuoteToken -  represents the type of the quote token
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

      /**
       * Default constructor.
       * Initialize base per quote to be 1.
       * @brief Default constructor.
       */
      price():base_per_quote(1ul){}

      /**
       * Construction for price given the base token and quote token
       * @brief Construction for price given the base token and quote token
       * @param base - base token
       * @param quote - quote token
       */
      price( BaseToken base, QuoteToken quote ) {
         assert( base  >= BaseToken(1ul), "invalid price" );
         assert( quote >= QuoteToken(1ul), "invalid price" );

         base_per_quote = base.quantity;
         base_per_quote *= precision;
         base_per_quote /= quote.quantity;
      }

       /**
        * Operator returns a quote token given a base token and the conversion price
        * @brief Operator returns a quote token given a base token and the conversion price
        * @param b - base token
        * @param q - price
        * @return quote token
        */
      friend QuoteToken operator / ( BaseToken b, const price& q ) {
         eos::print( "operator/ ", uint128(b.quantity), " * ", uint128( precision ), " / ", q.base_per_quote, "\n" );
         return QuoteToken( uint64_t((uint128(b.quantity) * uint128(precision)   / q.base_per_quote)) );
      }

       /**
        * Operator returns a base token given a quote token and the conversion price
        * @brief Operator returns a base token given a quote token and the conversion price
        * @param b - quote token
        * @param q - price
        * @return base token
        */
      friend BaseToken operator * ( const QuoteToken& b, const price& q ) {
         eos::print( "b: ", b, " \n" );
         eos::print( "operator* ", uint128(b.quantity), " * ", uint128( q.base_per_quote ), " / ", precision, "\n" );
         //return QuoteToken( uint64_t( mult_div_i128( b.quantity, q.base_per_quote, precision ) ) );
         return BaseToken( uint64_t((b.quantity * q.base_per_quote) / precision) );
      }

      /**
       * Less than or equal to comparison operator
       * @brief Less than or equal to comparison operator
       * @param a price to be compared
       * @param b price to be compared
       * @return true if base per quote of a is less than  or equal to base per quote of b
       */
      friend bool operator <= ( const price& a, const price& b ) { return a.base_per_quote <= b.base_per_quote; }

      /**
       * Less than comparison operator
       * @brief Less than comparison operator
       * @param a price to be compared
       * @param b price to be compared
       * @return true if base per quote of a is less than base per quote of b
       */
      friend bool operator <  ( const price& a, const price& b ) { return a.base_per_quote <  b.base_per_quote; }

      /**
       * Greater than or equal to comparison operator
       * @brief Greater than or equal to comparison operator
       * @param a price to be compared
       * @param b price to be compared
       * @return true if base per quote of a is greater than or equal to base per quote of b
       */
      friend bool operator >= ( const price& a, const price& b ) { return a.base_per_quote >= b.base_per_quote; }

      /**
       * Greater than comparison operator
       * @brief Greater than comparison operator
       * @param a price to be compared
       * @param b price to be compared
       * @return true if base per quote of a is greater than base per quote of b
       */
      friend bool operator >  ( const price& a, const price& b ) { return a.base_per_quote >  b.base_per_quote; }

      /**
       * Equality comparison operator
       * @brief Equality comparison operator
       * @param a price to be compared
       * @param b price to be compared
       * @return true if base per quote of a is equal to base per quote of b
       */
      friend bool operator == ( const price& a, const price& b ) { return a.base_per_quote == b.base_per_quote; }

      /**
       * Inequality comparison operator
       * @brief Inequality comparison operator
       * @param a price to be compared
       * @param b price to be compared
       * @return true if base per quote of a is not equal to base per quote of b
       */
      friend bool operator != ( const price& a, const price& b ) { return a.base_per_quote != b.base_per_quote; }

      /**
       * Print as string representing the conversion
       * @brief Print as string
       */
      inline void print() {
         eos::print( base_per_quote, ".", " ", Name(base_token_type::currency_type), "/", Name(quote_token_type::currency_type)  );
      }
      private:
      /**
       * Represented as number of base tokens to purchase 1 quote token
       * @brief Represented as number of base tokens to purchase 1 quote token
       */
      eos::uint128 base_per_quote;
   };

    /**
     * Defines an eos tokens
     * @brief Defines an eos tokens
     */
   typedef eos::token<uint64_t,N(eos)>   Tokens;


   /**
    *  The binary structure of the `transfer` message type for the `eos` contract.
    *  @brief The binary structure of the `transfer` message type for the `eos` contract.
    *  @ingroup tokens
    */
   struct Transfer {
     /**
      * Defines transfer action type
      * @brief Defines transfer action type
      */
     static const uint64_t action_type = N(transfer);
     /**
      * Name of the account who sends the token
      * @brief Name of the account who sends the token
      */
     AccountName  from;
     /**
      * Name of the account who receives the token
      * @brief Name of the account who receives the token
      */
     AccountName  to;
     /**
      * Quantity of token to be transferred
      * @brief Quantity of token to be transferred
      */
     Tokens       quantity;
   };
   /// @} tokenhppapi
} // namespace eos
