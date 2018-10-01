#pragma once

#include <websocketpp/transport/base/endpoint.hpp>
#include <websocketpp/transport/asio/connection.hpp>
#include <websocketpp/transport/asio/security/none.hpp>

#include <websocketpp/uri.hpp>
#include <websocketpp/logger/levels.hpp>

#include <websocketpp/common/functional.hpp>

#include <sstream>
#include <string>

namespace websocketpp {
namespace transport {
namespace asio {

namespace basic_socket {

class local_connection : public lib::enable_shared_from_this<local_connection> {
public:
    /// Type of this connection socket component
    typedef local_connection type;
    /// Type of a shared pointer to this connection socket component
    typedef lib::shared_ptr<type> ptr;

    /// Type of a pointer to the Asio io_service being used
    typedef lib::asio::io_service* io_service_ptr;
    /// Type of a pointer to the Asio io_service strand being used
    typedef lib::shared_ptr<lib::asio::io_service::strand> strand_ptr;
    /// Type of the ASIO socket being used
    typedef lib::asio::local::stream_protocol::socket socket_type;
    /// Type of a shared pointer to the socket being used.
    typedef lib::shared_ptr<socket_type> socket_ptr;

    explicit local_connection() : m_state(UNINITIALIZED) {
    }

    ptr get_shared() {
        return shared_from_this();
    }

    bool is_secure() const {
        return false;
    }

    lib::asio::local::stream_protocol::socket & get_socket() {
        return *m_socket;
    }

    lib::asio::local::stream_protocol::socket & get_next_layer() {
        return *m_socket;
    }

    lib::asio::local::stream_protocol::socket & get_raw_socket() {
        return *m_socket;
    }

    std::string get_remote_endpoint(lib::error_code & ec) const {
        return "UNIX Socket Endpoint";
    }

   void pre_init(init_handler callback) {
      if (m_state != READY) {
         callback(socket::make_error_code(socket::error::invalid_state));
         return;
      }

      m_state = READING;

      callback(lib::error_code());
   }

   void post_init(init_handler callback) {
        callback(lib::error_code());
   }
protected:
    lib::error_code init_asio (io_service_ptr service, strand_ptr, bool)
    {
        if (m_state != UNINITIALIZED) {
            return socket::make_error_code(socket::error::invalid_state);
        }

        m_socket = lib::make_shared<lib::asio::local::stream_protocol::socket>(
            lib::ref(*service));

        m_state = READY;

        return lib::error_code();
    }

    void set_handle(connection_hdl hdl) {
        m_hdl = hdl;
    }

    lib::asio::error_code cancel_socket() {
        lib::asio::error_code ec;
        m_socket->cancel(ec);
        return ec;
    }

    void async_shutdown(socket::shutdown_handler h) {
        lib::asio::error_code ec;
        m_socket->shutdown(lib::asio::ip::tcp::socket::shutdown_both, ec);
        h(ec);
    }

    lib::error_code get_ec() const {
        return lib::error_code();
    }

    template <typename ErrorCodeType>
    lib::error_code translate_ec(ErrorCodeType) {
        return make_error_code(transport::error::pass_through);
    }
    
    lib::error_code translate_ec(lib::error_code ec) {
        return ec;
    }
private:
    enum state {
        UNINITIALIZED = 0,
        READY = 1,
        READING = 2
    };

    socket_ptr          m_socket;
    state               m_state;

    connection_hdl      m_hdl;
    socket_init_handler m_socket_init_handler;
};

class local_endpoint {
public:
    /// The type of this endpoint socket component
    typedef local_endpoint type;

    /// The type of the corresponding connection socket component
    typedef local_connection socket_con_type;
    /// The type of a shared pointer to the corresponding connection socket
    /// component.
    typedef socket_con_type::ptr socket_con_ptr;

    explicit local_endpoint() {}

