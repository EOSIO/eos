#include <eosio/chain/name.hpp>
#include <fc/variant.hpp>
#include <boost/algorithm/string.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio::chain {

   void name::set( std::string_view str ) {
      const auto len = str.size();
      EOS_ASSERT(len <= NAME_LEN_LIMIT, name_type_exception, "Name is longer than 13 characters (${name}) ", ("name", std::string(str)));
      value = string_to_uint64_t(str);
      EOS_ASSERT(to_string() == str, name_type_exception,
                 "Name not properly normalized (name: ${name}, normalized: ${normalized}) ",
                 ("name", std::string(str))("normalized", to_string()));
   }

   // keep in sync with name::to_string() in contract definition for name
   std::string name::to_string()const {
     static const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";

      std::string str(NAME_LEN_LIMIT,'.');

      uint64_t tmp = value;
      for( uint8_t i = 0; i <= NAME_LEN_LIMIT-1; ++i ) {
         char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
         str[NAME_LEN_LIMIT-1-i] = c;
         tmp >>= (i == 0 ? 4 : 5);
      }

      boost::algorithm::trim_right_if( str, []( char c ){ return c == '.'; } );
      return str;
   }

} // eosio::chain

namespace fc {
  void to_variant(const eosio::chain::name& c, fc::variant& v) { v = c.to_string(); }
  void from_variant(const fc::variant& v, eosio::chain::name& check) { check.set( v.get_string() ); }
} // fc
