/**
 *  @file datastream.hpp
 *  @copyright defined in eos/LICENSE
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
 * @defgroup datastream Data Stream
 * @brief Defines data stream for reading and writing data in the form of bytes
 * @ingroup serialize
 * @{
 */

/**
 *  %A data stream for reading and writing data in the form of bytes
 *
 *  @brief %A data stream for reading and writing data in the form of bytes.
 *  @tparam T - Type of the datastream buffer
 */
template<typename T>
class datastream {
   public:
      /**
       * Construct a new datastream object given the size of the buffer and start position of the buffer
       *
       * @brief Construct a new datastream object
       * @param start - The start position of the buffer
       * @param s - The size of the buffer
       */
      datastream( T start, size_t s )
      :_start(start),_pos(start),_end(start+s){}

     /**
      *  Skips a specified number of bytes from this stream
      *
      *  @brief Skips a specific number of bytes from this stream
      *  @param s - The number of bytes to skip
      */
      inline void skip( size_t s ){ _pos += s; }

     /**
      *  Reads a specified number of bytes from the stream into a buffer
      *
      *  @brief Reads a specified number of bytes from this stream into a buffer
      *  @param d - The pointer to the destination buffer
      *  @param s - the number of bytes to read
      *  @return true
      */
      inline bool read( char* d, size_t s ) {
        eosio_assert( size_t(_end - _pos) >= (size_t)s, "read" );
        memcpy( d, _pos, s );
        _pos += s;
        return true;
      }

     /**
      *  Writes a specified number of bytes into the stream from a buffer
      *
      *  @brief Writes a specified number of bytes into the stream from a buffer
      *  @param d - The pointer to the source buffer
      *  @param s - The number of bytes to write
      *  @return true
      */
      inline bool write( const char* d, size_t s ) {
        eosio_assert( _end - _pos >= (int32_t)s, "write" );
        memcpy( (void*)_pos, d, s );
        _pos += s;
        return true;
      }

     /**
      *  Writes a byte into the stream
      *
      *  @brief Writes a byte into the stream
      *  @param c byte to write
      *  @return true
      */
      inline bool put(char c) {
        eosio_assert( _pos < _end, "put" );
        *_pos = c;
        ++_pos;
        return true;
      }

     /**
      *  Reads a byte from the stream
      *
      *  @brief Reads a byte from the stream
      *  @param c - The reference to destination byte
      *  @return true
      */
      inline bool get( unsigned char& c ) { return get( *(char*)&c ); }

     /**
      *  Reads a byte from the stream
      *
      *  @brief Reads a byte from the stream
      *  @param c - The reference to destination byte
      *  @return true
      */
      inline bool get( char& c )
      {
        eosio_assert( _pos < _end, "get" );
        c = *_pos;
        ++_pos;
        return true;
      }

     /**
      *  Retrieves the current position of the stream
      *
      *  @brief Retrieves the current position of the stream
      *  @return T - The current position of the stream
      */
      T pos()const { return _pos; }
      inline bool valid()const { return _pos <= _end && _pos >= _start;  }

     /**
      *  Sets the position within the current stream
      *
      *  @brief Sets the position within the current stream
      *  @param p - The offset relative to the origin
      *  @return true if p is within the range
      *  @return false if p is not within the rawnge
      */
      inline bool seekp(size_t p) { _pos = _start + p; return _pos <= _end; }

     /**
      *  Gets the position within the current stream
      *
      *  @brief Gets the position within the current stream
      *  @return p - The position within the current stream
      */
      inline size_t tellp()const      { return size_t(_pos - _start); }

     /**
      *  Returns the number of remaining bytes that can be read/skipped
      *
      *  @brief Returns the number of remaining bytes that can be read/skipped
      *  @return size_t - The number of remaining bytes
      */
      inline size_t remaining()const  { return _end - _pos; }
    private:
      /**
       * The start position of the buffer
       *
       * @brief The start position of the buffer
       */
      T _start;
      /**
       * The current position of the buffer
       *
       * @brief The current position of the buffer
       */
      T _pos;
      /**
       * The end position of the buffer
       *
       * @brief The end position of the buffer
       */
      T _end;
};

