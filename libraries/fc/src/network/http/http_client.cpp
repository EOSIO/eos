#include <fc/network/http/http_client.hpp>
#include <fc/io/json.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>

namespace fc {

class http_client_impl {
public:
   using host_key = std::tuple<std::string, std::string, uint16_t>;
   using raw_socket_ptr = std::unique_ptr<tcp::socket>;
   using ssl_socket_ptr = std::unique_ptr<ssl::stream<tcp::socket>>;
   using connection = static_variant<raw_socket_ptr, ssl_socket_ptr>;
   using connection_map = std::map<host_key, connection>;


   http_client_impl()
   :_ioc()
   ,_sslc(ssl::context::sslv23_client)
   ,_resolver(_ioc)
   {
   }

   void add_cert(const std::string& cert_pem_string) {
      boost::system::error_code ec;
      _sslc.add_certificate_authority(boost::asio::buffer(cert_pem_string.data(), cert_pem_string.size()), ec);
      FC_ASSERT(!ec, "Failed to add cert: ${msg}", ("msg", ec.message()));
   }

   host_key url_to_host_key( const url& dest ) {
      FC_ASSERT(dest.host(), "Provided URL has no host");
      uint16_t port = 80;
      if (dest.port()) {
         port = *dest.port();
      }

      return std::make_tuple(dest.proto(), *dest.host(), port);
   }

   connection_map::iterator create_raw_connection( const url& dest ) {
      auto key = url_to_host_key(dest);
      auto socket = std::make_unique<tcp::socket>(_ioc);
      const auto results = _resolver.resolve(*dest.host(), dest.port() ? std::to_string(*dest.port()) : "80");

      // Make the connection on the IP address we get from a lookup
      boost::asio::connect(*socket, results.begin(), results.end());

      auto res = _connections.emplace(std::piecewise_construct,
                           std::forward_as_tuple(key),
                           std::forward_as_tuple(std::move(socket)));

      return res.first;
   }

   connection_map::iterator create_ssl_connection( const url& dest ) {
      auto key = url_to_host_key(dest);
      auto ssl_socket = std::make_unique<ssl::stream<tcp::socket>>(_ioc, _sslc);

      // Set SNI Hostname (many hosts need this to handshake successfully)
      if(!SSL_set_tlsext_host_name(ssl_socket->native_handle(), dest.host()->c_str()))
      {
         boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
         FC_THROW("Unable to set SNI Host Name: ${msg}", ("msg", ec.message()));
      }

      const auto results = _resolver.resolve(*dest.host(), dest.port() ? std::to_string(*dest.port()) : "443");

      // Make the connection on the IP address we get from a lookup
      boost::asio::connect(ssl_socket->next_layer(), results.begin(), results.end());
      ssl_socket->handshake(ssl::stream_base::client);

      auto res = _connections.emplace(std::piecewise_construct,
                                      std::forward_as_tuple(key),
                                      std::forward_as_tuple(std::move(ssl_socket)));

      return res.first;
   }

   connection_map::iterator create_connection( const url& dest ) {
      if (dest.proto() == "http") {
         return create_raw_connection(dest);
      } else if (dest.proto() == "https") {
         return create_ssl_connection(dest);
      } else {
         FC_THROW("Unknown protocol ${proto}", ("proto", dest.proto()));
      }
   }

   struct check_closed_visitor : public visitor<bool> {
      bool operator() ( const raw_socket_ptr& ptr ) const {
         return !ptr->is_open();
      }

      bool operator() ( const ssl_socket_ptr& ptr ) const {
         return !ptr->lowest_layer().is_open();
      }
   };

   bool check_closed( const connection_map::iterator& conn_itr ) {
      if (conn_itr->second.visit(check_closed_visitor())) {
         _connections.erase(conn_itr);
         return true;
      } else {
         return false;
      }
   }

