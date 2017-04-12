#pragma once
#include <eos/types/ints.hpp>
#include <eos/types/Asset.hpp>

#include <vector>
#include <array>
#include <string>
#include <functional>


#include <fc/variant.hpp>
#include <fc/crypto/base64.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/io/datastream.hpp>

namespace EOS {
   using std::vector;
   using std::array;
   using std::string;


   template<size_t Size=16>
   struct FixedString {
      array<uint64_t,Size/8> value;
      static_assert( sizeof(value) == Size, "unexpected padding" );

      FixedString(){ memset( &value, 0, sizeof(value) ); }
      FixedString( const std::string& str ) {
         memset( &value, 0, sizeof(value) );
         if( str.size() <=  Size )
            memcpy( (char*)&value, str.c_str(), str.size() );
         else {
            memcpy( (char*)&value, str.c_str(), Size );
         }
      }
      operator std::string()const {
         const char* self = (const char*)&value;
         return std::string( self, self + size() );
      }

      uint32_t size()const {
         if( *(((const char*)&value)+ - 1) )
            return sizeof(value);
         return strnlen( (const char*)&value, Size );
      }

      friend bool operator == ( const FixedString& a, const FixedString& b ) {
         return a.value == b.value;
      }
      friend bool operator < ( const FixedString& a, const FixedString& b ) {
         return a.value < b.value;
      }
      friend bool operator == ( const FixedString& a, const char* b ) {
         const char* self = (const char*)&a.value;
         return 0 == strncmp( self, b, Size );
      }
   };
   typedef FixedString<16> FixedString16;
   typedef FixedString<32> FixedString32;

   typedef string TypeName;
   typedef string FieldName;

   struct Field {
      FieldName name;
      TypeName  type;
   };

   typedef fc::time_point_sec         Time;
   typedef fc::ecc::compact_signature Signature;
   typedef string                     String;
   typedef fc::sha256                 Checksum;
   typedef fc::ripemd160              BlockID;
   typedef fc::ripemd160              TransactionID;

   struct Struct {
      TypeName        name;
      TypeName        base;
      vector<Field>   fields;
   };

   struct Bytes {
      uint32_t      size()const             { return value.size(); }
      const char*   data()const             { return value.data(); }
      char*         data()                  { return value.data(); }
      void          resize( uint32_t size ) { value.resize(size);  }

      template<typename T>
      T as()const; 

      vector<char> value;
   };

   struct PublicKey
   {
       struct BinaryKey
       {
          BinaryKey() {}
          uint32_t                 check = 0;
          fc::ecc::public_key_data data;
       };
       fc::ecc::public_key_data key_data;


       PublicKey();
       PublicKey( const fc::ecc::public_key_data& data );
       PublicKey( const fc::ecc::public_key& pubkey );
       explicit PublicKey( const std::string& base58str );
       operator fc::ecc::public_key_data() const;
       operator fc::ecc::public_key() const;
       explicit operator std::string() const;
       friend bool operator == ( const PublicKey& p1, const fc::ecc::public_key& p2);
       friend bool operator == ( const PublicKey& p1, const PublicKey& p2);
       friend bool operator < ( const PublicKey& p1, const PublicKey& p2) { return p1.key_data < p2.key_data; }
       friend bool operator != ( const PublicKey& p1, const PublicKey& p2);
   };

   template<typename Stream>
   inline void toBinary( Stream& stream, const FixedString32& v ) {
      stream.write( (const char*)&v.value, sizeof(v.value) );
   }
   template<typename Stream>
   inline void fromBinary( Stream& stream, FixedString32& v ) {
      stream.read( (char*)&v.value, sizeof(v.value) );
   }
   template<typename Stream>
   inline void fromBinary( Stream& stream, FixedString16& v ) {
      stream.read( (char*)&v.value, sizeof(v.value) );
   }
   template<typename Stream>
   inline void toBinary( Stream& stream, const FixedString16& v ) {
      stream.write( (const char*)&v.value, sizeof(v.value) );
   }

   template<typename Stream>
   inline void toBinary( Stream& stream, const Signature& v ) {
      stream.write( (const char*)&v, sizeof(v) );
   }
   template<typename Stream>
   inline void fromBinary( Stream& stream, Signature& v ) {
      stream.read( (char*)&v, sizeof(v) );
   }


   inline fc::variant toVariant( const Int8& v )          { return static_cast<int8_t>(v);   }
   inline fc::variant toVariant( const Int16& v )         { return static_cast<int16_t>(v);  }
   inline fc::variant toVariant( const Int32& v )         { return static_cast<int32_t>(v);  }
   inline fc::variant toVariant( const Int64& v )         { return static_cast<int64_t>(v);  }
   inline fc::variant toVariant( const UInt8& v )         { return static_cast<uint8_t>(v);  }
   inline fc::variant toVariant( const UInt16& v )        { return static_cast<uint16_t>(v); }
   inline fc::variant toVariant( const UInt32& v )        { return static_cast<uint32_t>(v); }
   inline fc::variant toVariant( const UInt64& v )        { return static_cast<uint64_t>(v); }
   inline fc::variant toVariant( const FixedString16& v ) { return std::string(v); }
   inline fc::variant toVariant( const FixedString32& v ) { return std::string(v); }
   inline fc::variant toVariant( const PublicKey& v )     { return std::string(v); }
   inline fc::variant toVariant( const String& v )        { return v; }
   inline fc::variant toVariant( const Time& v )          { return fc::variant(v); }
   inline fc::variant toVariant( const Checksum& v )      { return fc::to_hex( (const char*)&v, sizeof(v) ); }
   inline fc::variant toVariant( const Signature& v )     { return fc::to_hex( (const char*)&v, sizeof(v) ); }

