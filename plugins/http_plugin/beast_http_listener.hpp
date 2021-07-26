#pragma once

#include "beast_http_session.hpp"
#include "common.hpp"
#include <type_traits>
#include <sstream>

extern fc::logger logger;

namespace eosio {

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
    // asio::any_io_executor,
    io_context::executor_type,
    beast::unlimited_rate_policy>;    
#endif    
#endif

    // Accepts incoming connections and launches the sessions
    // T must be a subclass of 
    template<typename session_type, typename protocol_type, typename socket_type> 
    //std::enable_if_t<std::is_base_of_v<beast_http_session,T> >
    class beast_http_listener : public std::enable_shared_from_this<beast_http_listener<session_type, protocol_type, socket_type> >
    {
        private: 
            bool isListening_; 

            // we have to use pointers here instead of references
            // because we need shared_from_this, whihc requires
            // a default constructor.  However when you use references as member 
            // varaibles, the default constuctor must be deleted
            // in the case of io_context, we have to use raw pointer because 
            // it is a stack allocated member variable fo application class
            asio::io_context* ioc_;
            std::shared_ptr<ssl::context> ctx_;
            std::shared_ptr<http_plugin_state> plugin_state_;

            typename protocol_type::acceptor acceptor_;
            typename protocol_type::endpoint listen_ep_;
            socket_type socket_;

        public:
            beast_http_listener() = default;
            beast_http_listener(const beast_http_listener&) = delete;
            beast_http_listener(beast_http_listener&&) = delete;

            beast_http_listener& operator=(const beast_http_listener&) = delete;
            beast_http_listener& operator=(beast_http_listener&&) = delete;

            beast_http_listener(asio::io_context* ioc, 
                                std::shared_ptr<ssl::context> ctx, 
                                std::shared_ptr<http_plugin_state> plugin_state) : 
                isListening_(false)
                , ioc_(ioc)
                , ctx_(ctx)
                , plugin_state_(plugin_state)
                , acceptor_(*ioc)
                , socket_(*ioc)
            { }

            virtual ~beast_http_listener() = default; 

            void listen(typename protocol_type::endpoint endpoint) {
                if (isListening_) return;

                // delete the old socket
                // TODO only need for stream protocol
                auto ss = std::stringstream();
                ss << endpoint;
                ::unlink(ss.str().c_str());

                beast::error_code ec;
                // Open the acceptor
                acceptor_.open(endpoint.protocol(), ec);
                if(ec)  {
                    fail(ec, "open");
                    return;
                }

                // Allow address reuse
                acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
                if(ec)  {
                    fail(ec, "set_option");
                    return;
                }

                // Bind to the server address
                acceptor_.bind(endpoint, ec);
                if(ec)  {
                    fail(ec, "bind");
                    return;
                }

                // Start listening for connections
                auto maxConnections = asio::socket_base::max_listen_connections;
                fc_ilog( logger, "acceptor_.listen()" ); 
                acceptor_.listen(maxConnections, ec);
                if(ec) {
                    fail(ec, "listen");
                    return;
                }
                isListening_ = true;        
            }

            // Start accepting incoming connections
            void start_accept()
            {
                if(!isListening_) return;
                do_accept();
            }

            bool is_listening() {
                return isListening_;
            }

            void stop_listening() {
                ioc_->stop();
            }

        private:
            void do_accept() {
                auto self = this->shared_from_this();
                acceptor_.async_accept(socket_, [self](beast::error_code ec) {
                        if(ec) {
                            fail(ec, "accept");
                        }
                        else {
                            // Create the session object and run it
                            std::make_shared<session_type>(
                                std::move(self->socket_),
                                self->ctx_,
                                self->plugin_state_,
                                self->ioc_)->run();        
                        }

                        // Accept another connection
                        self->do_accept();
                    }
                );
            }               
    }; // end class beast_http_Listener
} // end namespace