/**
 * @brief Specialization of datastream used to help determine the final size of a serialized value.
 * Specialization of datastream used to help determine the final size of a serialized value
 */
template<>
class datastream<size_t> {
   public:
      /**
       * Construct a new specialized datastream object given the initial size
       *
       * @brief Construct a new specialized datastream object
       * @param init_size - The initial size
       */
     datastream( size_t init_size = 0):_size(init_size){}

     /**
      *  Increment the size by s. This behaves the same as write( const char* ,size_t s ).
      *
      *  @brief Increase the size by s
      *  @param s - The amount of size to increase
      *  @return true
      */
     inline bool     skip( size_t s )                 { _size += s; return true;  }

     /**
      *  Increment the size by s. This behaves the same as skip( size_t s )
      *
      *  @brief Increase the size by s
      *  @param s - The amount of size to increase
      *  @return true
      */
     inline bool     write( const char* ,size_t s )  { _size += s; return true;  }

     /**
      *  Increment the size by one
      *
      *  @brief Increase the size by one
      *  @return true
      */
     inline bool     put(char )                      { ++_size; return  true;    }

     /**
      *  Check validity. It's always valid
      *
      *  @brief Check validity
      *  @return true
      */
     inline bool     valid()const                     { return true;              }

     /**
      * Set new size
      *
      * @brief Set new size
      * @param p - The new size
      * @return true
      */
     inline bool     seekp(size_t p)                  { _size = p;  return true;  }

     /**
      * Get the size
      *
      * @brief Get the size
      * @return size_t - The size
      */
     inline size_t   tellp()const                     { return _size;             }

     /**
      * Always returns 0
      *
      * @brief Always returns 0
      * @return size_t - 0
      */
     inline size_t   remaining()const                 { return 0;                 }
  private:
     /**
      * The size used to determine the final size of a serialized value.
      *
      * @brief The size used to determine the final size of a serialized value.
      */
     size_t _size;
};

