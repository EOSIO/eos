#include <eosio/chain/name.hpp>
#include <fc/variant.hpp>
#include <boost/algorithm/string.hpp>

namespace eosio::chain {

   void name::set( std::string_view str ) {
      value = string_to_uint64_t(str);
   }

   // keep in sync with name::to_string() in contract definition for name
   std::string name::to_string()const {
     static const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";

      std::string str(13,'.');

      uint64_t tmp = value;
      for( uint32_t i = 0; i <= 12; ++i ) {
         char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
         str[12-i] = c;
         tmp >>= (i == 0 ? 4 : 5);
      }

      boost::algorithm::trim_right_if( str, []( char c ){ return c == '.'; } );
      return str;
   }

   bool is_string_valid_name(std::string_view str)
   {
      size_t slen = str.size();
      if( slen > 13)
         return false;

      size_t len = (slen <= 12) ? slen : 12;
      for( size_t i = 0; i < len; ++i ) {
         char c = str[i];
         if ((c >= 'a' && c <= 'z') || (c >= '1' && c <= '5') || (c == '.'))
            continue;
         else
            return false;
      }

      if( slen == 13) {
         char c = str[12];
         if ((c >= 'a' && c <= 'j') || (c >= '1' && c <= '5') || (c == '.'))
            return true;
         else
            return false;
      }

      return true;
   }

} // eosio::chain

namespace fc {
  void to_variant(const eosio::chain::name& c, fc::variant& v) { v = c.to_string(); }
  void from_variant(const fc::variant& v, eosio::chain::name& check) { check.set( v.get_string() ); }
} // fc
