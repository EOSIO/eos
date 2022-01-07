#pragma once
#include <string>
#include <fc/reflect/reflect.hpp>
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
   inline constexpr uint64_t char_to_symbol( char c ) {
      if( c >= 'a' && c <= 'z' )
         return (c - 'a') + 6;
      if( c >= '1' && c <= '5' )
         return (c - '1') + 1;
      return 0;
   }

   inline constexpr uint64_t string_to_uint64_t( std::string_view str ) {
      uint64_t n = 0;
      int i = 0;
      for ( ; i < str.size() && i < 12; ++i) {
         // NOTE: char_to_symbol() returns char type, and without this explicit
         // expansion to uint64 type, the compilation fails at the point of usage
         // of string_to_name(), where the usage requires constant (compile time) expression.
         n |= (char_to_symbol(str[i]) & 0x1f) << (64 - 5 * (i + 1));
      }

      // The for-loop encoded up to 60 high bits into uint64 'name' variable,
      // if (strlen(str) > 12) then encode str[12] into the low (remaining)
      // 4 bits of 'name'
      if (i < str.size() && i == 12)
         n |= char_to_symbol(str[12]) & 0x0F;
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
   inline constexpr name string_to_name( std::string_view str )
   {
      return name( string_to_uint64_t( str ) );
   }

   inline namespace literals {
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
#endif
      template <typename T, T... Str>
      inline constexpr name operator""_n() {
         constexpr const char buf[] = {Str...};
         return name{std::integral_constant<uint64_t, string_to_uint64_t(std::string_view{buf, sizeof(buf)})>::value};
      }
#if defined(__clang__)
# pragma clang diagnostic pop
#endif
   } // namespace literals

#define N(X) eosio::chain::string_to_name(#X)

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
