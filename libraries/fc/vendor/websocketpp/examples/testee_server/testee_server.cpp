/*
 * Copyright (c) 2015, Peter Thorson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the WebSocket++ Project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PETER THORSON BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>
#include <iostream>

struct testee_config : public websocketpp::config::asio {
    // pull default settings from our core config
    typedef websocketpp::config::asio core;

    typedef core::concurrency_type concurrency_type;
    typedef core::request_type request_type;
    typedef core::response_type response_type;
    typedef core::message_type message_type;
    typedef core::con_msg_manager_type con_msg_manager_type;
    typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;

    typedef core::alog_type alog_type;
    typedef core::elog_type elog_type;
    typedef core::rng_type rng_type;
    typedef core::endpoint_base endpoint_base;

    static bool const enable_multithreading = true;

    struct transport_config : public core::transport_config {
        typedef core::concurrency_type concurrency_type;
        typedef core::elog_type elog_type;
        typedef core::alog_type alog_type;
        typedef core::request_type request_type;
        typedef core::response_type response_type;

        static bool const enable_multithreading = true;
    };

    typedef websocketpp::transport::asio::endpoint<transport_config>
        transport_type;

    static const websocketpp::log::level elog_level =
        websocketpp::log::elevel::none;
    static const websocketpp::log::level alog_level =
        websocketpp::log::alevel::none;
        
    /// permessage_compress extension
    struct permessage_deflate_config {};

    typedef websocketpp::extensions::permessage_deflate::enabled
        <permessage_deflate_config> permessage_deflate_type;
};

typedef websocketpp::server<testee_config> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

// Define a callback to handle incoming messages
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    s->send(hdl, msg->get_payload(), msg->get_opcode());
}

void on_socket_init(websocketpp::connection_hdl, boost::asio::ip::tcp::socket & s) {
    boost::asio::ip::tcp::no_delay option(true);
    s.set_option(option);
}

int main(int argc, char * argv[]) {
    // Create a server endpoint
    server testee_server;

    short port = 9002;
    size_t num_threads = 1;

    if (argc == 3) {
        port = atoi(argv[1]);
        num_threads = atoi(argv[2]);
    }

    try {
        // Total silence
        testee_server.clear_access_channels(websocketpp::log::alevel::all);
        testee_server.clear_error_channels(websocketpp::log::alevel::all);

        // Initialize ASIO
        testee_server.init_asio();
        testee_server.set_reuse_addr(true);

        // Register our message handler
        testee_server.set_message_handler(bind(&on_message,&testee_server,::_1,::_2));
        testee_server.set_socket_init_handler(bind(&on_socket_init,::_1,::_2));

        // Listen on specified port with extended listen backlog
        testee_server.set_listen_backlog(8192);
        testee_server.listen(port);

        // Start the server accept loop
        testee_server.start_accept();

        // Start the ASIO io_service run loop
        if (num_threads == 1) {
            testee_server.run();
        } else {
            typedef websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_ptr;
            std::vector<thread_ptr> ts;
            for (size_t i = 0; i < num_threads; i++) {
                ts.push_back(websocketpp::lib::make_shared<websocketpp::lib::thread>(&server::run, &testee_server));
            }

            for (size_t i = 0; i < num_threads; i++) {
                ts[i]->join();
            }
        }

    } catch (websocketpp::exception const & e) {
        std::cout << "exception: " << e.what() << std::endl;
    }
}
