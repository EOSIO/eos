#pragma once

#include "common.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <sstream>

extern fc::logger logger;

namespace eosio { 

    //------------------------------------------------------------------------------

    // Report a failure
    void
    fail(beast::error_code ec, char const* what)
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



    // Handles an HTTP server connection.
    // This uses the Curiously Recurring Template Pattern so that
    // the same code works with both SSL streams and regular sockets.
    template<class Derived>
    class beast_http_session : public detail::abstract_conn
    {
        public: 
            // Access the derived class, this is part of
            // the Curiously Recurring Template Pattern idiom.
            Derived&
            derived()
            {
                return static_cast<Derived&>(*this);
            }

            http::request<http::string_body> req_;
            // std::shared_ptr<void> res_;
            http::response<http::string_body> res_;
            // send_lambda lambda_;
            
            beast::flat_buffer buffer_;
            asio::io_context *ioc_;
            std::shared_ptr<http_plugin_state> plugin_state_;
            beast::error_code ec_;

            template<
                class Body, class Allocator>
            void
            handle_request(
                http::request<Body, http::basic_fields<Allocator>>&& req)
            {
                auto &res = res_;

                fc_ilog( logger, "res.version()" ); 
                res.version(req.version());
                res.set(http::field::content_type, "application/json");
                res.keep_alive(req.keep_alive());
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);

                // Returns a bad request response
                auto const bad_request =
                [](beast::string_view why, detail::abstract_conn& conn)
                {
                    //http::response<http::string_body> res{http::status::bad_request, req.version()};
                    conn.send_response(std::string(why), 
                                    static_cast<int>(http::status::bad_request));
                };

                // Returns a not found response
                auto const not_found =
                [](beast::string_view target, detail::abstract_conn& conn)
                {
                    conn.send_response("The resource '" + std::string(target) + "' was not found.",
                                static_cast<int>(http::status::not_found)); 
                };

                fc_ilog( logger, "detect bad target" ); 
                // Request path must be absolute and not contain "..".
                if( req.target().empty() ||
                    req.target()[0] != '/' ||
                    req.target().find("..") != beast::string_view::npos)
                    return bad_request("Illegal request-target", *this);

                // Cache the size since we need it after the move
                // auto const size = body.size();


                try {
                    // TODO implement allow_hosts
                    if(!allow_host(req))
                        return;

                    fc_ilog( logger, "set HTTP headers" ); 

                    if( !plugin_state_->access_control_allow_origin.empty()) {
                        res.set( "Access-Control-Allow-Origin", plugin_state_->access_control_allow_origin );
                    }
                    if( !plugin_state_->access_control_allow_headers.empty()) {
                        res.set( "Access-Control-Allow-Headers", plugin_state_->access_control_allow_headers );
                    }
                    if( !plugin_state_->access_control_max_age.empty()) {
                        res.set( "Access-Control-Max-Age", plugin_state_->access_control_max_age );
                    }
                    if( plugin_state_->access_control_allow_credentials ) {
                        res.set( "Access-Control-Allow-Credentials", "true" );
                    }

                    // Respond to options request
                    if(req.method() == http::verb::options)
                    {
                        fc_ilog( logger, "send_response(\"\")" ); 
                        send_response("", static_cast<int>(http::status::ok));
                        return;
                    }

                    // verfiy bytes in flight/requests in flight
                    if( !verify_max_bytes_in_flight() || !verify_max_requests_in_flight() ) return;

                    std::string resource = std::string(req.target());
                    // look for the URL handler to handle this reosouce
                    auto handler_itr = plugin_state_->url_handlers.find( resource );
                    if( handler_itr != plugin_state_->url_handlers.end()) {
                        // c
                        std::string body = req.body();
                        fc_ilog( logger, "invoking handler_itr-second() " ); 
                        handler_itr->second( derived().shared_from_this(), 
                                            std::move( resource ), 
                                            std::move( body ), 
                                            make_http_response_handler(*ioc_, *plugin_state_, derived().shared_from_this()) );
                    } else {
                        fc_dlog( logger, "404 - not found: ${ep}", ("ep", resource) );
                        not_found(resource, *this);                    
                    }
                } catch( ... ) {
                    handle_exception();
                }           
            }


        public:
            // shared_from_this() requires default constuctor
            beast_http_session() = default;  
            /*
            beast_http_session(const beast_http_session&) = delete;
            beast_http_session(beast_http_session&&) = delete;

            beast_http_session& operator=(const beast_http_session&) = delete;
            beast_http_session& operator=(beast_http_session&&) = delete;
            */

