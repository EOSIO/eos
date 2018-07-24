#pragma once
#include <eosiolib/core_symbol.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/system.h>
#include <tuple>
#include <limits>

namespace eosio {

  /**
   *  @defgroup symbolapi Symbol API
   *  @brief Defines API for managing symbols
   *  @ingroup contractdev
   */

  /**
   *  @defgroup symbolcppapi Symbol CPP API
   *  @brief Defines %CPP API for managing symbols
   *  @ingroup symbolapi
   *  @{
   */

   /**
    * Converts string to uint64_t representation of symbol
    *
    * @param precision - precision of symbol
    * @param str - the string representation of the symbol
    */
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

   /**
    * Macro for converting string to char representation of symbol
    *
    * @param precision - precision of symbol
    * @param str - the string representation of the symbol
    */
   #define S(P,X) ::eosio::string_to_symbol(P,#X)

   /**
    * uint64_t representation of a symbol name
    */
   typedef uint64_t symbol_name;

   /**
    * Checks if provided symbol name is valid.
    *
    * @param sym - symbol name of type symbol_name
    * @return true - if symbol is valid
    */
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

   /**
    * Returns the character length of the provided symbol
    *
    * @param sym - symbol to retrieve length for (uint64_t)
    * @return length - character length of the provided symbol
    */
   static constexpr uint32_t symbol_name_length( symbol_name sym ) {
      sym >>= 8; /// skip precision
      uint32_t length = 0;
      while( sym & 0xff && length <= 7) {
         ++length;
         sym >>= 8;
      }

      return length;
   }

   /**
    * \struct Stores information about a symbol
    *
    * @brief Stores information about a symbol
    */
   struct symbol_type {
     /**
      * The symbol name
      */
      symbol_name value;

      symbol_type() { }

      /**
       * What is the type of the symbol
       */
      symbol_type(symbol_name s): value(s) { }

      /**
       * Is this symbol valid
       */
      bool     is_valid()const  { return is_valid_symbol( value ); }

      /**
       * This symbol's precision
       */
      uint64_t precision()const { return value & 0xff; }

      /**
       * Returns uint64_t representation of symbol name
       */
      uint64_t name()const      { return value >> 8;   }

      /**
       * The length of this symbol
       */
      uint32_t name_length()const { return symbol_name_length( value ); }

      /**
       *
       */
      operator symbol_name()const { return value; }

      /**
       * %Print the symbol
       *
       * @brief %Print the symbol
       */
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

   /**
    * \struct Extended asset which stores the information of the owner of the symbol
    *
    */
   struct extended_symbol : public symbol_type
   {
     /**
      * The owner of the symbol
      *
      * @brief The owner of the symbol
      */
     account_name contract;

     extended_symbol( symbol_name sym = 0, account_name acc = 0 ):symbol_type{sym},contract(acc){}

      /**
       * %Print the extended symbol
       *
       * @brief %Print the extended symbol
       */
      void print()const {
         symbol_type::print();
         prints("@");
         printn( contract );
      }


      /**
       * Equivalency operator. Returns true if a == b (are the same)
       *
       * @brief Subtraction operator
       * @param a - The extended asset to be subtracted
       * @param b - The extended asset used to subtract
       * @return boolean - true if both provided symbols are the same
       */
      friend bool operator == ( const extended_symbol& a, const extended_symbol& b ) {
        return std::tie( a.value, a.contract ) == std::tie( b.value, b.contract );
      }

      /**
       * Inverted equivalency operator. Returns true if a != b (are different)
       *
       * @brief Subtraction operator
       * @param a - The extended asset to be subtracted
       * @param b - The extended asset used to subtract
       * @return boolean - true if both provided symbols are the same
       */
      friend bool operator != ( const extended_symbol& a, const extended_symbol& b ) {
        return std::tie( a.value, a.contract ) != std::tie( b.value, b.contract );
      }

      friend bool operator < ( const extended_symbol& a, const extended_symbol& b ) {
        return std::tie( a.value, a.contract ) < std::tie( b.value, b.contract );
      }

      EOSLIB_SERIALIZE( extended_symbol, (value)(contract) )
   };

   // }@ symbolapi

} /// namespace eosio
