#pragma once
#include <string>
#include <fc/reflect/reflect.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include <iosfwd>

namespace eosio::chain {
  struct name;
}
namespace fc {
  class variant;
  void to_variant(const eosio::chain::name& c, fc::variant& v);
  void from_variant(const fc::variant& v, eosio::chain::name& check);
} // fc

namespace eosio::chain {
   constexpr uint64_t char_to_symbol( char c ) {
      if( c >= 'a' && c <= 'z' )
         return (c - 'a') + 6;
      if( c >= '1' && c <= '5' )
         return (c - '1') + 1;
      else if( c == '.')
         return 0;
      else
         FC_THROW_EXCEPTION(name_type_exception, "Name contains invalid character: (${c}) ", ("c", std::string(1, c)));

      //unreachable
      return 0;
   }

   // true if std::string can be converted to name
   bool is_string_valid_name(std::string_view str);

   constexpr uint64_t string_to_uint64_t( std::string_view str ) {
      EOS_ASSERT(str.size() <= 13, name_type_exception, "Name is longer than 13 characters (${name}) ", ("name", std::string(str)));

      uint64_t n = 0;
      int i = (int) str.size();
      if (i >= 13) {
         // Only the first 12 characters can be full-range ([.1-5a-z]).
         i = 12;

         // The 13th character must be in the range [.1-5a-j] because it needs to be encoded
         // using only four bits (64_bits - 5_bits_per_char * 12_chars).
         n = char_to_symbol(str[12]);
         EOS_ASSERT(n <= 0x0Full, name_type_exception, "invalid 13th character: (${c})", ("c", std::string(1, str[12])));
      }
      // Encode full-range characters.
      while (--i >= 0) {
         n |= char_to_symbol(str[i]) << (64 - 5 * (i + 1));
      }
      return n;
   }

   /// Immutable except for fc::from_variant.
   struct name {
   private:
      uint64_t value = 0;

      friend struct fc::reflector<name>;
      friend void fc::from_variant(const fc::variant& v, eosio::chain::name& check);

      void set( std::string_view str );

   public:
      constexpr bool empty()const { return 0 == value; }
      constexpr bool good()const  { return !empty();   }

      explicit name( std::string_view str ) { set( str ); }
      constexpr explicit name( uint64_t v ) : value(v) {}
      constexpr name() = default;

      std::string to_string()const;
      constexpr uint64_t to_uint64_t()const { return value; }

      friend std::ostream& operator << ( std::ostream& out, const name& n ) {
         return out << n.to_string();
      }

      friend constexpr bool operator < ( const name& a, const name& b ) { return a.value < b.value; }
      friend constexpr bool operator > ( const name& a, const name& b ) { return a.value > b.value; }
      friend constexpr bool operator <= ( const name& a, const name& b ) { return a.value <= b.value; }
      friend constexpr bool operator >= ( const name& a, const name& b ) { return a.value >= b.value; }
      friend constexpr bool operator == ( const name& a, const name& b ) { return a.value == b.value; }
      friend constexpr bool operator != ( const name& a, const name& b ) { return a.value != b.value; }

      friend constexpr bool operator == ( const name& a, uint64_t b ) { return a.value == b; }
      friend constexpr bool operator != ( const name& a, uint64_t b ) { return a.value != b; }

      constexpr explicit operator bool()const { return value != 0; }
   };

   // Each char of the string is encoded into 5-bit chunk and left-shifted
   // to its 5-bit slot starting with the highest slot for the first char.
   // The 13th char, if str is long enough, is encoded into 4-bit chunk
   // and placed in the lowest 4 bits. 64 = 12 * 5 + 4
   constexpr name string_to_name( std::string_view str )
   {
      return name( string_to_uint64_t( str ) );
   }

   inline namespace literals {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
      template <typename T, T... Str>
      inline constexpr name operator""_n() {
         constexpr const char buf[] = {Str...};
         return name{std::integral_constant<uint64_t, string_to_uint64_t(std::string_view{buf, sizeof(buf)})>::value};
      }
#pragma clang diagnostic pop
   } // namespace literals

} // eosio::chain

namespace std {
   template<> struct hash<eosio::chain::name> : private hash<uint64_t> {
      typedef eosio::chain::name argument_type;
      size_t operator()(const argument_type& name) const noexcept
      {
         return hash<uint64_t>::operator()(name.to_uint64_t());
      }
   };
};

FC_REFLECT( eosio::chain::name, (value) )
