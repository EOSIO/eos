#pragma once

#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain/thread_utils.hpp> // for thread pool
#include <fc/utility.hpp>

#include <string>
#include <set>
#include <map>
#include <atomic>
#include <optional>
#include <regex>
#include <string>

#include <fc/time.hpp>
#include <fc/io/raw.hpp>
#include <fc/log/logger_config.hpp>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>

#include <boost/asio/detail/config.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_socket_iostream.hpp>
#include <boost/asio/basic_stream_socket.hpp>

namespace eosio {
    static uint16_t const uri_default_port = 80;
    /// Default port for wss://
    static uint16_t const uri_default_secure_port = 443;

    using std::string;
    using std::map;
    using std::set;

    namespace beast = boost::beast;           // from <boost/beast.hpp>
    namespace http = boost::beast::http;      // from <boost/beast/http.hpp>
    namespace asio = boost::asio;
    namespace ssl = boost::asio::ssl;
    using boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>


    namespace detail {
        /**
         * virtualized wrapper for the various underlying connection functions needed in req/resp processng
        */
        struct abstract_conn {
            virtual ~abstract_conn() = default;
            virtual bool verify_max_bytes_in_flight() = 0;
            virtual bool verify_max_requests_in_flight() = 0;
            virtual void handle_exception() = 0;

            virtual void send_response(std::optional<std::string> body, int code) = 0;
        };

        using abstract_conn_ptr = std::shared_ptr<abstract_conn>;

        /**
         * internal url handler that contains more parameters than the handlers provided by external systems
         */
        using internal_url_handler = std::function<void(abstract_conn_ptr, string, string, url_response_callback)>;

        /**
         * Helper method to calculate the "in flight" size of a fc::variant
         * This is an estimate based on fc::raw::pack if that process can be successfully executed
         *
         * @param v - the fc::variant
         * @return in flight size of v
         */
        static size_t in_flight_sizeof( const fc::variant& v ) {
           try {
              return fc::raw::pack_size( v );
           } catch( ... ) {}
           return 0;
        }

        /**
         * Helper method to calculate the "in flight" size of a std::optional<T>
         * When the optional doesn't contain value, it will return the size of 0
         *
         * @param o - the std::optional<T> where T is typename
         * @return in flight size of o
         */
        template<typename T>
        static size_t in_flight_sizeof( const std::optional<T>& o ) {
           if( o ) {
              return in_flight_sizeof( *o );
           }
           return 0;
        }

        /**
         * Helper method to calculate the "in flight" size of a string
         * @param s - the string
         * @return in flight size of s
         */
        static size_t in_flight_sizeof( const string& s ) {
            return s.size();
        }
    }

    // key -> priority, url_handler
    typedef map<string,detail::internal_url_handler> url_handlers_type;

    struct http_plugin_state {
        string                         access_control_allow_origin;
        string                         access_control_allow_headers;
        string                         access_control_max_age;
        bool                           access_control_allow_credentials = false;
        size_t                         max_body_size{1024*1024};

        std::atomic<size_t>                            bytes_in_flight{0};
        std::atomic<int32_t>                           requests_in_flight{0};
        size_t                                         max_bytes_in_flight = 0;
        int32_t                                        max_requests_in_flight = -1;
        fc::microseconds                               max_response_time{30*1000};

        std::optional<ssl::context> ctx; // only for ssl

        bool                     validate_host = true;
        set<string>              valid_hosts;

        url_handlers_type        url_handlers;
        bool                     keep_alive = false;

        uint16_t                                         thread_pool_size = 2;
        std::unique_ptr<eosio::chain::named_thread_pool> thread_pool;

        fc::logger& logger;

        explicit http_plugin_state(fc::logger& log)
            : logger(log)
        {}
    };

    /**
     * Helper type that wraps an object of type T and records its "in flight" size to
     * http_plugin_impl::bytes_in_flight using RAII semantics
     *
     * @tparam T - the contained Type
     */
    template<typename T>
    struct in_flight {
        in_flight(T&& object, std::shared_ptr<http_plugin_state> plugin_state)
        :_object(std::move(object))
        ,_plugin_state(std::move(plugin_state))
        {
            _count = detail::in_flight_sizeof(_object);
            _plugin_state->bytes_in_flight += _count;
        }