/**
 *  Serialize a public_key into a stream
 *
 *  @brief Serialize a public_key
 *  @param ds - The stream to write
 *  @param pubkey - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const public_key pubkey) {
  ds.write( (const char*)&pubkey, sizeof(pubkey));
  return ds;
}

/**
 *  Deserialize a public_key from a stream
 *
 *  @brief Deserialize a public_key
 *  @param ds - The stream to read
 *  @param pubkey - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, public_key& pubkey) {
  ds.read((char*)&pubkey, sizeof(pubkey));
  return ds;
}

/**
 *  Serialize a key256 into a stream
 *
 *  @brief Serialize a key256
 *  @param ds - The stream to write
 *  @param d - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const key256& d) {
  ds.write( (const char*)d.data(), d.size() );
  return ds;
}

/**
 *  Deserialize a key256 from a stream
 *
 *  @brief Deserialize a key256
 *  @param ds - The stream to read
 *  @param d - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, key256& d) {
  ds.read((char*)d.data(), d.size() );
  return ds;
}

/**
 *  Serialize a bool into a stream
 *
 *  @brief  Serialize a bool into a stream
 *  @param ds - The stream to read
 *  @param d - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const bool& d) {
  return ds << uint8_t(d);
}

/**
 *  Deserialize a bool from a stream
 *
 *  @brief Deserialize a bool
 *  @param ds - The stream to read
 *  @param d - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, bool& d) {
  uint8_t t;
  ds >> t;
  d = t;
  return ds;
}

/**
 *  Serialize a checksum256 into a stream
 *
 *  @brief Serialize a checksum256
 *  @param ds - The stream to write
 *  @param d - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const checksum256& d) {
   ds.write( (const char*)&d.hash[0], sizeof(d.hash) );
   return ds;
}

/**
 *  Deserialize a checksum256 from a stream
 *
 *  @brief Deserialize a checksum256
 *  @param ds - The stream to read
 *  @param d - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, checksum256& d) {
   ds.read((char*)&d.hash[0], sizeof(d.hash) );
   return ds;
}

/**
 *  Serialize a string into a stream
 *
 *  @brief Serialize a string
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream>
DataStream& operator << ( DataStream& ds, const std::string& v ) {
   ds << unsigned_int( v.size() );
   if (v.size())
      ds.write(v.data(), v.size());
   return ds;
}

/**
 *  Deserialize a string from a stream
 *
 *  @brief Deserialize a string
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
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

/**
 *  Serialize a fixed size array into a stream
 *
 *  @brief Serialize a fixed size array
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the object contained in the array
 *  @tparam N - Size of the array
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename T, std::size_t N>
DataStream& operator << ( DataStream& ds, const std::array<T,N>& v ) {
   for( const auto& i : v )
      ds << i;
   return ds;
}


/**
 *  Deserialize a fixed size array from a stream
 *
 *  @brief Deserialize a fixed size array
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the object contained in the array
 *  @tparam N - Size of the array
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename T, std::size_t N>
DataStream& operator >> ( DataStream& ds, std::array<T,N>& v ) {
   for( auto& i : v )
      ds >> i;
   return ds;
}

namespace _datastream_detail {
   /**
    * Check if type T is a pointer
    *
    * @brief Check if type T is a pointer
    * @tparam T - The type to be checked
    * @return true if T is a pointer
    * @return false otherwise
    */
   template<typename T>
   constexpr bool is_pointer() {
      return std::is_pointer<T>::value ||
             std::is_null_pointer<T>::value ||
             std::is_member_pointer<T>::value;
   }

   /**
    * Check if type T is a primitive type
    *
    * @brief Check if type T is a primitive type
    * @tparam T - The type to be checked
    * @return true if T is a primitive type
    * @return false otherwise
    */
   template<typename T>
   constexpr bool is_primitive() {
      return std::is_arithmetic<T>::value ||
             std::is_enum<T>::value;
   }
}

/**
 *  Pointer should not be serialized, so this function will always throws an error
 *
 *  @brief Deserialize a a pointer
 *  @param ds - The stream to read
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the pointer
 *  @return DataStream& - Reference to the datastream
 *  @post Throw an exception if it is a pointer
 */
template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_pointer<T>()>* = nullptr>
DataStream& operator >> ( DataStream& ds, T ) {
   static_assert(!_datastream_detail::is_pointer<T>(), "Pointers should not be serialized" );
   return ds;
}

