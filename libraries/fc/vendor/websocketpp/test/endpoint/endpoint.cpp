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
//#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE endpoint
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <sstream>

#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

BOOST_AUTO_TEST_CASE( construct_server_iostream ) {
    websocketpp::server<websocketpp::config::core> s;
}

BOOST_AUTO_TEST_CASE( construct_server_asio_plain ) {
    websocketpp::server<websocketpp::config::asio> s;
}

BOOST_AUTO_TEST_CASE( construct_server_asio_tls ) {
    websocketpp::server<websocketpp::config::asio_tls> s;
}

BOOST_AUTO_TEST_CASE( initialize_server_asio ) {
    websocketpp::server<websocketpp::config::asio> s;
    s.init_asio();
}

BOOST_AUTO_TEST_CASE( initialize_server_asio_external ) {
    websocketpp::server<websocketpp::config::asio> s;
    boost::asio::io_service ios;
    s.init_asio(&ios);
}

#ifdef _WEBSOCKETPP_MOVE_SEMANTICS_
BOOST_AUTO_TEST_CASE( move_construct_server_core ) {
    websocketpp::server<websocketpp::config::core> s1;
    
    websocketpp::server<websocketpp::config::core> s2(std::move(s1));
}

/*
// temporary disable because library doesn't pass
BOOST_AUTO_TEST_CASE( emplace ) {
    std::stringstream out1;
    std::stringstream out2;
    
    std::vector<websocketpp::server<websocketpp::config::asio_tls>> v;
    
    v.emplace_back();
    v.emplace_back();
    
    v[0].get_alog().set_ostream(&out1);
    v[0].get_alog().set_ostream(&out2);
    
    v[0].get_alog().write(websocketpp::log::alevel::app,"devel0");
    v[1].get_alog().write(websocketpp::log::alevel::app,"devel1");
    BOOST_CHECK( out1.str().size() > 0 );
    BOOST_CHECK( out2.str().size() > 0 );
}*/

#endif // _WEBSOCKETPP_MOVE_SEMANTICS_

struct endpoint_extension {
    endpoint_extension() : extension_value(5) {}

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

    typedef endpoint_extension endpoint_base;
};

BOOST_AUTO_TEST_CASE( endpoint_extensions ) {
    websocketpp::server<stub_config> s;

    BOOST_CHECK_EQUAL( s.extension_value, 5 );
    BOOST_CHECK_EQUAL( s.extension_method(), 5 );

    BOOST_CHECK( s.is_server() );
}

BOOST_AUTO_TEST_CASE( listen_after_listen_failure ) {
    using websocketpp::transport::asio::error::make_error_code;
    using websocketpp::transport::asio::error::pass_through;

    websocketpp::server<websocketpp::config::asio> server1;
    websocketpp::server<websocketpp::config::asio> server2;

    websocketpp::lib::error_code ec;

    server1.init_asio();
    server2.init_asio();

    boost::asio::ip::tcp::endpoint ep1(boost::asio::ip::address::from_string("127.0.0.1"), 12345);
    boost::asio::ip::tcp::endpoint ep2(boost::asio::ip::address::from_string("127.0.0.1"), 23456);

    server1.listen(ep1, ec);
    BOOST_CHECK(!ec);

    server2.listen(ep1, ec);
    BOOST_REQUIRE_EQUAL(ec, make_error_code(pass_through));

    server2.listen(ep2, ec);
    BOOST_CHECK(!ec);
}