   inline void fromVariant( Int8& v, const fc::variant& var )          { v = static_cast<int8_t>(var.as_int64());    }
   inline void fromVariant( Int16& v, const fc::variant& var )         { v = static_cast<int16_t>(var.as_int64());   }
   inline void fromVariant( Int32& v, const fc::variant& var )         { v = static_cast<int32_t>(var.as_int64());   }
   inline void fromVariant( Int64& v, const fc::variant& var )         { v = static_cast<int64_t>(var.as_int64());   }
   inline void fromVariant( UInt8& v, const fc::variant& var )         { v = static_cast<uint8_t>(var.as_uint64());  }
   inline void fromVariant( UInt16& v, const fc::variant& var )        { v = static_cast<uint16_t>(var.as_uint64()); }
   inline void fromVariant( UInt32& v, const fc::variant& var )        { v = static_cast<uint32_t>(var.as_uint64()); }
   inline void fromVariant( UInt64& v, const fc::variant& var )        { v = static_cast<uint64_t>(var.as_uint64()); }
   inline void fromVariant( FixedString16& v, const fc::variant& var ) { v = FixedString16( var.as_string() );  }
   inline void fromVariant( FixedString32& v, const fc::variant& var ) { v = FixedString32( var.as_string() );  }
   inline void fromVariant( String& v, const fc::variant& var )        { v = var.as_string();  }
   inline void fromVariant( Time& v, const fc::variant& var )          { v = var.as<Time>();   }
   inline void fromVariant( PublicKey& v, const fc::variant& var )     { v = PublicKey( var.as_string() );      }
   inline void fromVariant( Checksum& v, const fc::variant& var )      { fc::from_hex( var.as_string(), (char*)&v, sizeof(v)); }
   inline void fromVariant( Signature& v, const fc::variant& var )     { fc::from_hex( var.as_string(), (char*)&v, sizeof(v)); }

   template<typename T> fc::variant toVariant( const vector<T>& a );
   template<typename T> void fromVariant( vector<T>& data, const fc::variant& v );

   template<typename Stream, typename T> void toBinary( Stream& s, const vector<T>& a );
   template<typename Stream, typename T> void fromBinary( Stream& s, vector<T>& a );

   template<typename Stream> 
   void toBinary( Stream& s, const Bytes& data ) {
      uint32_t len = data.size();
      s.write( (const char*)&len, sizeof(len) );
      if( len ) s.write( data.data(), data.size() );
   }

   template<typename Stream> 
   void fromBinary( Stream& s, Bytes& data ) {
      uint32_t len = 0;
      s.read( (char*)&len, sizeof(len) );
      data.resize(len);
      if( len ) {
         s.read( data.data(), data.size() );
      }
   }

   template<typename Stream> 
   void toBinary( Stream& s, const String& data ) {
      uint32_t len = data.size();
      s.write( (const char*)&len, sizeof(len) );
      if( len ) s.write( data.data(), data.size() );
   }

   template<typename Stream> 
   void fromBinary( Stream& s, String& data ) {
      uint32_t len = 0;
      s.read( (char*)&len, sizeof(len) );
      data.resize(len);
      if( len ) {
         s.read( const_cast<char*>(data.data()), data.size() );
      }
   }


   inline fc::variant toVariant( const Bytes& data ) {
      if( !data.size() ) return "";
      return fc::base64_encode( data.data(), data.size() );
   }

   inline void fromVariant( Bytes& data, const fc::variant& v ) {
      auto decoded = fc::base64_decode( v.get_string() );
      data.value.resize( decoded.size() );
      memcpy( data.value.data(), decoded.c_str(), decoded.size() );
   }

    inline fc::variant toVariant( const Field& t ) { 
        fc::mutable_variant_object mvo; 
        mvo.reserve( 2 ); 
        mvo["name"] = EOS::toVariant( t.name );
        mvo["type"] = EOS::toVariant( t.type );
       return fc::variant( std::move(mvo) );  
    }

    template<typename Stream>
    void toBinary( Stream& stream, const Field& t ) { 
        EOS::toBinary( stream, t.name );
        EOS::toBinary( stream, t.type );
    }

    template<typename Stream>
    void fromBinary( Stream& stream, Field& t ) { 
        EOS::fromBinary( stream, t.name );
        EOS::fromBinary( stream, t.type );
    }

    template<typename T>
    T as( const vector<char>& data ) {
       fc::datastream<const char*> ds(data.data(), data.size() );
       T result;
       EOS::fromBinary( ds, result );
       return result;
    }
   
} // namespace EOS

FC_REFLECT( EOS::PublicKey::BinaryKey, (check)(data) )
FC_REFLECT( EOS::PublicKey, (key_data) )

namespace fc {
  void to_variant( const std::vector<EOS::Field>& c, fc::variant& v );
  void from_variant( const fc::variant& v, std::vector<EOS::Field>& check );
  void to_variant( const std::map<std::string,EOS::Struct>& c, fc::variant& v );
  void from_variant( const fc::variant& v, std::map<std::string,EOS::Struct>& check );
}
