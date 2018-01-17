/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/types.hpp>
#include <eoslib/varint.hpp>
#include <eoslib/datastream.hpp>
#include <eoslib/memory.hpp>
#include <eoslib/vector.hpp>
#include <eoslib/string.hpp>
#include <eoslib/reflect.hpp>

namespace eosio {
   using bytes = vector<char>;

   namespace raw {

   template<typename Stream, typename T, typename reflector<T>::is_reflected::value = 1>
   void pack( Stream& ds, const T& t ) {
      reflector<T>::visit( t, [&]( const auto& field ) {
         raw::pack( ds, field );
      });
   }

   template<typename Stream, typename T, typename reflector<T>::is_reflected::value = 1>
   void unpack( Stream& ds, T& t ) {
      reflector<T>::visit( t, [&]( auto& field ) {
         raw::unpack( ds, field );
      });
   }

  /**
   *  Serialize a list of values into a stream
   *  @param s    stream to write
   *  @param a0   value to be serialized
   *  @param args other values to be serialized
   template<typename Stream, typename Arg0, typename... Args>
   void pack( Stream& s, const Arg0& a0, Args... args ) {
      pack( s, a0 );
      pack( s, args... );
   }
   */

  /**
   *  Serialize a value into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream, typename T>
   void pack( Stream& s, const T& v ) {
      s << v;
   }
   
  /**
   *  Deserialize a value from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream, typename T> //, typename reflector<T>::is_reflected::value = 0>
   void unpack( Stream& s, T& v )
   {
      s >> v;
   }
   
  /**
   *  Serialize a signed_int into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> 
   void pack( Stream& s, const signed_int& v ) {
      uint32_t val = (v.value<<1) ^ (v.value>>31);
      do {
         uint8_t b = uint8_t(val) & 0x7f;
         val >>= 7;
         b |= ((val > 0) << 7);
         s.write((char*)&b,1);//.put(b);
      } while( val );
   }
  
  /**
   *  Serialize an unsigned_int into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> 
   void pack( Stream& s, const unsigned_int& v ) {
      uint64_t val = v.value;
      do {
         uint8_t b = uint8_t(val) & 0x7f;
         val >>= 7;
         b |= ((val > 0) << 7);
         s.write((char*)&b,1);//.put(b);
      }while( val );
   }

  /**
   *  Deserialize a signed_int from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> 
   void unpack( Stream& s, signed_int& vi ) {
      uint32_t v = 0; char b = 0; int by = 0;
      do {
         s.get(b);
         v |= uint32_t(uint8_t(b) & 0x7f) << by;
         by += 7;
      } while( uint8_t(b) & 0x80 );
      vi.value = ((v>>1) ^ (v>>31)) + (v&0x01);
      vi.value = v&0x01 ? vi.value : -vi.value;
      vi.value = -vi.value;
   }

  /**
   *  Deserialize an unsigned_int from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> 
   void unpack( Stream& s, unsigned_int& vi ) {
      uint64_t v = 0; char b = 0; uint8_t by = 0;
      do {
         s.get(b);
         v |= uint32_t(uint8_t(b) & 0x7f) << by;
         by += 7;
      } while( uint8_t(b) & 0x80 );
      vi.value = static_cast<uint32_t>(v);
   }

  /**
   *  Serialize a public_key into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> 
   void pack( Stream& s, const public_key& value ) {
      s.write( (char *)value.data, sizeof(public_key) );
   }
   
  /**
   *  Deserialize a public_key from a stream
   *  @param s stream to read
   *  @param v destination of deserialized public_key
   */
   template<typename Stream> 
   void unpack( Stream& s, public_key& value ) {
      s.read( (char *)value.data, sizeof(public_key) );
   }

