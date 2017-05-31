/*
 * Copyright (c) 2011, Peter Thorson. All rights reserved.
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
//#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE connection
#include <boost/test/unit_test.hpp>

#include "connection_tu2.hpp"

// Include special debugging transport
//#include <websocketpp/config/minimal_client.hpp>
#include <websocketpp/transport/debug/endpoint.hpp>

// NOTE: these tests currently test against hardcoded output values. I am not
// sure how problematic this will be. If issues arise like order of headers the
// output should be parsed by http::response and have values checked directly

BOOST_AUTO_TEST_CASE( basic_http_request ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
    std::string output = "HTTP/1.1 426 Upgrade Required\r\nServer: " +
                         std::string(websocketpp::user_agent)+"\r\n\r\n";

    std::string o2 = run_server_test(input);

    BOOST_CHECK(o2 == output);
}

struct connection_extension {
    connection_extension() : extension_value(5) {}

    int extension_method() {
        return extension_value;
    }

    bool is_server() const {
        return false;
    }

    int extension_value;
};

struct stub_config : public websocketpp::config::core {
    typedef core::concurrency_type concurrency_type;

    typedef core::request_type request_type;
    typedef core::response_type response_type;

    typedef core::message_type message_type;
    typedef core::con_msg_manager_type con_msg_manager_type;
    typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;

    typedef core::alog_type alog_type;
    typedef core::elog_type elog_type;

    typedef core::rng_type rng_type;

    typedef core::transport_type transport_type;

    typedef core::endpoint_base endpoint_base;
    typedef connection_extension connection_base;
};

struct debug_config_client : public websocketpp::config::core {
    typedef debug_config_client type;
    
    typedef core::concurrency_type concurrency_type;

    typedef core::request_type request_type;
    typedef core::response_type response_type;

    typedef core::message_type message_type;
    typedef core::con_msg_manager_type con_msg_manager_type;
    typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;

    typedef core::alog_type alog_type;
    typedef core::elog_type elog_type;

    typedef websocketpp::random::none::int_generator<uint32_t> rng_type;

    struct transport_config {
        typedef type::concurrency_type concurrency_type;
        typedef type::elog_type elog_type;
        typedef type::alog_type alog_type;
        typedef type::request_type request_type;
        typedef type::response_type response_type;

        /// Controls compile time enabling/disabling of thread syncronization
        /// code Disabling can provide a minor performance improvement to single
        /// threaded applications
        static bool const enable_multithreading = true;

        /// Default timer values (in ms)
        static const long timeout_socket_pre_init = 5000;
        static const long timeout_proxy = 5000;
        static const long timeout_socket_post_init = 5000;
        static const long timeout_connect = 5000;
        static const long timeout_socket_shutdown = 5000;
    };

    /// Transport Endpoint Component
    typedef websocketpp::transport::debug::endpoint<transport_config>
        transport_type;

    typedef core::endpoint_base endpoint_base;
    typedef connection_extension connection_base;
    
    static const websocketpp::log::level elog_level = websocketpp::log::elevel::none;
    static const websocketpp::log::level alog_level = websocketpp::log::alevel::none;
};

struct connection_setup {
    connection_setup(bool p_is_server) : c(p_is_server, "", alog, elog, rng) {}

    websocketpp::lib::error_code ec;
    stub_config::alog_type alog;
    stub_config::elog_type elog;
    stub_config::rng_type rng;
    websocketpp::connection<stub_config> c;
};

typedef websocketpp::client<debug_config_client> debug_client;
typedef websocketpp::server<debug_config_client> debug_server;

/*void echo_func(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    s->send(hdl, msg->get_payload(), msg->get_opcode());
}*/

void validate_func(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    s->send(hdl, msg->get_payload(), msg->get_opcode());
}

bool validate_set_ua(server* s, websocketpp::connection_hdl hdl) {
    server::connection_ptr con = s->get_con_from_hdl(hdl);
    con->replace_header("Server","foo");
    return true;
}

void http_func(server* s, websocketpp::connection_hdl hdl) {
    using namespace websocketpp::http;

    server::connection_ptr con = s->get_con_from_hdl(hdl);

    std::string res = con->get_resource();

    con->set_body(res);
    con->set_status(status_code::ok);

    BOOST_CHECK_EQUAL(con->get_response_code(), status_code::ok);
    BOOST_CHECK_EQUAL(con->get_response_msg(), status_code::get_string(status_code::ok));
}

