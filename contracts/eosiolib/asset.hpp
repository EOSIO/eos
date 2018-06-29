#pragma once
#include <eosiolib/serialize.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/system.h>
#include <eosiolib/symbol.hpp>
#include <tuple>
#include <limits>

namespace eosio {

  /**
   *  @defgroup assetapi Asset API
   *  @brief Defines API for managing assets
   *  @ingroup contractdev
   */

  /**
   *  @defgroup assetcppapi Asset CPP API
   *  @brief Defines %CPP API for managing assets
   *  @ingroup assetapi
   *  @{
   */

   /**
    * \struct Stores information for owner of asset
    *
    * @brief Stores information for owner of asset
    */

   struct asset {
      /**
       * The amount of the asset
       *
       * @brief The amount of the asset
       */
      int64_t      amount;

      /**
       * The symbol name of the asset
       *
       * @brief The symbol name of the asset
       */
      symbol_type  symbol;

      /**
       * Maximum amount possible for this asset. It's capped to 2^62 - 1
       *
       * @brief Maximum amount possible for this asset
       */
      static constexpr int64_t max_amount    = (1LL << 62) - 1;

      /**
       * Construct a new asset given the symbol name and the amount
       *
       * @brief Construct a new asset object
       * @param a - The amount of the asset
       * @param s - THe name of the symbol, default to CORE_SYMBOL
       */
      explicit asset( int64_t a = 0, symbol_type s = CORE_SYMBOL )
      :amount(a),symbol{s}
      {
         eosio_assert( is_amount_within_range(), "magnitude of asset amount must be less than 2^62" );
         eosio_assert( symbol.is_valid(),        "invalid symbol name" );
      }

      /**
       * Check if the amount doesn't exceed the max amount
       *
       * @brief Check if the amount doesn't exceed the max amount
       * @return true - if the amount doesn't exceed the max amount
       * @return false - otherwise
       */
      bool is_amount_within_range()const { return -max_amount <= amount && amount <= max_amount; }

      /**
       * Check if the asset is valid. %A valid asset has its amount <= max_amount and its symbol name valid
       *
       * @brief Check if the asset is valid
       * @return true - if the asset is valid
       * @return false - otherwise
       */
      bool is_valid()const               { return is_amount_within_range() && symbol.is_valid(); }

      /**
       * Set the amount of the asset
       *
       * @brief Set the amount of the asset
       * @param a - New amount for the asset
       */
      void set_amount( int64_t a ) {
         amount = a;
         eosio_assert( is_amount_within_range(), "magnitude of asset amount must be less than 2^62" );
      }

      /**
       * Unary minus operator
       *
       * @brief Unary minus operator
       * @return asset - New asset with its amount is the negative amount of this asset
       */
      asset operator-()const {
         asset r = *this;
         r.amount = -r.amount;
         return r;
      }

      /**
       * Subtraction assignment operator
       *
       * @brief Subtraction assignment operator
       * @param a - Another asset to subtract this asset with
       * @return asset& - Reference to this asset
       * @post The amount of this asset is subtracted by the amount of asset a
       */
      asset& operator-=( const asset& a ) {
         eosio_assert( a.symbol == symbol, "attempt to subtract asset with different symbol" );
         amount -= a.amount;
         eosio_assert( -max_amount <= amount, "subtraction underflow" );
         eosio_assert( amount <= max_amount,  "subtraction overflow" );
         return *this;
      }

      /**
       * Addition Assignment  operator
       *
       * @brief Addition Assignment operator
       * @param a - Another asset to subtract this asset with
       * @return asset& - Reference to this asset
       * @post The amount of this asset is added with the amount of asset a
       */
      asset& operator+=( const asset& a ) {
         eosio_assert( a.symbol == symbol, "attempt to add asset with different symbol" );
         amount += a.amount;
         eosio_assert( -max_amount <= amount, "addition underflow" );
         eosio_assert( amount <= max_amount,  "addition overflow" );
         return *this;
      }

      /**
       * Addition operator
       *
       * @brief Addition operator
       * @param a - The first asset to be added
       * @param b - The second asset to be added
       * @return asset - New asset as the result of addition
       */
      inline friend asset operator+( const asset& a, const asset& b ) {
         asset result = a;
         result += b;
         return result;
      }

