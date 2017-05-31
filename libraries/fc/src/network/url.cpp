#include <fc/network/url.hpp>
#include <fc/string.hpp>
#include <fc/io/sstream.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>
#include <sstream>

namespace fc
{
  namespace detail
  {
    class url_impl
    {
      public:
         void parse( const fc::string& s )
         {
           std::stringstream ss(s);
           std::string skip,_lpath,_largs,luser,lpass;
           std::getline( ss, _proto, ':' );
           std::getline( ss, skip, '/' );
           std::getline( ss, skip, '/' );
           
           if( s.find('@') != size_t(fc::string::npos) ) {
             fc::string user_pass;
             std::getline( ss, user_pass, '@' );
             std::stringstream upss(user_pass);
             if( user_pass.find( ':' ) != size_t(fc::string::npos) ) {
                std::getline( upss, luser, ':' );
                std::getline( upss, lpass, ':' );
                _user = fc::move(luser);
                _pass = fc::move(lpass);
             } else {
                _user = fc::move(user_pass);
             }
           }
           fc::string host_port;
           std::getline( ss, host_port, '/' );
           auto pos = host_port.find( ':' );
           if( pos != fc::string::npos ) {
              try {
              _port = static_cast<uint16_t>(to_uint64( host_port.substr( pos+1 ) ));
              } catch ( ... ) {
                FC_THROW_EXCEPTION( parse_error_exception, "Unable to parse port field in url",( "url", s ) );
              }
              _host = host_port.substr(0,pos);
           } else {
              _host = fc::move(host_port);
           }
           std::getline( ss, _lpath, '?' );
#ifdef WIN32
           // On windows, a URL like file:///c:/autoexec.bat would result in _lpath = c:/autoexec.bat
           // which is what we really want (it's already an absolute path)
           if (!stricmp(_proto.c_str(), "file"))
              _path = _lpath;
           else
              _path = fc::path( "/" ) / _lpath; // let other schemes behave like unix
#else
           // On unix, a URL like file:///etc/rc.local would result in _lpath = etc/rc.local
           // but we really want to make it the absolute path /etc/rc.local
           _path = fc::path( "/" ) / _lpath;
#endif
           std::getline( ss, _largs );
           if( _args.valid() && _args->size() ) 
           {
             // TODO: args = fc::move(_args);
           }
         }

         string                    _proto; 
         ostring                   _host;
         ostring                   _user;
         ostring                   _pass;
         opath                     _path;
         ovariant_object           _args;
         fc::optional<uint16_t>    _port;
    };
  }

  void to_variant( const url& u, fc::variant& v )
  {
    v = fc::string(u);
  }
  void from_variant( const fc::variant& v, url& u )
  {
    u  = url( v.as_string() ); 
  }

  url::operator string()const
  {
      std::stringstream ss;
      ss<<my->_proto<<"://";
      if( my->_user.valid() ) {
        ss << *my->_user;
        if( my->_pass.valid() ) {
          ss<<":"<<*my->_pass;
        }
        ss<<"@";
      }
      if( my->_host.valid() ) ss<<*my->_host;
      if( my->_port.valid() ) ss<<":"<<*my->_port;
      if( my->_path.valid() ) ss<<my->_path->generic_string();
    //  if( my->_args ) ss<<"?"<<*my->_args;
      return ss.str();
  }

  url::url( const fc::string& u )
  :my( std::make_shared<detail::url_impl>() )
  {
    my->parse(u);
  }

  std::shared_ptr<detail::url_impl> get_null_url()
  {
    static auto u = std::make_shared<detail::url_impl>();
    return u; 
  }

  url::url()
  :my( get_null_url() )
  { }

  url::url( const url& u )
  :my(u.my){}

  url::url( url&& u )
  :my( fc::move(u.my) )
  {
    u.my = get_null_url();
  }

  url::url( const mutable_url& mu )
  :my( std::make_shared<detail::url_impl>(*mu.my) )
  {

  }
  url::url( mutable_url&& mu )
  :my( fc::move( mu.my ) )
  { }

  url::~url(){}

  url& url::operator=(const url& u )
  {
     my = u.my;
     return *this;
  }

  url& url::operator=(url&& u )
  {
     if( this != &u )
     {
        my = fc::move(u.my);
        u.my= get_null_url();
     }
     return *this;
  }
  url& url::operator=(const mutable_url& u )
  {
     my = std::make_shared<detail::url_impl>(*u.my);
     return *this;
  }
  url& url::operator=(mutable_url&& u )
  {
     my = fc::move(u.my);
     return *this;
  }

  string                    url::proto()const
  {
    return my->_proto;
  } 
  ostring                   url::host()const
  {
    return my->_host;
  }
  ostring                   url::user()const
  {
    return my->_user;
  }
  ostring                   url::pass()const
  {
    return my->_pass;
  }
  opath                     url::path()const
  {
    return my->_path;
  }
  ovariant_object           url::args()const
  {
    return my->_args;
  }
  fc::optional<uint16_t>    url::port()const
  {
    return my->_port;
  }



}

