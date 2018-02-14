/**
 *  @file db.h
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/system.h>
#include <eosiolib/memory.h>
#include <eosiolib/vector.hpp>
#include <eosiolib/varint.hpp>
#include <string>


namespace eosio {
/**
 *  @brief A data stream for reading and writing data in the form of bytes
 */
template<typename T>
class datastream {
   public:
      datastream( T start, size_t s )
      :_start(start),_pos(start),_end(start+s){};
      
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
        memcpy( _pos, d, s );
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
      inline size_t tellp()const      { return _pos - _start; }
      
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

/**
 *  Serialize a uint256 into a stream
 *  @brief Serialize a uint256
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint256 d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint256 from a stream
 *  @brief Deserialize a uint256
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint256& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a uint128_t into a stream
 *  @brief Serialize a uint128_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint128_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint128_t from a stream
 *  @brief Deserialize a uint128_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint128_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a int128_t into a stream
 *  @brief Serialize a int128_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const int128_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a int128_t from a stream
 *  @brief Deserialize a int128_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, int128_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a int32_t into a stream
 *  @brief Serialize a int32_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const int32_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a int32_t from a stream
 *  @brief Deserialize a int32_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, int32_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a uint32_t into a stream
 *  @brief Serialize a uint32_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint32_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint32_t from a stream
 *  @brief Deserialize a uint32_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint32_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a int64_t into a stream
 *  @brief Serialize a int64_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const int64_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a int64_t from a stream
 *  @brief Deserialize a int64_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, int64_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a uint64_t into a stream
 *  @brief Serialize a uint64_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint64_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint64_t from a stream
 *  @brief Deserialize a uint64_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint64_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a int16_t into a stream
 *  @brief Serialize a int16_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const int16_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a int16_t from a stream
 *  @brief Deserialize a int16_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, int16_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a uint16_t into a stream
 *  @brief Serialize a uint16_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint16_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint16_t from a stream
 *  @brief Deserialize a uint16_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint16_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a int8_t into a stream
 *  @brief Serialize a int8_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const int8_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a int8_t from a stream
 *  @brief Deserialize a int8_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, int8_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a uint8_t into a stream
 *  @brief Serialize a uint8_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint8_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint8_t from a stream
 *  @brief Deserialize a uint8_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint8_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

template<typename DataStream>
DataStream& operator << ( DataStream& ds, const std::string& v ) {
   ds << unsigned_int( v.size() );
   for( const auto& i : v )
      ds << i;
   return ds;
}

template<typename DataStream>
DataStream& operator >> ( DataStream& ds, std::string& v ) {
   unsigned_int s;
   ds >> s;
   v.resize(s.value);
   for( auto& i : v )
      ds >> i;
   return ds;
}

template<typename DataStream, typename T>
DataStream& operator << ( DataStream& ds, const vector<T>& v ) {
   ds << unsigned_int( v.size() );
   for( const auto& i : v )
      ds << i;
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

template<typename T>
T unpack( const char* buffer, size_t len ) {
   T result;
   datastream<const char*> ds(buffer,len);
   ds >> result;
   return result;
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

}