/**
 *  Serialize a fixed size array of non-primitive and non-pointer type
 *
 *  @brief Serialize a fixed size array of non-primitive and non-pointer type
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the pointer
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename T, std::size_t N,
         std::enable_if_t<!_datastream_detail::is_primitive<T>() &&
                          !_datastream_detail::is_pointer<T>()>* = nullptr>
DataStream& operator << ( DataStream& ds, const T (&v)[N] ) {
   ds << unsigned_int( N );
   for( uint32_t i = 0; i < N; ++i )
      ds << v[i];
   return ds;
}

/**
 *  Serialize a fixed size array of non-primitive type
 *
 *  @brief Serialize a fixed size array of non-primitive type
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the pointer
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename T, std::size_t N,
         std::enable_if_t<_datastream_detail::is_primitive<T>()>* = nullptr>
DataStream& operator << ( DataStream& ds, const T (&v)[N] ) {
   ds << unsigned_int( N );
   ds.write((char*)&v[0], sizeof(v));
   return ds;
}

/**
 *  Deserialize a fixed size array of non-primitive and non-pointer type
 *
 *  @brief Deserialize a fixed size array of non-primitive and non-pointer type
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam T - Type of the object contained in the array
 *  @tparam N - Size of the array
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
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

/**
 *  Deserialize a fixed size array of non-primitive type
 *
 *  @brief Deserialize a fixed size array of non-primitive type
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam T - Type of the object contained in the array
 *  @tparam N - Size of the array
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename T, std::size_t N,
         std::enable_if_t<_datastream_detail::is_primitive<T>()>* = nullptr>
DataStream& operator >> ( DataStream& ds, T (&v)[N] ) {
   unsigned_int s;
   ds >> s;
   eosio_assert( N == s.value, "T[] size and unpacked size don't match");
   ds.read((char*)&v[0], sizeof(v));
   return ds;
}

/**
 *  Serialize a vector of char
 *
 *  @brief Serialize a vector of char
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream>
DataStream& operator << ( DataStream& ds, const vector<char>& v ) {
   ds << unsigned_int( v.size() );
   ds.write( v.data(), v.size() );
   return ds;
}

/**
 *  Serialize a vector
 *
 *  @brief Serialize a vector
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the object contained in the vector
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename T>
DataStream& operator << ( DataStream& ds, const vector<T>& v ) {
   ds << unsigned_int( v.size() );
   for( const auto& i : v )
      ds << i;
   return ds;
}

/**
 *  Deserialize a vector of char
 *
 *  @brief Deserialize a vector of char
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream>
DataStream& operator >> ( DataStream& ds, vector<char>& v ) {
   unsigned_int s;
   ds >> s;
   v.resize( s.value );
   ds.read( v.data(), v.size() );
   return ds;
}

/**
 *  Deserialize a vector
 *
 *  @brief Deserialize a vector
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the object contained in the vector
 *  @return DataStream& - Reference to the datastream
 */
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

/**
 *  Serialize a map
 *
 *  @brief Serialize a map
 *  @param ds - The stream to write
 *  @param m - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam K - Type of the key contained in the map
 *  @tparam V - Type of the value contained in the map
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename K, typename V>
DataStream& operator << ( DataStream& ds, const std::map<K,V>& m ) {
   ds << unsigned_int( m.size() );
   for( const auto& i : m ) {
      ds << i.first << i.second;
   }
   return ds;
}

/**
 *  Deserialize a map
 *
 *  @brief Deserialize a map
 *  @param ds - The stream to read
 *  @param m - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam K - Type of the key contained in the map
 *  @tparam V - Type of the value contained in the map
 *  @return DataStream& - Reference to the datastream
 */
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


/**
 *  Serialize a flat map
 *
 *  @brief Serialize a flat map
 *  @param ds - The stream to write
 *  @param m - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam K - Type of the key contained in the flat map
 *  @tparam V - Type of the value contained in the flat map
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename K, typename V>
DataStream& operator<<( DataStream& ds, const boost::container::flat_map<K,V>& m ) {
   ds << unsigned_int( m.size() );
   for( const auto& i : m )
      ds << i.first << i.second;
   return ds;
}

/**
 *  Deserialize a flat map
 *
 *  @brief Deserialize a flat map
 *  @param ds - The stream to read
 *  @param m - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam K - Type of the key contained in the flat map
 *  @tparam V - Type of the value contained in the flat map
 *  @return DataStream& - Reference to the datastream
 */
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

