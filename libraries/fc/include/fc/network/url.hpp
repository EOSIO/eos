#pragma once
#include <fc/string.hpp>
#include <stdint.h>
#include <fc/filesystem.hpp>
#include <fc/variant_object.hpp>
#include <memory>

namespace fc {

  typedef std::optional<fc::string>           ostring;
  typedef std::optional<fc::path>             opath;
  typedef std::optional<fc::variant_object>   ovariant_object;

  namespace detail { class url_impl; }

  class mutable_url;
  
  /**
   *  Used to pass an immutable URL and
   *  query its parts.
   */
  class url 
  {
    public:
      url();
      explicit url( const string& u );
      url( const url& c );
      url( url&& c );
      url( const string& proto, const ostring& host, const ostring& user, const ostring& pass,
           const opath& path, const ostring& query, const ovariant_object& args, const std::optional<uint16_t>& port);
      ~url();
      
      url& operator=( const url& c );
      url& operator=( url&& c );

      url& operator=( const mutable_url& c );
      url& operator=( mutable_url&& c );
      
      bool operator==( const url& cmp )const;
      
      operator string()const;
      
      //// file, ssh, tcp, http, ssl, etc...
      string                    proto()const; 
      ostring                   host()const;
      ostring                   user()const;
      ostring                   pass()const;
      opath                     path()const;
      ostring                   query()const;
      ovariant_object           args()const;
      std::optional<uint16_t>   port()const;

    private:
      friend class mutable_url;
      std::shared_ptr<detail::url_impl> my;
  };

  void to_variant( const url& u, fc::variant& v );
  void from_variant( const fc::variant& v, url& u );

} // namespace fc

