/**
 *  @file datastream.hpp
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/system.h>
#include <eosiolib/memory.h>
#include <eosiolib/vector.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>
#include <eosiolib/varint.hpp>
#include <array>
#include <set>
#include <map>
#include <string>

#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/include/for_each.hpp>

#include <boost/pfr.hpp>


namespace eosio {


/**
 *  @brief A data stream for reading and writing data in the form of bytes
 */
template<typename T>
class datastream {
   public:
      datastream( T start, size_t s )
      :_start(start),_pos(start),_end(start+s){}

     /**
      *  Skips a specified number of bytes from this stream
      *  @brief Skips a specific number of bytes from this stream
      *  @param s The number of bytes to skip
      */
      inline void skip( size_t s ){ _pos += s; }

     /**
      *  Reads a specified number of bytes from the stream into a buffer
      *  @brief Reads a specified number of bytes from this stream into a buffer
      *  @param d pointer to the destination buffer
      *  @param s the number of bytes to read
      */
      inline bool read( char* d, size_t s ) {
        eosio_assert( size_t(_end - _pos) >= (size_t)s, "read" );
        memcpy( d, _pos, s );
        _pos += s;
        return true;
      }

     /**
      *  Writes a specified number of bytes into the stream from a buffer
      *  @brief Writes a specified number of bytes into the stream from a buffer
      *  @param d pointer to the source buffer
      *  @param s The number of bytes to write
      */
      inline bool write( const char* d, size_t s ) {
        eosio_assert( _end - _pos >= (int32_t)s, "write" );
        memcpy( (void*)_pos, d, s );
        _pos += s;
        return true;
      }

     /**
      *  Writes a byte into the stream
      *  @brief Writes a byte into the stream
      *  @param c byte to write
      */
      inline bool put(char c) {
        eosio_assert( _pos < _end, "put" );
        *_pos = c;
        ++_pos;
        return true;
      }

     /**
      *  Reads a byte from the stream
      *  @brief Reads a byte from the stream
      *  @param c reference to destination byte
      */
      inline bool get( unsigned char& c ) { return get( *(char*)&c ); }
      inline bool get( char& c )
      {
        eosio_assert( _pos < _end, "get" );
        c = *_pos;
        ++_pos;
        return true;
      }

     /**
      *  Retrieves the current position of the stream
      *  @brief Retrieves the current position of the stream
      *  @return the current position of the stream
      */
      T pos()const { return _pos; }
      inline bool valid()const { return _pos <= _end && _pos >= _start;  }

     /**
      *  Sets the position within the current stream
      *  @brief Sets the position within the current stream
      *  @param p offset relative to the origin
      */
      inline bool seekp(size_t p) { _pos = _start + p; return _pos <= _end; }

     /**
      *  Gets the position within the current stream
      *  @brief Gets the position within the current stream
      *  @return p the position within the current stream
      */
      inline size_t tellp()const      { return size_t(_pos - _start); }

     /**
      *  Returns the number of remaining bytes that can be read/skipped
      *  @brief Returns the number of remaining bytes that can be read/skipped
      *  @return number of remaining bytes
      */
      inline size_t remaining()const  { return _end - _pos; }
    private:
      T _start;
      T _pos;
      T _end;
};

/**
 *  @brief Specialization of datastream used to help determine the final size of a serialized value
 */
