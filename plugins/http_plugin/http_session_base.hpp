// base class for HTTP session objects

#pragma once

#include "common.hpp"

extern fc::logger logger;

namespace eosio { 
    using std::shared_ptr;

    // Handles an HTTP server connection.    
    class http_session_base : public detail::abstract_conn {
        protected:
            // HTTP parser object
            http::request_parser<http::string_body> req_parser_;

            // HTTP response object
            http::response<http::string_body> res_;

            asio::io_context *ioc_;
            std::shared_ptr<http_plugin_state> plugin_state_;

            virtual bool is_secure() { return false; }

            virtual bool allow_host(const http::request<http::string_body>& req) = 0;

            virtual shared_ptr<detail::abstract_conn> get_shared_from_this() = 0;

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
                
                auto tgtStr = req.target().data();
                fc_ilog( logger, "detect bad target ${tgt}", ("tgt", tgtStr) ); 
                // Request path must be absolute and not contain "..".
                if( req.target().empty() ||
                    req.target()[0] != '/' ||
                    req.target().find("..") != beast::string_view::npos)
                    return bad_request("Illegal request-target", *this);

                // Cache the size since we need it after the move
                // auto const size = body.size();

                try {
                    if(!allow_host(req))
                        return;

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

                    // verfiy bytes in flight/requests in flight
                    if( !verify_max_bytes_in_flight() || !verify_max_requests_in_flight() ) return;

                    std::string resource = std::string(req.target());
                    // look for the URL handler to handle this reosouce
                    auto handler_itr = plugin_state_->url_handlers.find( resource );
                    if( handler_itr != plugin_state_->url_handlers.end()) {
                        std::string body = req.body();
                        fc_ilog( logger, "invoking handler_itr->second() body.size()= ${bs}", ("bs", body.size()) ); 
                        handler_itr->second( get_shared_from_this(), 
                                            std::move( resource ), 
                                            std::move( body ), 
                                            make_http_response_handler(*ioc_, *plugin_state_, get_shared_from_this()) );
                    } else {
                        fc_dlog( logger, "404 - not found: ${ep}", ("ep", resource) );
                        not_found(resource, *this);                    
                    }
                } catch( ... ) {
                    handle_exception();
                }           
            }

            void report_429_error(const std::string & what) {                
                send_response(std::string(what), 
                            static_cast<int>(http::status::too_many_requests));                
            }

        public:
            http_session_base(std::shared_ptr<http_plugin_state> plugin_state,
                         asio::io_context* ioc) 
                : ioc_(ioc)
                , plugin_state_(plugin_state) 
            {
                plugin_state_->requests_in_flight += 1;
                req_parser_.body_limit(plugin_state_->max_body_size);
            }

            virtual ~http_session_base() {
                plugin_state_->requests_in_flight -= 1;
            }

            virtual bool verify_max_bytes_in_flight() override {
                auto bytes_in_flight_size = plugin_state_->bytes_in_flight.load();
                if( bytes_in_flight_size > plugin_state_->max_bytes_in_flight ) {
                    fc_dlog( logger, "429 - too many bytes in flight: ${bytes}", ("bytes", bytes_in_flight_size) );
                    string what = "Too many bytes in flight: " + std::to_string( bytes_in_flight_size ) + ". Try again later.";;
                    report_429_error(what);
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
                    string what = "Too many requests in flight: " + std::to_string( requests_in_flight_num ) + ". Try again later.";
                    report_429_error(what);
                    return false;
                }
                return true;
            }           
    }; // end class
} // end namespace