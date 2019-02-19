#pragma once
#include <string>
#include <fc/reflect/reflect.hpp>
#include <iosfwd>

namespace eosio { namespace chain {
   using std::string;

   static constexpr uint64_t char_to_symbol( char c ) {
      if( c >= 'a' && c <= 'z' )
         return (c - 'a') + 6;
      if( c >= '1' && c <= '5' )
         return (c - '1') + 1;
      return 0;
   }

   struct name {
      uint64_t value = 0;
      bool empty()const { return 0 == value; }
      bool good()const  { return !empty();   }

      name( const char* str )   { set(str);           } 
      name( const string& str ) { set( str.c_str() ); }
      constexpr explicit name( uint64_t v ) : value(v) {};

      void set( const char* str );

      template<typename T>
      explicit name( T v ):value(v){}
      name() = default;

      uint64_t to_uint64_t()const { return value; }
      string to_string()const;

      name& operator=( uint64_t v ) {
         value = v;
         return *this;
      }

      name& operator=( const string& n ) {
         value = name(n).value;
         return *this;
      }
      name& operator=( const char* n ) {
         value = name(n).value;
         return *this;
      }

      friend std::ostream& operator << ( std::ostream& out, const name& n ) {
         return out << n.to_string();
      }

      friend bool operator < ( const name& a, const name& b ) { return a.value < b.value; }
      friend bool operator <= ( const name& a, const name& b ) { return a.value <= b.value; }
      friend bool operator > ( const name& a, const name& b ) { return a.value > b.value; }
      friend bool operator >=( const name& a, const name& b ) { return a.value >= b.value; }
      friend bool operator == ( const name& a, const name& b ) { return a.value == b.value; }

      friend bool operator == ( const name& a, uint64_t b ) { return a.value == b; }
      friend bool operator != ( const name& a, uint64_t b ) { return a.value != b; }

      friend bool operator != ( const name& a, const name& b ) { return a.value != b.value; }

      explicit operator bool()const            { return value; }
   };

   // Each char of the string is encoded into 5-bit chunk and left-shifted
   // to its 5-bit slot starting with the highest slot for the first char.
   // The 13th char, if str is long enough, is encoded into 4-bit chunk
   // and placed in the lowest 4 bits. 64 = 12 * 5 + 4
   static constexpr name string_to_name( const char* str )
   {
      uint64_t n = 0;
      int i = 0;
      for( ; str[i] && i < 12; ++i ) {
         // NOTE: char_to_symbol() returns char type, and without this explicit
         // expansion to uint64 type, the compilation fails at the point of usage
         // of string_to_name(), where the usage requires constant (compile time) expression.
         n |= (char_to_symbol( str[i] ) & 0x1f) << (64 - 5 * (i + 1));
      }

      // The for-loop encoded up to 60 high bits into uint64 'name' variable,
      // if (strlen(str) > 12) then encode str[12] into the low (remaining)
      // 4 bits of 'name'
      if( i == 12 )
         n |= char_to_symbol( str[12] ) & 0x0F;
      return name(n);
   }

#define N(X) eosio::chain::string_to_name(#X)

} } // eosio::chain

namespace std {
   template<> struct hash<eosio::chain::name> : private hash<uint64_t> {
      typedef eosio::chain::name argument_type;
      typedef typename hash<uint64_t>::result_type result_type;
      result_type operator()(const argument_type& name) const noexcept
      {
         return hash<uint64_t>::operator()(name.value);
      }
   };
};

namespace fc {
  class variant;
  void to_variant(const eosio::chain::name& c, fc::variant& v);
  void from_variant(const fc::variant& v, eosio::chain::name& check);
} // fc

FC_REFLECT( eosio::chain::name, (value) )

#include <fmt/format.h>

namespace fmt {
template<>
struct formatter<eosio::chain::name> {
   template<typename ParseContext>
   constexpr auto parse( ParseContext& ctx ) { return ctx.begin(); }

   template<typename FormatContext>
   auto format( const eosio::chain::name& v, FormatContext& ctx ) {
      return format_to( ctx.out(), "{}", v.to_string() );
   }
};
}