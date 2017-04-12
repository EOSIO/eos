#pragma once
#include <fc/exception/exception.hpp>
#include <string>
#include <eos/types/ints.hpp>

/// EOS with 8 digits of precision
#define EOS_SYMBOL  (uint64_t(8) | (uint64_t('E') << 8) | (uint64_t('O') << 16) | (uint64_t('S') << 24) ) 

/// VOTE with 8 digits of precision
#define VOTE_SYMBOL  (uint64_t(8) | (uint64_t('V') << 8) | (uint64_t('O') << 16) | (uint64_t('T') << 24) | (uint64_t('E') << 32) ) 

/// Defined to be largest power of 10 that fits in 53 bits of precision
#define EOS_MAX_SHARE_SUPPLY   int64_t(1000000000000000ll)

namespace EOS { 

   typedef uint64_t AssetSymbol;
   typedef Int64    ShareType;
   using std::string;


   struct Asset
   {
      Asset( ShareType a = 0, AssetSymbol id = EOS_SYMBOL )
      :amount(a),symbol(id){}

      ShareType   amount;
      AssetSymbol symbol;

      double to_real()const {
         return static_cast<double>(amount) / precision();
      }

      uint8_t     decimals()const;
      std::string symbol_name()const;
      int64_t     precision()const;
      void        set_decimals(uint8_t d);

      static Asset fromString( const string& from );
      string       toString()const;

      Asset& operator += ( const Asset& o )
      {
         FC_ASSERT( symbol == o.symbol );
         amount += o.amount;
         return *this;
      }

      Asset& operator -= ( const Asset& o )
      {
         FC_ASSERT( symbol == o.symbol );
         amount -= o.amount;
         return *this;
      }
      Asset operator -()const { return Asset( -amount, symbol ); }

      friend bool operator == ( const Asset& a, const Asset& b )
      {
         return std::tie( a.symbol, a.amount ) == std::tie( b.symbol, b.amount );
      }
      friend bool operator < ( const Asset& a, const Asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return std::tie(a.amount,a.symbol) < std::tie(b.amount,b.symbol);
      }
      friend bool operator <= ( const Asset& a, const Asset& b )
      {
         return (a == b) || (a < b);
      }

      friend bool operator != ( const Asset& a, const Asset& b )
      {
         return !(a == b);
      }
      friend bool operator > ( const Asset& a, const Asset& b )
      {
         return !(a <= b);
      }
      friend bool operator >= ( const Asset& a, const Asset& b )
      {
         return !(a < b);
      }

      friend Asset operator - ( const Asset& a, const Asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return Asset( a.amount - b.amount, a.symbol );
      }
      friend Asset operator + ( const Asset& a, const Asset& b )
      {
         FC_ASSERT( a.symbol == b.symbol );
         return Asset( a.amount + b.amount, a.symbol );
      }

   };

   struct Price
   {
      Price(const Asset& base = Asset(), const Asset quote = Asset())
         : base(base),quote(quote){}

      Asset base;
      Asset quote;

      static Price max(AssetSymbol base, AssetSymbol quote );
      static Price min(AssetSymbol base, AssetSymbol quote );

      Price max()const { return Price::max( base.symbol, quote.symbol ); }
      Price min()const { return Price::min( base.symbol, quote.symbol ); }

      double to_real()const { return base.to_real() / quote.to_real(); }

      bool is_null()const;
      void validate()const;
   };

   Price operator / ( const Asset& base, const Asset& quote );
   inline Price operator~( const Price& p ) { return Price{p.quote,p.base}; }

   bool  operator <  ( const Asset& a, const Asset& b );
   bool  operator <= ( const Asset& a, const Asset& b );
   bool  operator <  ( const Price& a, const Price& b );
   bool  operator <= ( const Price& a, const Price& b );
   bool  operator >  ( const Price& a, const Price& b );
   bool  operator >= ( const Price& a, const Price& b );
   bool  operator == ( const Price& a, const Price& b );
   bool  operator != ( const Price& a, const Price& b );
   Asset operator *  ( const Asset& a, const Price& b );

   inline fc::variant toVariant( const Asset& v ) { return v.toString(); }
   inline void        fromVariant( Asset& v, const fc::variant& var )  { v = Asset::fromString( var.get_string() ); }

    template<typename Stream>
    void toBinary( Stream& stream, const Asset& t ) { 
        EOS::toBinary( stream, t.amount );
        EOS::toBinary( stream, t.symbol );
    }

    template<typename Stream>
    void fromBinary( Stream& stream, Asset& t ) { 
        EOS::fromBinary( stream, t.amount );
        EOS::fromBinary( stream, t.symbol );
    }

    template<typename Stream>
    void toBinary( Stream& stream, const Price& t ) { 
        EOS::toBinary( stream, t.base );
        EOS::toBinary( stream, t.quote );
    }

    template<typename Stream>
    void fromBinary( Stream& stream, Price& t ) { 
        EOS::fromBinary( stream, t.base );
        EOS::fromBinary( stream, t.quote );
    }

} // namespace EOS

namespace fc {
    inline void to_variant( const EOS::Asset& var,  fc::variant& vo )   { vo = var.toString(); }
    inline void from_variant( const fc::variant& var,  EOS::Asset& vo ) { vo = EOS::Asset::fromString( var.get_string() ); }
}

FC_REFLECT( EOS::Asset, (amount)(symbol) )
FC_REFLECT( EOS::Price, (base)(quote) )

