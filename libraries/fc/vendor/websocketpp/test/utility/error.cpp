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
#define BOOST_TEST_MODULE error
#include <boost/test/unit_test.hpp>

#include <websocketpp/error.hpp>

BOOST_AUTO_TEST_CASE( constructing_exceptions ) {
    websocketpp::lib::error_code test_ec = websocketpp::error::make_error_code(websocketpp::error::test);
    websocketpp::lib::error_code general_ec = websocketpp::error::make_error_code(websocketpp::error::general);

    websocketpp::exception b("foo");
    websocketpp::exception c("foo",test_ec);
    websocketpp::exception d("");
    websocketpp::exception e("",test_ec);

    BOOST_CHECK_EQUAL(b.what(),"foo");
    BOOST_CHECK_EQUAL(b.code(),general_ec);

    BOOST_CHECK_EQUAL(c.what(),"foo");
    BOOST_CHECK_EQUAL(c.code(),test_ec);

    BOOST_CHECK_EQUAL(d.what(),"Generic error");
    BOOST_CHECK_EQUAL(d.code(),general_ec);

    BOOST_CHECK_EQUAL(e.what(),"Test Error");
    BOOST_CHECK_EQUAL(e.code(),test_ec);
}

