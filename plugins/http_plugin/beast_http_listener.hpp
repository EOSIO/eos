#pragma once

#include "beast_http_session.hpp"
#include "common.hpp"
#include <type_traits>

extern fc::logger logger;

namespace eosio {
    // Accepts incoming connections and launches the sessions
    // T must be a subclass of 
    template<typename T> 
    //std::enable_if_t<std::is_base_of_v<beast_http_session,T> >
    class beast_http_listener : public std::enable_shared_from_this<beast_http_listener<T> >
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
            tcp::acceptor acceptor_;
            tcp::endpoint listen_ep_;
            tcp_socket_t socket_;

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

            void listen(tcp::endpoint endpoint) {
                if (isListening_) return;

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
                auto sh_fr_ths = this->shared_from_this();

#if BOOST_VERSION < 107000 
                acceptor_.async_accept(
                    socket_,
                    std::bind(
                        &beast_http_listener::on_accept,
                        sh_fr_ths,
                        std::placeholders::_1
                    )
                );
#else
                // The new connection gets its own strand
                acceptor_.async_accept(
                    asio::make_strand(*ioc_),
                    beast::bind_front_handler(
                        &beast_http_listener::on_accept,
                        sh_fr_ths));                
#endif                        

            }

#if BOOST_VERSION < 107000
            void on_accept(beast::error_code ec) {
#else
            void on_accept(beast::error_code ec, tcp::socket socket) {
#endif
                if(ec) {
                    fail(ec, "accept");
                }
                else {
                    // Create the session object and run it
                    std::make_shared<T>(
//#if BOOST_VERSION < 107000                                            
                        std::move(socket_),
// #else
//                         std::move(socket),
// #endif
                        ctx_,
                        plugin_state_,
                        ioc_)->run();        
                }

                // Accept another connection
                do_accept();
            }   

               
    }; // end class beast_http_Listener
} // end namespace