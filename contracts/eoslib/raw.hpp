#pragma once
#include <eoslib/types.hpp>
#include <eoslib/varint.hpp>
#include <eoslib/datastream.hpp>
#include <eoslib/memory.hpp>

namespace eosio {
    namespace raw {    
  /**
   *  Serialize a list of values into a stream
   *  @param s    stream to write
   *  @param a0   value to be serialized
   *  @param args other values to be serialized
   */
   template<typename Stream, typename Arg0, typename... Args>
   inline void pack( Stream& s, const Arg0& a0, Args... args ) {
      pack( s, a0 );
      pack( s, args... );
   }

  /**
   *  Serialize a value into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream, typename T>
   inline void pack( Stream& s, const T& v )
   {
      s << v;
   }
   
  /**
   *  Deserialize a value from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream, typename T>
   inline void unpack( Stream& s, T& v )
   {
      s >> v;
   }
   
  /**
   *  Serialize an signed_int into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> inline void pack( Stream& s, const signed_int& v ) {
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
   template<typename Stream> inline void pack( Stream& s, const unsigned_int& v ) {
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
   template<typename Stream> inline void unpack( Stream& s, signed_int& vi ) {
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
   template<typename Stream> inline void unpack( Stream& s, unsigned_int& vi ) {
      uint64_t v = 0; char b = 0; uint8_t by = 0;
      do {
         s.get(b);
         v |= uint32_t(uint8_t(b) & 0x7f) << by;
         by += 7;
      } while( uint8_t(b) & 0x80 );
      vi.value = static_cast<uint32_t>(v);
   }

  /**
   *  Serialize a Bytes struct into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> inline void pack( Stream& s, const Bytes& value ) {
      eos::raw::pack( s, unsigned_int((uint32_t)value.len) );
      if( value.len )
         s.write( (char *)value.data, (uint32_t)value.len );
   }
   
  /**
   *  Deserialize a Bytes struct from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> inline void unpack( Stream& s, Bytes& value ) {
      unsigned_int size; eos::raw::unpack( s, size );
      value.len = size.value;
      if( value.len ) {
         value.data = (uint8_t *)eos::malloc(value.len);
         s.read( (char *)value.data, value.len );
      }
   }

  /**
   *  Serialize a string into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> inline void pack( Stream& s, const string& v )  {
      auto size = v.get_size();
      eosio::raw::pack( s, unsigned_int(size));
      if( size ) s.write( v.get_data(), size );
   }

  /**
   *  Deserialize a string from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> inline void unpack( Stream& s, string& v)  {
      unsigned_int size; eos::raw::unpack( s, size );
      v.assign((char*)s.pos(), size.value, true);
      s.skip(size.value);
   }

  /**
   *  Serialize a FixedString32 into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> inline void pack( Stream& s, const FixedString32& v )  {
      auto size = v.len;
      eos::raw::pack( s, unsigned_int(size));
      if( size ) s.write( v.str, size );
   }

  /**
   *  Deserialize a FixedString32 from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> inline void unpack( Stream& s, FixedString32& v)  {
      unsigned_int size; eos::raw::unpack( s, size );
      assert(size.value <= 32, "unpack FixedString32");
      s.read( (char *)v.str, size );
      v.len = size;
   }

  /**
   *  Serialize a FixedString16 into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> inline void pack( Stream& s, const FixedString16& v )  {
      auto size = v.len;
      eos::raw::pack( s, unsigned_int(size));
      if( size ) s.write( v.str, size );
   }

  /**
   *  Deserialize a FixedString16 from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> inline void unpack( Stream& s, FixedString16& v)  {
      unsigned_int size; eos::raw::unpack( s, size );
      assert(size.value <= 16, "unpack FixedString16");
      s.read( (char *)v.str, size );
      v.len = size;
   }
  
  /**
   *  Serialize a Price into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> inline void pack( Stream& s, const Price& v )  {
      eos::raw::pack(s, v.base);
      eos::raw::pack(s, v.quote);
   }

  /**
   *  Deserialize a Price from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> inline void unpack( Stream& s, Price& v)  {
      eos::raw::unpack(s, v.base);
      eos::raw::unpack(s, v.quote);
   }
  /**
   *  Serialize an Asset into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> inline void pack( Stream& s, const Asset& v )  {
      eos::raw::pack(s, v.amount);
      eos::raw::pack(s, v.symbol);
   }

  /**
   *  Deserialize an Asset from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> inline void unpack( Stream& s, Asset& v)  {
      eos::raw::unpack(s, v.amount);
      eos::raw::unpack(s, v.symbol);
   }
  /**
   *  Serialize a bool into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream> inline void pack( Stream& s, const bool& v ) { eos::raw::pack( s, uint8_t(v) );             }

  /**
   *  Deserialize a bool from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream> inline void unpack( Stream& s, bool& v )
   {
      uint8_t b;
      eos::raw::unpack( s, b );
      assert( (b & ~1) == 0, "unpack bool" );
      v=(b!=0);
   }
  
  /** Calculates the serialized size of a value
   *  @return serialized size of the value
   *  @param v value to calculate its serialized size
   */
   template<typename T>
   inline size_t pack_size( const T& v )
   {
      datastream<size_t> ps;
      eosio::raw::pack(ps,v );
      return ps.tellp();
   }
  
  /** Serialize a value into a Bytes struct
   *  @return Bytes struct with the serialized value 
   *  @param v value to be serialized
   */
   template<typename T>
   inline Bytes pack(  const T& v ) {
      datastream<size_t> ps;
      eos::raw::pack(ps, v);
      Bytes bytes;
      bytes.len = ps.tellp();
      bytes.data = (uint8_t*)eos::malloc(bytes.len);

      if( bytes.len ) {
         datastream<char*>  ds( (char*)bytes.data, bytes.len );
         eos::raw::pack(ds,v);
      }
      return bytes;
   }

  /** Serialize a value into a buffer
   *  @param d pointer to the buffer
   *  @param s size of the buffer
   *  @param v value to be serialized
   */
   template<typename T>
   inline void pack( char* d, uint32_t s, const T& v ) {
      datastream<char*> ds(d,s);
      eosio::raw::pack(ds,v );
    }

  /** Deserialize a value from a buffer
   *  @return the deserialized value 
   *  @param d pointer to the buffer
   *  @param s size of the buffer
   */
   template<typename T>
   inline T unpack( const char* d, uint32_t s )
   {
      T v;
      datastream<const char*>  ds( d, s );
      eosio::raw::unpack(ds,v);
      return v;
   }
  
  /** Deserialize a from a buffer
   *  @param d pointer to the buffer
   *  @param s size of the buffer
   *  @param v destination of deserialized value
   */
   template<typename T>
   inline void unpack( const char* d, uint32_t s, T& v )
   {
      datastream<const char*>  ds( d, s );
      eos::raw::unpack(ds,v);
   }

} } // namespace eos::raw