        ~in_flight() {
            if (_count) {
                _plugin_state->bytes_in_flight -= _count;
            }
        }

        // No copy constructor, but allow move
        in_flight(const in_flight&) = delete;
        in_flight(in_flight&& from)
        :_object(std::move(from._object))
        ,_count(from._count)
        ,_plugin_state(std::move(from._plugin_state))
        {
            from._count = 0;
        }

        // Delete copy/move assignment
        in_flight& operator=(const in_flight&) = delete;
        in_flight& operator=(in_flight&& from) = delete;

        /**
         * const accessor
         * @return const reference to the contained object
         */
        const T& obj() const {
            return _object;
        }

        /**
         * mutable accessor (can be moved from)
         * @return mutable reference to the contained object
         */
        T& obj() {
            return _object;
        }

        T _object;
        size_t _count;
        std::shared_ptr<http_plugin_state> _plugin_state;
    };

    /**
     * convenient wrapper to make an in_flight<T>
     */
    template<typename T>
    auto make_in_flight(T&& object, std::shared_ptr<http_plugin_state> plugin_state) {
        return std::make_shared<in_flight<T>>(std::forward<T>(object), std::move(plugin_state));
    }

    /**
     * Construct a lambda appropriate for url_response_callback that will
     * JSON-stringify the provided response
     *
     * @param plugin_state - plugin state object, shared state of http_plugin
     * @param session_ptr - beast_http_session object on which to invoke send_response
     * @return lambda suitable for url_response_callback
     */
    auto make_http_response_handler(std::shared_ptr<http_plugin_state> plugin_state, detail::abstract_conn_ptr session_ptr) {
    return [plugin_state{std::move(plugin_state)},
            session_ptr{std::move(session_ptr)}]( int code, std::optional<fc::variant> response )
         {
            auto tracked_response = make_in_flight(std::move(response), plugin_state);
            if (!session_ptr->verify_max_bytes_in_flight()) {
                return;
            }

            // post  back to an HTTP thread to to allow the response handler to be called from any thread
            boost::asio::post(  plugin_state->thread_pool->get_executor(),
                                [plugin_state, session_ptr, code, tracked_response=std::move(tracked_response)]() {
                try {
                    if( tracked_response->obj().has_value() ) {
                        std::string json = fc::json::to_string( *tracked_response->obj(), fc::time_point::now() + plugin_state->max_response_time );
                        auto tracked_json = make_in_flight( std::move( json ), plugin_state );
                        session_ptr->send_response( std::move( tracked_json->obj() ), code );
                    } else {
                        session_ptr->send_response( {}, code );
                    }
                } catch( ... ) {
                    session_ptr->handle_exception();
                }
            });
        };// end lambda
    }

    bool host_port_is_valid( const http_plugin_state& plugin_state, 
                             const std::string& header_host_port, 
                             const string& endpoint_local_host_port ) {
        return !plugin_state.validate_host 
                || header_host_port == endpoint_local_host_port 
                || plugin_state.valid_hosts.find(header_host_port) != plugin_state.valid_hosts.end();
    }

    bool host_is_valid( const http_plugin_state& plugin_state, 
                        const std::string& host, 
                        const string& endpoint_local_host_port, 
                        bool secure) {
        if (!plugin_state.validate_host) {
            return true;
        }

        // normalise the incoming host so that it always has the explicit port
        static auto has_port_expr = std::regex("[^:]:[0-9]+$"); /// ends in :<number> without a preceeding colon which implies ipv6
        if (std::regex_search(host, has_port_expr)) {
            return host_port_is_valid( plugin_state, host, endpoint_local_host_port );
        } else {
            // according to RFC 2732 ipv6 addresses should always be enclosed with brackets so we shouldn't need to special case here
            return host_port_is_valid( plugin_state, 
                                       host + ":" 
                                            + std::to_string(secure?
                                                    uri_default_secure_port 
                                                   : uri_default_port ), 
                                       endpoint_local_host_port);
        }
    }
   
} // end namespace eosio