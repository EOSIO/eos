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
#define BOOST_TEST_MODULE extension_permessage_deflate
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>
#include <websocketpp/common/memory.hpp>

#include <websocketpp/http/request.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

struct config {
    typedef websocketpp::http::parser::request request_type;
};
typedef websocketpp::extensions::permessage_deflate::enabled<config>
    compressor_type;

using namespace websocketpp;

BOOST_AUTO_TEST_CASE( deflate_init ) {
    /*compressor_type compressor;
    websocketpp::http::parser::attribute_list attributes;
    std::pair<lib::error_code,std::string> neg_ret;

    neg_ret = compressor.negotiate(attributes);

    BOOST_CHECK_EQUAL( neg_ret.first,
        extensions::permessage_deflate::error::invalid_parameters );*/

    /**
     * Window size is primarily controlled by the writer. A stream can only be
     * read by a window size equal to or greater than the one use to compress
     * it initially. The default windows size is also the maximum window size.
     * Thus:
     *
     * Outbound window size can be limited unilaterally under the assumption
     * that the opposite end will be using the default (maximum size which can
     * read anything)
     *
     * Inbound window size must be limited by asking the remote endpoint to
     * do so and it agreeing.
     *
     * Context takeover is also primarily controlled by the writer. If the
     * compressor does not clear its context between messages then the reader
     * can't either.
     *
     * Outbound messages may clear context between messages unilaterally.
     * Inbound messages must retain state unless the remote endpoint signals
     * otherwise.
     *
     * Negotiation options:
     * Client must choose from the following options:
     * - whether or not to request an inbound window limit
     * - whether or not to signal that it will honor an outbound window limit
     * - whether or not to request that the server disallow context takeover
     *
     * Server must answer in the following ways
     * - If client requested a window size limit, is the window size limit
     *   acceptable?
     * - If client allows window limit requests, should we send one?
     * - If client requested no context takeover, should we accept?
     *
     *
     *
     * All Defaults
     * Req: permessage-compress; method=deflate
     * Ans: permessage-compress; method=deflate
     *
     * # Client wants to limit the size of inbound windows from server
     * permessage-compress; method="deflate; s2c_max_window_bits=8, deflate"
     * Ans: permessage-compress; method="deflate; s2c_max_window_bits=8"
     * OR
     * Ans: permessage-compress; method=deflate
     *
     * # Server wants to limit the size of inbound windows from client
     * Client:
     * permessage-compress; method="deflate; c2s_max_window_bits, deflate"
     *
     * Server:
     * permessage-compress; method="deflate; c2s_max_window_bits=8"
     *
     * # Client wants to
     *
     *
     *
     *
     *
     *
     */




   /* processor::extensions::deflate_method d(true);
    http::parser::attribute_list attributes;
    lib::error_code ec;

    attributes.push_back(http::parser::attribute("foo","bar"));
    ec = d.init(attributes);
    BOOST_CHECK(ec == processor::extensions::error::unknown_method_parameter);

    attributes.clear();
    attributes.push_back(http::parser::attribute("s2c_max_window_bits","bar"));
    ec = d.init(attributes);
    BOOST_CHECK(ec == processor::extensions::error::invalid_algorithm_settings);

    attributes.clear();
    attributes.push_back(http::parser::attribute("s2c_max_window_bits","7"));
    ec = d.init(attributes);
    BOOST_CHECK(ec == processor::extensions::error::invalid_algorithm_settings);

    attributes.clear();
    attributes.push_back(http::parser::attribute("s2c_max_window_bits","16"));
    ec = d.init(attributes);
    BOOST_CHECK(ec == processor::extensions::error::invalid_algorithm_settings);

    attributes.clear();
    attributes.push_back(http::parser::attribute("s2c_max_window_bits","9"));
    ec = d.init(attributes);
    BOOST_CHECK( !ec);

    attributes.clear();
    ec = d.init(attributes);
    BOOST_CHECK( !ec);

    processor::extensions::deflate_engine de;

    unsigned char test_in[] = "HelloHelloHelloHello";
    unsigned char test_out[30];

    uLongf test_out_size = 30;

    int ret;

    ret = compress(test_out, &test_out_size, test_in, 20);

    std::cout << ret << std::endl
              << websocketpp::utility::to_hex(test_in,20) << std::endl
              << websocketpp::utility::to_hex(test_out,test_out_size) << std::endl;

    std::string input = "Hello";
    std::string output;
    ec = de.compress(input,output);

    BOOST_CHECK( ec == processor::extensions::error::uninitialized );

    //std::cout << ec.message() << websocketpp::utility::to_hex(output) << std::endl;

    ec = de.init(15,15,Z_DEFAULT_COMPRESSION,8,Z_FIXED);
    //ec = de.init();
    BOOST_CHECK( !ec );

    ec = de.compress(input,output);
    std::cout << ec.message() << std::endl
              << websocketpp::utility::to_hex(input) << std::endl
              << websocketpp::utility::to_hex(output) << std::endl;

    output.clear();

    ec = de.compress(input,output);
    std::cout << ec.message() << std::endl
              << websocketpp::utility::to_hex(input) << std::endl
              << websocketpp::utility::to_hex(output) << std::endl;

    input = output;
    output.clear();
    ec = de.decompress(input,output);
    std::cout << ec.message() << std::endl
              << websocketpp::utility::to_hex(input) << std::endl
              << websocketpp::utility::to_hex(output) << std::endl;
    */
}