template<>
class datastream<size_t> {
   public:
     datastream( size_t init_size = 0):_size(init_size){}
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

/**
 *  Serialize a public_key into a stream
 *  @brief Serialize a public_key
 *  @param ds stream to write
 *  @param pubkey value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const public_key pubkey) {
  ds.write( (const char*)&pubkey, sizeof(pubkey));
  return ds;
}
/**
 *  Deserialize a public_key from a stream
 *  @brief Deserialize a public_key
 *  @param ds stream to read
 *  @param pubkey destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, public_key& pubkey) {
  ds.read((char*)&pubkey, sizeof(pubkey));
  return ds;
}

/**
 *  Serialize a key256 into a stream
 *  @brief Serialize a key256
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const key256& d) {
  ds.write( (const char*)d.data(), d.size() );
  return ds;
}
/**
 *  Deserialize a key256 from a stream
 *  @brief Deserialize a key256
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, key256& d) {
  ds.read((char*)d.data(), d.size() );
  return ds;
}

template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const bool& d) {
  return ds << uint8_t(d);
}
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, bool& d) {
  uint8_t t;
  ds >> t;
  d = t;
  return ds;
}

/**
 *  Serialize a checksum256 into a stream
 *  @brief Serialize a checksum256
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const checksum256& d) {
   ds.write( (const char*)&d, sizeof(d) );
   return ds;
}
/**
 *  Deserialize a checksum256 from a stream
 *  @brief Deserialize a checksum256
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, checksum256& d) {
   ds.read((char*)&d, sizeof(d) );
   return ds;
}

template<typename DataStream>
DataStream& operator << ( DataStream& ds, const std::string& v ) {
   ds << unsigned_int( v.size() );
   if (v.size())
      ds.write(v.data(), v.size());
   return ds;
}

template<typename DataStream>
DataStream& operator >> ( DataStream& ds, std::string& v ) {
   std::vector<char> tmp;
   ds >> tmp;
   if( tmp.size() )
      v = std::string(tmp.data(),tmp.data()+tmp.size());
   else
      v = std::string();
   return ds;
}

template<typename DataStream, typename T, std::size_t N>
DataStream& operator << ( DataStream& ds, const std::array<T,N>& v ) {
   for( const auto& i : v )
      ds << i;
   return ds;
}

template<typename DataStream, typename T, std::size_t N>
DataStream& operator >> ( DataStream& ds, std::array<T,N>& v ) {
   for( auto& i : v )
      ds >> i;
   return ds;
}

namespace _datastream_detail {
   template<typename T>
   constexpr bool is_pointer() {
      return std::is_pointer<T>::value ||
             std::is_null_pointer<T>::value ||
             std::is_member_pointer<T>::value;
   }

   template<typename T>
   constexpr bool is_primitive() {
      return std::is_arithmetic<T>::value ||
             std::is_enum<T>::value;
   }
}

template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_pointer<T>()>* = nullptr>
DataStream& operator >> ( DataStream& ds, T ) {
   static_assert(!_datastream_detail::is_pointer<T>(), "Pointers should not be serialized" );
   return ds;
}

template<typename DataStream, typename T, std::size_t N,
         std::enable_if_t<!_datastream_detail::is_primitive<T>() &&
                          !_datastream_detail::is_pointer<T>()>* = nullptr>
DataStream& operator << ( DataStream& ds, const T (&v)[N] ) {
   ds << unsigned_int( N );
   for( uint32_t i = 0; i < N; ++i )
      ds << v[i];
   return ds;
}

template<typename DataStream, typename T, std::size_t N,
         std::enable_if_t<_datastream_detail::is_primitive<T>()>* = nullptr>
DataStream& operator << ( DataStream& ds, const T (&v)[N] ) {
   ds << unsigned_int( N );
   ds.write((char*)&v[0], sizeof(v));
   return ds;
}

template<typename DataStream, typename T, std::size_t N,
         std::enable_if_t<!_datastream_detail::is_primitive<T>() &&
                          !_datastream_detail::is_pointer<T>()>* = nullptr>
DataStream& operator >> ( DataStream& ds, T (&v)[N] ) {
   unsigned_int s;
   ds >> s;
   eosio_assert( N == s.value, "T[] size and unpacked size don't match");
   for( uint32_t i = 0; i < N; ++i )
      ds >> v[i];
   return ds;
}

template<typename DataStream, typename T, std::size_t N,
         std::enable_if_t<_datastream_detail::is_primitive<T>()>* = nullptr>
DataStream& operator >> ( DataStream& ds, T (&v)[N] ) {
   unsigned_int s;
   ds >> s;
   eosio_assert( N == s.value, "T[] size and unpacked size don't match");
   ds.read((char*)&v[0], sizeof(v));
   return ds;
}

template<typename DataStream>
DataStream& operator << ( DataStream& ds, const vector<char>& v ) {
   ds << unsigned_int( v.size() );
   ds.write( v.data(), v.size() );
   return ds;
}

template<typename DataStream, typename T>
DataStream& operator << ( DataStream& ds, const vector<T>& v ) {
   ds << unsigned_int( v.size() );
   for( const auto& i : v )
      ds << i;
   return ds;
}

template<typename DataStream>
DataStream& operator >> ( DataStream& ds, vector<char>& v ) {
   unsigned_int s;
   ds >> s;
   v.resize( s.value );
   ds.read( v.data(), v.size() );
   return ds;
}

template<typename DataStream, typename T>
DataStream& operator >> ( DataStream& ds, vector<T>& v ) {
   unsigned_int s;
   ds >> s;
   v.resize(s.value);
   for( auto& i : v )
      ds >> i;
   return ds;
}

template<typename DataStream, typename T>
DataStream& operator << ( DataStream& ds, const std::set<T>& s ) {
   ds << unsigned_int( s.size() );
   for( const auto& i : s ) {
      ds << i;
   }
   return ds;
}

template<typename DataStream, typename T>
DataStream& operator >> ( DataStream& ds, std::set<T>& s ) {
   s.clear();
   unsigned_int sz; ds >> sz;

   for( uint32_t i = 0; i < sz.value; ++i ) {
      T v;
      ds >> v;
      s.emplace( std::move(v) );
   }
   return ds;
}

template<typename DataStream, typename K, typename V>
DataStream& operator << ( DataStream& ds, const std::map<K,V>& m ) {
   ds << unsigned_int( m.size() );
   for( const auto& i : m ) {
      ds << i.first << i.second;
   }
   return ds;
}

template<typename DataStream, typename K, typename V>
DataStream& operator >> ( DataStream& ds, std::map<K,V>& m ) {
   m.clear();
   unsigned_int s; ds >> s;

   for (uint32_t i = 0; i < s.value; ++i) {
      K k; V v;
      ds >> k >> v;
      m.emplace( std::move(k), std::move(v) );
   }
   return ds;
}

template<typename DataStream, typename T>
DataStream& operator << ( DataStream& ds, const boost::container::flat_set<T>& s ) {
   ds << unsigned_int( s.size() );
   for( const auto& i : s ) {
      ds << i;
   }
   return ds;
}

template<typename DataStream, typename T>
DataStream& operator >> ( DataStream& ds, boost::container::flat_set<T>& s ) {
   s.clear();
   unsigned_int sz; ds >> sz;

   for( uint32_t i = 0; i < sz.value; ++i ) {
      T v;
      ds >> v;
      s.emplace( std::move(v) );
   }
   return ds;
}

template<typename DataStream, typename K, typename V>
DataStream& operator<<( DataStream& ds, const boost::container::flat_map<K,V>& m ) {
   ds << unsigned_int( m.size() );
   for( const auto& i : m )
      ds << i.first << i.second;
   return ds;
}

template<typename DataStream, typename K, typename V>
DataStream& operator>>( DataStream& ds, boost::container::flat_map<K,V>& m ) {
   m.clear();
   unsigned_int s; ds >> s;

   for( uint32_t i = 0; i < s.value; ++i ) {
      K k; V v;
      ds >> k >> v;
      m.emplace( std::move(k), std::move(v) );
   }
   return ds;
}

template<typename DataStream, typename... Args>
DataStream& operator<<( DataStream& ds, const std::tuple<Args...>& t ) {
   boost::fusion::for_each( t, [&]( const auto& i ) {
       ds << i;
   });
   return ds;
}

template<typename DataStream, typename... Args>
DataStream& operator>>( DataStream& ds, std::tuple<Args...>& t ) {
   boost::fusion::for_each( t, [&]( auto& i ) {
       ds >> i;
   });
   return ds;
}

template<typename DataStream, typename T, std::enable_if_t<std::is_class<T>::value>* = nullptr>
DataStream& operator<<( DataStream& ds, const T& v ) {
   boost::pfr::for_each_field(v, [&](const auto& field) {
      ds << field;
   });
   return ds;
}
template<typename DataStream, typename T, std::enable_if_t<std::is_class<T>::value>* = nullptr>
DataStream& operator>>( DataStream& ds, T& v ) {
   boost::pfr::for_each_field(v, [&](auto& field) {
      ds >> field;
   });
   return ds;
}

template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_primitive<T>()>* = nullptr>
DataStream& operator<<( DataStream& ds, const T& v ) {
   ds.write( (const char*)&v, sizeof(T) );
   return ds;
}

template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_primitive<T>()>* = nullptr>
DataStream& operator>>( DataStream& ds, T& v ) {
   ds.read( (char*)&v, sizeof(T) );
   return ds;
}

template<typename T>
T unpack( const char* buffer, size_t len ) {
   T result;
   datastream<const char*> ds(buffer,len);
   ds >> result;
   return result;
}

template<typename T>
T unpack( const vector<char>& bytes ) {
   return unpack<T>( bytes.data(), bytes.size() );
}

template<typename T>
size_t pack_size( const T& value ) {
  datastream<size_t> ps;
  ps << value;
  return ps.tellp();
}

template<typename T>
bytes pack( const T& value ) {
  bytes result;
  result.resize(pack_size(value));

  datastream<char*> ds( result.data(), result.size() );
  ds << value;
  return result;
}

template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const checksum160& cs) {
   ds.write((const char*)&cs, sizeof(cs));
   return ds;
}

template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, checksum160& cs) {
   ds.read((char*)&cs, sizeof(cs));
   return ds;
}

template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const checksum512& cs) {
   ds.write((const char*)&cs, sizeof(cs));
   return ds;
}

template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, checksum512& cs) {
   ds.read((char*)&cs, sizeof(cs));
   return ds;
}

}
