/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once


/**
 * @defgroup varint Variable Length Integer
 * @ingroup types
 * @{
 */

struct unsigned_int {
    unsigned_int( uint32_t v = 0 ):value(v){}

    template<typename T>
    unsigned_int( T v ):value(v){}

    //operator uint32_t()const { return value; }
    //operator uint64_t()const { return value; }

    template<typename T>
    operator T()const { return static_cast<T>(value); }

    unsigned_int& operator=( int32_t v ) { value = v; return *this; }
    
    uint32_t value;

    friend bool operator==( const unsigned_int& i, const uint32_t& v )     { return i.value == v; }
    friend bool operator==( const uint32_t& i, const unsigned_int& v )     { return i       == v.value; }
    friend bool operator==( const unsigned_int& i, const unsigned_int& v ) { return i.value == v.value; }

    friend bool operator!=( const unsigned_int& i, const uint32_t& v )     { return i.value != v; }
    friend bool operator!=( const uint32_t& i, const unsigned_int& v )     { return i       != v.value; }
    friend bool operator!=( const unsigned_int& i, const unsigned_int& v ) { return i.value != v.value; }

    friend bool operator<( const unsigned_int& i, const uint32_t& v )      { return i.value < v; }
    friend bool operator<( const uint32_t& i, const unsigned_int& v )      { return i       < v.value; }
    friend bool operator<( const unsigned_int& i, const unsigned_int& v )  { return i.value < v.value; }

    friend bool operator>=( const unsigned_int& i, const uint32_t& v )     { return i.value >= v; }
    friend bool operator>=( const uint32_t& i, const unsigned_int& v )     { return i       >= v.value; }
    friend bool operator>=( const unsigned_int& i, const unsigned_int& v ) { return i.value >= v.value; }
    template<typename DataStream>
    friend DataStream& operator << ( DataStream& ds, const unsigned_int& v ){
       uint64_t val = v.value;
       do {
          uint8_t b = uint8_t(val) & 0x7f;
          val >>= 7;
          b |= ((val > 0) << 7);
          ds.write((char*)&b,1);//.put(b);
       } while( val );
       return ds;
    }

    template<typename DataStream>
    friend DataStream& operator >> ( DataStream& ds, unsigned_int& vi ){
      uint64_t v = 0; char b = 0; uint8_t by = 0;
      do {
         ds.get(b);
         v |= uint32_t(uint8_t(b) & 0x7f) << by;
         by += 7;
      } while( uint8_t(b) & 0x80 );
      vi.value = static_cast<uint32_t>(v);
      return ds;
    }
};

/**
 *  @brief serializes a 32 bit signed integer in as few bytes as possible
 *
 *  Uses the google protobuf algorithm for seralizing signed numbers
 */
struct signed_int {
    signed_int( int32_t v = 0 ):value(v){}
    operator int32_t()const { return value; }
    template<typename T>
    signed_int& operator=( const T& v ) { value = v; return *this; }
    signed_int operator++(int) { return value++; }
    signed_int& operator++(){ ++value; return *this; }

    int32_t value;

    friend bool operator==( const signed_int& i, const int32_t& v )    { return i.value == v; }
    friend bool operator==( const int32_t& i, const signed_int& v )    { return i       == v.value; }
    friend bool operator==( const signed_int& i, const signed_int& v ) { return i.value == v.value; }

    friend bool operator!=( const signed_int& i, const int32_t& v )    { return i.value != v; }
    friend bool operator!=( const int32_t& i, const signed_int& v )    { return i       != v.value; }
    friend bool operator!=( const signed_int& i, const signed_int& v ) { return i.value != v.value; }

    friend bool operator<( const signed_int& i, const int32_t& v )     { return i.value < v; }
    friend bool operator<( const int32_t& i, const signed_int& v )     { return i       < v.value; }
    friend bool operator<( const signed_int& i, const signed_int& v )  { return i.value < v.value; }

    friend bool operator>=( const signed_int& i, const int32_t& v )    { return i.value >= v; }
    friend bool operator>=( const int32_t& i, const signed_int& v )    { return i       >= v.value; }
    friend bool operator>=( const signed_int& i, const signed_int& v ) { return i.value >= v.value; }



    template<typename DataStream>
    friend DataStream& operator << ( DataStream& ds, const signed_int& v ){
      uint32_t val = (v.value<<1) ^ (v.value>>31);
      do {
         uint8_t b = uint8_t(val) & 0x7f;
         val >>= 7;
         b |= ((val > 0) << 7);
         ds.write((char*)&b,1);//.put(b);
      } while( val );
       return ds;
    }
    template<typename DataStream>
    friend DataStream& operator >> ( DataStream& ds, signed_int& vi ){
      uint32_t v = 0; char b = 0; int by = 0;
      do {
         ds.get(b);
         v |= uint32_t(uint8_t(b) & 0x7f) << by;
         by += 7;
      } while( uint8_t(b) & 0x80 );
      vi.value = ((v>>1) ^ (v>>31)) + (v&0x01);
      vi.value = v&0x01 ? vi.value : -vi.value;
      vi.value = -vi.value;
      return ds;
    }
};

/// @}