      /**
       * Subtraction operator
       *
       * @brief Subtraction operator
       * @param a - The asset to be subtracted
       * @param b - The asset used to subtract
       * @return asset - New asset as the result of subtraction of a with b
       */
      inline friend asset operator-( const asset& a, const asset& b ) {
         asset result = a;
         result -= b;
         return result;
      }

      /**
       * Multiplication assignment operator. Multiply the amount of this asset with a number and then assign the value to itself.
       *
       * @brief Multiplication assignment operator, with a number
       * @param a - The multiplier for the asset's amount
       * @return asset - Reference to this asset
       * @post The amount of this asset is multiplied by a
       */
      asset& operator*=( int64_t a ) {
         eosio_assert( a == 0 || (amount * a) / a == amount, "multiplication overflow or underflow" );
         eosio_assert( -max_amount <= amount, "multiplication underflow" );
         eosio_assert( amount <= max_amount,  "multiplication overflow" );
         amount *= a;
         return *this;
      }

      /**
       * Multiplication operator, with a number proceeding
       *
       * @brief Multiplication operator, with a number proceeding
       * @param a - The asset to be multiplied
       * @param b - The multiplier for the asset's amount
       * @return asset - New asset as the result of multiplication
       */
      friend asset operator*( const asset& a, int64_t b ) {
         asset result = a;
         result *= b;
         return result;
      }


      /**
       * Multiplication operator, with a number preceeding
       *
       * @brief Multiplication operator, with a number preceeding
       * @param a - The multiplier for the asset's amount
       * @param b - The asset to be multiplied
       * @return asset - New asset as the result of multiplication
       */
      friend asset operator*( int64_t b, const asset& a ) {
         asset result = a;
         result *= b;
         return result;
      }

      /**
       * Division assignment operator. Divide the amount of this asset with a number and then assign the value to itself.
       *
       * @brief Division assignment operator, with a number
       * @param a - The divisor for the asset's amount
       * @return asset - Reference to this asset
       * @post The amount of this asset is divided by a
       */
      asset& operator/=( int64_t a ) {
         amount /= a;
         return *this;
      }

      /**
       * Division operator, with a number proceeding
       *
       * @brief Division operator, with a number proceeding
       * @param a - The asset to be divided
       * @param b - The divisor for the asset's amount
       * @return asset - New asset as the result of division
       */
      friend asset operator/( const asset& a, int64_t b ) {
         asset result = a;
         result /= b;
         return result;
      }

