#pragma once
#include <fc/exception/exception.hpp>
#include <eos/types/native.hpp>

/// eos with 8 digits of precision
#define EOS_SYMBOL  (int64_t(4) | (uint64_t('E') << 8) | (uint64_t('O') << 16) | (uint64_t('S') << 24))

/// Defined to be largest power of 10 that fits in 53 bits of precision
#define EOS_MAX_SHARE_SUPPLY   int64_t(1'000'000'000'000'000ll)

namespace eos { namespace types {

   using AssetSymbol = uint64_t;
   using ShareType   = Int64;

   struct Asset
   {
      Asset(ShareType a = 0, AssetSymbol id = EOS_SYMBOL)
      :amount(a),symbol(id){}

      ShareType   amount;
      AssetSymbol symbol;

      double to_real()const { return static_cast<double>(amount) / precision(); }

      uint8_t     decimals()const;
      String      symbol_name()const;
      int64_t     precision()const;
      void        set_decimals(uint8_t d);

      static Asset fromString(const String& from);
      String       toString()const;

      Asset& operator += (const Asset& o)
      {
         FC_ASSERT(symbol == o.symbol);
         amount += o.amount;
         return *this;
      }

      Asset& operator -= (const Asset& o)
      {
         FC_ASSERT(symbol == o.symbol);
         amount -= o.amount;
         return *this;
      }
      Asset operator -()const { return Asset(-amount, symbol); }

      friend bool operator == (const Asset& a, const Asset& b)
      {
         return std::tie(a.symbol, a.amount) == std::tie(b.symbol, b.amount);
      }
      friend bool operator < (const Asset& a, const Asset& b)
      {
         FC_ASSERT(a.symbol == b.symbol);
         return std::tie(a.amount,a.symbol) < std::tie(b.amount,b.symbol);
      }
      friend bool operator <= (const Asset& a, const Asset& b) { return (a == b) || (a < b); }
      friend bool operator != (const Asset& a, const Asset& b) { return !(a == b); }
      friend bool operator > (const Asset& a, const Asset& b)  { return !(a <= b); }
      friend bool operator >= (const Asset& a, const Asset& b) { return !(a < b);  }

      friend Asset operator - (const Asset& a, const Asset& b) {
         FC_ASSERT(a.symbol == b.symbol);
         return Asset(a.amount - b.amount, a.symbol);
      }

      friend Asset operator + (const Asset& a, const Asset& b) {
         FC_ASSERT(a.symbol == b.symbol);
         return Asset(a.amount + b.amount, a.symbol);
      }

      friend std::ostream& operator << (std::ostream& out, const Asset& a) { return out << a.toString(); }

   };

   struct Price
   {
      Asset base;
      Asset quote;

      Price(const Asset& base = Asset(), const Asset quote = Asset())
      :base(base),quote(quote){}

      static Price max(AssetSymbol base, AssetSymbol quote);
      static Price min(AssetSymbol base, AssetSymbol quote);

      Price max()const { return Price::max(base.symbol, quote.symbol); }
      Price min()const { return Price::min(base.symbol, quote.symbol); }

      double to_real()const { return base.to_real() / quote.to_real(); }

      bool is_null()const;
      void validate()const;
   };

   Price operator / (const Asset& base, const Asset& quote);
   inline Price operator~(const Price& p) { return Price{p.quote,p.base}; }

   bool  operator <  (const Asset& a, const Asset& b);
   bool  operator <= (const Asset& a, const Asset& b);
   bool  operator <  (const Price& a, const Price& b);
   bool  operator <= (const Price& a, const Price& b);
   bool  operator >  (const Price& a, const Price& b);
   bool  operator >= (const Price& a, const Price& b);
   bool  operator == (const Price& a, const Price& b);
   bool  operator != (const Price& a, const Price& b);
   Asset operator *  (const Asset& a, const Price& b);

}} // namespace eos::types

namespace fc {
    inline void to_variant(const eos::types::Asset& var, fc::variant& vo) { vo = var.toString(); }
    inline void from_variant(const fc::variant& var, eos::types::Asset& vo) {
       vo = eos::types::Asset::fromString(var.get_string());
    }
}

FC_REFLECT(eos::types::Asset, (amount)(symbol))
FC_REFLECT(eos::types::Price, (base)(quote))

