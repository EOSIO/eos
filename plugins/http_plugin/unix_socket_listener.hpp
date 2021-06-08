#pragma once

#include "common.hpp"
#include "http_session_base.hpp"

#include <boost/asio/detail/config.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_socket_iostream.hpp>
#include <boost/asio/basic_stream_socket.hpp>

#include <string>

namespace eosio {

  using boost::asio::local::stream_protocol;

  class unix_socket_session 
    : public std::enable_shared_from_this<unix_socket_session>
    , public http_session_base
  {    
    protected:
      // The socket used to communicate with the client.
      stream_protocol::socket socket_;

      // Buffer used to store data received from the client.
      std::array<char, 4096> in_data_;

      virtual bool allow_host(const http::request<http::string_body>& req) override { 
          // TODO 
          return true;
      }

      void do_read() {
        auto self(shared_from_this());

        std::function<void(boost::system::error_code,std::size_t)> read_until_done;
        std::size_t bytes_read = 0;

        read_until_done = [this, self, &bytes_read, &read_until_done](boost::system::error_code ec, std::size_t length) {
          if (ec) {
            auto errCodeStr = std::to_string(ec.value());
            fc_elog( logger, "unix_socket_session::do_read() error code ${ec}", ("ec", errCodeStr));
            return;
          }
          bytes_read += length;
          if(bytes_read > plugin_state_->max_body_size) {
            fc_elog( logger, "unix_socket_session::do_read() max_body_size ${mb} exceeded", 
                    ("mb", plugin_state_->max_body_size));
            return;
            // do_write(length);
          }

          beast::error_code parse_ec;
          req_parser_.put(boost::asio::buffer(in_data_), parse_ec);

          // we have successfully parsed the message
          if(!parse_ec) {
              // get serialized request object
              auto req = req_parser_.get();

              // handle the HTTP request
              handle_request(std::move(req));
              return;
          }

          // error occured
          else if (parse_ec != http::error::need_more) { 
            auto errCodeStr = std::to_string(parse_ec.value());
            fc_elog( logger, "unix_socket_listene::do_read exception parsing HTTP request error code ${ec}", ("ec", errCodeStr));
            return;
          }

          // error code is http::error::need_more, so keep reading data
          socket_.async_read_some(boost::asio::buffer(in_data_), read_until_done);
        };

        socket_.async_read_some(boost::asio::buffer(in_data_), read_until_done);
      }

      void do_write() {
        // initialize the serializer with response object
        http::response_serializer<http::string_body> sr{res_};

        beast::error_code ec;        
        boost::system::error_code write_ec;

        // keep calling sr.next() until all data is written or error occured
        do {
            sr.next(ec,
                [&sr, &write_ec, this](beast::error_code& ec, auto const& buffer)
                {
                    ec = {};
                    
                    // use a synchronous write because we want to be sure 
                    // buffer is used before calling sr.next()
                    asio::write(socket_, buffer, write_ec);                    

                    sr.consume(buffer_size(buffer));
                });
        }
        while(! ec && ! sr.is_done() && !write_ec);
        if(ec) {
            auto errCodeStr = std::to_string(ec.value());
            fc_elog( logger, "unix_socket_listene::do_write() exception serializing HTTP response error code ${ec}", ("ec", errCodeStr));
        }            
        else if (write_ec) {
            auto errCodeStr = std::to_string(write_ec.value());
            fc_elog( logger, "unix_socket_listene::do_write() exception writing HTTP response error code ${ec}", ("ec", errCodeStr));
        }
        else if (!sr.is_done()) {
            fc_elog( logger, "unix_socket_listene::do_write() error serializer.is_done() returned false");
        }

        // TODO close the connectino or read another request if keep-alive
      }

      bool is_secure() override { return false; };

    public:
      unix_socket_session(std::shared_ptr<http_plugin_state> plugin_state, 
                          asio::io_context* ioc, 
                          stream_protocol::socket sock)
        : http_session_base(plugin_state, ioc)
        , socket_(std::move(sock)) 
      { 
      }

      virtual ~unix_socket_session() = default;

      void run() {
        do_read();
      }
      
      virtual void handle_exception() override {
        fc_elog( logger, "unix_socket_session: exception occured");
      }

     virtual void send_response(std::optional<std::string> body, int code) override {
        res_.result(code);
        if(body.has_value())
            res_.body() = *body;        

        res_.prepare_payload();

        do_write();
      }

      virtual shared_ptr<detail::abstract_conn> get_shared_from_this() override {
        return shared_from_this();
      }
  }; // end class unix_socket_session

  class unix_socket_listener
  {
    private:
      std::shared_ptr<http_plugin_state> plugin_state_;
      asio::io_context* ioc_;
      stream_protocol::acceptor acceptor_;

    public:    
      unix_socket_listener(const unix_socket_listener&) = delete;
      unix_socket_listener(unix_socket_listener&&) = delete;

      unix_socket_listener& operator=(const unix_socket_listener&) = delete;
      unix_socket_listener& operator=(unix_socket_listener&&) = delete;

      unix_socket_listener(asio::io_context* ioc, 
                           stream_protocol::endpoint endpoint,
                           std::shared_ptr<http_plugin_state> plugin_state)
        : plugin_state_(plugin_state)
        , ioc_(ioc)
        , acceptor_(*ioc, endpoint)
        //, acceptor_(asio::make_strand(*ioc))
      { do_accept(); }

      virtual ~unix_socket_listener() = default;
      

    private:
      void do_accept()
      {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, stream_protocol::socket socket)
            {
              if (!ec) {
                std::make_shared<unix_socket_session>(plugin_state_, ioc_, std::move(socket))->run();
              }
              do_accept();
            });
      }      
  }; // end class unix_socket_lixtener
    

} // end namespace eosio