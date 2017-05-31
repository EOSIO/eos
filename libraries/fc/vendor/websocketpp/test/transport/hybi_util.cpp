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
#define BOOST_TEST_MODULE hybi_util
#include <boost/test/unit_test.hpp>

#include <iostream>

#include "../../src/processors/hybi_util.hpp"
#include "../../src/network_utilities.hpp"

BOOST_AUTO_TEST_CASE( circshift_0 ) {
    if (sizeof(size_t) == 8) {
        size_t test = 0x0123456789abcdef;

        test = websocketpp::processor::hybi_util::circshift_prepared_key(test,0);

        BOOST_CHECK( test == 0x0123456789abcdef);
    } else {
        size_t test = 0x01234567;

        test = websocketpp::processor::hybi_util::circshift_prepared_key(test,0);

        BOOST_CHECK( test == 0x01234567);
    }
}

BOOST_AUTO_TEST_CASE( circshift_1 ) {
    if (sizeof(size_t) == 8) {
        size_t test = 0x0123456789abcdef;

        test = websocketpp::processor::hybi_util::circshift_prepared_key(test,1);

        BOOST_CHECK( test == 0xef0123456789abcd);
    } else {
        size_t test = 0x01234567;

        test = websocketpp::processor::hybi_util::circshift_prepared_key(test,1);

        BOOST_CHECK( test == 0x67012345);
    }
}

BOOST_AUTO_TEST_CASE( circshift_2 ) {
    if (sizeof(size_t) == 8) {
        size_t test = 0x0123456789abcdef;

        test = websocketpp::processor::hybi_util::circshift_prepared_key(test,2);

        BOOST_CHECK( test == 0xcdef0123456789ab);
    } else {
        size_t test = 0x01234567;

        test = websocketpp::processor::hybi_util::circshift_prepared_key(test,2);

        BOOST_CHECK( test == 0x45670123);
    }
}

BOOST_AUTO_TEST_CASE( circshift_3 ) {
    if (sizeof(size_t) == 8) {
        size_t test = 0x0123456789abcdef;

        test = websocketpp::processor::hybi_util::circshift_prepared_key(test,3);

        BOOST_CHECK( test == 0xabcdef0123456789);
    } else {
        size_t test = 0x01234567;

        test = websocketpp::processor::hybi_util::circshift_prepared_key(test,3);

        BOOST_CHECK( test == 0x23456701);
    }
}