/**
 *  Serialize a tuple
 *
 *  @brief Serialize a tuple
 *  @param ds - The stream to write
 *  @param t - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam Args - Type of the objects contained in the tuple
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename... Args>
DataStream& operator<<( DataStream& ds, const std::tuple<Args...>& t ) {
   boost::fusion::for_each( t, [&]( const auto& i ) {
       ds << i;
   });
   return ds;
}

/**
 *  Deserialize a tuple
 *
 *  @brief Deserialize a tuple
 *  @param ds - The stream to read
 *  @param t - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam Args - Type of the objects contained in the tuple
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename... Args>
DataStream& operator>>( DataStream& ds, std::tuple<Args...>& t ) {
   boost::fusion::for_each( t, [&]( auto& i ) {
       ds >> i;
   });
   return ds;
}

/**
 *  Serialize a class
 *
 *  @brief Serialize a class
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of class
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename T, std::enable_if_t<std::is_class<T>::value>* = nullptr>
DataStream& operator<<( DataStream& ds, const T& v ) {
   boost::pfr::for_each_field(v, [&](const auto& field) {
      ds << field;
   });
   return ds;
}

/**
 *  Deserialize a class
 *
 *  @brief Deserialize a class
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of class
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename T, std::enable_if_t<std::is_class<T>::value>* = nullptr>
DataStream& operator>>( DataStream& ds, T& v ) {
   boost::pfr::for_each_field(v, [&](auto& field) {
      ds >> field;
   });
   return ds;
}

/**
 *  Serialize a primitive type
 *
 *  @brief Serialize a primitive type
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the primitive type
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_primitive<T>()>* = nullptr>
DataStream& operator<<( DataStream& ds, const T& v ) {
   ds.write( (const char*)&v, sizeof(T) );
   return ds;
}

/**
 *  Deserialize a primitive type
 *
 *  @brief Deserialize a primitive type
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the primitive type
 *  @return DataStream& - Reference to the datastream
 */
template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_primitive<T>()>* = nullptr>
DataStream& operator>>( DataStream& ds, T& v ) {
   ds.read( (char*)&v, sizeof(T) );
   return ds;
}

/**
 * Unpack data inside a fixed size buffer as T
 *
 * @brief Unpack data inside a fixed size buffer as T
 * @tparam T - Type of the unpacked data
 * @param buffer - Pointer to the buffer
 * @param len - Length of the buffer
 * @return T - The unpacked data
 */
template<typename T>
T unpack( const char* buffer, size_t len ) {
   T result;
   datastream<const char*> ds(buffer,len);
   ds >> result;
   return result;
}

/**
 * Unpack data inside a variable size buffer as T
 *
 * @brief Unpack data inside a variable size buffer as T
 * @tparam T - Type of the unpacked data
 * @param bytes - Buffer
 * @return T - The unpacked data
 */
template<typename T>
T unpack( const vector<char>& bytes ) {
   return unpack<T>( bytes.data(), bytes.size() );
}

/**
 * Get the size of the packed data
 *
 * @brief Get the size of the packed data
 * @tparam T - Type of the data to be packed
 * @param value - Data to be packed
 * @return size_t - Size of the packed data
 */
template<typename T>
size_t pack_size( const T& value ) {
  datastream<size_t> ps;
  ps << value;
  return ps.tellp();
}

/**
 * Get packed data
 *
 * @brief Get packed data
 * @tparam T - Type of the data to be packed
 * @param value - Data to be packed
 * @return bytes - The packed data
 */
template<typename T>
bytes pack( const T& value ) {
  bytes result;
  result.resize(pack_size(value));

  datastream<char*> ds( result.data(), result.size() );
  ds << value;
  return result;
}

/**
 *  Serialize  a checksum160 type
 *
 *  @brief Serialize a checksum160 type
 *  @param ds - The stream to write
 *  @param cs - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const checksum160& cs) {
   ds.write((const char*)&cs.hash[0], sizeof(cs.hash));
   return ds;
}

/**
 *  Deserialize  a checksum160 type
 *
 *  @brief Deserialize  a checksum160 type
 *  @param ds - The stream to read
 *  @param cs - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, checksum160& cs) {
   ds.read((char*)&cs.hash[0], sizeof(cs.hash));
   return ds;
}

/**
 *  Serialize  a checksum512 type
 *
 *  @brief Serialize a checksum512 type
 *  @param ds - The stream to write
 *  @param cs - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const checksum512& cs) {
   ds.write((const char*)&cs.hash[0], sizeof(cs.hash));
   return ds;
}

/**
 *  Deserialize  a checksum512 type
 *
 *  @brief Deserialize  a checksum512 type
 *  @param ds - The stream to read
 *  @param cs - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, checksum512& cs) {
   ds.read((char*)&cs.hash[0], sizeof(cs.hash));
   return ds;
}
/// @} datastream
}