    bool is_secure() const {
        return false;
    }
};
}

/// Asio based endpoint transport component
/**
 * transport::asio::endpoint implements an endpoint transport component using
 * Asio.
 */
template <typename config>
class local_endpoint : public config::socket_type {
public:
    /// Type of this endpoint transport component
    typedef local_endpoint<config> type;

    /// Type of the concurrency policy
    typedef typename config::concurrency_type concurrency_type;
    /// Type of the socket policy
    typedef typename config::socket_type socket_type;
    /// Type of the error logging policy
    typedef typename config::elog_type elog_type;
    /// Type of the access logging policy
    typedef typename config::alog_type alog_type;

    /// Type of the socket connection component
    typedef typename socket_type::socket_con_type socket_con_type;
    /// Type of a shared pointer to the socket connection component
    typedef typename socket_con_type::ptr socket_con_ptr;

    /// Type of the connection transport component associated with this
    /// endpoint transport component
    typedef asio::connection<config> transport_con_type;
    /// Type of a shared pointer to the connection transport component
    /// associated with this endpoint transport component
    typedef typename transport_con_type::ptr transport_con_ptr;

    /// Type of a pointer to the ASIO io_service being used
    typedef lib::asio::io_service * io_service_ptr;
    /// Type of a shared pointer to the acceptor being used
    typedef lib::shared_ptr<lib::asio::local::stream_protocol::acceptor> acceptor_ptr;
    /// Type of timer handle
    typedef lib::shared_ptr<lib::asio::steady_timer> timer_ptr;
    /// Type of a shared pointer to an io_service work object
    typedef lib::shared_ptr<lib::asio::io_service::work> work_ptr;

    // generate and manage our own io_service
    explicit local_endpoint()
      : m_io_service(NULL)
      , m_state(UNINITIALIZED)
    {
        //std::cout << "transport::asio::endpoint constructor" << std::endl;
    }

    ~local_endpoint() {
        if (m_acceptor && m_state == LISTENING)
            ::unlink(m_acceptor->local_endpoint().path().c_str());

        // Explicitly destroy local objects
        m_acceptor.reset();
        m_work.reset();
    }

    /// transport::asio objects are moveable but not copyable or assignable.
    /// The following code sets this situation up based on whether or not we
    /// have C++11 support or not
#ifdef _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
    local_endpoint(const local_endpoint & src) = delete;
    local_endpoint& operator= (const local_endpoint & rhs) = delete;
#else
private:
    local_endpoint(const local_endpoint & src);
    local_endpoint & operator= (const local_endpoint & rhs);
public:
#endif // _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_

#ifdef _WEBSOCKETPP_MOVE_SEMANTICS_
    local_endpoint (local_endpoint && src)
      : config::socket_type(std::move(src))
      , m_tcp_pre_init_handler(src.m_tcp_pre_init_handler)
      , m_tcp_post_init_handler(src.m_tcp_post_init_handler)
      , m_io_service(src.m_io_service)
      , m_acceptor(src.m_acceptor)
      , m_elog(src.m_elog)
      , m_alog(src.m_alog)
      , m_state(src.m_state)
    {
        src.m_io_service = NULL;
        src.m_acceptor = NULL;
        src.m_state = UNINITIALIZED;
    }

#endif // _WEBSOCKETPP_MOVE_SEMANTICS_

    /// Return whether or not the endpoint produces secure connections.
    bool is_secure() const {
        return socket_type::is_secure();
    }

    /// initialize asio transport with external io_service (exception free)
    /**
     * Initialize the ASIO transport policy for this endpoint using the provided
     * io_service object. asio_init must be called exactly once on any endpoint
     * that uses transport::asio before it can be used.
     *
     * @param ptr A pointer to the io_service to use for asio events
     * @param ec Set to indicate what error occurred, if any.
     */
    void init_asio(io_service_ptr ptr, lib::error_code & ec) {
        if (m_state != UNINITIALIZED) {
            m_elog->write(log::elevel::library,
                "asio::init_asio called from the wrong state");
            using websocketpp::error::make_error_code;
            ec = make_error_code(websocketpp::error::invalid_state);
            return;
        }

        m_alog->write(log::alevel::devel,"asio::init_asio");

        m_io_service = ptr;
        m_acceptor = lib::make_shared<lib::asio::local::stream_protocol::acceptor>(
            lib::ref(*m_io_service));

        m_state = READY;
        ec = lib::error_code();
    }

