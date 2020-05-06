#pragma once
#include <fc/string.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/io/raw_fwd.hpp>
#include <fc/crypto/city.hpp>
#include <fc/reflect/reflect.hpp>

namespace fc {

  namespace ip {
    class address {
      public:
        address( uint32_t _ip = 0 );
        address( const fc::string& s );

        address& operator=( const fc::string& s );
        operator fc::string()const;
        operator uint32_t()const;

        friend bool operator==( const address& a, const address& b );
        friend bool operator!=( const address& a, const address& b );

        /**
         *  @return true if the ip is in the following ranges:
         *
         *  10.0.0.0    to 10.255.255.255
         *  172.16.0.0  to 172.31.255.255
         *  192.168.0.0 to 192.168.255.255
         *  169.254.0.0 to 169.254.255.255
         *
         */
        bool is_private_address()const;
        /**
         *  224.0.0.0 to 239.255.255.255
         */
        bool is_multicast_address()const;

        /** !private & !multicast */
        bool is_public_address()const;
      private:
        uint32_t _ip;
    };
    
    class endpoint {
      public:
        endpoint();
        endpoint( const address& i, uint16_t p = 0);

        /** Converts "IP:PORT" to an endpoint */
        static endpoint from_string( const string& s );
        /** returns "IP:PORT" */
        operator string()const;

        void           set_port(uint16_t p ) { _port = p; }
        uint16_t       port()const;
        const address& get_address()const;

        friend bool operator==( const endpoint& a, const endpoint& b );
        friend bool operator!=( const endpoint& a, const endpoint& b );
        friend bool operator< ( const endpoint& a, const endpoint& b );
    
      private:
        /**
         *  The compiler pads endpoint to a full 8 bytes, so while
         *  a port number is limited in range to 16 bits, we specify
         *  a full 32 bits so that memcmp can be used with sizeof(), 
         *  otherwise 2 bytes will be 'random' and you do not know 
         *  where they are stored.
         */
        uint32_t _port; 
        address  _ip;
    };

  }
  class variant;
  void to_variant( const ip::endpoint& var,  variant& vo );
  void from_variant( const variant& var,  ip::endpoint& vo );

  void to_variant( const ip::address& var,  variant& vo );
  void from_variant( const variant& var,  ip::address& vo );


  namespace raw 
  {
    template<typename Stream> 
    inline void pack( Stream& s, const ip::address& v )
    {
       fc::raw::pack( s, uint32_t(v) );
    }
    template<typename Stream> 
    inline void unpack( Stream& s, ip::address& v )
    {
       uint32_t _ip;
       fc::raw::unpack( s, _ip );
       v = ip::address(_ip);
    }

    template<typename Stream> 
    inline void pack( Stream& s, const ip::endpoint& v )
    {
       fc::raw::pack( s, v.get_address() );
       fc::raw::pack( s, v.port() );
    }
    template<typename Stream> 
    inline void unpack( Stream& s, ip::endpoint& v )
    {
       ip::address a;
       uint16_t p;
       fc::raw::unpack( s, a );
       fc::raw::unpack( s, p );
       v = ip::endpoint(a,p);
    }

  }
} // namespace fc
FC_REFLECT_TYPENAME( fc::ip::address ) 
FC_REFLECT_TYPENAME( fc::ip::endpoint ) 
namespace std
{
    template<>
    struct hash<fc::ip::endpoint>
    {
       size_t operator()( const fc::ip::endpoint& e )const
       {
           return fc::city_hash_size_t( (char*)&e, sizeof(e) );
       }
    };
}
