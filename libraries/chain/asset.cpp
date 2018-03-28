/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/asset.hpp>
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <fc/reflect/variant.hpp>
#include <eosio/chain/exceptions.hpp>

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
      asset result;

      string s = fc::trim(from);

      // Find space in order to split amount and symbol
      auto space_pos = s.find(' ');
      EOS_ASSERT((space_pos != string::npos), asset_type_exception, "Asset's amount and symbol should be separated with space");
      auto symbol_str = fc::trim(s.substr(space_pos + 1));
      auto amount_str = s.substr(0, space_pos);

      // Ensure that if decimal point is used (.), decimal fraction is specified
      auto dot_pos = amount_str.find('.');
      if (dot_pos != string::npos) {
         EOS_ASSERT((dot_pos != amount_str.size() - 1), asset_type_exception, "Missing decimal fraction after decimal point");
      }

      // Parse symbol
      string precision_digit_str;
      if (dot_pos != string::npos) {
         precision_digit_str = eosio::chain::to_string(amount_str.size() - dot_pos - 1);
      } else {
         precision_digit_str = "0";
      }
      string symbol_part = precision_digit_str + ',' + symbol_str;
      result.sym = symbol::from_string(symbol_part);

      // Parse amount
      int64_t int_part, fract_part = 0;
      if (dot_pos != string::npos) {
         int_part = fc::to_int64(amount_str.substr(0, dot_pos));
         fract_part = fc::to_int64(amount_str.substr(dot_pos + 1));
         if (amount_str[0] == '-') fract_part *= -1;
      } else {
         int_part = fc::to_int64(amount_str);
      }
      result.amount = int_part;
      result.amount *= int64_t(result.precision());
      result.amount += fract_part;

      return result;
   }
   FC_CAPTURE_LOG_AND_RETHROW( (from) )
}

} }  // eosio::types
