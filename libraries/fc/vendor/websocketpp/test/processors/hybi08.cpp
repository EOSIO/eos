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
#define BOOST_TEST_MODULE hybi_08_processor
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>

#include <websocketpp/processors/hybi08.hpp>
#include <websocketpp/http/request.hpp>
#include <websocketpp/http/response.hpp>
#include <websocketpp/message_buffer/message.hpp>
#include <websocketpp/message_buffer/alloc.hpp>
#include <websocketpp/extensions/permessage_deflate/disabled.hpp>
#include <websocketpp/random/none.hpp>

struct stub_config {
    typedef websocketpp::http::parser::request request_type;
    typedef websocketpp::http::parser::response response_type;

    typedef websocketpp::message_buffer::message
        <websocketpp::message_buffer::alloc::con_msg_manager> message_type;
    typedef websocketpp::message_buffer::alloc::con_msg_manager<message_type>
        con_msg_manager_type;

    typedef websocketpp::random::none::int_generator<uint32_t> rng_type;

    static const size_t max_message_size = 16000000;

    /// Extension related config
    static const bool enable_extensions = false;

    /// Extension specific config

    /// permessage_deflate_config
    struct permessage_deflate_config {
        typedef stub_config::request_type request_type;
    };

    typedef websocketpp::extensions::permessage_deflate::disabled
        <permessage_deflate_config> permessage_deflate_type;
};

BOOST_AUTO_TEST_CASE( exact_match ) {
    stub_config::request_type r;
    stub_config::response_type response;
    stub_config::con_msg_manager_type::ptr msg_manager;
    stub_config::rng_type rng;
    websocketpp::processor::hybi08<stub_config> p(false,true,msg_manager,rng);
    websocketpp::lib::error_code ec;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 8\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(r));
    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == p.get_version());
    ec = p.validate_handshake(r);
    BOOST_CHECK(!ec);

    websocketpp::uri_ptr u;

    u = p.get_uri(r);

    BOOST_CHECK(u->get_valid() == true);
    BOOST_CHECK(u->get_secure() == false);
    BOOST_CHECK(u->get_host() == "www.example.com");
    BOOST_CHECK(u->get_resource() == "/");
    BOOST_CHECK(u->get_port() == websocketpp::uri_default_port);

    p.process_handshake(r,"",response);

    BOOST_CHECK(response.get_header("Connection") == "upgrade");
    BOOST_CHECK(response.get_header("Upgrade") == "websocket");
    BOOST_CHECK(response.get_header("Sec-WebSocket-Accept") == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

BOOST_AUTO_TEST_CASE( non_get_method ) {
    stub_config::request_type r;
    stub_config::response_type response;
    stub_config::rng_type rng;
    stub_config::con_msg_manager_type::ptr msg_manager;
    websocketpp::processor::hybi08<stub_config> p(false,true,msg_manager,rng);
    websocketpp::lib::error_code ec;

    std::string handshake = "POST / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 8\r\nSec-WebSocket-Key: foo\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(r));
    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == p.get_version());
    ec = p.validate_handshake(r);
    BOOST_CHECK( ec == websocketpp::processor::error::invalid_http_method );
}

BOOST_AUTO_TEST_CASE( old_http_version ) {
    stub_config::request_type r;
    stub_config::response_type response;
    stub_config::con_msg_manager_type::ptr msg_manager;
    stub_config::rng_type rng;
    websocketpp::processor::hybi08<stub_config> p(false,true,msg_manager,rng);
    websocketpp::lib::error_code ec;

    std::string handshake = "GET / HTTP/1.0\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 8\r\nSec-WebSocket-Key: foo\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(r));
    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == p.get_version());
    ec = p.validate_handshake(r);
    BOOST_CHECK( ec == websocketpp::processor::error::invalid_http_version );
}

BOOST_AUTO_TEST_CASE( missing_handshake_key1 ) {
    stub_config::request_type r;
    stub_config::response_type response;
    stub_config::con_msg_manager_type::ptr msg_manager;
    stub_config::rng_type rng;
    websocketpp::processor::hybi08<stub_config> p(false,true,msg_manager,rng);
    websocketpp::lib::error_code ec;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 8\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(r));
    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == p.get_version());
    ec = p.validate_handshake(r);
    BOOST_CHECK( ec == websocketpp::processor::error::missing_required_header );
}

BOOST_AUTO_TEST_CASE( missing_handshake_key2 ) {
    stub_config::request_type r;
    stub_config::response_type response;
    stub_config::con_msg_manager_type::ptr msg_manager;
    stub_config::rng_type rng;
    websocketpp::processor::hybi08<stub_config> p(false,true,msg_manager,rng);
    websocketpp::lib::error_code ec;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 8\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(r));
    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == p.get_version());
    ec = p.validate_handshake(r);
    BOOST_CHECK( ec == websocketpp::processor::error::missing_required_header );
}

BOOST_AUTO_TEST_CASE( bad_host ) {
    stub_config::request_type r;
    stub_config::response_type response;
    stub_config::con_msg_manager_type::ptr msg_manager;
    stub_config::rng_type rng;
    websocketpp::processor::hybi08<stub_config> p(false,true,msg_manager,rng);
    websocketpp::uri_ptr u;
    websocketpp::lib::error_code ec;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com:70000\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 8\r\nSec-WebSocket-Key: foo\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(r));
    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == p.get_version());
    ec = p.validate_handshake(r);
    BOOST_CHECK( !ec );

    u = p.get_uri(r);

    BOOST_CHECK( !u->get_valid() );
}

