#include <fc/network/ip.hpp>
#include <fc/variant.hpp>
#include <fc/exception/exception.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <string>

namespace fc { namespace ip {

  address::address( uint32_t ip )
  :_ip(ip){}

  address::address( const fc::string& s ) 
  {
    try
    {
      _ip = boost::asio::ip::address_v4::from_string(s.c_str()).to_ulong();
    }
    FC_RETHROW_EXCEPTIONS(error, "Error parsing IP address ${address}", ("address", s))
  }

  bool operator==( const address& a, const address& b ) {
    return uint32_t(a) == uint32_t(b);
  }
  bool operator!=( const address& a, const address& b ) {
    return uint32_t(a) != uint32_t(b);
  }

  address& address::operator=( const fc::string& s ) 
  {
    try
    {
      _ip = boost::asio::ip::address_v4::from_string(s.c_str()).to_ulong();
    }
    FC_RETHROW_EXCEPTIONS(error, "Error parsing IP address ${address}", ("address", s))
    return *this;
  }

  address::operator fc::string()const 
  {
    try
    {
      return boost::asio::ip::address_v4(_ip).to_string().c_str();
    }
    FC_RETHROW_EXCEPTIONS(error, "Error parsing IP address to string")
  }
  address::operator uint32_t()const {
    return _ip;
  }


  endpoint::endpoint()
  :_port(0){  }
  endpoint::endpoint(const address& a, uint16_t p)
  :_port(p),_ip(a){}

  bool operator==( const endpoint& a, const endpoint& b ) {
    return a._port == b._port  && a._ip == b._ip;
  }
  bool operator!=( const endpoint& a, const endpoint& b ) {
    return a._port != b._port || a._ip != b._ip;
  }

  bool operator< ( const endpoint& a, const endpoint& b )
  {
     return  uint32_t(a.get_address()) < uint32_t(b.get_address()) ||
             (uint32_t(a.get_address()) == uint32_t(b.get_address()) &&
              uint32_t(a.port()) < uint32_t(b.port()));
  }

  uint16_t       endpoint::port()const    { return _port; }
  const address& endpoint::get_address()const { return _ip;   }

  endpoint endpoint::from_string( const string& endpoint_string )
  {
    try
    {
      endpoint ep;
      auto pos = endpoint_string.find(':');
      ep._ip   = boost::asio::ip::address_v4::from_string(endpoint_string.substr( 0, pos ) ).to_ulong();
      ep._port = boost::lexical_cast<uint16_t>( endpoint_string.substr( pos+1, endpoint_string.size() ) );
      return ep;
    }
    FC_RETHROW_EXCEPTIONS(warn, "error converting string to IP endpoint")
  }

  endpoint::operator string()const 
  {
    try
    {
      return string(_ip) + ':' + fc::string(boost::lexical_cast<std::string>(_port).c_str());
    }
    FC_RETHROW_EXCEPTIONS(warn, "error converting IP endpoint to string")
  }

  /**
   *  @return true if the ip is in the following ranges:
   *
   *  10.0.0.0    to 10.255.255.255
   *  172.16.0.0  to 172.31.255.255
   *  192.168.0.0 to 192.168.255.255
   *  169.254.0.0 to 169.254.255.255
   *
   */
  bool address::is_private_address()const
  {
    static address min10_ip("10.0.0.0");
    static address max10_ip("10.255.255.255");
    static address min172_ip("172.16.0.0");
    static address max172_ip("172.31.255.255");
    static address min192_ip("192.168.0.0");
    static address max192_ip("192.168.255.255");
    static address min169_ip("169.254.0.0");
    static address max169_ip("169.254.255.255");
    if( _ip >= min10_ip._ip && _ip <= max10_ip._ip ) return true;
    if( _ip >= min172_ip._ip && _ip <= max172_ip._ip ) return true;
    if( _ip >= min192_ip._ip && _ip <= max192_ip._ip ) return true;
    if( _ip >= min169_ip._ip && _ip <= max169_ip._ip ) return true;
    return false;
  }

  /**
   *  224.0.0.0 to 239.255.255.255
   */
  bool address::is_multicast_address()const
  {
    static address min_ip("224.0.0.0");
    static address max_ip("239.255.255.255");
    return  _ip >= min_ip._ip  && _ip <= max_ip._ip;
  }

  /** !private & !multicast */
  bool address::is_public_address()const
  {
    return !( is_private_address() || is_multicast_address() );
  }

}  // namespace ip

  void to_variant( const ip::endpoint& var,  variant& vo )
  {
      vo = fc::string(var);
  }
  void from_variant( const variant& var,  ip::endpoint& vo )
  {
     vo = ip::endpoint::from_string(var.as_string());
  }

  void to_variant( const ip::address& var,  variant& vo )
  {
    vo = fc::string(var);
  }
  void from_variant( const variant& var,  ip::address& vo )
  {
    vo = ip::address(var.as_string());
  }

} 
