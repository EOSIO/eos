/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/asset.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <fc/reflect/variant.hpp>

namespace eosio { namespace chain {
typedef boost::multiprecision::int128_t  int128_t;

uint8_t asset::decimals()const {
   auto a = (const char*)&symbol;
   return a[0];
}
void asset::set_decimals(uint8_t d){
   auto a = (char*)&symbol;
   a[0] = d;
}

string asset::symbol_name()const {
   auto a = (const char*)&symbol;
   assert( a[7] == 0 );
   return &a[1];
}

int64_t asset::precision()const {

   static int64_t table[] = {
      1, 10, 100, 1000, 10000,
      100000, 1000000, 10000000, 100000000ll,
      1000000000ll, 10000000000ll,
      100000000000ll, 1000000000000ll,
      10000000000000ll, 100000000000000ll
   };
   return table[ decimals() ];
}

string asset::to_string()const {
   string result = fc::to_string( static_cast<int64_t>(amount) / precision());
   if( decimals() )
   {
      auto fract = static_cast<int64_t>(amount) % precision();
      result += "." + fc::to_string(precision() + fract).erase(0,1);
   }
   return result + " " + symbol_name();
}

asset asset::from_string(const string& from)
{
   try
   {
      string s = fc::trim( from );
      auto space_pos = s.find( " " );
      auto dot_pos = s.find( "." );

      asset result;
      result.symbol = uint64_t(0);
      auto sy = (char*)&result.symbol;
      *sy = (char) dot_pos; // Mask due to undefined architecture behavior

      auto intpart = s.substr( 0, dot_pos );
      result.amount = fc::to_int64(intpart);
      string fractpart;
      if( dot_pos != string::npos )
      {
         auto fractpart = "1" + s.substr( dot_pos + 1, space_pos - dot_pos - 1 );
         result.set_decimals( fractpart.size() - 1 );

         result.amount *= int64_t(result.precision());
         result.amount += int64_t(fc::to_int64(fractpart));
         result.amount -= int64_t(result.precision());
      }
      auto symbol = s.substr( space_pos + 1 );

      if( symbol.size() )
         memcpy( sy+1, symbol.c_str(), std::min(symbol.size(),size_t(6)) );

      return result;
   }
   FC_CAPTURE_LOG_AND_RETHROW( (from) )
}

bool operator == ( const price& a, const price& b )
{
   if( std::tie( a.base.symbol, a.quote.symbol ) != std::tie( b.base.symbol, b.quote.symbol ) )
      return false;

   const auto amult = uint128_t( b.quote.amount ) * uint128_t(a.base.amount);
   const auto bmult = uint128_t( a.quote.amount ) * uint128_t(b.base.amount);

   return amult == bmult;
}

bool operator < ( const price& a, const price& b )
{
   if( a.base.symbol < b.base.symbol ) return true;
   if( a.base.symbol > b.base.symbol ) return false;
   if( a.quote.symbol < b.quote.symbol ) return true;
   if( a.quote.symbol > b.quote.symbol ) return false;

   const auto amult = uint128_t( b.quote.amount ) * uint128_t(a.base.amount);
   const auto bmult = uint128_t( a.quote.amount ) * uint128_t(b.base.amount);

   return amult < bmult;
}

bool operator <= ( const price& a, const price& b )
{
   return (a == b) || (a < b);
}

bool operator != ( const price& a, const price& b )
{
   return !(a == b);
}

bool operator > ( const price& a, const price& b )
{
   return !(a <= b);
}

bool operator >= ( const price& a, const price& b )
{
   return !(a < b);
}

asset operator * ( const asset& a, const price& b )
{
   if( a.symbol_name() == b.base.symbol_name() )
   {
      FC_ASSERT( static_cast<int64_t>(b.base.amount) > 0 );
      auto result = (uint128_t(a.amount) * uint128_t(b.quote.amount))/uint128_t(b.base.amount);
      return asset( int64_t(result), b.quote.symbol );
   }

   else if( a.symbol_name() == b.quote.symbol_name() )
   {
      FC_ASSERT( static_cast<int64_t>(b.quote.amount) > 0 );
      auto result = (uint128_t(a.amount) *uint128_t(b.base.amount))/uint128_t(b.quote.amount);
      return asset( int64_t(result), b.base.symbol );
   }
   FC_THROW_EXCEPTION( fc::assert_exception, "invalid asset * price", ("asset",a)("price",b) );
}

price operator / ( const asset& base, const asset& quote )
try {
   FC_ASSERT( base.symbol_name() != quote.symbol_name() );
   return price{ base, quote };
} FC_CAPTURE_AND_RETHROW( (base)(quote) )

price price::max( asset_symbol base, asset_symbol quote ) { return asset( share_type(EOS_MAX_SHARE_SUPPLY), base ) / asset( share_type(1), quote); }
price price::min( asset_symbol base, asset_symbol quote ) { return asset( 1, base ) / asset( EOS_MAX_SHARE_SUPPLY, quote); }

bool price::is_null() const { return *this == price(); }

void price::validate() const
try {
   FC_ASSERT( base.amount > share_type(0) );
   FC_ASSERT( quote.amount > share_type(0) );
   FC_ASSERT( base.symbol_name() != quote.symbol_name() );
} FC_CAPTURE_AND_RETHROW( (base)(quote) )


} }  // eosio::types