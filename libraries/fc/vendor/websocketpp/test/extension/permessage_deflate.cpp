/*
 * Copyright (c) 2013, Peter Thorson. All rights reserved.
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
#define BOOST_TEST_MODULE permessage_deflate
#include <boost/test/unit_test.hpp>

#include <websocketpp/error.hpp>

#include <websocketpp/extensions/extension.hpp>
#include <websocketpp/extensions/permessage_deflate/disabled.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

#include <string>

#include <websocketpp/utilities.hpp>
#include <iostream>

class config {};

typedef websocketpp::extensions::permessage_deflate::enabled<config> enabled_type;
typedef websocketpp::extensions::permessage_deflate::disabled<config> disabled_type;

struct ext_vars {
    enabled_type exts;
    enabled_type extc;
    websocketpp::lib::error_code ec;
    websocketpp::err_str_pair esp;
    websocketpp::http::attribute_list attr;
};
namespace pmde = websocketpp::extensions::permessage_deflate::error;
namespace pmd_mode = websocketpp::extensions::permessage_deflate::mode;

// Ensure the disabled extension behaves appropriately disabled

BOOST_AUTO_TEST_CASE( disabled_is_disabled ) {
    disabled_type exts;
    BOOST_CHECK( !exts.is_implemented() );
}

BOOST_AUTO_TEST_CASE( disabled_is_off ) {
    disabled_type exts;
    BOOST_CHECK( !exts.is_enabled() );
}

// Ensure the enabled version actually works

BOOST_AUTO_TEST_CASE( enabled_is_enabled ) {
    ext_vars v;
    BOOST_CHECK( v.exts.is_implemented() );
    BOOST_CHECK( v.extc.is_implemented() );
}


BOOST_AUTO_TEST_CASE( enabled_starts_disabled ) {
    ext_vars v;
    BOOST_CHECK( !v.exts.is_enabled() );
    BOOST_CHECK( !v.extc.is_enabled() );
}

BOOST_AUTO_TEST_CASE( negotiation_empty_attr ) {
    ext_vars v;

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate");
}

BOOST_AUTO_TEST_CASE( negotiation_invalid_attr ) {
    ext_vars v;
    v.attr["foo"] = "bar";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( !v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, pmde::make_error_code(pmde::invalid_attributes) );
    BOOST_CHECK_EQUAL( v.esp.second, "");
}

// Negotiate server_no_context_takeover
BOOST_AUTO_TEST_CASE( negotiate_server_no_context_takeover_invalid ) {
    ext_vars v;
    v.attr["server_no_context_takeover"] = "foo";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( !v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, pmde::make_error_code(pmde::invalid_attribute_value) );
    BOOST_CHECK_EQUAL( v.esp.second, "");
}

BOOST_AUTO_TEST_CASE( negotiate_server_no_context_takeover ) {
    ext_vars v;
    v.attr["server_no_context_takeover"].clear();

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_no_context_takeover");
}

BOOST_AUTO_TEST_CASE( negotiate_server_no_context_takeover_server_initiated ) {
    ext_vars v;

    v.exts.enable_server_no_context_takeover();
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_no_context_takeover");
}

// Negotiate client_no_context_takeover
BOOST_AUTO_TEST_CASE( negotiate_client_no_context_takeover_invalid ) {
    ext_vars v;
    v.attr["client_no_context_takeover"] = "foo";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( !v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, pmde::make_error_code(pmde::invalid_attribute_value) );
    BOOST_CHECK_EQUAL( v.esp.second, "");
}

BOOST_AUTO_TEST_CASE( negotiate_client_no_context_takeover ) {
    ext_vars v;
    v.attr["client_no_context_takeover"].clear();

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; client_no_context_takeover");
}

BOOST_AUTO_TEST_CASE( negotiate_client_no_context_takeover_server_initiated ) {
    ext_vars v;

    v.exts.enable_client_no_context_takeover();
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; client_no_context_takeover");
}


// Negotiate server_max_window_bits
BOOST_AUTO_TEST_CASE( negotiate_server_max_window_bits_invalid ) {
    ext_vars v;

    std::vector<std::string> values;
    values.push_back("");
    values.push_back("foo");
    values.push_back("7");
    values.push_back("16");

    std::vector<std::string>::const_iterator it;
    for (it = values.begin(); it != values.end(); ++it) {
        v.attr["server_max_window_bits"] = *it;

        v.esp = v.exts.negotiate(v.attr);
        BOOST_CHECK( !v.exts.is_enabled() );
        BOOST_CHECK_EQUAL( v.esp.first, pmde::make_error_code(pmde::invalid_attribute_value) );
        BOOST_CHECK_EQUAL( v.esp.second, "");
    }
}

BOOST_AUTO_TEST_CASE( negotiate_server_max_window_bits_valid ) {
    ext_vars v;
    v.attr["server_max_window_bits"] = "8";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_max_window_bits=8");

    v.attr["server_max_window_bits"] = "15";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate");
}

BOOST_AUTO_TEST_CASE( invalid_set_server_max_window_bits ) {
    ext_vars v;

    v.ec = v.exts.set_server_max_window_bits(7,pmd_mode::decline);
    BOOST_CHECK_EQUAL(v.ec,pmde::make_error_code(pmde::invalid_max_window_bits));

    v.ec = v.exts.set_server_max_window_bits(16,pmd_mode::decline);
    BOOST_CHECK_EQUAL(v.ec,pmde::make_error_code(pmde::invalid_max_window_bits));
}

BOOST_AUTO_TEST_CASE( negotiate_server_max_window_bits_decline ) {
    ext_vars v;
    v.attr["server_max_window_bits"] = "8";

    v.ec = v.exts.set_server_max_window_bits(15,pmd_mode::decline);
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate");
}

BOOST_AUTO_TEST_CASE( negotiate_server_max_window_bits_accept ) {
    ext_vars v;
    v.attr["server_max_window_bits"] = "8";

    v.ec = v.exts.set_server_max_window_bits(15,pmd_mode::accept);
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_max_window_bits=8");
}

BOOST_AUTO_TEST_CASE( negotiate_server_max_window_bits_largest ) {
    ext_vars v;
    v.attr["server_max_window_bits"] = "8";

    v.ec = v.exts.set_server_max_window_bits(15,pmd_mode::largest);
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_max_window_bits=8");
}

BOOST_AUTO_TEST_CASE( negotiate_server_max_window_bits_smallest ) {
    ext_vars v;
    v.attr["server_max_window_bits"] = "8";

    v.ec = v.exts.set_server_max_window_bits(15,pmd_mode::smallest);
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_max_window_bits=8");
}

// Negotiate server_max_window_bits
BOOST_AUTO_TEST_CASE( negotiate_client_max_window_bits_invalid ) {
    ext_vars v;

    std::vector<std::string> values;
    values.push_back("foo");
    values.push_back("7");
    values.push_back("16");

    std::vector<std::string>::const_iterator it;
    for (it = values.begin(); it != values.end(); ++it) {
        v.attr["client_max_window_bits"] = *it;

        v.esp = v.exts.negotiate(v.attr);
        BOOST_CHECK( !v.exts.is_enabled() );
        BOOST_CHECK_EQUAL( v.esp.first, pmde::make_error_code(pmde::invalid_attribute_value) );
        BOOST_CHECK_EQUAL( v.esp.second, "");
    }
}

BOOST_AUTO_TEST_CASE( negotiate_client_max_window_bits_valid ) {
    ext_vars v;

    v.attr["client_max_window_bits"].clear();
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate");

    v.attr["client_max_window_bits"] = "8";
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; client_max_window_bits=8");

    v.attr["client_max_window_bits"] = "15";
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate");
}

BOOST_AUTO_TEST_CASE( invalid_set_client_max_window_bits ) {
    ext_vars v;

    v.ec = v.exts.set_client_max_window_bits(7,pmd_mode::decline);
    BOOST_CHECK_EQUAL(v.ec,pmde::make_error_code(pmde::invalid_max_window_bits));

    v.ec = v.exts.set_client_max_window_bits(16,pmd_mode::decline);
    BOOST_CHECK_EQUAL(v.ec,pmde::make_error_code(pmde::invalid_max_window_bits));
}

BOOST_AUTO_TEST_CASE( negotiate_client_max_window_bits_decline ) {
    ext_vars v;
    v.attr["client_max_window_bits"] = "8";

    v.ec = v.exts.set_client_max_window_bits(8,pmd_mode::decline);
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate");
}

BOOST_AUTO_TEST_CASE( negotiate_client_max_window_bits_accept ) {
    ext_vars v;
    v.attr["client_max_window_bits"] = "8";

    v.ec = v.exts.set_client_max_window_bits(15,pmd_mode::accept);
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; client_max_window_bits=8");
}

BOOST_AUTO_TEST_CASE( negotiate_client_max_window_bits_largest ) {
    ext_vars v;
    v.attr["client_max_window_bits"] = "8";

    v.ec = v.exts.set_client_max_window_bits(15,pmd_mode::largest);
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; client_max_window_bits=8");
}

BOOST_AUTO_TEST_CASE( negotiate_client_max_window_bits_smallest ) {
    ext_vars v;
    v.attr["client_max_window_bits"] = "8";

    v.ec = v.exts.set_client_max_window_bits(15,pmd_mode::smallest);
    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; client_max_window_bits=8");
}


// Combinations with 2
BOOST_AUTO_TEST_CASE( negotiate_two_client_initiated1 ) {
    ext_vars v;

    v.attr["server_no_context_takeover"].clear();
    v.attr["client_no_context_takeover"].clear();

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_no_context_takeover; client_no_context_takeover");
}

BOOST_AUTO_TEST_CASE( negotiate_two_client_initiated2 ) {
    ext_vars v;

    v.attr["server_no_context_takeover"].clear();
    v.attr["server_max_window_bits"] = "10";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_no_context_takeover; server_max_window_bits=10");
}

BOOST_AUTO_TEST_CASE( negotiate_two_client_initiated3 ) {
    ext_vars v;

    v.attr["server_no_context_takeover"].clear();
    v.attr["client_max_window_bits"] = "10";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_no_context_takeover; client_max_window_bits=10");
}

BOOST_AUTO_TEST_CASE( negotiate_two_client_initiated4 ) {
    ext_vars v;

    v.attr["client_no_context_takeover"].clear();
    v.attr["server_max_window_bits"] = "10";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; client_no_context_takeover; server_max_window_bits=10");
}

BOOST_AUTO_TEST_CASE( negotiate_two_client_initiated5 ) {
    ext_vars v;

    v.attr["client_no_context_takeover"].clear();
    v.attr["client_max_window_bits"] = "10";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; client_no_context_takeover; client_max_window_bits=10");
}

BOOST_AUTO_TEST_CASE( negotiate_two_client_initiated6 ) {
    ext_vars v;

    v.attr["server_max_window_bits"] = "10";
    v.attr["client_max_window_bits"] = "10";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_max_window_bits=10; client_max_window_bits=10");
}

BOOST_AUTO_TEST_CASE( negotiate_three_client_initiated1 ) {
    ext_vars v;

    v.attr["server_no_context_takeover"].clear();
    v.attr["client_no_context_takeover"].clear();
    v.attr["server_max_window_bits"] = "10";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_no_context_takeover; client_no_context_takeover; server_max_window_bits=10");
}

BOOST_AUTO_TEST_CASE( negotiate_three_client_initiated2 ) {
    ext_vars v;

    v.attr["server_no_context_takeover"].clear();
    v.attr["client_no_context_takeover"].clear();
    v.attr["client_max_window_bits"] = "10";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_no_context_takeover; client_no_context_takeover; client_max_window_bits=10");
}

BOOST_AUTO_TEST_CASE( negotiate_three_client_initiated3 ) {
    ext_vars v;

    v.attr["server_no_context_takeover"].clear();
    v.attr["server_max_window_bits"] = "10";
    v.attr["client_max_window_bits"] = "10";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_no_context_takeover; server_max_window_bits=10; client_max_window_bits=10");
}

BOOST_AUTO_TEST_CASE( negotiate_three_client_initiated4 ) {
    ext_vars v;

    v.attr["client_no_context_takeover"].clear();
    v.attr["server_max_window_bits"] = "10";
    v.attr["client_max_window_bits"] = "10";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; client_no_context_takeover; server_max_window_bits=10; client_max_window_bits=10");
}

BOOST_AUTO_TEST_CASE( negotiate_four_client_initiated ) {
    ext_vars v;

    v.attr["server_no_context_takeover"].clear();
    v.attr["client_no_context_takeover"].clear();
    v.attr["server_max_window_bits"] = "10";
    v.attr["client_max_window_bits"] = "10";

    v.esp = v.exts.negotiate(v.attr);
    BOOST_CHECK( v.exts.is_enabled() );
    BOOST_CHECK_EQUAL( v.esp.first, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( v.esp.second, "permessage-deflate; server_no_context_takeover; client_no_context_takeover; server_max_window_bits=10; client_max_window_bits=10");
}

// Compression
BOOST_AUTO_TEST_CASE( compress_data ) {
    ext_vars v;

    std::string compress_in = "Hello";
    std::string compress_out;
    std::string decompress_out;

    v.exts.init(true);

    v.ec = v.exts.compress(compress_in,compress_out);
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );

    v.ec = v.exts.decompress(reinterpret_cast<const uint8_t *>(compress_out.data()),compress_out.size(),decompress_out);
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( compress_in, decompress_out );
}

BOOST_AUTO_TEST_CASE( compress_data_multiple ) {
    ext_vars v;

    v.exts.init(true);

    for (int i = 0; i < 2; i++) {
        std::string compress_in = "Hello";
        std::string compress_out;
        std::string decompress_out;

        v.ec = v.exts.compress(compress_in,compress_out);
        BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );

        v.ec = v.exts.decompress(reinterpret_cast<const uint8_t *>(compress_out.data()),compress_out.size(),decompress_out);
        BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
        BOOST_CHECK_EQUAL( compress_in, decompress_out );
    }
}

BOOST_AUTO_TEST_CASE( compress_data_large ) {
    ext_vars v;

    std::string compress_in(600,'*');
    std::string compress_out;
    std::string decompress_out;

    websocketpp::http::attribute_list alist;

    alist["server_max_window_bits"] = "8";
    v.exts.set_server_max_window_bits(8,websocketpp::extensions::permessage_deflate::mode::smallest);

    v.exts.negotiate(alist);
    v.exts.init(true);

    v.ec = v.exts.compress(compress_in,compress_out);
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );

    v.ec = v.exts.decompress(reinterpret_cast<const uint8_t *>(compress_out.data()),compress_out.size(),decompress_out);
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( compress_in, decompress_out );
}

BOOST_AUTO_TEST_CASE( compress_data_no_context_takeover ) {
    ext_vars v;

    std::string compress_in = "Hello";
    std::string compress_out1;
    std::string compress_out2;
    std::string decompress_out;

    websocketpp::http::attribute_list alist;

    alist["server_no_context_takeover"].clear();
    v.exts.enable_server_no_context_takeover();

    v.exts.negotiate(alist);
    v.exts.init(true);

    v.ec = v.exts.compress(compress_in,compress_out1);
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );

    v.ec = v.exts.decompress(
        reinterpret_cast<const uint8_t *>(compress_out1.data()),
        compress_out1.size(),
        decompress_out
    );
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( compress_in, decompress_out );

    decompress_out.clear();

    v.ec = v.exts.compress(compress_in,compress_out2);
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );

    v.ec = v.exts.decompress(
        reinterpret_cast<const uint8_t *>(compress_out2.data()),
        compress_out2.size(),
        decompress_out
    );
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( compress_in, decompress_out );

    BOOST_CHECK_EQUAL( compress_out1, compress_out2 );
}

BOOST_AUTO_TEST_CASE( compress_empty ) {
    ext_vars v;

    std::string compress_in;
    std::string compress_out;
    std::string decompress_out;

    v.exts.init(true);

    v.ec = v.exts.compress(compress_in,compress_out);
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );

    v.ec = v.exts.decompress(reinterpret_cast<const uint8_t *>(compress_out.data()),compress_out.size(),decompress_out);

    compress_out.clear();
    decompress_out.clear();

    v.ec = v.exts.compress(compress_in,compress_out);
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );

    v.ec = v.exts.decompress(reinterpret_cast<const uint8_t *>(compress_out.data()),compress_out.size(),decompress_out);
    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( compress_in, decompress_out );
}

/// @todo: more compression tests
/**
 * - compress at different compression levels
 */

// Decompression
BOOST_AUTO_TEST_CASE( decompress_data ) {
    ext_vars v;

    uint8_t in[11] = {0xf2, 0x48, 0xcd, 0xc9, 0xc9, 0x07, 0x00, 0x00, 0x00, 0xff, 0xff};
    std::string out;
    std::string reference = "Hello";

    v.exts.init(true);

    v.ec = v.exts.decompress(in,11,out);

    BOOST_CHECK_EQUAL( v.ec, websocketpp::lib::error_code() );
    BOOST_CHECK_EQUAL( out, reference );
}
