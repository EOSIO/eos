#include <enumivo/chain/name.hpp>
#include <fc/variant.hpp>
#include <boost/algorithm/string.hpp>
#include <fc/exception/exception.hpp>
#include <enumivo/chain/exceptions.hpp>

namespace enumivo { namespace chain { 

   void name::set( const char* str ) {
      const auto len = strnlen(str, 14);

      //////////////////////////////////////////////////////////////////////////////////
      //the code below is exlusive for enumivo to treat enumivo.prods as a special case
      if (string(str) == "enumivo.prods"){
        value = string_to_name(str);
        return; //allow special account
      }
      ENU_ASSERT(string_to_name("enumivo.prods") != string_to_name(str), name_type_exception, "Name will collide with enumivo.prods (${name}) ", ("name", string(str)));
      //////////////////////////////////////////////////////////////////////////////////

      ENU_ASSERT(len <= 13, name_type_exception, "Name is longer than 13 characters (${name}) ", ("name", string(str)));
      value = string_to_name(str);
      ENU_ASSERT(to_string() == string(str), name_type_exception,
                 "Name not properly normalized (name: ${name}, normalized: ${normalized}) ",
                 ("name", string(str))("normalized", to_string()));
   }

   // keep in sync with name::to_string() in contract definition for name
   name::operator string()const {
      static const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";

      //////////////////////////////////////////////////////////////////////////////////
      //the code below is exlusive for enumivo to treat enumivo.prods as a special case
      if (value == string_to_name("enumivo.prods")){
        return "enumivo.prods";
      }
      //////////////////////////////////////////////////////////////////////////////////

      string str(13,'.');

      uint64_t tmp = value;
      for( uint32_t i = 0; i <= 12; ++i ) {
          char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
          str[12-i] = c;
          tmp >>= (i == 0 ? 4 : 5);
      }

      boost::algorithm::trim_right_if( str, []( char c ){ return c == '.'; } );
      return str;
   }

} } /// enumivo::chain

namespace fc {
  void to_variant(const enumivo::chain::name& c, fc::variant& v) { v = std::string(c); }
  void from_variant(const fc::variant& v, enumivo::chain::name& check) { check = v.get_string(); }
} // fc
