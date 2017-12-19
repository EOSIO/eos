/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/exception/exception.hpp>
#include <eos/types/native.hpp>

/// eos with 4 digits of precision
#define EOS_SYMBOL  (int64_t(4) | (uint64_t('E') << 8) | (uint64_t('O') << 16) | (uint64_t('S') << 24))

/// Defined to be largest power of 10 that fits in 53 bits of precision
#define EOS_MAX_SHARE_SUPPLY   int64_t(1'000'000'000'000'000ll)

namespace eosio { namespace types {

   using asset_symbol = uint64_t;
   using share_type   = int64;

   struct asset
   {
      asset(share_type a = 0, asset_symbol id = EOS_SYMBOL)
      :amount(a),symbol(id){}

      share_type   amount;
      asset_symbol symbol;

      double to_real()const { return static_cast<double>(amount) / precision(); }

      uint8_t     decimals()const;
      string      symbol_name()const;
      int64_t     precision()const;
      void        set_decimals(uint8_t d);

      static asset from_string(const string& from);
      string       to_string()const;

      asset& operator += (const asset& o)
      {
         FC_ASSERT(symbol == o.symbol);
         amount += o.amount;
         return *this;
      }

      asset& operator -= (const asset& o)
      {
         FC_ASSERT(symbol == o.symbol);
         amount -= o.amount;
         return *this;
      }
      asset operator -()const { return asset(-amount, symbol); }

      friend bool operator == (const asset& a, const asset& b)
      {
         return std::tie(a.symbol, a.amount) == std::tie(b.symbol, b.amount);
      }
      friend bool operator < (const asset& a, const asset& b)
      {
         FC_ASSERT(a.symbol == b.symbol);
         return std::tie(a.amount,a.symbol) < std::tie(b.amount,b.symbol);
      }
      friend bool operator <= (const asset& a, const asset& b) { return (a == b) || (a < b); }
      friend bool operator != (const asset& a, const asset& b) { return !(a == b); }
      friend bool operator > (const asset& a, const asset& b)  { return !(a <= b); }
      friend bool operator >= (const asset& a, const asset& b) { return !(a < b);  }

      friend asset operator - (const asset& a, const asset& b) {
         FC_ASSERT(a.symbol == b.symbol);
         return asset(a.amount - b.amount, a.symbol);
      }

      friend asset operator + (const asset& a, const asset& b) {
         FC_ASSERT(a.symbol == b.symbol);
         return asset(a.amount + b.amount, a.symbol);
      }

      friend std::ostream& operator << (std::ostream& out, const asset& a) { return out << a.to_string(); }

   };

   struct price
   {
      asset base;
      asset quote;

      price(const asset& base = asset(), const asset quote = asset())
      :base(base),quote(quote){}

      static price max(asset_symbol base, asset_symbol quote);
      static price min(asset_symbol base, asset_symbol quote);

      price max()const { return price::max(base.symbol, quote.symbol); }
      price min()const { return price::min(base.symbol, quote.symbol); }

      double to_real()const { return base.to_real() / quote.to_real(); }

      bool is_null()const;
      void validate()const;
   };

   price operator / (const asset& base, const asset& quote);
   inline price operator~(const price& p) { return price{p.quote,p.base}; }

   bool  operator <  (const asset& a, const asset& b);
   bool  operator <= (const asset& a, const asset& b);
   bool  operator <  (const price& a, const price& b);
   bool  operator <= (const price& a, const price& b);
   bool  operator >  (const price& a, const price& b);
   bool  operator >= (const price& a, const price& b);
   bool  operator == (const price& a, const price& b);
   bool  operator != (const price& a, const price& b);
   asset operator *  (const asset& a, const price& b);

}} // namespace eosio::types

namespace fc {
    inline void to_variant(const eosio::types::asset& var, fc::variant& vo) { vo = var.to_string(); }
    inline void from_variant(const fc::variant& var, eosio::types::asset& vo) {
       vo = eosio::types::asset::from_string(var.get_string());
    }
}

FC_REFLECT(eosio::types::asset, (amount)(symbol))
FC_REFLECT(eosio::types::price, (base)(quote))

