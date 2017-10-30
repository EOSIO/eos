#pragma once
#include <eoslib/varint.hpp>
#include <eoslib/datastream.hpp>
#include <eoslib/memory.hpp>

namespace eos {
    namespace raw {

    template<typename Stream, typename Arg0, typename... Args>
    inline void pack( Stream& s, const Arg0& a0, Args... args ) {
       pack( s, a0 );
       pack( s, args... );
    }

    // generic
    template<typename Stream, typename T>
    inline void pack( Stream& s, const T& v )
    {
       s.write( (const char*)&v, sizeof(v) );
    }

    template<typename Stream, typename T>
    inline void unpack( Stream& s, T& v )
    {
       s.read( (char*)&v, sizeof(v) );
    }

    template<typename Stream> inline void pack( Stream& s, const signed_int& v ) {
      uint32_t val = (v.value<<1) ^ (v.value>>31);
      do {
        uint8_t b = uint8_t(val) & 0x7f;
        val >>= 7;
        b |= ((val > 0) << 7);
        s.write((char*)&b,1);//.put(b);
      } while( val );
    }

    template<typename Stream> inline void pack( Stream& s, const unsigned_int& v ) {
      uint64_t val = v.value;
      do {
        uint8_t b = uint8_t(val) & 0x7f;
        val >>= 7;
        b |= ((val > 0) << 7);
        s.write((char*)&b,1);//.put(b);
      }while( val );
    }

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
    template<typename Stream> inline void unpack( Stream& s, unsigned_int& vi ) {
      uint64_t v = 0; char b = 0; uint8_t by = 0;
      do {
          s.get(b);
          v |= uint32_t(uint8_t(b) & 0x7f) << by;
          by += 7;
      } while( uint8_t(b) & 0x80 );
      vi.value = static_cast<uint32_t>(v);
    }

    // template<typename Stream, typename T> inline void unpack( Stream& s, const T& vi )
    // {
    //    T tmp;
    //    eos::raw::unpack( s, tmp );
    //    FC_ASSERT( vi == tmp );
    // }

    // Bytes
    template<typename Stream> inline void pack( Stream& s, const Bytes& value ) {
      eos::raw::pack( s, unsigned_int((uint32_t)value.len) );
      if( value.len )
        s.write( (char *)value.data, (uint32_t)value.len );
    }
    template<typename Stream> inline void unpack( Stream& s, Bytes& value ) {
      unsigned_int size; eos::raw::unpack( s, size );
      //assert( size.value < MAX_ARRAY_ALLOC_SIZE, "unpack Bytes" );
      value.len = size.value;
      if( value.len )
        s.read( (char *)value.data, value.len );
    }

    // String
    template<typename Stream> inline void pack( Stream& s, const string& v )  {
      auto size = v.get_size();
      eos::raw::pack( s, unsigned_int(size));
      if( size ) s.write( v.get_data(), size );
    }

    template<typename Stream> inline void unpack( Stream& s, string& v, bool copy=true )  {
      unsigned_int size; eos::raw::unpack( s, size );
      v.assign(s.pos(), size.value, copy);
      s.skip(size.value);
    }

    // bool
    template<typename Stream> inline void pack( Stream& s, const bool& v ) { eos::raw::pack( s, uint8_t(v) );             }
    template<typename Stream> inline void unpack( Stream& s, bool& v )
    {
       uint8_t b;
       eos::raw::unpack( s, b );
       assert( (b & ~1) == 0, "unpack bool" );
       v=(b!=0);
    }

    template<typename T>
    inline size_t pack_size( const T& v )
    {
      datastream<size_t> ps;
      eos::raw::pack(ps,v );
      return ps.tellp();
    }

    template<typename T>
    inline void pack( char* d, uint32_t s, const T& v ) {
      datastream<char*> ds(d,s);
      eos::raw::pack(ds,v );
    }

    template<typename T>
    inline T unpack( const char* d, uint32_t s )
    {
      T v;
      datastream<const char*>  ds( d, s );
      eos::raw::unpack(ds,v);
      return v;
    }

    template<typename T>
    inline void unpack( const char* d, uint32_t s, T& v )
    {
      datastream<const char*>  ds( d, s );
      eos::raw::unpack(ds,v);
      return v;
    }

} } // namespace eos::raw