void defer_http_func(server* s, bool * deferred, websocketpp::connection_hdl hdl) {
    *deferred = true;
    
    server::connection_ptr con = s->get_con_from_hdl(hdl);
    
    websocketpp::lib::error_code ec = con->defer_http_response();
    BOOST_CHECK_EQUAL(ec, websocketpp::lib::error_code());
}

void check_on_fail(server* s, websocketpp::lib::error_code ec, bool & called, 
    websocketpp::connection_hdl hdl)
{
    server::connection_ptr con = s->get_con_from_hdl(hdl);

    BOOST_CHECK_EQUAL(ec, con->get_ec());
    called = true;
}

void on_open_print(server* s, websocketpp::connection_hdl hdl)
{
    server::connection_ptr con = s->get_con_from_hdl(hdl);

    std::cout << con->get_uri() << std::endl;
}

void fail_on_open(websocketpp::connection_hdl) {
    BOOST_CHECK(false);
}
void fail_on_http(websocketpp::connection_hdl) {
    BOOST_CHECK(false);
}

BOOST_AUTO_TEST_CASE( connection_extensions ) {
    connection_setup env(true);

    BOOST_CHECK( env.c.extension_value == 5 );
    BOOST_CHECK( env.c.extension_method() == 5 );

    BOOST_CHECK( env.c.is_server() == true );
}

BOOST_AUTO_TEST_CASE( basic_websocket_request ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";
    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nServer: ";
    output+=websocketpp::user_agent;
    output+="\r\nUpgrade: websocket\r\n\r\n";

    server s;
    s.set_message_handler(bind(&echo_func,&s,::_1,::_2));

    BOOST_CHECK(run_server_test(s,input) == output);
}

BOOST_AUTO_TEST_CASE( http_request ) {
    std::string input = "GET /foo/bar HTTP/1.1\r\nHost: www.example.com\r\nOrigin: http://www.example.com\r\n\r\n";
    std::string output = "HTTP/1.1 200 OK\r\nContent-Length: 8\r\nServer: ";
    output+=websocketpp::user_agent;
    output+="\r\n\r\n/foo/bar";

    server s;
    s.set_http_handler(bind(&http_func,&s,::_1));

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
}

BOOST_AUTO_TEST_CASE( deferred_http_request ) {
    std::string input = "GET /foo/bar HTTP/1.1\r\nHost: www.example.com\r\nOrigin: http://www.example.com\r\n\r\n";
    std::string output = "HTTP/1.1 200 OK\r\nContent-Length: 8\r\nServer: ";
    output+=websocketpp::user_agent;
    output+="\r\n\r\n/foo/bar";

    server s;
    server::connection_ptr con;
    bool deferred = false;
    s.set_http_handler(bind(&defer_http_func,&s, &deferred,::_1));

    s.clear_access_channels(websocketpp::log::alevel::all);
    s.clear_error_channels(websocketpp::log::elevel::all);
    
    std::stringstream ostream;
    s.register_ostream(&ostream);

    con = s.get_connection();
    con->start();
    
    BOOST_CHECK(!deferred);
    BOOST_CHECK_EQUAL(ostream.str(), "");
    con->read_some(input.data(),input.size());
    BOOST_CHECK(deferred);
    BOOST_CHECK_EQUAL(ostream.str(), "");

    con->set_body(con->get_resource());
    con->set_status(websocketpp::http::status_code::ok);
    
    websocketpp::lib::error_code ec;
    s.send_http_response(con->get_handle(),ec);
    BOOST_CHECK_EQUAL(ec, websocketpp::lib::error_code());
    BOOST_CHECK_EQUAL(ostream.str(), output);
    con->send_http_response(ec);
    BOOST_CHECK_EQUAL(ec, make_error_code(websocketpp::error::invalid_state));
    
}

BOOST_AUTO_TEST_CASE( request_no_server_header ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";
    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nUpgrade: websocket\r\n\r\n";

    server s;
    s.set_user_agent("");
    s.set_message_handler(bind(&echo_func,&s,::_1,::_2));

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
}

BOOST_AUTO_TEST_CASE( request_no_server_header_override ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";
    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nServer: foo\r\nUpgrade: websocket\r\n\r\n";

    server s;
    s.set_user_agent("");
    s.set_message_handler(bind(&echo_func,&s,::_1,::_2));
    s.set_validate_handler(bind(&validate_set_ua,&s,::_1));

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
}

