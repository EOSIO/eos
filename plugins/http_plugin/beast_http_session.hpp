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

    using std::chrono::steady_clock;

// Boost 1.70 introduced a breaking change that causes problems with construction of strand objects from tcp_socket
// this is suggested fix OK'd Beast author (V. Falco) to handle both versions gracefully
// see https://stackoverflow.com/questions/58453017/boost-asio-tcp-socket-1-70-not-backward-compatible
#if BOOST_VERSION < 107000                
    typedef tcp::socket tcp_socket_t;
#else
    typedef asio::basic_stream_socket<asio::ip::tcp, asio::io_context::executor_type> tcp_socket_t;
#endif

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

    //------------------------------------------------------------------------------
    // fail() 
    //  this function is generally reserved in the case of a severe error which results 
    //  in immediate termiantion of the session, with no response sent back to the client
    //  currently also includes SSL "short read" error for security reasons:
    // 
    //  https://github.com/boostorg/beast/issues/38
    //  https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
    void fail(beast::error_code ec, char const* what)
    {
        fc_elog(logger, "${w}: ${m}", ("w", what)("m", ec.message()));
        fc_elog(logger, "closing connection");
    }


    template<class T>
    bool allow_host(const http::request<http::string_body>& req, T& session, std::shared_ptr<http_plugin_state> plugin_state) {
        auto is_conn_secure = session.is_secure();

        auto& socket = session.socket();
#if BOOST_VERSION < 107000                
        auto& lowest_layer = beast::get_lowest_layer<tcp_socket_t&>(socket);
#else
        auto& lowest_layer = beast::get_lowest_layer(socket);
#endif                
        auto local_endpoint = lowest_layer.local_endpoint();
        auto local_socket_host_port = std::make_pair(local_endpoint.address().to_v6(), local_endpoint.port());

        const auto& host_str = req["Host"].to_string();
        if (host_str.empty() 
            || !host_is_valid(*plugin_state, 
                                host_str, 
                                local_socket_host_port, 
                                is_conn_secure)) 
        {
            return false;
        }

        return true;
    }

    // Handle HTTP conneciton using boost::beast for TCP communication
    // Subclasses of this class (plain_session, ssl_session, etc.) 
    // use the Curiously Recurring Template Pattern so that
    // the same code works with both SSL streams, regular TCP sockets and UNIX sockets
    template<class Derived> 
    class beast_http_session :  public detail::abstract_conn
    {
        protected:
            beast::flat_buffer buffer_;

            // time points for timeout measurement and perf metrics 
            steady_clock::time_point session_begin_, read_begin_, handle_begin_, write_begin_;
            uint64_t read_time_us_ = 0, handle_time_us_ = 0, write_time_us_ = 0;

            // HTTP parser object
            std::optional<http::request_parser<http::string_body>> req_parser_;

            // HTTP response object
            std::optional<http::response<http::string_body>> res_;

            std::shared_ptr<http_plugin_state> plugin_state_;

            // whether response should be sent back to client when an exception occurs
            bool is_send_exception_response_ = true; 
            
            std::shared_ptr<eosio::chain::named_thread_pool> thread_pool_;

            template<
                class Body, class Allocator>
            void
            handle_request(
                http::request<Body, http::basic_fields<Allocator>>&& req)
            {
                res_->version(req.version());
                res_->set(http::field::content_type, "application/json");
                res_->keep_alive(req.keep_alive());
                res_->set(http::field::server, BOOST_BEAST_VERSION_STRING);

                // Returns a bad request response
                auto const bad_request =
                [](beast::string_view why, detail::abstract_conn& conn)
                {
                    conn.send_response(std::string(why), 
                                    static_cast<int>(http::status::bad_request));
                };

                // Returns a not found response
                auto const not_found =
                [](const std::string& target, detail::abstract_conn& conn)
                {
                    conn.send_response("The resource '" + target + "' was not found.",
                                static_cast<int>(http::status::not_found)); 
                };
                
                // Request path must be absolute and not contain "..".
                if( req.target().empty() ||
                    req.target()[0] != '/' ||
                    req.target().find("..") != beast::string_view::npos)
                    return bad_request("Illegal request-target", *this);

                try {
                    if(!derived().allow_host(req))
                        return;

                    if( !plugin_state_->access_control_allow_origin.empty()) {
                        res_->set( "Access-Control-Allow-Origin", plugin_state_->access_control_allow_origin );
                    }
                    if( !plugin_state_->access_control_allow_headers.empty()) {
                        res_->set( "Access-Control-Allow-Headers", plugin_state_->access_control_allow_headers );
                    }
                    if( !plugin_state_->access_control_max_age.empty()) {
                        res_->set( "Access-Control-Max-Age", plugin_state_->access_control_max_age );
                    }
                    if( plugin_state_->access_control_allow_credentials ) {
                        res_->set( "Access-Control-Allow-Credentials", "true" );
                    }

                    // Respond to options request
                    if(req.method() == http::verb::options)
                    {
                        send_response("", static_cast<int>(http::status::ok));
                        return;
                    }

                    // verfiy bytes in flight/requests in flight
                    if( !verify_max_bytes_in_flight() ) return;

                    std::string resource = std::string(req.target());
                    // look for the URL handler to handle this reosouce
                    auto handler_itr = plugin_state_->url_handlers.find( resource );
                    if( handler_itr != plugin_state_->url_handlers.end()) {
                        fc_dlog( logger, "resource found: ${ep}", ("ep", resource) );
                        std::string body = req.body();
                        handler_itr->second( derived().shared_from_this(), 
                                            std::move( resource ), 
                                            std::move( body ), 
                                            make_http_response_handler(thread_pool_, plugin_state_, derived().shared_from_this()) );
                    } else {
                        fc_dlog( logger, "404 - not found: ${ep}", ("ep", resource) );
                        not_found(resource, *this);                    
                    }
                } catch (...) {
                    handle_exception();
                }
            }

            void report_429_error(std::string what) {                
                send_response(std::move(what), 
                            static_cast<int>(http::status::too_many_requests));                
            }

        public:
            virtual bool verify_max_bytes_in_flight() override {
                auto bytes_in_flight_size = plugin_state_->bytes_in_flight.load();
                if( bytes_in_flight_size > plugin_state_->max_bytes_in_flight ) {
                    fc_dlog( logger, "429 - too many bytes in flight: ${bytes}", ("bytes", bytes_in_flight_size) );
                    std::string what = "Too many bytes in flight: " + std::to_string( bytes_in_flight_size ) + ". Try again later.";;
                    report_429_error(std::move(what));
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
                    std::string what = "Too many requests in flight: " + std::to_string( requests_in_flight_num ) + ". Try again later.";
                    report_429_error(std::move(what));
                    return false;
                }
                return true;
            }           

            // Access the derived class, this is part of
            // the Curiously Recurring Template Pattern idiom.
            Derived& derived()
            {
                return static_cast<Derived&>(*this);
            }
            
        public: 
            // shared_from_this() requires default constructor
            beast_http_session() = default;

            beast_http_session(
                std::shared_ptr<http_plugin_state> plugin_state,  
                std::shared_ptr<eosio::chain::named_thread_pool> thread_pool)
                : plugin_state_(std::move(plugin_state))
                , thread_pool_(std::move(thread_pool))
            {  
                plugin_state_->requests_in_flight += 1;
                req_parser_.emplace();
                req_parser_->body_limit(plugin_state_->max_body_size);
                res_.emplace();

                session_begin_ = steady_clock::now();
                read_time_us_ = handle_time_us_ = write_time_us_ = 0;

                // default to true
                is_send_exception_response_ = true;
            }

            virtual ~beast_http_session() {
                is_send_exception_response_ = false;
                plugin_state_->requests_in_flight -= 1;
                auto session_time = steady_clock::now() - session_begin_;
                auto session_time_us = std::chrono::duration_cast<std::chrono::microseconds>(session_time).count();
                fc_dlog(logger, "session time    ${t}", ("t", session_time_us));                            
                fc_dlog(logger, "        read    ${t}", ("t", read_time_us_));
                fc_dlog(logger, "        handle  ${t}", ("t", handle_time_us_));                                            
                fc_dlog(logger, "        write   ${t}", ("t", write_time_us_));                            
            }    

            void do_read()
            {
                read_begin_ = steady_clock::now();

                // Read a request
                auto self = derived().shared_from_this();
                http::async_read(
                    derived().stream(),
                    buffer_,
                    *req_parser_,
                    [self](beast::error_code ec, std::size_t bytes_transferred) { 
                        self->on_read(ec, bytes_transferred);
                    }
                );                
            }

            void on_read(beast::error_code ec,
                         std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);

                // This means they closed the connection
                if(ec == http::error::end_of_stream)
                    return derived().do_eof();

                if(ec) {
                    return fail(ec, "read");
                }

                auto req = req_parser_->release();

                handle_begin_ = steady_clock::now();
                auto dt = handle_begin_ - read_begin_;
                read_time_us_ += std::chrono::duration_cast<std::chrono::microseconds>(dt).count();

                // Send the response
                handle_request(std::move(req));
            }

            void on_write(beast::error_code ec,
                          std::size_t bytes_transferred, 
                          bool close)
            {
                boost::ignore_unused(bytes_transferred);

                if(ec) {
                    return fail(ec, "write");
                }

                auto dt = steady_clock::now() - write_begin_;
                write_time_us_ += std::chrono::duration_cast<std::chrono::microseconds>(dt).count();

                if(close)
                {
                    // This means we should close the connection, usually because
                    // the response indicated the "Connection: close" semantic.
                    return derived().do_eof();
                }

                // create a new parser to clear state 
                req_parser_.emplace();
                req_parser_->body_limit(plugin_state_->max_body_size);
                // create a new response object
                res_.emplace();

                // Read another request
                do_read();
            }           

            virtual void handle_exception() override {
                std::string err_str;
                try { 
                    throw;
                } catch(const fc::exception& e) {
                    err_str = e.to_detail_string();
                    fc_elog( logger, "fc::exception: ${w}", ("w", err_str) );
                } catch(std::exception& e) {
                    err_str = e.what(); 
                    fc_elog( logger, "std::exception: ${w}", ("w", err_str) );
                } catch( ... ) {
                    err_str = "unknown";
                    fc_elog( logger, "unkonwn exception");
                } 

                if(is_send_exception_response_) {
                    res_->set(http::field::content_type, "text/plain");
                    res_->keep_alive(false);
                    res_->set(http::field::server, BOOST_BEAST_VERSION_STRING);

                    http::status stat = http::status::internal_server_error;
                    auto resp_str = "Internal Server Error\n\nUnhandled Exception: " + err_str;
                    send_response(resp_str, static_cast<int>(stat));
                    derived().do_eof();
                }
            }

            virtual void send_response(std::optional<std::string> body, int code) override {
                write_begin_ = steady_clock::now();
                auto dt = write_begin_ - handle_begin_;
                handle_time_us_ += std::chrono::duration_cast<std::chrono::microseconds>(dt).count();
                
                res_->result(code);
                if(body.has_value())
                    res_->body() = *body;        

                res_->prepare_payload();

                // Determine if we should close the connection after
                bool close = !(plugin_state_->keep_alive) || res_->need_eof();

                // Write the response
                auto self = derived().shared_from_this();
                http::async_write(
                    derived().stream(),
                    *res_,
                    [self, close](beast::error_code ec, std::size_t bytes_transferred) {
                        self->on_write(ec, bytes_transferred, close);
                    }
                );
            }

            void run_session() {
                if(!verify_max_requests_in_flight())
                    return derived().do_eof();
                
                derived().run();
            }
    }; // end class beast_http_session

    // Handles a plain HTTP connection
    class plain_session
        : public beast_http_session<plain_session> 
        , public std::enable_shared_from_this<plain_session>
    {
        tcp_socket_t socket_;

        public:      
            // Create the session
            plain_session(
                tcp_socket_t socket,
                std::shared_ptr<ssl::context> ctx,
                std::shared_ptr<http_plugin_state> plugin_state, 
                std::shared_ptr<eosio::chain::named_thread_pool> thread_pool)
                : beast_http_session<plain_session>(std::move(plugin_state), std::move(thread_pool))
                , socket_(std::move(socket))
            {}

            tcp_socket_t& stream() { return socket_; }
            tcp_socket_t& socket() { return socket_; }

            // Start the asynchronous operation
            void run()
            {
                do_read();
            }

            void do_eof()
            {
                is_send_exception_response_ = false;
                try {
                    // Send a TCP shutdown
                    beast::error_code ec;
                    socket_.shutdown(tcp::socket::shutdown_send, ec);
                    // At this point the connection is closed gracefully
                } catch (...) { 
                    handle_exception();
                }
            }

            bool is_secure() { return false; };

            bool allow_host(const http::request<http::string_body>& req) { 
                return eosio::allow_host(req, *this, plugin_state_);
            }

            static constexpr auto name() {
                return "plain_session";
            }
    }; // end class plain_session

    // Handles an SSL HTTP connection
    class ssl_session
        : public beast_http_session<ssl_session>
        , public std::enable_shared_from_this<ssl_session>
    {   
        ssl::stream<tcp_socket_t> stream_;

        public:
            // Create the session

            ssl_session(
                tcp_socket_t socket,
                std::shared_ptr<ssl::context> ctx,
                std::shared_ptr<http_plugin_state> plugin_state, 
                std::shared_ptr<eosio::chain::named_thread_pool> thread_pool)
                : beast_http_session<ssl_session>(std::move(plugin_state), std::move(thread_pool))
                , stream_(std::move(socket), *ctx)
            { }


            ssl::stream<tcp_socket_t> &stream() { return stream_; }
#if BOOST_VERSION < 107000                        
            tcp_socket_t& socket() { return beast::get_lowest_layer<tcp_socket_t&>(stream_); }
#else
            tcp_socket_t& socket() { return beast::get_lowest_layer(stream_); }
#endif
            // Start the asynchronous operation
            void run()
            {
                auto self = shared_from_this();
                self->stream_.async_handshake(
                    ssl::stream_base::server,
                    self->buffer_.data(),
                    [self](beast::error_code ec, std::size_t bytes_used) {
                        self->on_handshake(ec, bytes_used);
                    }
                );
            }

            void on_handshake(beast::error_code ec, std::size_t bytes_used)
            {
                if(ec)
                    return fail(ec, "handshake");

                buffer_.consume(bytes_used);

                do_read();
            }

            void do_eof()
            {
                // Perform the SSL shutdown
                auto self = shared_from_this();
                stream_.async_shutdown(
                    [self](beast::error_code ec) {
                        self->on_shutdown(ec);
                    }
                );            
            }

            void on_shutdown(beast::error_code ec)
            {
                if(ec)
                    return fail(ec, "shutdown");
                // At this point the connection is closed gracefully
            }

            bool is_secure() { return true; }

            bool allow_host(const http::request<http::string_body>& req) { 
                return eosio::allow_host(req, *this, plugin_state_);
            }

            static constexpr auto name() {
                return "ssl_session";
            }
    }; // end class ssl_session


    // unix domain sockets
    class unix_socket_session 
        : public std::enable_shared_from_this<unix_socket_session>
        , public beast_http_session<unix_socket_session>
    {    
        
        // The socket used to communicate with the client.
        stream_protocol::socket socket_;

        public:
            unix_socket_session(stream_protocol::socket sock, 
                            std::shared_ptr<ssl::context> ctx,
                            std::shared_ptr<http_plugin_state> plugin_state, 
                            std::shared_ptr<eosio::chain::named_thread_pool> thread_pool)
            : beast_http_session(std::move(plugin_state), std::move(thread_pool))
            , socket_(std::move(sock)) 
            {  }

            virtual ~unix_socket_session() = default;

            bool allow_host(const http::request<http::string_body>& req) { 
                // always allow local hosts
                return true;
            }

            void do_eof()
            {
                is_send_exception_response_ = false;
                try {
                    // Send a shutdown signal
                    boost::system::error_code ec;
                    socket_.shutdown(stream_protocol::socket::shutdown_send, ec);
                    // At this point the connection is closed gracefully
                } catch (...) { 
                    handle_exception(); 
                }
            }

            bool is_secure() { return false; };

            void run() {
                do_read();
            }

            stream_protocol::socket& stream() { return socket_; }
            stream_protocol::socket& socket() { return socket_; } 
            
            static constexpr auto name() {
                return "unix_socket_session";
            }
    }; // end class unix_socket_session

} // end namespace