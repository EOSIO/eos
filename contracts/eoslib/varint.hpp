/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

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
};

/**
 *  @brief serializes a 32 bit signed interger in as few bytes as possible
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
};
