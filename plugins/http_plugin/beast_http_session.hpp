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

        fc_elog(logger, "${w}: ${m}", ("w", what)("m", ec.message()));
    }

    // this needs to be declared outside of the class, because it doesn't work for 
    // UNIX socket session, since it lacks address() function in endpoint
    //  Since this is a virtual function, std::enable_if_t/SFNIAE doesn't work 
    template<class T>
    bool allow_host(const http::request<http::string_body>& req, T& session) {
        auto is_conn_secure = session.is_secure();

        auto& socket = session.socket();
#if BOOST_VERSION < 107000                
        auto& lowest_layer = beast::get_lowest_layer<tcp_socket_t&>(socket);
#else
        auto& lowest_layer = beast::get_lowest_layer(socket);
#endif                
        auto local_endpoint = lowest_layer.local_endpoint();
        auto local_socket_host_port = local_endpoint.address().to_string() 
                + ":" + std::to_string(local_endpoint.port());
        const auto& host_str = req["Host"].to_string();
        if (host_str.empty() 
            || !host_is_valid(*session.plugin_state_, 
                                host_str, 
                                local_socket_host_port, 
                                is_conn_secure)) 
        {
            return false;
        }

        return true;
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

            // time points for timeout measurement and perf metrics 
            steady_clock::time_point session_begin_, read_begin_, handle_begin_, write_begin_;
            uint64_t read_time_us_, handle_time_us_, write_time_us_;

            // Access the derived class, this is part of
            // the Curiously Recurring Template Pattern idiom.
            Derived& derived()
            {
                return static_cast<Derived&>(*this);
            }
            
        public: 
            // shared_from_this() requires default constuctor
            beast_http_session() = default;
      
            // Take ownership of the buffer
            beast_http_session(
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc)
                : http_session_base(plugin_state, ioc)
            {  
                session_begin_ = steady_clock::now();
                read_time_us_ = handle_time_us_ = write_time_us_ = 0;
            }

            virtual ~beast_http_session() {
#if PRINT_PERF_METRICS                
                auto session_time = steady_clock::now() - session_begin_;
                auto session_time_us = std::chrono::duration_cast<std::chrono::microseconds>(session_time).count();           
                fc_elog(logger, "session time    ${t}", ("t", session_time_us));                            
                fc_elog(logger, "        read    ${t}", ("t", read_time_us_));
                fc_elog(logger, "        handle  ${t}", ("t", handle_time_us_));                                            
                fc_elog(logger, "        write   ${t}", ("t", write_time_us_));                            
#endif
            }    

            bool check_timeout() {
                auto t = steady_clock::now();
                auto dt = t - session_begin_;
                auto t_elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(dt).count();        
                return (t_elapsed_us > plugin_state_->max_response_time.count());
            }

            void do_read()
            {
                beast::error_code ec;

                // wait for data to become available on the socket if we don't do this
                // clients can hang the session by not sending any data
                boost::asio::socket_base::bytes_readable command(true);
                auto &socket = derived().socket();
                std::size_t bytes_readable = 0; 
                
                bool timeout = false;
                while (bytes_readable < 1 && !timeout) {
                    socket.io_control(command);
                    bytes_readable = command.get();
                    timeout = check_timeout();
                }

                if(timeout) {
                    fc_elog(logger, "connection timeout - no data sent. closing");
                    return derived().do_eof();
                }

                read_begin_ = steady_clock::now();

                // just use sync reads for now - P. Raphael
                auto bytes_read = http::read(derived().stream(), buffer_, req_parser_, ec);
                on_read(ec, bytes_read);                
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

                if (check_timeout()) {
                    fc_elog(logger, "connection timeout - could not read data within time period. closing");
                    return derived().do_eof();
                }

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
                    ec_ = ec;
                    handle_exception();
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

                write_begin_ = steady_clock::now();
                auto dt = write_begin_ - handle_begin_;
                handle_time_us_ += std::chrono::duration_cast<std::chrono::microseconds>(dt).count();
                
                res_.result(code);
                if(body.has_value())
                    res_.body() = *body;        

                res_.prepare_payload();

                if (check_timeout()) {
                    fc_elog(logger, "connection timeout - could not process request within time period. closing");
                    return derived().do_eof();
                }

                // just use sync writes for now - P. Raphael
                beast::error_code ec;
                auto bytes_written = http::write(derived().stream(), res_, ec);
                on_write(ec, bytes_written, close);
            }
    }; // end class beast_http_session

    // Handles a plain HTTP connection
    class plain_session
        : public beast_http_session<plain_session> 
        , public std::enable_shared_from_this<plain_session>
    {
        tcp_socket_t socket_;
        asio::io_context::strand strand_;

        public:      
            // Create the session
            plain_session(
                tcp_socket_t socket,
                std::shared_ptr<ssl::context> ctx,
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc
                )
                : beast_http_session<plain_session>(plugin_state, ioc)
                , socket_(std::move(socket))
                , strand_(socket_.get_executor().context())
            {}

            tcp_socket_t& stream() { return socket_; }
            tcp_socket_t& socket() { return socket_; }

            // Start the asynchronous operation
            void run()
            {
                // catch any loose exceptions so that nodeos will return zero exit code
                try {
                    do_read();
                } catch (fc::exception& e) {
                    fc_dlog( logger, "fc::exception thrown while invoking unix_socket_session::run()");
                    fc_dlog( logger, "Details: ${e}", ("e", e.to_detail_string()) );
                } catch (std::exception& e) {
                    fc_elog( logger, "STD exception thrown while invoking beast_http_session::run()");
                    fc_dlog( logger, "Exception Details: ${e}", ("e", e.what()) );
                } catch (...) {
                    fc_elog( logger, "Unknown exception thrown while invoking beast_http_session::run()");
                }
            }

            void do_eof()
            {
                // Send a TCP shutdown
                beast::error_code ec;

                socket_.shutdown(tcp::socket::shutdown_send, ec);
                // At this point the connection is closed gracefully
            }

            virtual shared_ptr<detail::abstract_conn> get_shared_from_this() override {
                return shared_from_this();
            }

            virtual bool allow_host(const http::request<http::string_body>& req) override { 
                return eosio::allow_host(req, *this);
            }
    }; // end class plain_session

    // Handles an SSL HTTP connection
    class ssl_session
        : public beast_http_session<ssl_session>
        , public std::enable_shared_from_this<ssl_session>
    {   
        asio::io_context::strand strand_;
        ssl::stream<tcp_socket_t> stream_;

        public:
            // Create the session

            ssl_session(
                tcp_socket_t socket,
                std::shared_ptr<ssl::context> ctx,
                std::shared_ptr<http_plugin_state> plugin_state,
                asio::io_context* ioc)
                : beast_http_session<ssl_session>(plugin_state, ioc)
                , strand_(socket.get_executor().context())
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
                beast::error_code ec;
                stream_.handshake(ssl::stream_base::server, ec);
                on_handshake(ec);
            }

            void on_handshake(beast::error_code ec)
            {
                
                if(ec)
                    return fail(ec, "handshake");

                do_read();
            }

            void do_eof()
            {
                // Perform the SSL shutdown
                beast::error_code ec;
                stream_.shutdown(ec);
                on_shutdown(ec);
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

            virtual bool allow_host(const http::request<http::string_body>& req) override { 
                return eosio::allow_host(req, *this);
            }
    }; // end class ssl_session


    // unix domain sockets
    class unix_socket_session 
        : public std::enable_shared_from_this<unix_socket_session>
        , public beast_http_session<unix_socket_session>
    {    
        protected:
            // The socket used to communicate with the client.
            stream_protocol::socket socket_;
            asio::io_context::strand strand_;


        public:
            unix_socket_session(stream_protocol::socket sock, 
                            std::shared_ptr<ssl::context> ctx,
                            std::shared_ptr<http_plugin_state> plugin_state, 
                            asio::io_context* ioc) 
            : beast_http_session(plugin_state, ioc)
            , socket_(std::move(sock)) 
            , strand_(*((boost::asio::io_context*)(&socket_.get_executor().context())))
            {  }

            virtual ~unix_socket_session() = default;

            virtual bool allow_host(const http::request<http::string_body>& req) override { 
                // TODO allow host make sense here ? 
                return true;
            }

            void do_eof()
            {
                // Send a TCP shutdown
                boost::system::error_code ec;
                socket_.shutdown(stream_protocol::socket::shutdown_send, ec);
                // At this point the connection is closed gracefully
            }

            bool is_secure() override { return false; };

            void run() {
                // catch any loose exceptions so that nodeos will return zero exit code
                try {
                    do_read();
                } catch (fc::exception& e) {
                    fc_dlog( logger, "fc::exception thrown while invoking unix_socket_session::run()");
                    fc_dlog( logger, "Details: ${e}", ("e", e.to_detail_string()) );
                } catch (std::exception& e) {
                    fc_elog( logger, "STD exception thrown while invoking unix_socket_session::run()");
                    fc_dlog( logger, "Exception Details: ${e}", ("e", e.what()) );
                } catch (...) {
                    fc_elog( logger, "Unknown exception thrown while invoking unix_socket_session::run()");
                }
            }

            stream_protocol::socket& stream() { return socket_; }
            stream_protocol::socket& socket() { return socket_; } 
            
            virtual shared_ptr<detail::abstract_conn> get_shared_from_this() override {
                return shared_from_this();
            }
    }; // end class unix_socket_session

} // end namespace


