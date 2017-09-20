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

#define N(X) eos::types::string_to_name(#X)

namespace eos { namespace types {
   using namespace boost::multiprecision;

   template<typename... T>
   using Vector       = std::vector<T...>;

   template<typename... T>
   using Array        = std::array<T...>;

   using String        = std::string;
   using Time          = fc::time_point_sec;
   using Signature     = fc::ecc::compact_signature;
   using Checksum      = fc::sha256;
   using FieldName     = fc::fixed_string<>;
   using FixedString32 = fc::fixed_string<fc::array<uint64_t,4>>;// std::tuple<uint64_t,uint64_t,uint64_t,uint64_t>>; 
   using FixedString16 = fc::fixed_string<>; 
   using TypeName      = FixedString32;;
   using Bytes         = Vector<char>;

   template<size_t Size>
   using UInt = number<cpp_int_backend<Size, Size, unsigned_magnitude, unchecked, void> >;
   template<size_t Size>
   using Int = number<cpp_int_backend<Size, Size, signed_magnitude, unchecked, void> >;

   using UInt8     = UInt<8>; 
   using UInt16    = UInt<16>; 
   using UInt32    = UInt<32>;
   using UInt64    = UInt<64>;
   using UInt128   = boost::multiprecision::uint128_t;
   using UInt256   = boost::multiprecision::uint256_t;
   using Int8      = int8_t;//Int<8>;  these types are different sizes than native...
   using Int16     = int16_t; //Int<16>;
   using Int32     = int32_t; //Int<32>;
   using Int64     = int64_t; //Int<64>; 
   using Int128    = boost::multiprecision::int128_t;
   using Int256    = boost::multiprecision::int256_t;
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
   
   struct Name {
      uint64_t value = 0;
      bool valid()const { return true; }
      bool empty()const { return 0 == value; }
      bool good()const  { return !empty() && valid();  }

      Name( const char* str )   { set(str);           } 
      Name( const String& str ) { set( str.c_str() ); }

      void set( const char* str ) {
      try {
         const auto len = strnlen(str,14);
         FC_ASSERT( len <= 13 );
         value = string_to_name(str);
         FC_ASSERT( toString() == String(str), "name not properly normalized", ("name",String(str))("normalized",toString())  );
      }FC_CAPTURE_AND_RETHROW( (str) ) }

      Name( uint64_t v = 0 ):value(v){}

      explicit operator String()const {
        static const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";

         String str(13,'.');

         uint64_t tmp = value;
         for( uint32_t i = 0; i <= 12; ++i ) {
            char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
            str[12-i] = c;
            tmp >>= (i == 0 ? 4 : 5);
         }

         boost::algorithm::trim_right_if( str, []( char c ){ return c == '.'; } );
         return str;
      }
      String toString() const { return String(*this); }

      Name& operator=( uint64_t v ) {
         value = v;
         return *this;
      }

      Name& operator=( const String& n ) {
         value = Name(n).value;
         return *this;
      }
      Name& operator=( const char* n ) {
         value = Name(n).value;
         return *this;
      }

      template<typename Stream>
      friend Stream& operator << ( Stream& out, const Name& n ) {
         return out << String(n);
      }

      friend bool operator < ( const Name& a, const Name& b ) { return a.value < b.value; }
      friend bool operator <= ( const Name& a, const Name& b ) { return a.value <= b.value; }
      friend bool operator > ( const Name& a, const Name& b ) { return a.value > b.value; }
      friend bool operator >=( const Name& a, const Name& b ) { return a.value >= b.value; }
      friend bool operator == ( const Name& a, const Name& b ) { return a.value == b.value; }
      friend bool operator != ( const Name& a, const Name& b ) { return a.value != b.value; }

      operator bool()const            { return value; }
      operator uint64_t()const        { return value; }
   };


   struct Field {
      FieldName name;
      TypeName  type;

      bool operator==(const Field& other) const;
   };

   struct Struct {
      TypeName        name;
      TypeName        base;
      Vector<Field>   fields;

      bool operator==(const Struct& other) const;
   };

   using Fields = Vector<Field>;

   template<typename T>
   struct GetStruct{};

    template<> struct GetStruct<Field> { 
        static const Struct& type() { 
           static Struct result = { "Field ", "", {
                {"name", "FieldName"},
                {"type", "TypeName"}
              }
           };
           return result;
         }
    };
    template<> struct GetStruct<Struct> { 
        static const Struct& type() { 
           static Struct result = { "Struct ", "", {
                {"name", "TypeName"},
                {"base", "TypeName"},
                {"fields", "Field[]"}
              }
           };
           return result;
         }
    };


   /// TODO: make sure this works with FC raw
   template<typename Stream, typename Number>
   void fromBinary(Stream& st, boost::multiprecision::number<Number>& value) {
      unsigned char data[(std::numeric_limits<decltype(value)>::digits+1)/8];
      st.read((char*)data, sizeof(data));
      boost::multiprecision::import_bits(value, data, data + sizeof(data), 1);
   }
   template<typename Stream, typename Number>
   void toBinary(Stream& st, const boost::multiprecision::number<Number>& value) {
      unsigned char data[(std::numeric_limits<decltype(value)>::digits+1)/8];
      boost::multiprecision::export_bits(value, data, 1);
      st.write((const char*)data, sizeof(data));
   }
   
}} // namespace eos::types

namespace fc {
  void to_variant(const eos::types::Name& c, fc::variant& v);
  void from_variant(const fc::variant& v, eos::types::Name& check);
  void to_variant(const std::vector<eos::types::Field>& c, fc::variant& v);
  void from_variant(const fc::variant& v, std::vector<eos::types::Field>& check);
  void to_variant(const std::map<std::string,eos::types::Struct>& c, fc::variant& v);
  void from_variant(const fc::variant& v, std::map<std::string,eos::types::Struct>& check);
}

FC_REFLECT(eos::types::Name, (value))
FC_REFLECT(eos::types::Field, (name)(type))
FC_REFLECT(eos::types::Struct, (name)(base)(fields))
