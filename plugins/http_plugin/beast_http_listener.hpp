#pragma once

#include "beast_http_session.hpp"
#include "common.hpp"
#include <type_traits>
#include <sstream>

namespace eosio {
    // Accepts incoming connections and launches the sessions
    // session_type should be a subclass of beast_http_session
    // protocol type must have sub types acceptor and endpoint, e.g. boost::asio::ip::tcp;
    // socket type must be the socket e.g, boost::asio::ip::tcp::socket
    template<typename session_type, typename protocol_type, typename socket_type> 
    
    class beast_http_listener : public std::enable_shared_from_this<beast_http_listener<session_type, protocol_type, socket_type> >
    {
        private: 
            bool isListening_; 

            std::shared_ptr<eosio::chain::named_thread_pool> thread_pool_;
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

            beast_http_listener(std::shared_ptr<eosio::chain::named_thread_pool> thread_pool, 
                                std::shared_ptr<ssl::context> ctx, 
                                std::shared_ptr<http_plugin_state> plugin_state) : 
                isListening_(false)
                , thread_pool_(thread_pool)
                , ctx_(ctx)
                , plugin_state_(plugin_state)
                , acceptor_(thread_pool_->get_executor())
                , socket_(thread_pool_->get_executor())
            { }

            virtual ~beast_http_listener() {
                try {
                     stop_listening();
                } catch( ... ) { }
            }; 

            void listen(typename protocol_type::endpoint endpoint) {
                if (isListening_) return;

                // for unix sockets we should delete the old socket
                if(std::is_same<socket_type, stream_protocol::socket>::value) { 
                    auto ss = std::stringstream();
                    ss << endpoint;
                    ::unlink(ss.str().c_str());
                }

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
                if(isListening_) {
                    thread_pool_->stop();
                    isListening_ = false;
                }
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
                                self->plugin_state_)->run_session();        
                        }

                        // Accept another connection
                        self->do_accept();
                    }
                );
            }               
    }; // end class beast_http_Listener
} // end namespace