    /// initialize asio transport with external io_service
    /**
     * Initialize the ASIO transport policy for this endpoint using the provided
     * io_service object. asio_init must be called exactly once on any endpoint
     * that uses transport::asio before it can be used.
     *
     * @param ptr A pointer to the io_service to use for asio events
     */
    void init_asio(io_service_ptr ptr) {
        lib::error_code ec;
        init_asio(ptr,ec);
        if (ec) { throw exception(ec); }
    }

    /// Sets the tcp pre init handler
    /**
     * The tcp pre init handler is called after the raw tcp connection has been
     * established but before any additional wrappers (proxy connects, TLS
     * handshakes, etc) have been performed.
     *
     * @since 0.3.0
     *
     * @param h The handler to call on tcp pre init.
     */
    void set_tcp_pre_init_handler(tcp_init_handler h) {
        m_tcp_pre_init_handler = h;
    }

    /// Sets the tcp pre init handler (deprecated)
    /**
     * The tcp pre init handler is called after the raw tcp connection has been
     * established but before any additional wrappers (proxy connects, TLS
     * handshakes, etc) have been performed.
     *
     * @deprecated Use set_tcp_pre_init_handler instead
     *
     * @param h The handler to call on tcp pre init.
     */
    void set_tcp_init_handler(tcp_init_handler h) {
        set_tcp_pre_init_handler(h);
    }

    /// Sets the tcp post init handler
    /**
     * The tcp post init handler is called after the tcp connection has been
     * established and all additional wrappers (proxy connects, TLS handshakes,
     * etc have been performed. This is fired before any bytes are read or any
     * WebSocket specific handshake logic has been performed.
     *
     * @since 0.3.0
     *
     * @param h The handler to call on tcp post init.
     */
    void set_tcp_post_init_handler(tcp_init_handler h) {
        m_tcp_post_init_handler = h;
    }

    /// Retrieve a reference to the endpoint's io_service
    /**
     * The io_service may be an internal or external one. This may be used to
     * call methods of the io_service that are not explicitly wrapped by the
     * endpoint.
     *
     * This method is only valid after the endpoint has been initialized with
     * `init_asio`. No error will be returned if it isn't.
     *
     * @return A reference to the endpoint's io_service
     */
    lib::asio::io_service & get_io_service() {
        return *m_io_service;
    }

    /// Set up endpoint for listening manually (exception free)
    /**
     * Bind the internal acceptor using the specified settings. The endpoint
     * must have been initialized by calling init_asio before listening.
     *
     * @param ep An endpoint to read settings from
     * @param ec Set to indicate what error occurred, if any.
     */
    void listen(lib::asio::local::stream_protocol::endpoint const & ep, lib::error_code & ec)
    {
        if (m_state != READY) {
            m_elog->write(log::elevel::library,
                "asio::listen called from the wrong state");
            using websocketpp::error::make_error_code;
            ec = make_error_code(websocketpp::error::invalid_state);
            return;
        }

        m_alog->write(log::alevel::devel,"asio::listen");

        lib::asio::error_code bec;

        {
            boost::system::error_code test_ec;
            lib::asio::local::stream_protocol::socket test_socket(get_io_service());
            test_socket.connect(ep, test_ec);

            //looks like a service is already running on that socket, probably another keosd, don't touch it
            if(test_ec == boost::system::errc::success)
               bec = boost::system::errc::make_error_code(boost::system::errc::address_in_use);
            //socket exists but no one home, go ahead and remove it and continue on
            else if(test_ec == boost::system::errc::connection_refused)
               ::unlink(ep.path().c_str());
            else if(test_ec != boost::system::errc::no_such_file_or_directory)
               bec = test_ec;
        }

        if (!bec) {
            m_acceptor->open(ep.protocol(),bec);
        }
        if (!bec) {
            m_acceptor->bind(ep,bec);
        }
        if (!bec) {
            m_acceptor->listen(boost::asio::socket_base::max_listen_connections,bec);
        }
        if (bec) {
            if (m_acceptor->is_open()) {
                m_acceptor->close();
            }
            log_err(log::elevel::info,"asio listen",bec);
            ec = bec;
        } else {
            m_state = LISTENING;
            ec = lib::error_code();
        }
    }

