#pragma once
#include <fc/utility.hpp>
#include <string.h>
#include <stdint.h>

#include <boost/multiprecision/cpp_int.hpp>

namespace fc {

namespace detail
{
  NO_RETURN void throw_datastream_range_error( const char* file, size_t len, int64_t over );
}

/**
 *  The purpose of this datastream is to provide a fast, effecient, means
 *  of calculating the amount of data "about to be written" and then
 *  writing it.  This means having two modes of operation, "test run" where
 *  you call the entire pack sequence calculating the size, and then
 *  actually packing it after doing a single allocation.
 */
template<typename T>
class datastream {
   public:
      datastream( T start, size_t s )
      :_start(start),_pos(start),_end(start+s){};


      inline void skip( size_t s ){ _pos += s; }
      inline bool read( char* d, size_t s ) {
        if( size_t(_end - _pos) >= (size_t)s ) {
          memcpy( d, _pos, s );
          _pos += s;
          return true;
        }
        detail::throw_datastream_range_error( "read", _end-_start, int64_t(-((_end-_pos) - 1)));
      }

      inline bool write( const char* d, size_t s ) {
        if( _end - _pos >= (int32_t)s ) {
          memcpy( _pos, d, s );
          _pos += s;
          return true;
        }
        detail::throw_datastream_range_error( "write", _end-_start, int64_t(-((_end-_pos) - 1)));
      }

      inline bool   put(char c) {
        if( _pos < _end ) {
          *_pos = c;
          ++_pos;
          return true;
        }
        detail::throw_datastream_range_error( "put", _end-_start, int64_t(-((_end-_pos) - 1)));
      }

      inline bool   get( unsigned char& c ) { return get( *(char*)&c ); }
      inline bool   get( char& c )
      {
        if( _pos < _end ) {
          c = *_pos;
          ++_pos;
          return true;
        }
        detail::throw_datastream_range_error( "get", _end-_start, int64_t(-((_end-_pos) - 1)));
      }

      T               pos()const        { return _pos;                               }
      inline bool     valid()const      { return _pos <= _end && _pos >= _start;  }
      inline bool     seekp(size_t p) { _pos = _start + p; return _pos <= _end; }
      inline size_t   tellp()const      { return _pos - _start;                     }
      inline size_t   remaining()const  { return _end - _pos;                       }
    private:
      T _start;
      T _pos;
      T _end;
};

template<>
class datastream<size_t> {
   public:
     datastream( size_t init_size = 0):_size(init_size){};
     inline bool     skip( size_t s )                 { _size += s; return true;  }
     inline bool     write( const char* ,size_t s )  { _size += s; return true;  }
     inline bool     put(char )                      { ++_size; return  true;    }
     inline bool     valid()const                     { return true;              }
     inline bool     seekp(size_t p)                  { _size = p;  return true;  }
     inline size_t   tellp()const                     { return _size;             }
     inline size_t   remaining()const                 { return 0;                 }
  private:
     size_t _size;
};

template<typename ST>
inline datastream<ST>& operator<<(datastream<ST>& ds, const __int128& d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}

template<typename ST, typename DATA>
inline datastream<ST>& operator>>(datastream<ST>& ds, __int128& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

template<typename ST>
inline datastream<ST>& operator<<(datastream<ST>& ds, const unsigned __int128& d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}

template<typename ST, typename DATA>
inline datastream<ST>& operator>>(datastream<ST>& ds, unsigned __int128& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

template<typename ST>
inline datastream<ST>& operator<<(datastream<ST>& ds, const int64_t& d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}

template<typename ST, typename DATA>
inline datastream<ST>& operator>>(datastream<ST>& ds, int64_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

template<typename ST>
inline datastream<ST>& operator<<(datastream<ST>& ds, const uint64_t& d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}

template<typename ST, typename DATA>
inline datastream<ST>& operator>>(datastream<ST>& ds, uint64_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

template<typename ST>
inline datastream<ST>& operator<<(datastream<ST>& ds, const int32_t& d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}

template<typename ST, typename DATA>
inline datastream<ST>& operator>>(datastream<ST>& ds, int32_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

template<typename ST>
inline datastream<ST>& operator<<(datastream<ST>& ds, const uint32_t& d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}

template<typename ST, typename DATA>
inline datastream<ST>& operator>>(datastream<ST>& ds, uint32_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

template<typename ST>
inline datastream<ST>& operator<<(datastream<ST>& ds, const int16_t& d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}

template<typename ST, typename DATA>
inline datastream<ST>& operator>>(datastream<ST>& ds, int16_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

template<typename ST>
inline datastream<ST>& operator<<(datastream<ST>& ds, const uint16_t& d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}

template<typename ST, typename DATA>
inline datastream<ST>& operator>>(datastream<ST>& ds, uint16_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}
template<typename ST>
inline datastream<ST>& operator<<(datastream<ST>& ds, const int8_t& d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}

template<typename ST, typename DATA>
inline datastream<ST>& operator>>(datastream<ST>& ds, int8_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}
template<typename ST>
inline datastream<ST>& operator<<(datastream<ST>& ds, const uint8_t& d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}

template<typename ST, typename DATA>
inline datastream<ST>& operator>>(datastream<ST>& ds, uint8_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}
/*
template<typename ST, typename T>
inline datastream<ST>& operator<<(datastream<ST>& ds, const boost::multiprecision::number<T>& n) {
   unsigned char data[(std::numeric_limits<decltype(n)>::digits+1)/8];
   ds.read( (char*)data, sizeof(data) );
   boost::multiprecision::import_bits( n, data, data + sizeof(data), 1 );
}

template<typename ST, typename T>
inline datastream<ST>& operator>>(datastream<ST>& ds, boost::multiprecision::number<T>& n) {
   unsigned char data[(std::numeric_limits<decltype(n)>::digits+1)/8];
   boost::multiprecision::export_bits( n, data, 1 );
   ds.write( (const char*)data, sizeof(data) );
}
*/


} // namespace fc
