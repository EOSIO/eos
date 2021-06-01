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
            // varaibles, you must the default constuctor must be deleted
            // inthe case of io_context, we have to use raw pointer because 
            // it is a stack allocated member variable fo application class
            asio::io_context* ioc_;
            std::shared_ptr<ssl::context> ctx_;
            std::shared_ptr<http_plugin_state> plugin_state_;
            tcp::acceptor acceptor_;
            tcp::endpoint listen_ep_;

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
                , acceptor_(asio::make_strand(*ioc))
                { fc_ilog( logger, "constructor called" ); }

            virtual ~beast_http_listener() = default; 

            void listen(tcp::endpoint endpoint) {
                fc_ilog( logger, "acceptor_.open()" ); 

                if (isListening_) return;

                beast::error_code ec;
                // Open the acceptor
                acceptor_.open(endpoint.protocol(), ec);
                if(ec)  {
                    fail(ec, "open");
                    return;
                }


                fc_ilog( logger, "acceptor_.set_option()" ); 
                // Allow address reuse
                acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
                if(ec)  {
                    fail(ec, "set_option");
                    return;
                }

                fc_ilog( logger, "acceptor_.bind()" ); 
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
                fc_ilog( logger, "shared_from_this()" ); 
                auto sh_fr_ths = this->shared_from_this();

                fc_ilog( logger, "acceptor_.async_accept()" ); 
                // The new connection gets its own strand
                acceptor_.async_accept(
                    asio::make_strand(*ioc_),
                    beast::bind_front_handler(
                        &beast_http_listener::on_accept,
                        sh_fr_ths));
                fc_ilog( logger, "acceptor_.async_accept() done" ); 
            }

            void on_accept(beast::error_code ec, tcp::socket socket) {
                if(ec) {
                    fail(ec, "accept");
                }
                else {
                    fc_ilog( logger, "launch session" ); 
                    // Create the session object and run it
                    std::make_shared<T>(
                        std::move(socket),
                        ctx_,
                        plugin_state_,
                        ioc_)->run();        
                }

                // Accept another connection
                do_accept();
            }   

               
    }; // end class beast_http_Listener
} // end namespace