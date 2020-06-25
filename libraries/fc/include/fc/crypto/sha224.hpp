#pragma once
#include <unordered_map>
#include <fc/fwd.hpp>
#include <fc/io/raw_fwd.hpp>
#include <fc/string.hpp>

namespace fc
{

class sha224 
{
  public:
    sha224();
    explicit sha224( const string& hex_str );

    string str()const;
    operator string()const;

    char*       data();
    const char* data()const;
    size_t data_size()const { return 224 / 8; }

    static sha224 hash( const char* d, uint32_t dlen );
    static sha224 hash( const string& );

    template<typename T>
    static sha224 hash( const T& t ) 
    { 
      sha224::encoder e; 
      fc::raw::pack(e,t);
      return e.result(); 
    } 

    class encoder 
    {
      public:
        encoder();
        ~encoder();

        void write( const char* d, uint32_t dlen );
        void put( char c ) { write( &c, 1 ); }
        void reset();
        sha224 result();

      private:
        struct      impl;
        fc::fwd<impl,112> my;
    };

    template<typename T>
    inline friend T& operator<<( T& ds, const sha224& ep ) {
      static_assert( sizeof(ep) == (8*3+4), "sha224 size mismatch" );
      ds.write( ep.data(), sizeof(ep) );
      return ds;
    }

    template<typename T>
    inline friend T& operator>>( T& ds, sha224& ep ) {
      ds.read( ep.data(), sizeof(ep) );
      return ds;
    }
    friend sha224 operator << ( const sha224& h1, uint32_t i       );
    friend bool   operator == ( const sha224& h1, const sha224& h2 );
    friend bool   operator != ( const sha224& h1, const sha224& h2 );
    friend sha224 operator ^  ( const sha224& h1, const sha224& h2 );
    friend bool   operator >= ( const sha224& h1, const sha224& h2 );
    friend bool   operator >  ( const sha224& h1, const sha224& h2 ); 
    friend bool   operator <  ( const sha224& h1, const sha224& h2 ); 
    friend std::size_t hash_value( const sha224& v ) { return uint64_t(v._hash[1])<<32 | v._hash[2]; }
                             
    uint32_t _hash[7]; 
};

  class variant;
  void to_variant( const sha224& bi, variant& v );
  void from_variant( const variant& v, sha224& bi );

} // fc
namespace std
{
    template<>
    struct hash<fc::sha224>
    {
       size_t operator()( const fc::sha224& s )const
       {
           return  *((size_t*)&s);
       }
    };
}
#include <fc/reflect/reflect.hpp>
FC_REFLECT_TYPENAME( fc::sha224 )