            // Take ownership of the buffer
            beast_http_session(
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc)
                : ioc_(ioc)
                , plugin_state_(plugin_state)
            {
                plugin_state_->requests_in_flight += 1;
            }

            virtual ~beast_http_session() {
                plugin_state_->requests_in_flight -= 1;
            }

            void
            do_read()
            {
                fc_ilog( logger, "get_lowest_layer()" ); 
                // Set the timeout.
                beast::get_lowest_layer(
                    derived().stream()).expires_after(std::chrono::seconds(30));

                fc_ilog( logger, "async_read()" ); 
                // Read a request
                http::async_read(
                    derived().stream(),
                    buffer_,
                    req_,
                    beast::bind_front_handler(
                        &beast_http_session::on_read,
                        derived().shared_from_this()));
            }

            void
            on_read(
                beast::error_code ec,
                std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);

                // This means they closed the connection
                if(ec == http::error::end_of_stream)
                    return derived().do_eof();

                if(ec && ec != asio::ssl::error::stream_truncated)
                    return fail(ec, "read");

                // Send the response
                handle_request(std::move(req_));
            }

            void
            on_write(
                bool close,
                beast::error_code ec,
                std::size_t bytes_transferred)
            {
                fc_ilog( logger, "ignore_unused()" ); 
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

                // We're done with the response so delete it
                // res_ = nullptr;

                fc_ilog( logger, "do_read()" ); 

                // Read another request
                do_read();
            }


            // TODO 
            bool allow_host(const http::request<http::string_body>& req) {
                /*
                bool is_secure = con->get_uri()->get_secure();
                const auto& local_endpoint = con->get_socket().lowest_layer().local_endpoint();
                auto local_socket_host_port = local_endpoint.address().to_string() + ":" + std::to_string(local_endpoint.port());

                const auto& host_str = req.get_header("Host");
                if (host_str.empty() || !host_is_valid(*plugin_state, host_str, local_socket_host_port, is_secure)) {
                con->set_status(websocketpp::http::status_code::bad_request);
                return false;
                }
                */
                return true;
            }

            void report_429_error(const std::string & what) {
                /*
                error_results::error_info ei;
                ei.code = websocketpp::http::status_code::too_many_requests;
                ei.name = "Busy";
                ei.what = what;
                error_results results{websocketpp::http::status_code::too_many_requests, "Busy", ei};
                // con->set_body(  ));
                // con->set_status( websocketpp::http::status_code::too_many_requests );
                // con->send_response(fc::json::to_string( results, fc::time_point::maximum(), ec);
                */
                send_response(std::string(what), 
                                static_cast<int>(http::status::too_many_requests));                
            }

            virtual bool verify_max_bytes_in_flight() override {
                auto bytes_in_flight_size = plugin_state_->bytes_in_flight.load();
                if( bytes_in_flight_size > plugin_state_->max_bytes_in_flight ) {
                    fc_dlog( logger, "429 - too many bytes in flight: ${bytes}", ("bytes", bytes_in_flight_size) );
                    string what = "Too many bytes in flight: " + std::to_string( bytes_in_flight_size ) + ". Try again later.";;
                    report_429_error(what);
                    return false;
                }
                return true;
            }

            virtual bool verify_max_requests_in_flight() override {
                if (plugin_state_->max_requests_in_flight < 0)
                    return true;

                auto requests_in_flight_num = plugin_state_->requests_in_flight.load();
                if( requests_in_flight_num > plugin_state_->max_requests_in_flight ) {
                    fc_dlog( logger, "429 - too many requests in flight: ${requests}", ("requests", requests_in_flight_num) );
                    string what = "Too many requests in flight: " + std::to_string( requests_in_flight_num ) + ". Try again later.";
                    report_429_error(what);
                    return false;
                }
                return true;
            }

            virtual void handle_exception() override {
                auto errCodeStr = std::to_string(ec_.value());
                fc_elog( logger, "beast_websession_exception: error code ${ec}", ("ec", errCodeStr));
            }

            virtual void send_response(std::optional<std::string> body, int code) override {
                // Determine if we should close the connection after
                // close_ = res_.need_eof();                

                res_.result(code);
                if(body.has_value())
                    res_.body() = *body;        

                fc_ilog( logger, "res_.prepare_payload()" ); 

                res_.prepare_payload();

                // Write the response
                http::async_write(
                    derived().stream(),
                    res_,
                    beast::bind_front_handler(
                        &beast_http_session::on_write,
                        derived().shared_from_this(),
                        true) // self._res.need_eof())
                    );
            }
    }; // end class beast_http_session

    // Handles a plain HTTP connection
    class plain_session
        : public beast_http_session<plain_session> 
        , public std::enable_shared_from_this<plain_session>
    {
        beast::tcp_stream stream_;

        public:
        /*
            plain_session() = default;
            plain_session(const plain_session&) = delete;
            plain_session(plain_session&&) = delete;

            plain_session& operator=(const plain_session&) = delete;
            plain_session& operator=(plain_session&&) = delete;
            */

            // Create the session
            plain_session(
                tcp::socket&& socket,
                std::shared_ptr<ssl::context> ctx,
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc
                )
                : beast_http_session<plain_session>(plugin_state, ioc)
                , stream_(std::move(socket))
            {
            }

            // Called by the base class
            beast::tcp_stream&
            stream()
            {
                return stream_;
            }

            // Start the asynchronous operation
            void
            run()
            {
                // Set the timeout.
                beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

                // We need to be executing within a strand to perform async operations
                // on the I/O objects in this session. Although not strictly necessary
                // for single-threaded contexts, this example code is written to be
                // thread-safe by default.
                asio::dispatch(stream_.get_executor(),
                            beast::bind_front_handler(
                                &plain_session::do_read,
                                shared_from_this()));
            }

            void
            do_eof()
            {
                // Send a TCP shutdown
                beast::error_code ec;
                stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

                // At this point the connection is closed gracefully
            }
    }; // end class plain_session

    // Handles an SSL HTTP connection
    class ssl_session
        : public beast_http_session<ssl_session>
        , public std::enable_shared_from_this<ssl_session>
    {
        // std::unique_ptr<beast::ssl_stream<beast::tcp_stream> > stream_;
        beast::ssl_stream<beast::tcp_stream> stream_;

        public:
        /*
            ssl_session() = default;
            ssl_session(const ssl_session&) = delete;
            ssl_session(ssl_session&&) = delete;

            ssl_session& operator=(const ssl_session&) = delete;
            ssl_session& operator=(ssl_session&&) = delete;
        */
            // Create the session
            ssl_session(
                tcp::socket&& socket,
                std::shared_ptr<ssl::context> ctx,
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc)
                : beast_http_session<ssl_session>(plugin_state, ioc),
                    stream_(std::move(socket), *ctx)
            {
                // stream_ = std::make_unique<beast::ssl_stream<beast::tcp_stream> >(std::move(socket), *ctx);
            }

            // Called by the base class
            beast::ssl_stream<beast::tcp_stream>&
            stream()
            {
                return stream_;
            }

            // Start the asynchronous operation
            void
            run()
            {
                
                // Set the timeout.
                // beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(30));

                auto self = shared_from_this();

            
                // We need to be executing within a strand to perform async operations
                // on the I/O objects in this session.
                asio::dispatch(stream_.get_executor(), [self]() {
                    // Set the timeout.
                    
                    beast::get_lowest_layer(self->derived().stream_).expires_after(
                        std::chrono::seconds(30));
                    

                    // Perform the SSL handshake
                    // Note, this is the buffered version of the handshake.
                    
                    self->derived().stream_.async_handshake(
                        ssl::stream_base::server,
                        self->buffer_.data(),
                        beast::bind_front_handler(
                            &ssl_session::on_handshake,
                            self));
                });
                
            }

            void
            on_handshake(
                beast::error_code ec,
                std::size_t bytes_used)
            {
                
                if(ec)
                    return fail(ec, "handshake");

                // Consume the portion of the buffer used by the handshake
                buffer_.consume(bytes_used);

                do_read();
            }

            void
            do_eof()
            {
                /*
                // Set the timeout.
                beast::get_lowest_layer(*stream_).expires_after(std::chrono::seconds(30));

                // Perform the SSL shutdown
                stream_->async_shutdown(
                    beast::bind_front_handler(
                        &ssl_session::on_shutdown,
                        shared_from_this()));
                        */
            }

            void
            on_shutdown(beast::error_code ec)
            {
                if(ec)
                    return fail(ec, "shutdown");

                // At this point the connection is closed gracefully
            }
    }; // end class ssl_session

} // end namespace