    /// Set up endpoint for listening manually
    /**
     * Bind the internal acceptor using the settings specified by the endpoint e
     *
     * @param ep An endpoint to read settings from
     */
    void listen(lib::asio::local::stream_protocol::endpoint const & ep) {
        lib::error_code ec;
        listen(ep,ec);
        if (ec) { throw exception(ec); }
    }

    /// Stop listening (exception free)
    /**
     * Stop listening and accepting new connections. This will not end any
     * existing connections.
     *
     * @since 0.3.0-alpha4
     * @param ec A status code indicating an error, if any.
     */
    void stop_listening(lib::error_code & ec) {
        if (m_state != LISTENING) {
            m_elog->write(log::elevel::library,
                "asio::listen called from the wrong state");
            using websocketpp::error::make_error_code;
            ec = make_error_code(websocketpp::error::invalid_state);
            return;
        }

        ::unlink(m_acceptor->local_endpoint().path().c_str());
        m_acceptor->close();
        m_state = READY;
        ec = lib::error_code();
    }

    /// Stop listening
    /**
     * Stop listening and accepting new connections. This will not end any
     * existing connections.
     *
     * @since 0.3.0-alpha4
     */
    void stop_listening() {
        lib::error_code ec;
        stop_listening(ec);
        if (ec) { throw exception(ec); }
    }

    /// Check if the endpoint is listening
    /**
     * @return Whether or not the endpoint is listening.
     */
    bool is_listening() const {
        return (m_state == LISTENING);
    }

    /// wraps the run method of the internal io_service object
    std::size_t run() {
        return m_io_service->run();
    }

    /// wraps the run_one method of the internal io_service object
    /**
     * @since 0.3.0-alpha4
     */
    std::size_t run_one() {
        return m_io_service->run_one();
    }

    /// wraps the stop method of the internal io_service object
    void stop() {
        m_io_service->stop();
    }

    /// wraps the poll method of the internal io_service object
    std::size_t poll() {
        return m_io_service->poll();
    }

    /// wraps the poll_one method of the internal io_service object
    std::size_t poll_one() {
        return m_io_service->poll_one();
    }

    /// wraps the reset method of the internal io_service object
    void reset() {
        m_io_service->reset();
    }

    /// wraps the stopped method of the internal io_service object
    bool stopped() const {
        return m_io_service->stopped();
    }

    /// Marks the endpoint as perpetual, stopping it from exiting when empty
    /**
     * Marks the endpoint as perpetual. Perpetual endpoints will not
     * automatically exit when they run out of connections to process. To stop
     * a perpetual endpoint call `end_perpetual`.
     *
     * An endpoint may be marked perpetual at any time by any thread. It must be
     * called either before the endpoint has run out of work or before it was
     * started
     *
     * @since 0.3.0
     */
    void start_perpetual() {
        m_work = lib::make_shared<lib::asio::io_service::work>(
            lib::ref(*m_io_service)
        );
    }

