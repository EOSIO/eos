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
#define BOOST_TEST_MODULE close
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>

#include <websocketpp/close.hpp>
#include <websocketpp/utilities.hpp>

using namespace websocketpp;

BOOST_AUTO_TEST_CASE( reserved_values ) {
    BOOST_CHECK( !close::status::reserved(999) );
    BOOST_CHECK( close::status::reserved(1004) );
    BOOST_CHECK( close::status::reserved(1014) );
    BOOST_CHECK( close::status::reserved(1016) );
    BOOST_CHECK( close::status::reserved(2999) );
    BOOST_CHECK( !close::status::reserved(1000) );
}

BOOST_AUTO_TEST_CASE( invalid_values ) {
    BOOST_CHECK( close::status::invalid(0) );
    BOOST_CHECK( close::status::invalid(999) );
    BOOST_CHECK( !close::status::invalid(1000) );
    BOOST_CHECK( close::status::invalid(1005) );
    BOOST_CHECK( close::status::invalid(1006) );
    BOOST_CHECK( close::status::invalid(1015) );
    BOOST_CHECK( !close::status::invalid(2999) );
    BOOST_CHECK( !close::status::invalid(3000) );
    BOOST_CHECK( close::status::invalid(5000) );
}

BOOST_AUTO_TEST_CASE( value_extraction ) {
    lib::error_code ec;
    std::string payload = "oo";

    // Value = 1000
    payload[0] = 0x03;
    payload[1] = char(0xe8);
    BOOST_CHECK( close::extract_code(payload,ec) == close::status::normal );
    BOOST_CHECK( !ec );

    // Value = 1004
    payload[0] = 0x03;
    payload[1] = char(0xec);
    BOOST_CHECK( close::extract_code(payload,ec) == 1004 );
    BOOST_CHECK( ec == error::reserved_close_code );

    // Value = 1005
    payload[0] = 0x03;
    payload[1] = char(0xed);
    BOOST_CHECK( close::extract_code(payload,ec) == close::status::no_status );
    BOOST_CHECK( ec == error::invalid_close_code );

    // Value = 3000
    payload[0] = 0x0b;
    payload[1] = char(0xb8);
    BOOST_CHECK( close::extract_code(payload,ec) == 3000 );
    BOOST_CHECK( !ec );
}

BOOST_AUTO_TEST_CASE( extract_empty ) {
    lib::error_code ec;
    std::string payload;

    BOOST_CHECK( close::extract_code(payload,ec) == close::status::no_status );
    BOOST_CHECK( !ec );
}

BOOST_AUTO_TEST_CASE( extract_short ) {
    lib::error_code ec;
    std::string payload = "0";

    BOOST_CHECK( close::extract_code(payload,ec) == close::status::protocol_error );
    BOOST_CHECK( ec == error::bad_close_code );
}

BOOST_AUTO_TEST_CASE( extract_reason ) {
    lib::error_code ec;
    std::string payload = "00Foo";

    BOOST_CHECK( close::extract_reason(payload,ec) == "Foo" );
    BOOST_CHECK( !ec );

    payload.clear();
    BOOST_CHECK( close::extract_reason(payload,ec).empty() );
    BOOST_CHECK( !ec );

    payload = "00";
    BOOST_CHECK( close::extract_reason(payload,ec).empty() );
    BOOST_CHECK( !ec );

    payload = "000";
    payload[2] = char(0xFF);

    close::extract_reason(payload,ec);
    BOOST_CHECK( ec == error::invalid_utf8 );
}
