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
   return sym.decimals();
}

string asset::symbol_name()const {
   return sym.name();
}

int64_t asset::precision()const {
   return sym.precision();
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
   try { 
      string s = fc::trim(from);
      auto dot_pos = s.find(".");
      FC_ASSERT(dot_pos != string::npos, "dot missing in asset from string");
      auto space_pos = s.find(" ", dot_pos);
      FC_ASSERT(space_pos != string::npos, "space missing in asset from string");

      asset result;
      
      auto intpart = s.substr(0, dot_pos);
      result.amount = fc::to_int64(intpart);
      string symbol_part;
      if (dot_pos != string::npos && space_pos != string::npos) {
         symbol_part = eosio::chain::to_string(space_pos - dot_pos - 1);
         symbol_part += ',';
         symbol_part += s.substr(space_pos + 1);
      }
      result.sym = symbol::from_string(symbol_part);
      if (dot_pos != string::npos) {
         auto fractpart = "1" + s.substr(dot_pos + 1, space_pos - dot_pos - 1);
         
         result.amount *= int64_t(result.precision());
         result.amount += int64_t(fc::to_int64(fractpart));
         result.amount -= int64_t(result.precision());
      }
      
      return result;
   }
   FC_CAPTURE_LOG_AND_RETHROW( (from) )
}

bool operator == ( const price& a, const price& b )
{
   if (a.base.symbol() != b.base.symbol() || a.quote.symbol() != b.quote.symbol())
      return false;
   const auto amult = uint128_t( b.quote.amount ) * uint128_t(a.base.amount);
   const auto bmult = uint128_t( a.quote.amount ) * uint128_t(b.base.amount);

   return amult == bmult;
}

bool operator < ( const price& a, const price& b )
{
   if( a.base.symbol() < b.base.symbol() ) return true;
   if( a.base.symbol() > b.base.symbol() ) return false;
   if( a.quote.symbol() < b.quote.symbol() ) return true;
   if( a.quote.symbol() > b.quote.symbol() ) return false;

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
      return asset( int64_t(result), b.quote.symbol() );
   }

   else if( a.symbol_name() == b.quote.symbol_name() )
   {
      FC_ASSERT( static_cast<int64_t>(b.quote.amount) > 0 );
      auto result = (uint128_t(a.amount) *uint128_t(b.base.amount))/uint128_t(b.quote.amount);
      return asset( int64_t(result), b.base.symbol() );
   }
   FC_THROW_EXCEPTION( fc::assert_exception, "invalid asset * price", ("asset",a)("price",b) );
}

price operator / ( const asset& base, const asset& quote )
try {
   FC_ASSERT( base.symbol_name() != quote.symbol_name() );
   return price{ base, quote };
} FC_CAPTURE_AND_RETHROW( (base)(quote) )

price price::max( const symbol& base, const symbol& quote ) { return asset( share_type(EOS_MAX_SHARE_SUPPLY), base ) / asset( share_type(1), quote); }
price price::min( const symbol& base, const symbol& quote ) { return asset( 1, base ) / asset( EOS_MAX_SHARE_SUPPLY, quote); }

bool price::is_null() const { return *this == price(); }

void price::validate() const
try {
   FC_ASSERT( base.amount > share_type(0) );
   FC_ASSERT( quote.amount > share_type(0) );
   FC_ASSERT( base.symbol_name() != quote.symbol_name() );
} FC_CAPTURE_AND_RETHROW( (base)(quote) )


} }  // eosio::types
