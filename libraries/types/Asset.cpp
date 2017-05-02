#include <eos/types/Asset.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <fc/reflect/variant.hpp>

namespace eos { namespace types {
      typedef boost::multiprecision::int128_t  int128_t;

      uint8_t Asset::decimals()const {
         auto a = (const char*)&symbol;
         return a[0];
      }
      void Asset::set_decimals(uint8_t d){
         auto a = (char*)&symbol;
         a[0] = d;
      }

      String Asset::symbol_name()const {
         auto a = (const char*)&symbol;
         assert( a[7] == 0 );
         return &a[1];
      }

       int64_t Asset::precision()const {

         static int64_t table[] = {
                           1, 10, 100, 1000, 10000,
                           100000, 1000000, 10000000, 100000000ll,
                           1000000000ll, 10000000000ll,
                           100000000000ll, 1000000000000ll,
                           10000000000000ll, 100000000000000ll
                         };
         return table[ decimals() ];
      }

      String Asset::toString()const {
         String result = fc::to_string( static_cast<int64_t>(amount) / precision());
         if( decimals() )
         {
            auto fract = static_cast<int64_t>(amount) % precision();
            result += "." + fc::to_string(precision() + fract).erase(0,1);
         }
         return result + " " + symbol_name();
      }

      Asset Asset::fromString( const String& from )
      {
         try
         {
            String s = fc::trim( from );
            auto space_pos = s.find( " " );
            auto dot_pos = s.find( "." );

            Asset result;
            result.symbol = uint64_t(0);
            auto sy = (char*)&result.symbol;
            *sy = (char) dot_pos; // Mask due to undefined architecture behavior

            auto intpart = s.substr( 0, dot_pos );
            result.amount = fc::to_int64(intpart);
            String fractpart;
            if( dot_pos != String::npos )
            {
               auto fractpart = "1" + s.substr( dot_pos + 1, space_pos - dot_pos - 1 );
               result.set_decimals( fractpart.size() - 1 );

               result.amount *= Int64(result.precision());
               result.amount += Int64(fc::to_int64(fractpart));
               result.amount -= Int64(result.precision());
            }
            auto symbol = s.substr( space_pos + 1 );

            if( symbol.size() )
               memcpy( sy+1, symbol.c_str(), std::min(symbol.size(),size_t(6)) );

            return result;
         }
         FC_CAPTURE_LOG_AND_RETHROW( (from) )
      }

      bool operator == ( const Price& a, const Price& b )
      {
         if( std::tie( a.base.symbol, a.quote.symbol ) != std::tie( b.base.symbol, b.quote.symbol ) )
             return false;

         const auto amult = UInt128( b.quote.amount ) * UInt128(a.base.amount);
         const auto bmult = UInt128( a.quote.amount ) * UInt128(b.base.amount);

         return amult == bmult;
      }

      bool operator < ( const Price& a, const Price& b )
      {
         if( a.base.symbol < b.base.symbol ) return true;
         if( a.base.symbol > b.base.symbol ) return false;
         if( a.quote.symbol < b.quote.symbol ) return true;
         if( a.quote.symbol > b.quote.symbol ) return false;

         const auto amult = UInt128( b.quote.amount ) * UInt128(a.base.amount);
         const auto bmult = UInt128( a.quote.amount ) * UInt128(b.base.amount);

         return amult < bmult;
      }

      bool operator <= ( const Price& a, const Price& b )
      {
         return (a == b) || (a < b);
      }

      bool operator != ( const Price& a, const Price& b )
      {
         return !(a == b);
      }

      bool operator > ( const Price& a, const Price& b )
      {
         return !(a <= b);
      }

      bool operator >= ( const Price& a, const Price& b )
      {
         return !(a < b);
      }

      Asset operator * ( const Asset& a, const Price& b )
      {
         if( a.symbol_name() == b.base.symbol_name() )
         {
            FC_ASSERT( static_cast<int64_t>(b.base.amount) > 0 );
            auto result = (UInt128(a.amount) * UInt128(b.quote.amount))/UInt128(b.base.amount);
            return Asset( Int64(result), b.quote.symbol );
         }

         else if( a.symbol_name() == b.quote.symbol_name() )
         {
            FC_ASSERT( static_cast<int64_t>(b.quote.amount) > 0 );
            auto result = (UInt128(a.amount) *UInt128(b.base.amount))/UInt128(b.quote.amount);
            return Asset( Int64(result), b.base.symbol );
         }
         FC_THROW_EXCEPTION( fc::assert_exception, "invalid Asset * Price", ("Asset",a)("Price",b) );
      }

      Price operator / ( const Asset& base, const Asset& quote )
      { try {
         FC_ASSERT( base.symbol_name() != quote.symbol_name() );
         return Price{ base, quote };
      } FC_CAPTURE_AND_RETHROW( (base)(quote) ) }

      Price Price::max( AssetSymbol base, AssetSymbol quote ) { return Asset( ShareType(EOS_MAX_SHARE_SUPPLY), base ) / Asset( ShareType(1), quote); }
      Price Price::min( AssetSymbol base, AssetSymbol quote ) { return Asset( 1, base ) / Asset( EOS_MAX_SHARE_SUPPLY, quote); }

      bool Price::is_null() const { return *this == Price(); }

      void Price::validate() const
      { try {
         FC_ASSERT( base.amount > ShareType(0) );
         FC_ASSERT( quote.amount > ShareType(0) );
         FC_ASSERT( base.symbol_name() != quote.symbol_name() );
      } FC_CAPTURE_AND_RETHROW( (base)(quote) ) }


}}  // eos::types