  /**
   *  Serialize a string into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> 
   void pack( Stream& s, const string& v )  {
      auto size = v.get_size();
      eosio::raw::pack( s, unsigned_int(size));
      if( size ) s.write( v.get_data(), size );
   }

  /**
   *  Serialize a string into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream, typename T> 
   void pack( Stream& s, const vector<T>& v )  {
      auto size = v.size();
      eosio::raw::pack( s, unsigned_int(size));
      for( const auto& value : v )
         eosio::raw::pack( s, value );
   }

   template<typename Stream, typename T> 
   void unpack( Stream& ds, vector<T>& v )  {
      unsigned_int size; 
      eosio::raw::unpack( ds, size );
      v.resize(size);
      for( auto& value : v )
         eosio::raw::unpack( ds, value );
   }

  /**
   *  Deserialize a string from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> 
   void unpack( Stream& s, string& v)  {
      unsigned_int size; eosio::raw::unpack( s, size );
      v.assign((char*)s.pos(), size.value, true);
      s.skip(size.value);
   }

  /**
   *  Serialize a fixed_string32 into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> 
   void pack( Stream& s, const fixed_string32& v )  {
      auto size = v.len;
      eosio::raw::pack( s, unsigned_int(size));
      if( size ) s.write( v.str, size );
   }

  /**
   *  Deserialize a fixed_string32 from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> 
   void unpack( Stream& s, fixed_string32& v)  {
      unsigned_int size; eosio::raw::unpack( s, size );
      assert(size.value <= 32, "unpack fixed_string32");
      s.read( (char *)v.str, size );
      v.len = size;
   }

  /**
   *  Serialize a fixed_string16 into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> 
   void pack( Stream& s, const fixed_string16& v )  {
      auto size = v.len;
      eosio::raw::pack( s, unsigned_int(size));
      if( size ) s.write( v.str, size );
   }


  /**
   *  Deserialize a fixed_string16 from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> 
   void unpack( Stream& s, fixed_string16& v)  {
      unsigned_int size; eosio::raw::unpack( s, size );
      assert(size.value <= 16, "unpack fixed_string16");
      s.read( (char *)v.str, size );
      v.len = size;
   }
  
   template<typename Stream>
   void pack(Stream &s, const ::account_permission &pe) {
      eosio::raw::pack(s, pe.account);
      eosio::raw::pack(s, pe.permission);
   }

   template<typename Stream> 
   void unpack(Stream &s, ::account_permission &pe) {
      eosio::raw::unpack(s, pe.account);
      eosio::raw::unpack(s, pe.permission);
   }
  /**
   *  Serialize a bool into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> 
   void pack( Stream& s, const bool& v ) { 
       eosio::raw::pack( s, uint8_t(v) );
   }

  /**
   *  Serialize an array of types into a stream
   *  @param s stream to write
   *  @param varr array of values to be serialized
   *  @param count the number of values to be serialized
   */
   template<typename Stream, typename T> 
   void pack( Stream& s, const T* varr, size_t count ) {
      eosio::raw::pack( s, unsigned_int(count) );
      for (size_t idx = 0; idx < count; idx++) {
         eosio::raw::pack( s, varr[idx] );
      }
   }

   template<typename Stream, typename T> 
   void unpack( Stream& s, T* varr, size_t &count, size_t max_count ) {
      unsigned_int packed_count;
      eosio::raw::unpack( s, packed_count );
      count = packed_count.value;
      assert(count <= max_count, "array too large to read");

      for (size_t idx = 0; idx < count; idx++) {
         eosio::raw::unpack( s, varr[idx] );
      }
   }


   template<typename Stream> 
   void pack( Stream& s, const char *str, size_t len ) {
     eosio::raw::pack( s, unsigned_int(len) );
     if( len )
        s.write( str, len );
   }

   template<typename Stream> 
   void unpack( Stream& s, char *str, size_t &len, size_t max_len ) {
      unsigned_int packed_len;
      eosio::raw::unpack( s, packed_len );
      len = packed_len.value;
      assert(len <= max_len, "raw string too large to read");
      if( len )
         s.read( str, len );
   }

   /**
   *  Deserialize a bool from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> 
   void unpack( Stream& s, bool& v )
   {
      uint8_t b;
      eosio::raw::unpack( s, b );
      assert( (b & ~1) == 0, "unpack bool" );
      v=(b!=0);
   }
  
  /** Calculates the serialized size of a value
   *  @return serialized size of the value
   *  @param v value to calculate its serialized size
   */
   template<typename T>
   size_t pack_size( const T& v )
   {
      datastream<size_t> ps;
      eosio::raw::pack(ps,v );
      return ps.tellp();
   }
  
  /** Serialize a value into a bytes struct
   *  @return bytes struct with the serialized value 
   *  @param v value to be serialized
   */
   template<typename T>
   bytes pack(  const T& v ) {
      datastream<size_t> ps;
      eosio::raw::pack(ps, v);
      const auto size = ps.tellp();
      bytes b;

      if( size ) {
         b.resize(ps.tellp());
         datastream<char*>  ds( b.data(), b.size() );
         eosio::raw::pack(ds,v);
      }
      return b;
   }

  /** Serialize a value into a buffer
   *  @param d pointer to the buffer
   *  @param s size of the buffer
   *  @param v value to be serialized
   */
   template<typename T>
   void pack( char* d, uint32_t s, const T& v ) {
      datastream<char*> ds(d,s);
      eosio::raw::pack(ds,v );
   }

  /** Deserialize a value from a buffer
   *  @return the deserialized value 
   *  @param d pointer to the buffer
   *  @param s size of the buffer
   */
   template<typename T>
   T unpack( const char* d, uint32_t s )
   {
      T v;
      datastream<const char*>  ds( d, s );
      eosio::raw::unpack(ds,v);
      return v;
   }
  
  /** Deserialize a value from a buffer
   *  @param d pointer to the buffer
   *  @param s size of the buffer
   *  @param v destination of deserialized value
   */
   template<typename T>
   void unpack( const char* d, uint32_t s, T& v )
   {
      datastream<const char*>  ds( d, s );
      eosio::raw::unpack(ds,v);
   }

} } // namespace eosio::raw