      /**
       * Division operator, with another asset
       *
       * @brief Division operator, with another asset
       * @param a - The asset which amount acts as the dividend
       * @param b - The asset which amount acts as the divisor
       * @return int64_t - the resulted amount after the division
       * @pre Both asset must have the same symbol
       */
      friend int64_t operator/( const asset& a, const asset& b ) {
         eosio_assert( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
         return a.amount / b.amount;
      }

      /**
       * Equality operator
       *
       * @brief Equality operator
       * @param a - The first asset to be compared
       * @param b - The second asset to be compared
       * @return true - if both asset has the same amount
       * @return false - otherwise
       * @pre Both asset must have the same symbol
       */
      friend bool operator==( const asset& a, const asset& b ) {
         eosio_assert( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
         return a.amount == b.amount;
      }

      /**
       * Inequality operator
       *
       * @brief Inequality operator
       * @param a - The first asset to be compared
       * @param b - The second asset to be compared
       * @return true - if both asset doesn't have the same amount
       * @return false - otherwise
       * @pre Both asset must have the same symbol
       */
      friend bool operator!=( const asset& a, const asset& b ) {
         return !( a == b);
      }

      /**
       * Less than operator
       *
       * @brief Less than operator
       * @param a - The first asset to be compared
       * @param b - The second asset to be compared
       * @return true - if the first asset's amount is less than the second asset amount
       * @return false - otherwise
       * @pre Both asset must have the same symbol
       */
      friend bool operator<( const asset& a, const asset& b ) {
         eosio_assert( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
         return a.amount < b.amount;
      }

      /**
       * Less or equal to operator
       *
       * @brief Less or equal to operator
       * @param a - The first asset to be compared
       * @param b - The second asset to be compared
       * @return true - if the first asset's amount is less or equal to the second asset amount
       * @return false - otherwise
       * @pre Both asset must have the same symbol
       */
      friend bool operator<=( const asset& a, const asset& b ) {
         eosio_assert( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
         return a.amount <= b.amount;
      }

      /**
       * Greater than operator
       *
       * @brief Greater than operator
       * @param a - The first asset to be compared
       * @param b - The second asset to be compared
       * @return true - if the first asset's amount is greater than the second asset amount
       * @return false - otherwise
       * @pre Both asset must have the same symbol
       */
      friend bool operator>( const asset& a, const asset& b ) {
         eosio_assert( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
         return a.amount > b.amount;
      }

      /**
       * Greater or equal to operator
       *
       * @brief Greater or equal to operator
       * @param a - The first asset to be compared
       * @param b - The second asset to be compared
       * @return true - if the first asset's amount is greater or equal to the second asset amount
       * @return false - otherwise
       * @pre Both asset must have the same symbol
       */
      friend bool operator>=( const asset& a, const asset& b ) {
         eosio_assert( a.symbol == b.symbol, "comparison of assets with different symbols is not allowed" );
         return a.amount >= b.amount;
      }

      /**
       * %Print the asset
       *
       * @brief %Print the asset
       */
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

      EOSLIB_SERIALIZE( asset, (amount)(symbol) )
   };

  /**
   * \struct Extended asset which stores the information of the owner of the asset
   *
   * @brief Extended asset which stores the information of the owner of the asset
   */
   struct extended_asset : public asset {
      /**
       * The owner of the asset
       *
       * @brief The owner of the asset
       */
      account_name contract;

      /**
       * Get the extended symbol of the asset
       *
       * @brief Get the extended symbol of the asset
       * @return extended_symbol - The extended symbol of the asset
       */
      extended_symbol get_extended_symbol()const { return extended_symbol( symbol, contract ); }

      /**
       * Default constructor
       *
       * @brief Construct a new extended asset object
       */
      extended_asset() = default;

       /**
       * Construct a new extended asset given the amount and extended symbol
       *
       * @brief Construct a new extended asset object
       */
      extended_asset( int64_t v, extended_symbol s ):asset(v,s),contract(s.contract){}
      /**
       * Construct a new extended asset given the asset and owner name
       *
       * @brief Construct a new extended asset object
       */
      extended_asset( asset a, account_name c ):asset(a),contract(c){}

      /**
       * %Print the extended asset
       *
       * @brief %Print the extended asset
       */
      void print()const {
         asset::print();
         prints("@");
         printn(contract);
      }

       /**
       *  Unary minus operator
       *
       *  @brief Unary minus operator
       *  @return extended_asset - New extended asset with its amount is the negative amount of this extended asset
       */
      extended_asset operator-()const {
         asset r = this->asset::operator-();
         return {r, contract};
      }

      /**
       * Subtraction operator. This subtracts the amount of the extended asset.
       *
       * @brief Subtraction operator
       * @param a - The extended asset to be subtracted
       * @param b - The extended asset used to subtract
       * @return extended_asset - New extended asset as the result of subtraction
       * @pre The owner of both extended asset need to be the same
       */
      friend extended_asset operator - ( const extended_asset& a, const extended_asset& b ) {
         eosio_assert( a.contract == b.contract, "type mismatch" );
         asset r = static_cast<const asset&>(a) - static_cast<const asset&>(b);
         return {r, a.contract};
      }

      /**
       * Addition operator. This adds the amount of the extended asset.
       *
       * @brief Addition operator
       * @param a - The extended asset to be added
       * @param b - The extended asset to be added
       * @return extended_asset - New extended asset as the result of addition
       * @pre The owner of both extended asset need to be the same
       */
      friend extended_asset operator + ( const extended_asset& a, const extended_asset& b ) {
         eosio_assert( a.contract == b.contract, "type mismatch" );
         asset r = static_cast<const asset&>(a) + static_cast<const asset&>(b);
         return {r, a.contract};
      }

      EOSLIB_SERIALIZE( extended_asset, (amount)(symbol)(contract) )
   };

/// @} asset type
} /// namespace eosio