    /// Clears the endpoint's perpetual flag, allowing it to exit when empty
    /**
     * Clears the endpoint's perpetual flag. This will cause the endpoint's run
     * method to exit normally when it runs out of connections. If there are
     * currently active connections it will not end until they are complete.
     *
     * @since 0.3.0
     */
    void stop_perpetual() {
        m_work.reset();
    }

    /// Call back a function after a period of time.
    /**
     * Sets a timer that calls back a function after the specified period of
     * milliseconds. Returns a handle that can be used to cancel the timer.
     * A cancelled timer will return the error code error::operation_aborted
     * A timer that expired will return no error.
     *
     * @param duration Length of time to wait in milliseconds
     * @param callback The function to call back when the timer has expired
     * @return A handle that can be used to cancel the timer if it is no longer
     * needed.
     */
    timer_ptr set_timer(long duration, timer_handler callback) {
        timer_ptr new_timer = lib::make_shared<lib::asio::steady_timer>(
            *m_io_service,
             lib::asio::milliseconds(duration)
        );

        new_timer->async_wait(
            lib::bind(
                &type::handle_timer,
                this,
                new_timer,
                callback,
                lib::placeholders::_1
            )
        );

        return new_timer;
    }

    /// Timer handler
    /**
     * The timer pointer is included to ensure the timer isn't destroyed until
     * after it has expired.
     *
     * @param t Pointer to the timer in question
     * @param callback The function to call back
     * @param ec A status code indicating an error, if any.
     */
    void handle_timer(timer_ptr, timer_handler callback,
        lib::asio::error_code const & ec)
    {
        if (ec) {
            if (ec == lib::asio::error::operation_aborted) {
                callback(make_error_code(transport::error::operation_aborted));
            } else {
                m_elog->write(log::elevel::info,
                    "asio handle_timer error: "+ec.message());
                log_err(log::elevel::info,"asio handle_timer",ec);
                callback(ec);
            }
        } else {
            callback(lib::error_code());
        }
    }

    /// Accept the next connection attempt and assign it to con (exception free)
    /**
     * @param tcon The connection to accept into.
     * @param callback The function to call when the operation is complete.
     * @param ec A status code indicating an error, if any.
     */
    void async_accept(transport_con_ptr tcon, accept_handler callback,
        lib::error_code & ec)
    {
        if (m_state != LISTENING) {
            using websocketpp::error::make_error_code;
            ec = make_error_code(websocketpp::error::async_accept_not_listening);
            return;
        }

        m_alog->write(log::alevel::devel, "asio::async_accept");

        if (config::enable_multithreading) {
            m_acceptor->async_accept(
                tcon->get_raw_socket(),
                tcon->get_strand()->wrap(lib::bind(
                    &type::handle_accept,
                    this,
                    callback,
                    lib::placeholders::_1
                ))
            );
        } else {
            m_acceptor->async_accept(
                tcon->get_raw_socket(),
                lib::bind(
                    &type::handle_accept,
                    this,
                    callback,
                    lib::placeholders::_1
                )
            );
        }
    }

    /// Accept the next connection attempt and assign it to con.
    /**
     * @param tcon The connection to accept into.
     * @param callback The function to call when the operation is complete.
     */
    void async_accept(transport_con_ptr tcon, accept_handler callback) {
        lib::error_code ec;
        async_accept(tcon,callback,ec);
        if (ec) { throw exception(ec); }
    }
protected:
    /// Initialize logging
    /**
     * The loggers are located in the main endpoint class. As such, the
     * transport doesn't have direct access to them. This method is called
     * by the endpoint constructor to allow shared logging from the transport
     * component. These are raw pointers to member variables of the endpoint.
     * In particular, they cannot be used in the transport constructor as they
     * haven't been constructed yet, and cannot be used in the transport
     * destructor as they will have been destroyed by then.
     */
    void init_logging(alog_type* a, elog_type* e) {
        m_alog = a;
        m_elog = e;
    }

