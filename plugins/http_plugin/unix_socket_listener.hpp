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

#define ASYNC_WRITE 1
#define ASYNC_READ 1

namespace eosio {

  using boost::asio::local::stream_protocol;

  using local_stream = beast::basic_stream<
    stream_protocol,
  #if BOOST_VERSION < 107300
    asio::executor,
  #else
    asio::any_io_executor,
  #endif
    beast::unlimited_rate_policy>;

  class unix_socket_session 
    : public std::enable_shared_from_this<unix_socket_session>
    , public http_session_base 
    
  {    
    public:
      // The socket used to communicate with the client.
      // stream_protocol::socket socket_;
      local_stream stream_; 

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
          // Set the timeout.
          beast::get_lowest_layer(
              stream_).expires_after(std::chrono::seconds(30));

          // Read a request
#if ASYNC_READ
          http::async_read(
              stream_,
              buffer_,
              req_parser_,
              beast::bind_front_handler(
                  &unix_socket_session::on_read,
                  shared_from_this()));                  
#else
        http::response_serializer<http::string_body> sr{res_};
        auto bytes_read = http::read(stream_, buffer_, req_parser_, beast_ec_);
        on_read(beast_ec, bytes_transferred);
#endif 
      }

      void on_read(beast::error_code ec,
                    std::size_t bytes_transferred)
      {
          boost::ignore_unused(bytes_transferred);

          // This means they closed the connection
          if(ec == http::error::end_of_stream)
          // if(ec == asio::error::eof)
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
                    // boost::system::error_code ec,
                    beast::error_code ec,
                    std::size_t bytes_transferred)
      {
          boost::ignore_unused(bytes_transferred);

          if(ec) {
              // beast_ec_ = ec;
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
          stream_.socket().shutdown(stream_protocol::socket::shutdown_send, ec);

          // At this point the connection is closed gracefully
      }

      bool is_secure() override { return false; };

    public:
      unix_socket_session(std::shared_ptr<http_plugin_state> plugin_state, 
                          asio::io_context* ioc, 
                          stream_protocol::socket sock)
        : http_session_base(plugin_state, ioc)
        //, socket_(std::move(sock)) 
        , stream_(std::move(sock)) 
      { 
      }

      virtual ~unix_socket_session() = default;

      void run() {
        // do_read();

        // Set the timeout.
        // beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        
        asio::dispatch(stream_.get_executor(),
                    beast::bind_front_handler(
                        &unix_socket_session::do_read,
                        shared_from_this()));
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

#if ASYNC_WRITE
        http::async_write(
            stream_,
            res_,
            beast::bind_front_handler(
                 &unix_socket_session::on_write,
                 shared_from_this(),
                 close)
        );
#else
        http::response_serializer<http::string_body> sr{res_};
        auto bytes_written = http::write(stream_, sr, beast_ec_);
        on_write(close, beast_ec_, bytes_written);
#endif 
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
        , acceptor_(asio::make_strand(*ioc))
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
            //throw boost::system::system_error(ec, "stream_protocol::acceptor::bind()");
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
        // The new connection gets its own strand
        acceptor_.async_accept(
            asio::make_strand(*ioc_),
            beast::bind_front_handler(
                &unix_socket_listener::on_accept,
                this->shared_from_this()));
      }

      void on_accept(beast::error_code ec, stream_protocol::socket socket) {
          if(ec) {
              auto errCodeStr = std::to_string(ec.value());
              fc_elog( logger, "unix_socket_listener::on_accept() error code ${ec}", ("ec", errCodeStr));
          }
          else {
              // Create the session object and run it
              std::make_shared<unix_socket_session>(plugin_state_, ioc_, std::move(socket))->run();       
          }

          // Accept another connection
          do_accept();
      }         
  }; // end class unix_socket_lixtener
    

} // end namespace eosio