   connection_map::iterator get_connection( const url& dest ) {
      auto key = url_to_host_key(dest);
      auto conn_itr = _connections.find(key);
      if (conn_itr == _connections.end() || check_closed(conn_itr)) {
         return create_connection(dest);
      } else {
         return conn_itr;
      }
   }

   struct write_request_visitor : visitor<boost::beast::error_code> {
      write_request_visitor(http::request<http::string_body>& req)
      :req(req)
      {}

      template<typename S>
      boost::beast::error_code operator() ( S& stream ) const {
         boost::beast::error_code ec;
         http::write(*stream, req, ec);
         return ec;
      }

      http::request<http::string_body>& req;
   };

   struct read_response_visitor : visitor<boost::beast::error_code> {
      read_response_visitor(boost::beast::flat_buffer& buffer, http::response<http::string_body>& res)
      :buffer(buffer)
      ,res(res)
      {}

      template<typename S>
      boost::beast::error_code operator() ( S& stream ) const {
         boost::beast::error_code ec;
         http::read(*stream, buffer, res, ec);
         return ec;
      }

      boost::beast::flat_buffer& buffer;
      http::response<http::string_body>& res;
   };

   variant post_sync(const url& dest, const variant& payload) {
      string path = dest.path() ? dest.path()->generic_string() : "/";
      if (dest.query()) {
         path = path + "?" + *dest.query();
      }

      http::request<http::string_body> req{http::verb::post, dest.path()->generic_string(), 11};
      req.set(http::field::host, *dest.host());
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      req.set(http::field::content_type, "application/json");
      req.keep_alive(true);
      req.body() = json::to_string(payload);
      req.prepare_payload();

      auto conn_iter = get_connection(dest);
      // Send the HTTP request to the remote host
      boost::beast::error_code ec = conn_iter->second.visit(write_request_visitor(req));
      FC_ASSERT(!ec, "Failed to send request: ${message}", ("message",ec.message()));

      // This buffer is used for reading and must be persisted
      boost::beast::flat_buffer buffer;

      // Declare a container to hold the response
      http::response<http::string_body> res;

      // Receive the HTTP response
      ec = conn_iter->second.visit(read_response_visitor(buffer, res));
      FC_ASSERT(!ec, "Failed to read response: ${message}", ("message",ec.message()));

      if (!res.keep_alive()) {
         // close the connection
         _connections.erase(conn_iter);
      }

      auto result = json::from_string(res.body());
      if (res.result() == http::status::internal_server_error) {
         fc::exception_ptr excp;
         try {
            auto err_var = result.get_object()["error"].get_object();
            excp = std::make_shared<fc::exception>(err_var["code"].as_int64(), err_var["name"].as_string(), err_var["what"].as_string());

            if (err_var.contains("details")) {
               auto details = err_var["details"].get_array();
               for (const auto dvar : details) {
                  excp->append_log(FC_LOG_MESSAGE(error, dvar.get_object()["message"].as_string()));
               }
            }
         } catch( ... ) {

         }

         if (excp) {
            throw *excp;
         } else {
            FC_THROW("Request failed with 500 response, but response was not parseable");
         }
      } else if (res.result() == http::status::not_found) {
         FC_THROW("URL not found: ${url}", ("url", (std::string)dest));
      }

      return result;
   }

   microseconds _timeout;  /// the time out for connect operations



   boost::asio::io_context  _ioc;
   ssl::context             _sslc;
   tcp::resolver            _resolver;
   connection_map           _connections;
};


http_client::http_client()
:_my(new http_client_impl())
{

}

variant http_client::post_sync(const url& dest, const variant& payload) {
   return _my->post_sync(dest, payload);
}

void http_client::set_timeout(microseconds timeout) {
   _my->_timeout = timeout;
}

void http_client::add_cert(const std::string& cert_pem_string) {
   _my->add_cert(cert_pem_string);
}

http_client::~http_client() {

}

}