    void handle_accept(accept_handler callback, lib::asio::error_code const & 
        asio_ec)
    {
        lib::error_code ret_ec;

        m_alog->write(log::alevel::devel, "asio::handle_accept");

        if (asio_ec) {
            if (asio_ec == lib::asio::errc::operation_canceled) {
                ret_ec = make_error_code(websocketpp::error::operation_canceled);
            } else {
                log_err(log::elevel::info,"asio handle_accept",asio_ec);
                ret_ec = asio_ec;
            }
        }

        callback(ret_ec);
    }

    /// Asio connect timeout handler
    /**
     * The timer pointer is included to ensure the timer isn't destroyed until
     * after it has expired.
     *
     * @param tcon Pointer to the transport connection that is being connected
     * @param con_timer Pointer to the timer in question
     * @param callback The function to call back
     * @param ec A status code indicating an error, if any.
     */
    void handle_connect_timeout(transport_con_ptr tcon, timer_ptr,
        connect_handler callback, lib::error_code const & ec)
    {
        lib::error_code ret_ec;

        if (ec) {
            if (ec == transport::error::operation_aborted) {
                m_alog->write(log::alevel::devel,
                    "asio handle_connect_timeout timer cancelled");
                return;
            }

            log_err(log::elevel::devel,"asio handle_connect_timeout",ec);
            ret_ec = ec;
        } else {
            ret_ec = make_error_code(transport::error::timeout);
        }

        m_alog->write(log::alevel::devel,"TCP connect timed out");
        tcon->cancel_socket_checked();
        callback(ret_ec);
    }

    void handle_connect(transport_con_ptr tcon, timer_ptr con_timer,
        connect_handler callback, lib::asio::error_code const & ec)
    {
        if (ec == lib::asio::error::operation_aborted ||
            lib::asio::is_neg(con_timer->expires_from_now()))
        {
            m_alog->write(log::alevel::devel,"async_connect cancelled");
            return;
        }

        con_timer->cancel();

        if (ec) {
            log_err(log::elevel::info,"asio async_connect",ec);
            callback(ec);
            return;
        }

        if (m_alog->static_test(log::alevel::devel)) {
            m_alog->write(log::alevel::devel,
                "Async connect to "+tcon->get_remote_endpoint()+" successful.");
        }

        callback(lib::error_code());
    }

    /// Initialize a connection
    /**
     * init is called by an endpoint once for each newly created connection.
     * It's purpose is to give the transport policy the chance to perform any
     * transport specific initialization that couldn't be done via the default
     * constructor.
     *
     * @param tcon A pointer to the transport portion of the connection.
     *
     * @return A status code indicating the success or failure of the operation
     */
    lib::error_code init(transport_con_ptr tcon) {
        m_alog->write(log::alevel::devel, "transport::asio::init");

        lib::error_code ec;

        ec = tcon->init_asio(m_io_service);
        if (ec) {return ec;}

        tcon->set_tcp_pre_init_handler(m_tcp_pre_init_handler);
        tcon->set_tcp_post_init_handler(m_tcp_post_init_handler);

        return lib::error_code();
    }
private:
    /// Convenience method for logging the code and message for an error_code
    template <typename error_type>
    void log_err(log::level l, char const * msg, error_type const & ec) {
        std::stringstream s;
        s << msg << " error: " << ec << " (" << ec.message() << ")";
        m_elog->write(l,s.str());
    }

    enum state {
        UNINITIALIZED = 0,
        READY = 1,
        LISTENING = 2
    };

    // Handlers
    tcp_init_handler    m_tcp_pre_init_handler;
    tcp_init_handler    m_tcp_post_init_handler;

    // Network Resources
    io_service_ptr      m_io_service;
    acceptor_ptr        m_acceptor;
    work_ptr            m_work;

    elog_type* m_elog;
    alog_type* m_alog;

    // Transport state
    state               m_state;
};

} // namespace asio
} // namespace transport
} // namespace websocketpp
