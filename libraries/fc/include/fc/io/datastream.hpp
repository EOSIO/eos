#pragma once
#include <fc/utility.hpp>
#include <fc/exception/exception.hpp>
#include <string.h>
#include <stdint.h>
#include <type_traits>

#include <boost/multiprecision/cpp_int.hpp>

namespace fc {

namespace detail
{
  NO_RETURN void throw_datastream_range_error( const char* file, size_t len, int64_t over );
}

template <typename Storage, typename Enable = void>
class datastream;

/**
 *  The purpose of this datastream is to provide a fast, efficient, means
 *  of calculating the amount of data "about to be written" and then
 *  writing it.  This means having two modes of operation, "test run" where
 *  you call the entire pack sequence calculating the size, and then
 *  actually packing it after doing a single allocation.
 */
template<typename T>
class datastream<T, std::enable_if_t<std::is_same_v<T, char*> || std::is_same_v<T, const char*>>>  {
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
class datastream<size_t, void> {
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

template <typename Streambuf>
class datastream<Streambuf, typename std::enable_if_t<std::is_base_of_v<std::streambuf, Streambuf>>> {
 private:
   Streambuf buf;

 public:
   template <typename... Args>
   datastream(Args&&... args)
       : buf(std::forward<Args>(args)...) {}

   size_t read(char* data, size_t n) { return buf.sgetn(data, n); }
   size_t write(const char* data, size_t n) { return buf.sputn(data, n); }
   size_t tellp() { return this->pubseekoff(0, std::ios::cur); }
   bool   skip(size_t p) { this->pubseekoff(p, std::ios::cur);  return true;  }
   bool   get(char& c) {
      c = buf.sbumpc();
      return true;
   }
   bool seekp(size_t off) {
      buf.pubseekoff(off, std::ios::beg);
      return true;
   }
   bool remaining() { return buf.in_avail(); }

   Streambuf&       storage() { return buf; }
   const Streambuf& storage() const { return buf; }
};

template <typename Container>
class datastream<Container, typename std::enable_if_t<(std::is_same_v<std::vector<char>, Container> ||
                                                       std::is_same_v<std::deque<char>, Container>)>> {
 private:
   Container _container;
   size_t    cur;

 public:
   template <typename... Args>
   datastream(Args&&... args)
       : _container(std::forward<Args>(args)...)
       , cur(0) {}

   size_t read(char* s, size_t n) {
      if (cur + n > _container.size()) {
         FC_THROW_EXCEPTION(out_of_range_exception,
                            "read datastream<std::vector<char>> of length ${len} over by ${over}",
                            ("len", _container.size())("over", _container.size() - n));
      }
      std::copy_n(_container.begin() + cur, n, s);
      cur += n;
      return n;
   }

   size_t write(const char* s, size_t n) {
      _container.resize(std::max(cur + n, _container.size()));
      std::copy_n(s, n, _container.begin() + cur);
      cur += n;
      return n;
   }

   bool seekp(size_t off) {
      cur = off;
      return true;
   }

   size_t tellp() const { return cur; }
   bool   skip(size_t p) { cur += p;  return true;  }

   bool get(char& c) {
      this->read(&c, 1);
      return true;
   }

   size_t remaining() const { return _container.size() - cur; }

   Container&       storage() { return _container; }
   const Container& storage() const { return _container; }
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
