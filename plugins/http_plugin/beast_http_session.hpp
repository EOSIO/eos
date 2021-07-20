#pragma once

#include "common.hpp"
#include "http_session_base.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <sstream>

namespace eosio { 

    //------------------------------------------------------------------------------
    // Report a failure
    void fail(beast::error_code ec, char const* what)
    {
        // ssl::error::stream_truncated, also known as an SSL "short read",
        // indicates the peer closed the connection without performing the
        // required closing handshake (for example, Google does this to
        // improve performance). Generally this can be a security issue,
        // but if your communication protocol is self-terminated (as
        // it is with both HTTP and WebSocket) then you may simply
        // ignore the lack of close_notify.
        //
        // https://github.com/boostorg/beast/issues/38
        //
        // https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
        //
        // When a short read would cut off the end of an HTTP message,
        // Beast returns the error beast::http::error::partial_message.
        // Therefore, if we see a short read here, it has occurred
        // after the message has been completed, so it is safe to ignore it.

        if(ec == asio::ssl::error::stream_truncated)
            return;

        std::cerr << what << ": " << ec.message() << "\n";
    }

    // Handle HTTP conneciton using boost::beast for TCP communication
    // This uses the Curiously Recurring Template Pattern so that
    // the same code works with both SSL streams and regular sockets.
    template<class Derived> 
    class beast_http_session : public http_session_base
    {
        protected:
            beast::flat_buffer buffer_;
            beast::error_code ec_;
            boost::asio::strand<boost::asio::io_context::executor_type> strand_;

            // Access the derived class, this is part of
            // the Curiously Recurring Template Pattern idiom.
            Derived& derived()
            {
                return static_cast<Derived&>(*this);
            }

            bool allow_host(const http::request<http::string_body>& req) override {
                auto is_conn_secure = is_secure();
                auto& lowest_layer = beast::get_lowest_layer<tcp::socket&>(derived().stream());
                auto local_endpoint = lowest_layer.local_endpoint();
                auto local_socket_host_port = local_endpoint.address().to_string() 
                        + ":" + std::to_string(local_endpoint.port());
                const auto& host_str = req["Host"].to_string();
                if (host_str.empty() 
                    || !host_is_valid(*plugin_state_, 
                                      host_str, 
                                      local_socket_host_port, 
                                      is_conn_secure)) 
                {
                    return false;
                }

                return true;
            }

        public: 
            // shared_from_this() requires default constuctor
            beast_http_session() = default;  
      
            // Take ownership of the buffer
            beast_http_session(
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc)
                : http_session_base(plugin_state, ioc)
                , strand_(ioc->get_executor())                    
            { }

            virtual ~beast_http_session() = default;            

            void do_read()
            {
                // Set the timeout.
                
                // beast::get_lowest_layer(
                //     derived().stream()).expires_after(std::chrono::seconds(30));

                // Read a request
                http::async_read(
                    derived().stream(),
                    buffer_,
                    req_parser_,
                    // beast::bind_front_handler(
                    //     &beast_http_session::on_read,
                    //     derived().shared_from_this())
                    boost::asio::bind_executor(
                        strand_,
                        std::bind(
                            &beast_http_session::on_read,
                            derived().shared_from_this(),
                            std::placeholders::_1,
                            std::placeholders::_2
                        )
                    )
                );
            }

            void on_read(beast::error_code ec,
                         std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);

                // This means they closed the connection
                if(ec == http::error::end_of_stream)
                    return derived().do_eof();

                if(ec && ec != asio::ssl::error::stream_truncated)
                    return fail(ec, "read");


                auto req = req_parser_.get();
                // Send the response
                handle_request(std::move(req));
            }

            void on_write(beast::error_code ec,
                          std::size_t bytes_transferred, 
                          bool close)
            {
                boost::ignore_unused(bytes_transferred);

                if(ec) {
                    ec_ = ec;
                    handle_exception();
                    return fail(ec, "write");
                }

                if(close)
                {
                    // This means we should close the connection, usually because
                    // the response indicated the "Connection: close" semantic.
                    return derived().do_eof();
                }

                // Read another request
                do_read();
            }           

            virtual void handle_exception() override {
                auto errCodeStr = std::to_string(ec_.value());
                fc_elog( logger, "beast_websession_exception: error code ${ec}", ("ec", errCodeStr));
            }

