/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <vector>
#include <array>
#include <string>
#include <functional>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <fc/variant.hpp>
#include <fc/crypto/base64.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/io/datastream.hpp>
#include <fc/time.hpp>
#include <fc/fixed_string.hpp>
#include <fc/string.hpp>

#include <fc/reflect/reflect.hpp>

#define N(X) eosio::types::string_to_name(#X)

namespace eosio { namespace types {
   using namespace boost::multiprecision;

   template<typename... T>
   using vector         = std::vector<T...>;

   template<typename... T>
   using array          = std::array<T...>;

   using string         = std::string;
   using time           = fc::time_point_sec;
   using signature      = fc::ecc::compact_signature;
   using checksum       = fc::sha256;
   using field_name     = fc::fixed_string<>;
   using fixed_string32 = fc::fixed_string<fc::array<uint64_t,4>>;// std::tuple<uint64_t,uint64_t,uint64_t,uint64_t>>;
   using fixed_string16 = fc::fixed_string<>;
   using type_name      = fixed_string32;;
   using bytes          = vector<char>;

   template<size_t Size>
   using uint_t = number<cpp_int_backend<Size, Size, unsigned_magnitude, unchecked, void> >;
   template<size_t Size>
   using int_t = number<cpp_int_backend<Size, Size, signed_magnitude, unchecked, void> >;

   using uint8     = uint_t<8>;
   using uint16    = uint_t<16>;
   using uint32    = uint_t<32>;
   using uint64    = uint_t<64>;
   using uint128   = boost::multiprecision::uint128_t;
   using uint256   = boost::multiprecision::uint256_t;
   using int8      = int8_t;//int_t<8>;  these types are different sizes than native...
   using int16     = int16_t; //int_t<16>;
   using int32     = int32_t; //int_t<32>;
   using int64     = int64_t; //int_t<64>;
   using int128    = boost::multiprecision::int128_t;
   using int256    = boost::multiprecision::int256_t;
   using uint128_t = unsigned __int128; /// native clang/gcc 128 intrinisic
   
   static constexpr char char_to_symbol( char c ) {
      if( c >= 'a' && c <= 'z' )
         return (c - 'a') + 6;
      if( c >= '1' && c <= '5' )
         return (c - '1') + 1;
      return 0;
   }

   static constexpr uint64_t string_to_name( const char* str ) {

      uint32_t len = 0;
      while( str[len] ) ++len;

      uint64_t value = 0;

      for( uint32_t i = 0; i <= 12; ++i ) {
         uint64_t c = 0;
         if( i < len && i <= 12 ) c = char_to_symbol( str[i] );

         if( i < 12 ) {
            c &= 0x1f;
            c <<= 64-5*(i+1);
         }
         else {
            c &= 0x0f;
         }

         value |= c;
      }

      return value;
   }
   
   struct name {
      uint64_t value = 0;
      bool valid()const { return true; }
      bool empty()const { return 0 == value; }
      bool good()const  { return !empty() && valid();  }

      name( const char* str )   { set(str);           }
      name( const string& str ) { set( str.c_str() ); }

      void set( const char* str ) {
      try {
         const auto len = strnlen(str,14);
         FC_ASSERT( len <= 13 );
         value = string_to_name(str);
         FC_ASSERT(to_string() == string(str), "name not properly normalized", ("name",string(str))("normalized",
                                                                                                    to_string())  );
      }FC_CAPTURE_AND_RETHROW( (str) ) }

      name( uint64_t v = 0 ):value(v){}

      explicit operator string()const {
        static const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";

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
      string to_string() const { return string(*this); }

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

      template<typename Stream>
      friend Stream& operator << ( Stream& out, const name& n ) {
         return out << string(n);
      }

      friend bool operator < ( const name& a, const name& b ) { return a.value < b.value; }
      friend bool operator <= ( const name& a, const name& b ) { return a.value <= b.value; }
      friend bool operator > ( const name& a, const name& b ) { return a.value > b.value; }
      friend bool operator >=( const name& a, const name& b ) { return a.value >= b.value; }
      friend bool operator == ( const name& a, const name& b ) { return a.value == b.value; }
      friend bool operator != ( const name& a, const name& b ) { return a.value != b.value; }

      operator bool()const            { return value; }
      operator uint64_t()const        { return value; }
   };


   struct field {
      field_name name;
      type_name  type;

      bool operator==(const field& other) const;
   };

   struct struct_t {
      type_name        name;
      type_name        base;
      vector<field>    fields;

      bool operator==(const struct_t& other) const;
   };

   using fields = vector<field>;

   template<typename T>
   struct get_struct{};

    template<> struct get_struct<field> {
        static const struct_t& type() {
           static struct_t result = { "field ", "", {
                {"name", "field_name"},
                {"type", "type_name"}
              }
           };
           return result;
         }
    };
    template<> struct get_struct<struct_t> {
        static const struct_t& type() {
           static struct_t result = { "struct_t ", "", {
                {"name", "type_name"},
                {"base", "type_name"},
                {"fields", "field[]"}
              }
           };
           return result;
         }
    };


   /// TODO: make sure this works with FC raw
   template<typename Stream, typename Number>
   void from_binary(Stream& st, boost::multiprecision::number<Number>& value) {
      unsigned char data[(std::numeric_limits<decltype(value)>::digits+1)/8];
      st.read((char*)data, sizeof(data));
      boost::multiprecision::import_bits(value, data, data + sizeof(data), 1);
   }
   template<typename Stream, typename Number>
   void to_binary(Stream& st, const boost::multiprecision::number<Number>& value) {
      unsigned char data[(std::numeric_limits<decltype(value)>::digits+1)/8];
      boost::multiprecision::export_bits(value, data, 1);
      st.write((const char*)data, sizeof(data));
   }
   
}} // namespace eosio::types

namespace fc {
  void to_variant(const eosio::types::name& c, fc::variant& v);
  void from_variant(const fc::variant& v, eosio::types::name& check);
  void to_variant(const std::vector<eosio::types::field>& c, fc::variant& v);
  void from_variant(const fc::variant& v, std::vector<eosio::types::field>& check);
  void to_variant(const std::map<std::string,eosio::types::struct_t>& c, fc::variant& v);
  void from_variant(const fc::variant& v, std::map<std::string,eosio::types::struct_t>& check);
}

FC_REFLECT(eosio::types::name, (value))
FC_REFLECT(eosio::types::field, (name)(type))
FC_REFLECT(eosio::types::struct_t, (name)(base)(fields))