BOOST_AUTO_TEST_CASE( basic_client_websocket ) {
    std::string uri = "ws://localhost";

    //std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nServer: foo\r\nUpgrade: websocket\r\n\r\n";

    std::string ref = "GET / HTTP/1.1\r\nConnection: Upgrade\r\nFoo: Bar\r\nHost: localhost\r\nSec-WebSocket-Key: AAAAAAAAAAAAAAAAAAAAAA==\r\nSec-WebSocket-Version: 13\r\nUpgrade: websocket\r\nUser-Agent: foo\r\n\r\n";

    std::stringstream output;

    client e;
    e.set_access_channels(websocketpp::log::alevel::none);
    e.set_error_channels(websocketpp::log::elevel::none);
    e.set_user_agent("foo");
    e.register_ostream(&output);

    client::connection_ptr con;
    websocketpp::lib::error_code ec;
    con = e.get_connection(uri, ec);
    con->append_header("Foo","Bar");
    e.connect(con);

    BOOST_CHECK_EQUAL(ref, output.str());
}

BOOST_AUTO_TEST_CASE( set_max_message_size ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
    
    // After the handshake, add a single frame with a message that is too long.
    char frame0[10] = {char(0x82), char(0x83), 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01};
    input.append(frame0, 10);
    
    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nServer: foo\r\nUpgrade: websocket\r\n\r\n";

    // After the handshake, add a single frame with a close message with message too big
    // error code.
    char frame1[4] = {char(0x88), 0x19, 0x03, char(0xf1)};
    output.append(frame1, 4);
    output.append("A message was too large");

    server s;
    s.set_user_agent("");
    s.set_validate_handler(bind(&validate_set_ua,&s,::_1));
    s.set_max_message_size(2);

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
}

