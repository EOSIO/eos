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
    class beast_http_session : public detail::abstract_conn, 
        public std::enable_shared_from_this<beast_http_session<Derived> >
    {
        // Access the derived class, this is part of
        // the Curiously Recurring Template Pattern idiom.
        Derived&
        derived()
        {
            return static_cast<Derived&>(*this);
        }

        // This is the C++11 equivalent of a generic lambda.
        // The function object is used to send an HTTP message.
        /*
        struct send_lambda
        {
            session& self_;

            explicit
            send_lambda(session& self)
                : self_(self)
            {
            }

            template<bool isRequest, class Body, class Fields>
            void
            operator()(http::message<isRequest, Body, Fields>&& msg) const
            {
                // The lifetime of the message has to extend
                // for the duration of the async operation so
                // we use a shared_ptr to manage it.
                auto sp = std::make_shared<
                    http::message<isRequest, Body, Fields>>(std::move(msg));

                // Store a type-erased version of the shared
                // pointer in the class to keep it alive.
                self_.res_ = sp;

                // Write the response
                http::async_write(
                    self_.derived().stream(),
                    *sp,
                    beast::bind_front_handler(
                        &session::on_write,
                        self_.derived().shared_from_this(),
                        sp->need_eof()));
            }
        };
        */


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

            // Returns a server error response
            /*
            auto const server_error =
            [](beast::string_view what, detail::abstract_conn& conn)
            {
                conn.send_response("An error occurred: '" + std::string(what) + "'", 
                            static_cast<int>(http::status::internal_server_error));
            };
            */

            // Make sure we can handle the method
            /*
            if( req.method() != http::verb::get &&
                req.method() != http::verb::head)
                return send(bad_request("Unknown HTTP-method"));
            */

            fc_ilog( logger, "detect bad target" ); 
            // Request path must be absolute and not contain "..".
            if( req.target().empty() ||
                req.target()[0] != '/' ||
                req.target().find("..") != beast::string_view::npos)
                return bad_request("Illegal request-target", *this);

            // Cache the size since we need it after the move
            // auto const size = body.size();


            try {
                // TODO
                /* 
                if(!allow_host<T>(req, con))
                    return;
                */
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

                // TODO: verfiy bytes in flight/requests in flight
                // auto abstract_conn_ptr = make_abstract_conn_ptr<T>(con, shared_from_this());
                // if( !verify_max_bytes_in_flight( con ) || !verify_max_requests_in_flight( con ) ) return;

                std::string resource = std::string(req.target());
                // look for the URL handler to handle this reosouce
                auto handler_itr = plugin_state_->url_handlers.find( resource );
                if( handler_itr != plugin_state_->url_handlers.end()) {
                    // c
                    std::string body = req.body();
                    fc_ilog( logger, "invoking handler_itr-second() " ); 
                    handler_itr->second( this->shared_from_this(), 
                                        std::move( resource ), 
                                        std::move( body ), 
                                        make_http_response_handler(*ioc_, *plugin_state_, this->shared_from_this()) );
                } else {
                    fc_dlog( logger, "404 - not found: ${ep}", ("ep", resource) );
                    not_found(resource, *this);
                    /*
                    error_results results{websocketpp::http::status_code::not_found,
                                        "Not Found", error_results::error_info(fc::exception( FC_LOG_MESSAGE( error, "Unknown Endpoint" )), verbose_http_errors )};
                    con->set_body( fc::json::to_string( results, fc::time_point::now() + max_response_time ));
                    con->set_status( websocketpp::http::status_code::not_found );
                    con->send_http_response();
                    */
                }
            } catch( ... ) {
                handle_exception();
            }

            // Respond to GET request
            /*
            http::response<http::file_body> res{
                std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(http::status::ok, req.version())};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            return send(std::move(res));
            */
        }


        public:
            // shared_from_this() requires default constuctor
            beast_http_session() = default;  
            beast_http_session(const beast_http_session&) = delete;
            beast_http_session(beast_http_session&&) = delete;

            beast_http_session& operator=(const beast_http_session&) = delete;
            beast_http_session& operator=(beast_http_session&&) = delete;

            // Take ownership of the buffer
            beast_http_session(
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc)
                : ioc_(ioc)
                , plugin_state_(plugin_state)
            {
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

                if(ec)
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

                if(ec)
                    return fail(ec, "write");

                if(close)
                {
                    // This means we should close the connection, usually because
                    // the response indicated the "Connection: close" semantic.
                    return derived().do_eof();
                }

                // We're done with the response so delete it
                res_ = nullptr;

                fc_ilog( logger, "do_read()" ); 

                // Read another request
                do_read();
            }

            

            // TODO
            virtual bool verify_max_bytes_in_flight() override {
                return true;
            }

            // TODO
            virtual bool verify_max_requests_in_flight() override {
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

                // We need the serializer here because the serializer requires
                // a non-const file_body, and the message oriented version of
                // http::write only works with const messages.
                
                
                // http::serializer<false, std::string> sr{res_};
                // http::write(derived().stream_, sr, ec_)

                if(ec_)
                    handle_exception();
            }
    }; // end class beast_http_session

    // Handles a plain HTTP connection
    class plain_session
        : public beast_http_session<plain_session>
    {
        std::unique_ptr<beast::tcp_stream> stream_;

        public:
            plain_session() = default;
            plain_session(const plain_session&) = delete;
            plain_session(plain_session&&) = delete;

            plain_session& operator=(const plain_session&) = delete;
            plain_session& operator=(plain_session&&) = delete;

            // Create the session
            plain_session(
                tcp::socket&& socket,
                std::shared_ptr<ssl::context> ctx,
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc
                )
                : beast_http_session<plain_session>(
                    plugin_state,ioc)
            {
                stream_ = std::make_unique<beast::tcp_stream>(std::move(socket));
            }

            // Called by the base class
            beast::tcp_stream&
            stream()
            {
                return *stream_;
            }

            // Start the asynchronous operation
            void
            run()
            {
                // Set the timeout.
                beast::get_lowest_layer(*stream_).expires_after(std::chrono::seconds(30));

                // We need to be executing within a strand to perform async operations
                // on the I/O objects in this session. Although not strictly necessary
                // for single-threaded contexts, this example code is written to be
                // thread-safe by default.
                asio::dispatch(stream_->get_executor(),
                            beast::bind_front_handler(
                                &plain_session::do_read,
                                shared_from_this()));
            }

            void
            do_eof()
            {
                // Send a TCP shutdown
                beast::error_code ec;
                stream_->socket().shutdown(tcp::socket::shutdown_send, ec);

                // At this point the connection is closed gracefully
            }
    }; // end class plain_session

    // Handles an SSL HTTP connection
    /*

    class ssl_session
        : public beast_http_session<ssl_session>
    {
        beast::ssl_stream<beast::tcp_stream> stream_;

        public:
            ssl_session() = default;
            // Create the session
            ssl_session(
                tcp::socket&& socket,
                std::shared_ptr<ssl::context> ctx,
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc)
                : beast_http_session<ssl_session>(
                    plugin_state, 
                    ioc))
            {
                stream_ = make_unique<beast::tcp_stream>(std::move(sockeet));
            }

            // Called by the base class
            beast::ssl_stream<beast::tcp_stream>&
            stream()
            {
                return *stream_;
            }

            // Start the asynchronous operation
            void
            run()
            {
                auto self = shared_from_this();
                // We need to be executing within a strand to perform async operations
                // on the I/O objects in this session.
                asio::dispatch(stream_.get_executor(), [self]() {
                    // Set the timeout.
                    beast::get_lowest_layer(self->*stream_).expires_after(
                        std::chrono::seconds(30));

                    // Perform the SSL handshake
                    // Note, this is the buffered version of the handshake.
                    self->stream_->async_handshake(
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
                // Set the timeout.
                beast::get_lowest_layer(*stream_).expires_after(std::chrono::seconds(30));

                // Perform the SSL shutdown
                stream_->async_shutdown(
                    beast::bind_front_handler(
                        &ssl_session::on_shutdown,
                        shared_from_this()));
            }

            void
            on_shutdown(beast::error_code ec)
            {
                if(ec)
                    return fail(ec, "shutdown");

                // At this point the connection is closed gracefully
            }
    }; // end class ssl_session
    */

} // end namespace


