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
#define BOOST_TEST_MODULE utility
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>

#include <websocketpp/utilities.hpp>

BOOST_AUTO_TEST_SUITE ( utility )

BOOST_AUTO_TEST_CASE( substr_found ) {
    std::string haystack = "abc123";
    std::string needle = "abc";

    BOOST_CHECK(websocketpp::utility::ci_find_substr(haystack,needle) ==haystack.begin());
}

BOOST_AUTO_TEST_CASE( substr_found_ci ) {
    std::string haystack = "abc123";
    std::string needle = "aBc";

    BOOST_CHECK(websocketpp::utility::ci_find_substr(haystack,needle) ==haystack.begin());
}

BOOST_AUTO_TEST_CASE( substr_not_found ) {
    std::string haystack = "abd123";
    std::string needle = "abcd";

    BOOST_CHECK(websocketpp::utility::ci_find_substr(haystack,needle) == haystack.end());
}

BOOST_AUTO_TEST_CASE( to_lower ) {
    std::string in = "AbCd";

    BOOST_CHECK_EQUAL(websocketpp::utility::to_lower(in), "abcd");
}

BOOST_AUTO_TEST_CASE( string_replace_all ) {
    std::string source = "foo \"bar\" baz";
    std::string dest = "foo \\\"bar\\\" baz";

    using websocketpp::utility::string_replace_all;
    BOOST_CHECK_EQUAL(string_replace_all(source,"\"","\\\""),dest);
}

BOOST_AUTO_TEST_SUITE_END()
