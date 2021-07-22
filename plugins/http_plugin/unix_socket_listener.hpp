#pragma once

// listener and session classes for HTTP requests over POSIX sockets

#include "common.hpp"
#include "http_session_base.hpp"

#include <boost/asio/detail/config.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_socket_iostream.hpp>
#include <boost/asio/basic_stream_socket.hpp>

#include <string>
#include <sstream>


namespace eosio {
  using boost::asio::local::stream_protocol;
  
  
#if BOOST_VERSION < 107000
  using local_stream = boost::asio::basic_stream_socket<stream_protocol>;
#else 
  #if BOOST_VERSION < 107300
    using local_stream = beast::basic_stream<
      stream_protocol,
      asio::executor,
      beast::unlimited_rate_policy>;
  #else      
  using local_stream = beast::basic_stream<
      stream_protocol,
      asio::any_io_executor,
      beast::unlimited_rate_policy>;    
  #endif    
#endif

  class unix_socket_session 
    : public std::enable_shared_from_this<unix_socket_session>
    , public http_session_base 
    
  {    
    public:
      // The socket used to communicate with the client.
      stream_protocol::socket socket_;
#if BOOST_VERSION < 107000      
      boost::asio::strand<boost::asio::io_context::executor_type> strand_;
#endif      

      // Buffer used to store data received from the client.
      beast::flat_buffer buffer_;
      beast::error_code beast_ec_;

    public:
      virtual bool allow_host(const http::request<http::string_body>& req) override { 
          // TODO 
          return true;
      }

      void do_read()
      {
          // Read a request
        http::response_serializer<http::string_body> sr{res_};
        auto bytes_read = http::read(socket_, buffer_, req_parser_, beast_ec_);
        on_read(beast_ec_, bytes_read);
      }

      void on_read(beast::error_code ec,
                    std::size_t bytes_transferred)
      {
          boost::ignore_unused(bytes_transferred);

          // This means they closed the connection
          if(ec == http::error::end_of_stream)
              return do_eof();

          if(ec) {
              auto errCodeStr = std::to_string(ec.value());
              fc_elog( logger, "unix_socket_session::on_read() failed error code ${ec}", ("ec", errCodeStr));
              
              return;
          }

          // Hande the parsed request
          auto req = req_parser_.get();
          handle_request(std::move(req));
      }

      void on_write(bool close,
                    beast::error_code ec,
                    std::size_t bytes_transferred)
      {
          boost::ignore_unused(bytes_transferred);

          if(ec) {
              fc_elog( logger, "unix_socket_session::on_write() failed");
              handle_exception();
              return;
          }

          if(close)
          {
              // This means we should close the connection, usually because
              // the response indicated the "Connection: close" semantic.
              return do_eof();
          }

          // Read another request if connection has not been closed
          do_read();
      }

      void do_eof()
      {
          // Send a TCP shutdown
          boost::system::error_code ec;
          socket_.shutdown(stream_protocol::socket::shutdown_send, ec);
          // At this point the connection is closed gracefully
      }

      bool is_secure() override { return false; };

    public:
      unix_socket_session(std::shared_ptr<http_plugin_state> plugin_state, 
                          asio::io_context* ioc, 
                          stream_protocol::socket sock)
        : http_session_base(plugin_state, ioc)
        , socket_(std::move(sock)) 
#if BOOST_VERSION < 107000        
        , strand_(socket_.get_executor()) 
#endif        
      {  }

      virtual ~unix_socket_session() = default;

      void run() {
          do_read();
      }
      
      virtual void handle_exception() override {
          fc_elog( logger, "unix_socket_session: exception occured");
      }

     virtual void send_response(std::optional<std::string> body, int code) override {
          // Determine if we should close the connection after
          bool close = !(plugin_state_->keep_alive) || res_.need_eof();

          res_.result(code);
          if(body.has_value()) {
              res_.body() = *body;        
          }
          else { 
              fc_ilog( logger, "unix_socket_session: send_response() body has no value");
          }

          res_.prepare_payload();

          http::response_serializer<http::string_body> sr{res_};
          auto bytes_written = http::write(socket_, sr, beast_ec_);
          on_write(close, beast_ec_, bytes_written);
      }

      virtual shared_ptr<detail::abstract_conn> get_shared_from_this() override {
          return shared_from_this();
      }
  }; // end class unix_socket_session