BOOST_AUTO_TEST_CASE( websocket_fail_parse_error ) {
    std::string input = "asdf\r\n\r\n";

    server s;
    websocketpp::lib::error_code ec = make_error_code(websocketpp::error::http_parse_error);
    bool called = false;
    s.set_fail_handler(bind(&check_on_fail,&s,ec,websocketpp::lib::ref(called),::_1));

    run_server_test(s,input,false);
    BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE( websocket_fail_invalid_version ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: foo\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";

    server s;
    websocketpp::lib::error_code ec = make_error_code(websocketpp::error::invalid_version);
    bool called = false;
    s.set_fail_handler(bind(&check_on_fail,&s,ec,websocketpp::lib::ref(called),::_1));

    run_server_test(s,input,false);
    BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE( websocket_fail_unsupported_version ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 12\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";

    server s;
    websocketpp::lib::error_code ec = make_error_code(websocketpp::error::unsupported_version);
    bool called = false;
    s.set_fail_handler(bind(&check_on_fail,&s,ec,websocketpp::lib::ref(called),::_1));

    run_server_test(s,input,false);
    BOOST_CHECK(called);
}

// BOOST_AUTO_TEST_CASE( websocket_fail_invalid_uri ) {
//     std::string input = "GET http://345.123.123.123/foo HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";

//     server s;
//     websocketpp::lib::error_code ec = make_error_code(websocketpp::error::unsupported_version);
//     bool called = false;
//     s.set_fail_handler(bind(&check_on_fail,&s,ec,websocketpp::lib::ref(called),::_1));
//     s.set_open_handler(bind(&on_open_print,&s,::_1));

//     std::cout << run_server_test(s,input,true) << std::endl;
//     BOOST_CHECK(called);
// }

// BOOST_AUTO_TEST_CASE( websocket_fail_invalid_uri_http ) {
//     std::string input = "GET http://345.123.123.123/foo HTTP/1.1\r\nHost: www.example.com\r\nOrigin: http://www.example.com\r\n\r\n";

//     server s;
//     websocketpp::lib::error_code ec = make_error_code(websocketpp::error::unsupported_version);
//     bool called = false;
//     s.set_fail_handler(bind(&check_on_fail,&s,ec,websocketpp::lib::ref(called),::_1));
//     s.set_open_handler(bind(&on_open_print,&s,::_1));

//     std::cout << run_server_test(s,input,true) << std::endl;
//     BOOST_CHECK(called);
// }

BOOST_AUTO_TEST_CASE( websocket_fail_upgrade_required ) {
    std::string input = "GET /foo/bar HTTP/1.1\r\nHost: www.example.com\r\nOrigin: http://www.example.com\r\n\r\n";

    server s;
    websocketpp::lib::error_code ec = make_error_code(websocketpp::error::upgrade_required);
    bool called = false;
    s.set_fail_handler(bind(&check_on_fail,&s,ec,websocketpp::lib::ref(called),::_1));

    run_server_test(s,input,false);
    BOOST_CHECK(called);
}

// TODO: set max message size in client endpoint test case
// TODO: set max message size mid connection test case
// TODO: [maybe] set max message size in open handler



// BOOST_AUTO_TEST_CASE( user_reject_origin ) {
//     std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example2.com\r\n\r\n";
//     std::string output = "HTTP/1.1 403 Forbidden\r\nServer: "+websocketpp::USER_AGENT+"\r\n\r\n";

//     BOOST_CHECK(run_server_test(input) == output);
// }

// BOOST_AUTO_TEST_CASE( basic_text_message ) {
//     std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";

//     unsigned char frames[8] = {0x82,0x82,0xFF,0xFF,0xFF,0xFF,0xD5,0xD5};
//     input.append(reinterpret_cast<char*>(frames),8);

//     std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nServer: "+websocketpp::USER_AGENT+"\r\nUpgrade: websocket\r\n\r\n**";

//     BOOST_CHECK( run_server_test(input) == output);
// }






BOOST_AUTO_TEST_CASE( client_handshake_timeout_race1 ) {
    debug_client c;

    websocketpp::lib::error_code ec;
    debug_client::connection_ptr con = c.get_connection("ws://localhost:9002", ec);

    BOOST_CHECK(!ec);

    // This test the case where a handshake times out immediately before the 
    // handler that would have completed it gets invoked. This situation happens
    // when clients are connecting to overloaded servers and on servers that are
    // overloaded. 
    c.connect(con);
    
    con->expire_timer(websocketpp::lib::error_code());
    // Fullfil the write to simulate the write completing immediately after
    // timer expires
    con->fullfil_write();
    
    BOOST_CHECK_EQUAL(con->get_ec(), make_error_code(websocketpp::error::open_handshake_timeout));
}

BOOST_AUTO_TEST_CASE( client_handshake_timeout_race2 ) {
    debug_client c;

    websocketpp::lib::error_code ec;
    debug_client::connection_ptr con = c.get_connection("ws://localhost:9002", ec);

    BOOST_CHECK(!ec);

    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: ICX+Yqv66kxgM0FcWaLWlFLwTAI=\r\nServer: foo\r\nUpgrade: websocket\r\n\r\n";

    // This test the case where a handshake times out immediately before the 
    // handler that would have completed it gets invoked. This situation happens
    // when clients are connecting to overloaded servers and on servers that are
    // overloaded. 
    c.connect(con);
    con->fullfil_write();
    
    con->expire_timer(websocketpp::lib::error_code());
    // Read valid handshake to simulate receiving the handshake response
    // immediately after the timer expires
    con->read_all(output.data(),output.size());
    
    BOOST_CHECK_EQUAL(con->get_ec(), make_error_code(websocketpp::error::open_handshake_timeout));
}

BOOST_AUTO_TEST_CASE( server_handshake_timeout_race1 ) {
    debug_server s;

    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: AAAAAAAAAAAAAAAAAAAAAA==\r\n\r\n";

    debug_server::connection_ptr con = s.get_connection();
    con->start();
    
    con->expire_timer(websocketpp::lib::error_code());
    // Read handshake immediately after timer expire
    con->read_all(input.data(), input.size());
    
    BOOST_CHECK_EQUAL(con->get_ec(), make_error_code(websocketpp::error::open_handshake_timeout));
}

BOOST_AUTO_TEST_CASE( server_handshake_timeout_race2 ) {
    debug_server s;

    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: AAAAAAAAAAAAAAAAAAAAAA==\r\n\r\n";

    debug_server::connection_ptr con = s.get_connection();
    con->start();
    
    con->read_all(input.data(), input.size());
    
    con->expire_timer(websocketpp::lib::error_code());
    // Complete write immediately after timer expire
    con->fullfil_write();
    
    BOOST_CHECK_EQUAL(con->get_ec(), make_error_code(websocketpp::error::open_handshake_timeout));
}