            virtual void send_response(std::optional<std::string> body, int code) override {
                // Determine if we should close the connection after
                bool close = !(plugin_state_->keep_alive) || res_.need_eof();

                res_.result(code);
                if(body.has_value())
                    res_.body() = *body;        

                res_.prepare_payload();

                // Write the response
                http::async_write(
                    derived().stream(),
                    res_,
                    // beast::bind_front_handler(
                    //     &beast_http_session::on_write,
                    //     derived().shared_from_this(),
                    //     close) 
                    // );
                    boost::asio::bind_executor(
                        strand_,
                        std::bind(
                            &beast_http_session::on_write,
                            derived().shared_from_this(),
                            std::placeholders::_1,
                            std::placeholders::_2,
                            close))
                );
            }
    }; // end class beast_http_session

    // Handles a plain HTTP connection
    class plain_session
        : public beast_http_session<plain_session> 
        , public std::enable_shared_from_this<plain_session>
    {
        // beast::tcp_stream stream_;
        tcp::socket socket_;
        boost::asio::strand<boost::asio::io_context::executor_type> strand_;

        public:      

            // Create the session
            plain_session(
                tcp::socket socket,
                std::shared_ptr<ssl::context> ctx,
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc
                )
                : beast_http_session<plain_session>(plugin_state, ioc)
                , socket_(std::move(socket))
                , strand_(socket_.get_executor())
            {
            }

            // Called by the base class
            //beast::tcp_stream&
            tcp::socket&
            stream()
            {
                return socket_;
            }

            // Start the asynchronous operation
            void run()
            {
                do_read();

                // Set the timeout.
                // beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

                // We need to be executing within a strand to perform async operations
                // on the I/O objects in this session. Although not strictly necessary
                // for single-threaded contexts, this example code is written to be
                // thread-safe by default.
                // asio::dispatch(stream_.get_executor(),
                //             beast::bind_front_handler(
                //                 &plain_session::do_read,
                //                 shared_from_this()));
            }

            void do_eof()
            {
                // Send a TCP shutdown
                beast::error_code ec;
                //stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
                socket_.shutdown(tcp::socket::shutdown_send, ec);

                // At this point the connection is closed gracefully
            }

            virtual shared_ptr<detail::abstract_conn> get_shared_from_this() override {
                return shared_from_this();
            }

    }; // end class plain_session

    // Handles an SSL HTTP connection
    class ssl_session
        : public beast_http_session<ssl_session>
        , public std::enable_shared_from_this<ssl_session>
    {
        // std::unique_ptr<beast::ssl_stream<beast::tcp_stream> > stream_;
        //beast::ssl_stream<beast::tcp_stream> stream_;
        tcp::socket socket_;
        ssl::stream<tcp::socket&> stream_;
        boost::asio::strand<boost::asio::io_context::executor_type> strand_;

        public:
            // Create the session
            ssl_session(
                tcp::socket socket,
                std::shared_ptr<ssl::context> ctx,
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc)
                : beast_http_session<ssl_session>(plugin_state, ioc)
                , socket_(std::move(socket))
                , stream_(socket_, *ctx)
                , strand_(stream_.get_executor())                    
            { }

            // Called by the base class
            //beast::ssl_stream<beast::tcp_stream>&  stream()
            ssl::stream<tcp::socket&> &stream()
            {
                return stream_;
            }

            // Start the asynchronous operation
            void run()
            {
                
                // Set the timeout.
                // beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

                auto self = shared_from_this();
            
                // We need to be executing within a strand to perform async operations
                // on the I/O objects in this session.
                // asio::dispatch(stream_.get_executor(), [self]() {
                    // Set the timeout.
                    
                    /*beast::get_lowest_layer(self->derived().stream_).expires_after(
                        std::chrono::seconds(30));
                    */

                    // Perform the SSL handshake
                    // Note, this is the buffered version of the handshake.
                    
                    self->derived().stream_.async_handshake(
                        ssl::stream_base::server,
                        self->buffer_.data(),
                        // beast::bind_front_handler(
                        //     &ssl_session::on_handshake,
                        //     self)
                        boost::asio::bind_executor(
                            strand_,
                            std::bind(
                                &ssl_session::on_handshake,
                                shared_from_this(),
                                std::placeholders::_1,
                                std::placeholders::_2
                            )
                        )
                    );
                // });
            }

            void on_handshake(beast::error_code ec,
                              std::size_t bytes_used)
            {
                
                if(ec)
                    return fail(ec, "handshake");

                // Consume the portion of the buffer used by the handshake
                buffer_.consume(bytes_used);

                do_read();
            }

            void do_eof()
            {
                // Set the timeout.
                // beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

                // Perform the SSL shutdown
                stream_.async_shutdown(
                    // beast::bind_front_handler(
                    //     &ssl_session::on_shutdown,
                    //     shared_from_this())
                    boost::asio::bind_executor(
                        strand_,
                        std::bind(
                            &ssl_session::on_shutdown,
                            shared_from_this(),
                            std::placeholders::_1
                        )
                    )
                );                        
            }

            void on_shutdown(beast::error_code ec)
            {
                if(ec)
                    return fail(ec, "shutdown");

                // At this point the connection is closed gracefully
            }

            virtual bool is_secure() override { return true; }

            virtual shared_ptr<detail::abstract_conn> get_shared_from_this() override {
                return shared_from_this();
            }
    }; // end class ssl_session

} // end namespace


