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

extern fc::logger logger;

namespace eosio {
    static uint16_t const uri_default_port = 80;
    /// Default port for wss://
    static uint16_t const uri_default_secure_port = 443;

    using std::string;
    using std::map;
    using std::set;

    namespace beast = boost::beast;                 // from <boost/beast.hpp>
    namespace http = boost::beast::http;                   // from <boost/beast/http.hpp>
    namespace asio = boost::asio;
    namespace ssl = boost::asio::ssl;
    using boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

    // host/port pair 
    typedef std::pair<string, uint16_t> host_port_t;

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
            } catch(...) {}
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

        bool                      validate_host = true;
        // valid hosts as pair of <host, port>
        set<host_port_t> valid_hosts;

        bool verbose_http_errors = false;

        url_handlers_type  url_handlers;
        bool                     keep_alive = false;
        bool                     ipv6 = false;
    };

    /**
     * Breakds down addresses (either domain or IPv4/IPv6) into host/port combo
     * @param addr_str input address string as domain, IPv4 or IPv6 address, Example: '127.0.0.1:8000', 'localhost:8888'
     * @param default_port_num input the default port number if port not present in address string
     * 
     * @returns Pair of <string, uint16_t> representing host and port number
     */
    host_port_t get_host_port(const string& addr_str, uint16_t default_port_num) {
        string host_str;
        string port_str = "";
        uint16_t port_num = default_port_num;

        // determine if IPv6 or IPv4 address   
        // for now we will assume addresses which contain '[' are IPv6, otehrwise IPv4
        auto idx_open_br = addr_str.find('[');

        fc_dlog(logger, "addr_str: ${a} idx_open_br ${i}", ("a", addr_str)("i", idx_open_br));
        // IPv4 or domain name
        if (idx_open_br == string::npos) { 
            auto idx_colon = addr_str.find(':');
            auto nchars = std::min(addr_str.size(), idx_colon);
            host_str = addr_str.substr(0, idx_colon);
            if (idx_colon < addr_str.size() - 1) {
                nchars = addr_str.size() - (idx_colon + 1);
                port_str = addr_str.substr( idx_colon + 1, nchars);
            } else {
                host_str = addr_str;
            }
        }
        // IPv6 
        else { 
            // IPv6 addresses which specfiy a port # must use '[address]:port' scheme
            auto idx_close_br = addr_str.find(']');
            auto nchars = std::min(addr_str.size(), idx_close_br) - (idx_open_br + 1);
            host_str = addr_str.substr(idx_open_br + 1, nchars);
            if (idx_close_br != string::npos) {
                if (idx_close_br < addr_str.size() - 2) {
                    nchars = addr_str.size() - (idx_close_br + 2);
                    port_str = addr_str.substr(idx_close_br + 2, nchars);
                }
            }
        }   
        fc_dlog(logger, "host_str: ${h} port_str ${p}", ("h", host_str)("p", port_str));
        
        if (port_str != "") {
            // attempt to convert port number to string
            try { 
                auto port_num_ul = std::stoul(port_str);
                if (port_num_ul > 65535)
                    throw std::out_of_range("port number out of range 0 to 65535");
                port_num = static_cast<uint16_t>(port_num_ul);
            } catch ( const std::exception& e ) {
                fc_elog(logger,  "port number conversion to  failed: addr_str= '${a}' host_str: '${h}' port_str: '${p}' exception: ${e}", 
                         ("a", addr_str)("h", host_str)("p", port_str)("e", e.what()) );
                throw;
            }
        }

        return std::make_pair(host_str, port_num);
    }

    /**
     * Helper type that wraps an object of type T and records its "in flight" size to
     * http_plugin_impl::bytes_in_flight using RAII semantics
     *
     * @tparam T - the contained Type
     */
    template<typename T>
    struct in_flight {
        in_flight(T&& object, http_plugin_state& plugin_state)
        :_object(std::move(object))
        ,_plugin_state(plugin_state)
        {
            _count = detail::in_flight_sizeof(_object);
            _plugin_state.bytes_in_flight += _count;
        }

        ~in_flight() {
            if (_count) {
                _plugin_state.bytes_in_flight -= _count;
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
        http_plugin_state& _plugin_state;
    };

    /**
     * convenient wrapper to make an in_flight<T>
     */
    template<typename T>
    auto make_in_flight(T&& object, http_plugin_state& plugin_state) {
        return std::make_shared<in_flight<T>>(std::forward<T>(object), plugin_state);
    }

    /**
     * Construct a lambda appropriate for url_response_callback that will
     * JSON-stringify the provided response
     *
     * @param thread_pool - the thread pool object, executor to pass to asio::post() method
     * @param plugin_state - plugin state object, shared state of http_plugin
     * @param session_ptr - beast_http_session object on which to invoke send_response
     * @return lambda suitable for url_response_callback
     */
    auto make_http_response_handler(std::shared_ptr<eosio::chain::named_thread_pool> thread_pool, std::shared_ptr<http_plugin_state> plugin_state, detail::abstract_conn_ptr session_ptr) {
    return [thread_pool{std::move(thread_pool)}, 
           plugin_state{std::move(plugin_state)}, 
           session_ptr{std::move(session_ptr)}] ( int code, std::optional<fc::variant> response )
         {
            auto tracked_response = make_in_flight(std::move(response), *plugin_state);
            if (!session_ptr->verify_max_bytes_in_flight()) {
                return;
            }

            // post  back to an HTTP thread to to allow the response handler to be called from any thread
            boost::asio::post(  thread_pool->get_executor(),
                                [plugin_state, session_ptr, code, tracked_response=std::move(tracked_response)]() {
                try {
                    if( tracked_response->obj().has_value() ) {
                        std::string json = fc::json::to_string( *tracked_response->obj(), fc::time_point::now() + plugin_state->max_response_time );
                        auto tracked_json = make_in_flight( std::move( json ), *plugin_state );
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
                             const host_port_t& header_host_port, 
                             const host_port_t& endpoint_local_host_port ) {
        return !plugin_state.validate_host 
                || header_host_port == endpoint_local_host_port 
                || plugin_state.valid_hosts.find(header_host_port) != plugin_state.valid_hosts.end();
    }

    bool host_is_valid( const http_plugin_state& plugin_state, 
                        const string& host, 
                        const host_port_t& endpoint_local_host_port, 
                        bool secure) {
        if (!plugin_state.validate_host) {
            return true;
        }

        // normalise the incoming host so that it always has the explicit port
        string host_str;
        auto default_port = secure? uri_default_secure_port : uri_default_port;
        auto host_port = get_host_port(host, default_port);

        return host_port_is_valid( plugin_state, host_port, endpoint_local_host_port );
    }
   
} // end namespace eosio