  class unix_socket_listener  : public std::enable_shared_from_this<unix_socket_listener>
  {
    private:
      std::shared_ptr<http_plugin_state> plugin_state_;
      asio::io_context* ioc_;
      stream_protocol::acceptor acceptor_;
      local_stream socket_;

      bool isListening_;

    public:    
      unix_socket_listener() = delete;
      unix_socket_listener(const unix_socket_listener&) = delete;
      unix_socket_listener(unix_socket_listener&&) = delete;

      unix_socket_listener& operator=(const unix_socket_listener&) = delete;
      unix_socket_listener& operator=(unix_socket_listener&&) = delete;

      unix_socket_listener(asio::io_context* ioc, 
                           std::shared_ptr<http_plugin_state> plugin_state)
        : plugin_state_(plugin_state)
        , ioc_(ioc)
        , acceptor_(*ioc)
        , socket_(*ioc)
        , isListening_(false)
      { }

      virtual ~unix_socket_listener() = default; 

      void listen(stream_protocol::endpoint endpoint) {
          if (isListening_) return;

          // delete the old socket
          auto ss = std::stringstream();
          ss << endpoint;
          ::unlink(ss.str().c_str());

          boost::system::error_code ec;
          // Open the acceptor
          acceptor_.open(endpoint.protocol(), ec);
          if(ec)  {
              auto errCodeStr = std::to_string(ec.value());
              fc_elog(logger, "unix_socket_listener::listen() failed to open endpoint error code ${ec}", ("ec", errCodeStr));
              throw boost::system::system_error(ec, "stream_protocol::acceptor::open()");
          }

          // Allow address reuse
          acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
          if(ec)  {
              auto errCodeStr = std::to_string(ec.value());
              fc_elog(logger, "unix_socket_listener::listen() failed to set_option 'reuse_address' error code ${ec}", ("ec", errCodeStr));
              throw boost::system::system_error(ec, "stream_protocol::acceptor::set_option()");
          }

          // Bind to the server address
          acceptor_.bind(endpoint, ec);
          if(ec)  {
              auto errCodeStr = std::to_string(ec.value());
              fc_elog(logger, "unix_socket_listener::listen() failed to bind to endpoint  error code ${ec}", ("ec", errCodeStr));
          }

          // Start listening for connections
          ec = {};
          auto maxConnections = asio::socket_base::max_listen_connections;
          fc_ilog( logger, "acceptor_.listen()" ); 
          acceptor_.listen(maxConnections, ec);
          if(ec) {
              auto errCodeStr = std::to_string(ec.value());
              fc_elog(logger, "unix_socket_listener::listen() acceptor_.listen() failed  error code ${ec}", ("ec", errCodeStr));
              throw boost::system::system_error(ec, "stream_protocol::acceptor::listen()");
          }
          isListening_ = true; 
      }

      void start_accept() {
        if (!isListening_) return;

        do_accept();
      }
      

    private:
      void do_accept()
      {
        
#if BOOST_VERSION < 107000        
        acceptor_.async_accept(
          socket_,
          std::bind(
            &unix_socket_listener::on_accept,
            this->shared_from_this(),
            std::placeholders::_1
          )
        );
#else
        // The new connection gets its own strand
        acceptor_.async_accept(
          asio::make_strand(*ioc_),
          beast::bind_front_handler(
              &unix_socket_listener::on_accept,
              this->shared_from_this()
          )
        );
#endif
      }

#if BOOST_VERSION < 107000        
      void on_accept(beast::error_code ec) {
#else
      void on_accept(beast::error_code ec, stream_protocol::socket socket) {
#endif
          if(ec) {
              auto errCodeStr = std::to_string(ec.value());
              fc_elog( logger, "unix_socket_listener::on_accept() error code ${ec}", ("ec", errCodeStr));
          }
          else {
              // Create the session object and run it
#if BOOST_VERSION < 107000       
              std::make_shared<unix_socket_session>(plugin_state_, ioc_, std::move(socket_))->run();         
#else               
              std::make_shared<unix_socket_session>(plugin_state_, ioc_, std::move(socket))->run();         
#endif                              
              
          }

          // Accept another connection
          do_accept();
      }         
  }; // end class unix_socket_lixtener
    

} // end namespace eosio