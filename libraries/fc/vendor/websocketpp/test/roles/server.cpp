/*
 * Copyright (c) 2014, Peter Thorson. All rights reserved.
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
#define BOOST_TEST_MODULE server
#include <boost/test/unit_test.hpp>

#include <iostream>

// Test Environment:
// server, no TLS, no locks, iostream based transport
#include <websocketpp/config/core.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::core> server;
typedef websocketpp::config::core::message_type::ptr message_ptr;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

/*struct stub_config : public websocketpp::config::core {
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
};*/

/* Run server and return output test rig */
std::string run_server_test(server& s, std::string input) {
    server::connection_ptr con;
    std::stringstream output;

    s.register_ostream(&output);
    s.clear_access_channels(websocketpp::log::alevel::all);
    s.clear_error_channels(websocketpp::log::elevel::all);

    con = s.get_connection();
    con->start();

    std::stringstream channel;

    channel << input;
    channel >> *con;

    return output.str();
}

/* handler library*/
void echo_func(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    s->send(hdl, msg->get_payload(), msg->get_opcode());
}

bool validate_func_subprotocol(server* s, std::string* out, std::string accept,
    websocketpp::connection_hdl hdl)
{
    server::connection_ptr con = s->get_con_from_hdl(hdl);

    std::stringstream o;

    const std::vector<std::string> & protocols = con->get_requested_subprotocols();
    std::vector<std::string>::const_iterator it;

    for (it = protocols.begin(); it != protocols.end(); ++it) {
        o << *it << ",";
    }

    *out = o.str();

    if (!accept.empty()) {
        con->select_subprotocol(accept);
    }

    return true;
}

void open_func_subprotocol(server* s, std::string* out, websocketpp::connection_hdl hdl) {
    server::connection_ptr con = s->get_con_from_hdl(hdl);

    *out = con->get_subprotocol();
}

/* Tests */
BOOST_AUTO_TEST_CASE( basic_websocket_request ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";
    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nServer: test\r\nUpgrade: websocket\r\n\r\n";

    server s;
    s.set_user_agent("test");

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
}

BOOST_AUTO_TEST_CASE( invalid_websocket_version ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: a\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";
    std::string output = "HTTP/1.1 400 Bad Request\r\nServer: test\r\n\r\n";

    server s;
    s.set_user_agent("test");
    //s.set_message_handler(bind(&echo_func,&s,::_1,::_2));

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
}

BOOST_AUTO_TEST_CASE( unimplemented_websocket_version ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 14\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";

    std::string output = "HTTP/1.1 400 Bad Request\r\nSec-WebSocket-Version: 0,7,8,13\r\nServer: test\r\n\r\n";

    server s;
    s.set_user_agent("test");

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
}

BOOST_AUTO_TEST_CASE( list_subprotocol_empty ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\nSec-WebSocket-Protocol: foo\r\n\r\n";

    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nServer: test\r\nUpgrade: websocket\r\n\r\n";

    std::string subprotocol;

    server s;
    s.set_user_agent("test");
    s.set_open_handler(bind(&open_func_subprotocol,&s,&subprotocol,::_1));

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
    BOOST_CHECK_EQUAL(subprotocol, "");
}

BOOST_AUTO_TEST_CASE( list_subprotocol_one ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\nSec-WebSocket-Protocol: foo\r\n\r\n";

    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nServer: test\r\nUpgrade: websocket\r\n\r\n";

    std::string validate;
    std::string open;

    server s;
    s.set_user_agent("test");
    s.set_validate_handler(bind(&validate_func_subprotocol,&s,&validate,"",::_1));
    s.set_open_handler(bind(&open_func_subprotocol,&s,&open,::_1));

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
    BOOST_CHECK_EQUAL(validate, "foo,");
    BOOST_CHECK_EQUAL(open, "");
}

BOOST_AUTO_TEST_CASE( accept_subprotocol_one ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\nSec-WebSocket-Protocol: foo\r\n\r\n";

    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nSec-WebSocket-Protocol: foo\r\nServer: test\r\nUpgrade: websocket\r\n\r\n";

    std::string validate;
    std::string open;

    server s;
    s.set_user_agent("test");
    s.set_validate_handler(bind(&validate_func_subprotocol,&s,&validate,"foo",::_1));
    s.set_open_handler(bind(&open_func_subprotocol,&s,&open,::_1));

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
    BOOST_CHECK_EQUAL(validate, "foo,");
    BOOST_CHECK_EQUAL(open, "foo");
}

BOOST_AUTO_TEST_CASE( accept_subprotocol_invalid ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\nSec-WebSocket-Protocol: foo\r\n\r\n";

    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nSec-WebSocket-Protocol: foo\r\nServer: test\r\nUpgrade: websocket\r\n\r\n";

    std::string validate;
    std::string open;

    server s;
    s.set_user_agent("test");
    s.set_validate_handler(bind(&validate_func_subprotocol,&s,&validate,"foo2",::_1));
    s.set_open_handler(bind(&open_func_subprotocol,&s,&open,::_1));

    std::string o;

    BOOST_CHECK_THROW(o = run_server_test(s,input), websocketpp::exception);
}

BOOST_AUTO_TEST_CASE( accept_subprotocol_two ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\nSec-WebSocket-Protocol: foo, bar\r\n\r\n";

    std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nSec-WebSocket-Protocol: bar\r\nServer: test\r\nUpgrade: websocket\r\n\r\n";

    std::string validate;
    std::string open;

    server s;
    s.set_user_agent("test");
    s.set_validate_handler(bind(&validate_func_subprotocol,&s,&validate,"bar",::_1));
    s.set_open_handler(bind(&open_func_subprotocol,&s,&open,::_1));

    BOOST_CHECK_EQUAL(run_server_test(s,input), output);
    BOOST_CHECK_EQUAL(validate, "foo,bar,");
    BOOST_CHECK_EQUAL(open, "bar");
}

/*BOOST_AUTO_TEST_CASE( user_reject_origin ) {
    std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example2.com\r\n\r\n";
    std::string output = "HTTP/1.1 403 Forbidden\r\nServer: test\r\n\r\n";

    server s;
    s.set_user_agent("test");

    BOOST_CHECK(run_server_test(s,input) == output);
}*/
