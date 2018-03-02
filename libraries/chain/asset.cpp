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
         if ( intpart[0] == '-' ) {
            result.amount -= int64_t(fc::to_int64(fractpart));
            result.amount += int64_t(result.precision());
         } else {
            result.amount += int64_t(fc::to_int64(fractpart));
            result.amount -= int64_t(result.precision());
         }
         
      }
      return result;
   }
   FC_CAPTURE_LOG_AND_RETHROW( (from) )
}

} }  // eosio::types
