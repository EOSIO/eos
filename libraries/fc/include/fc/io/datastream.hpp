#pragma once
#include <fc/utility.hpp>
#include <fc/exception/exception.hpp>

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

/**
 * A circular_buffer specialization of a datastream accesses a circular
 * buffer of elements in the range buffer_start+buffer_size. The buffer
 * will begin reading at the offset of start in this buffer, and will read
 * up to s entries.
 * The buffer cannot be "infinite" -- i.e. s must be less than buffer_size
 */

template<typename T>
struct circular_buffer {};

template<typename T>
class datastream<circular_buffer<T>> {
   public:
      datastream(T buffer_start, size_t buffer_size, size_t start, size_t s) :
       _buffer_start(buffer_start), _buffer_end(buffer_start+buffer_size),
       _pos(buffer_start+start), _start(_pos), _remaining(s), _orig_size(s)
      {
         if(start >= buffer_size)
            FC_THROW_EXCEPTION(out_of_range_exception, "circular_buffer datastream start>buffer_size");
         if(s > buffer_size) 
            FC_THROW_EXCEPTION(out_of_range_exception, "circular_buffer datastream s>buffer_size");
      }

      inline bool skip(size_t s) {
         if(s > _remaining)
            detail::throw_datastream_range_error("circular skip", _remaining, s-_remaining);
         _remaining -= s;
         _pos += s;
         if(_pos > _buffer_end)
            _pos = _buffer_start + (_pos-_buffer_end);
         return true;
      }

      inline bool read(char* d, size_t s) {
         if(s > _remaining)
            detail::throw_datastream_range_error("circular read", _remaining, s-_remaining);
         _remaining -= s;
         if(_pos + s > _buffer_end) {
            size_t first_read = _buffer_end - _pos;
            memcpy(d, _pos, first_read);
            memcpy(d + first_read, _buffer_start, s - first_read);
            _pos = _buffer_start + s - first_read;
         }
         else {
            memcpy(d, _pos, s);
            _pos += s;
         }
         return true;
      }

      inline bool write( const char* d, size_t s ) {
         if(s > _remaining)
            detail::throw_datastream_range_error("circular write", _remaining, s-_remaining);
         _remaining -= s;
         if(_pos + s > _buffer_end) {
            size_t first_write = _buffer_end - _pos;
            memcpy(_pos, d, first_write);
            memcpy(_buffer_start, d + first_write, s - first_write);
            _pos = _buffer_start + s - first_write;
         }
         else {
            memcpy(_pos, d, s);
            _pos += s;
         }
         return true;
      }

      inline bool put(char c) { 
         if(!_remaining)
            FC_THROW_EXCEPTION(out_of_range_exception, "circular_buffer put no room"); 
         --_remaining;
         *_pos = c; 
         if(++_pos == _buffer_end)
            _pos = _buffer_start; 
         return true;
      }

      inline bool get(unsigned char& c) { return get( *(char*)&c ); }
      inline bool get(char& c) {
         if(!_remaining)
            FC_THROW_EXCEPTION(out_of_range_exception, "circular_buffer get no room"); 
         --_remaining;
         c = *_pos;
         if(++_pos == _buffer_end)
            _pos = _buffer_start; 
         return true;
      }

      inline bool seekp(size_t p) {
         if(p >= _orig_size)
            detail::throw_datastream_range_error("circular seekp", _orig_size, _orig_size-p);
         _pos = _start + p;
         if(_pos >= _buffer_end)
            _pos = _buffer_start + (_pos - _buffer_end);
         _remaining = _orig_size - p;
         return true;
       }

     T               pos()const        { return _pos; }
     inline bool     valid()const      { return true; }
     inline size_t   tellp()const      { return _orig_size - _remaining; }
     inline size_t   remaining()const  { return _remaining; }

  private:
     T _buffer_start;
     T _buffer_end;
     T _pos;
     T _start;
     size_t _remaining;
     size_t _orig_size;
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

