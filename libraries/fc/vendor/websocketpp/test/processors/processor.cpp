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
#define BOOST_TEST_MODULE processors
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>

#include <websocketpp/processors/processor.hpp>
#include <websocketpp/http/request.hpp>

BOOST_AUTO_TEST_CASE( exact_match ) {
    websocketpp::http::parser::request r;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(r));
}

BOOST_AUTO_TEST_CASE( non_match ) {
    websocketpp::http::parser::request r;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(!websocketpp::processor::is_websocket_handshake(r));
}

BOOST_AUTO_TEST_CASE( ci_exact_match ) {
    websocketpp::http::parser::request r;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: UpGrAde\r\nUpgrade: WebSocket\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(r));
}

BOOST_AUTO_TEST_CASE( non_exact_match1 ) {
    websocketpp::http::parser::request r;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade,foo\r\nUpgrade: websocket,foo\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(r));
}

BOOST_AUTO_TEST_CASE( non_exact_match2 ) {
    websocketpp::http::parser::request r;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: keep-alive,Upgrade,foo\r\nUpgrade: foo,websocket,bar\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(r));
}

BOOST_AUTO_TEST_CASE( version_blank ) {
    websocketpp::http::parser::request r;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nUpgrade: websocket\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == 0);
}

BOOST_AUTO_TEST_CASE( version_7 ) {
    websocketpp::http::parser::request r;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 7\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == 7);
}

BOOST_AUTO_TEST_CASE( version_8 ) {
    websocketpp::http::parser::request r;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 8\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == 8);
}

BOOST_AUTO_TEST_CASE( version_13 ) {
    websocketpp::http::parser::request r;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == 13);
}

BOOST_AUTO_TEST_CASE( version_non_numeric ) {
    websocketpp::http::parser::request r;

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nUpgrade: websocket\r\nSec-WebSocket-Version: abc\r\n\r\n";

    r.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::get_websocket_version(r) == -1);
}