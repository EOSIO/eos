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
#define BOOST_TEST_MODULE http_parser
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>

#include <websocketpp/http/request.hpp>
#include <websocketpp/http/response.hpp>

BOOST_AUTO_TEST_CASE( is_token_char ) {
    // Valid characters

    // misc
    BOOST_CHECK( websocketpp::http::is_token_char('!') );
    BOOST_CHECK( websocketpp::http::is_token_char('#') );
    BOOST_CHECK( websocketpp::http::is_token_char('$') );
    BOOST_CHECK( websocketpp::http::is_token_char('%') );
    BOOST_CHECK( websocketpp::http::is_token_char('&') );
    BOOST_CHECK( websocketpp::http::is_token_char('\'') );
    BOOST_CHECK( websocketpp::http::is_token_char('*') );
    BOOST_CHECK( websocketpp::http::is_token_char('+') );
    BOOST_CHECK( websocketpp::http::is_token_char('-') );
    BOOST_CHECK( websocketpp::http::is_token_char('.') );
    BOOST_CHECK( websocketpp::http::is_token_char('^') );
    BOOST_CHECK( websocketpp::http::is_token_char('_') );
    BOOST_CHECK( websocketpp::http::is_token_char('`') );
    BOOST_CHECK( websocketpp::http::is_token_char('~') );

    // numbers
    for (int i = 0x30; i < 0x3a; i++) {
        BOOST_CHECK( websocketpp::http::is_token_char((unsigned char)(i)) );
    }

    // upper
    for (int i = 0x41; i < 0x5b; i++) {
        BOOST_CHECK( websocketpp::http::is_token_char((unsigned char)(i)) );
    }

    // lower
    for (int i = 0x61; i < 0x7b; i++) {
        BOOST_CHECK( websocketpp::http::is_token_char((unsigned char)(i)) );
    }

    // invalid characters

    // lower unprintable
    for (int i = 0; i < 33; i++) {
        BOOST_CHECK( !websocketpp::http::is_token_char((unsigned char)(i)) );
    }

    // misc
    BOOST_CHECK( !websocketpp::http::is_token_char('(') );
    BOOST_CHECK( !websocketpp::http::is_token_char(')') );
    BOOST_CHECK( !websocketpp::http::is_token_char('<') );
    BOOST_CHECK( !websocketpp::http::is_token_char('>') );
    BOOST_CHECK( !websocketpp::http::is_token_char('@') );
    BOOST_CHECK( !websocketpp::http::is_token_char(',') );
    BOOST_CHECK( !websocketpp::http::is_token_char(';') );
    BOOST_CHECK( !websocketpp::http::is_token_char(':') );
    BOOST_CHECK( !websocketpp::http::is_token_char('\\') );
    BOOST_CHECK( !websocketpp::http::is_token_char('"') );
    BOOST_CHECK( !websocketpp::http::is_token_char('/') );
    BOOST_CHECK( !websocketpp::http::is_token_char('[') );
    BOOST_CHECK( !websocketpp::http::is_token_char(']') );
    BOOST_CHECK( !websocketpp::http::is_token_char('?') );
    BOOST_CHECK( !websocketpp::http::is_token_char('=') );
    BOOST_CHECK( !websocketpp::http::is_token_char('{') );
    BOOST_CHECK( !websocketpp::http::is_token_char('}') );

    // upper unprintable and out of ascii range
    for (int i = 127; i < 256; i++) {
        BOOST_CHECK( !websocketpp::http::is_token_char((unsigned char)(i)) );
    }

    // is not
    BOOST_CHECK( !websocketpp::http::is_not_token_char('!') );
    BOOST_CHECK( websocketpp::http::is_not_token_char('(') );
}

BOOST_AUTO_TEST_CASE( extract_token ) {
    std::string d1 = "foo";
    std::string d2 = " foo ";

    std::pair<std::string,std::string::const_iterator> ret;

    ret = websocketpp::http::parser::extract_token(d1.begin(),d1.end());
    BOOST_CHECK( ret.first == "foo" );
    BOOST_CHECK( ret.second == d1.begin()+3 );

    ret = websocketpp::http::parser::extract_token(d2.begin(),d2.end());
    BOOST_CHECK( ret.first.empty() );
    BOOST_CHECK( ret.second == d2.begin()+0 );

    ret = websocketpp::http::parser::extract_token(d2.begin()+1,d2.end());
    BOOST_CHECK( ret.first == "foo" );
    BOOST_CHECK( ret.second == d2.begin()+4 );
}

BOOST_AUTO_TEST_CASE( extract_quoted_string ) {
    std::string d1 = "\"foo\"";
    std::string d2 = "\"foo\\\"bar\\\"baz\"";
    std::string d3 = "\"foo\"     ";
    std::string d4;
    std::string d5 = "foo";

    std::pair<std::string,std::string::const_iterator> ret;

    using websocketpp::http::parser::extract_quoted_string;

    ret = extract_quoted_string(d1.begin(),d1.end());
    BOOST_CHECK( ret.first == "foo" );
    BOOST_CHECK( ret.second == d1.end() );

    ret = extract_quoted_string(d2.begin(),d2.end());
    BOOST_CHECK( ret.first == "foo\"bar\"baz" );
    BOOST_CHECK( ret.second == d2.end() );

    ret = extract_quoted_string(d3.begin(),d3.end());
    BOOST_CHECK( ret.first == "foo" );
    BOOST_CHECK( ret.second == d3.begin()+5 );

    ret = extract_quoted_string(d4.begin(),d4.end());
    BOOST_CHECK( ret.first.empty() );
    BOOST_CHECK( ret.second == d4.begin() );

    ret = extract_quoted_string(d5.begin(),d5.end());
    BOOST_CHECK( ret.first.empty() );
    BOOST_CHECK( ret.second == d5.begin() );
}

BOOST_AUTO_TEST_CASE( extract_all_lws ) {
    std::string d1 = " foo     bar";
    d1.append(1,char(9));
    d1.append("baz\r\n d\r\n  \r\n  e\r\nf");

    std::string::const_iterator ret;

    ret = websocketpp::http::parser::extract_all_lws(d1.begin(),d1.end());
    BOOST_CHECK( ret == d1.begin()+1 );

    ret = websocketpp::http::parser::extract_all_lws(d1.begin()+1,d1.end());
    BOOST_CHECK( ret == d1.begin()+1 );

    ret = websocketpp::http::parser::extract_all_lws(d1.begin()+4,d1.end());
    BOOST_CHECK( ret == d1.begin()+9 );

    ret = websocketpp::http::parser::extract_all_lws(d1.begin()+12,d1.end());
    BOOST_CHECK( ret == d1.begin()+13 );

    ret = websocketpp::http::parser::extract_all_lws(d1.begin()+16,d1.end());
    BOOST_CHECK( ret == d1.begin()+19 );

    ret = websocketpp::http::parser::extract_all_lws(d1.begin()+20,d1.end());
    BOOST_CHECK( ret == d1.begin()+28 );

    ret = websocketpp::http::parser::extract_all_lws(d1.begin()+29,d1.end());
    BOOST_CHECK( ret == d1.begin()+29 );
}

BOOST_AUTO_TEST_CASE( extract_attributes_blank ) {
    std::string s;

    websocketpp::http::attribute_list a;
    std::string::const_iterator it;

    it = websocketpp::http::parser::extract_attributes(s.begin(),s.end(),a);
    BOOST_CHECK( it == s.begin() );
    BOOST_CHECK_EQUAL( a.size(), 0 );
}

BOOST_AUTO_TEST_CASE( extract_attributes_simple ) {
    std::string s = "foo";

    websocketpp::http::attribute_list a;
    std::string::const_iterator it;

    it = websocketpp::http::parser::extract_attributes(s.begin(),s.end(),a);
    BOOST_CHECK( it == s.end() );
    BOOST_CHECK_EQUAL( a.size(), 1 );
    BOOST_CHECK( a.find("foo") != a.end() );
    BOOST_CHECK_EQUAL( a.find("foo")->second, "" );
}

BOOST_AUTO_TEST_CASE( extract_parameters ) {
    std::string s1;
    std::string s2 = "foo";
    std::string s3 = " foo \r\nAbc";
    std::string s4 = "  \r\n   foo  ";
    std::string s5 = "foo,bar";
    std::string s6 = "foo;bar";
    std::string s7 = "foo;baz,bar";
    std::string s8 = "foo;bar;baz";
    std::string s9 = "foo;bar=baz";
    std::string s10 = "foo;bar=baz;boo";
    std::string s11 = "foo;bar=baz;boo,bob";
    std::string s12 = "foo;bar=\"a b c\"";
    std::string s13 = "foo;bar=\"a \\\"b\\\" c\"";


    std::string sx = "foo;bar=\"a \\\"b\\\" c\"";
    websocketpp::http::parameter_list p;
    websocketpp::http::attribute_list a;
    std::string::const_iterator it;

    using websocketpp::http::parser::extract_parameters;

    it = extract_parameters(s1.begin(),s1.end(),p);
    BOOST_CHECK( it == s1.begin() );

    p.clear();
    it = extract_parameters(s2.begin(),s2.end(),p);
    BOOST_CHECK( it == s2.end() );
    BOOST_CHECK_EQUAL( p.size(), 1 );
    BOOST_CHECK( p[0].first == "foo" );
    BOOST_CHECK_EQUAL( p[0].second.size(), 0 );

    p.clear();
    it = extract_parameters(s3.begin(),s3.end(),p);
    BOOST_CHECK( it == s3.begin()+5 );
    BOOST_CHECK_EQUAL( p.size(), 1 );
    BOOST_CHECK( p[0].first == "foo" );
    BOOST_CHECK_EQUAL( p[0].second.size(), 0 );

    p.clear();
    it = extract_parameters(s4.begin(),s4.end(),p);
    BOOST_CHECK( it == s4.end() );
    BOOST_CHECK_EQUAL( p.size(), 1 );
    BOOST_CHECK( p[0].first == "foo" );
    BOOST_CHECK_EQUAL( p[0].second.size(), 0 );

    p.clear();
    it = extract_parameters(s5.begin(),s5.end(),p);
    BOOST_CHECK( it == s5.end() );
    BOOST_CHECK_EQUAL( p.size(), 2 );
    BOOST_CHECK( p[0].first == "foo" );
    BOOST_CHECK_EQUAL( p[0].second.size(), 0 );
    BOOST_CHECK( p[1].first == "bar" );
    BOOST_CHECK_EQUAL( p[1].second.size(), 0 );

    p.clear();
    it = extract_parameters(s6.begin(),s6.end(),p);
    BOOST_CHECK( it == s6.end() );
    BOOST_CHECK_EQUAL( p.size(), 1 );
    BOOST_CHECK( p[0].first == "foo" );
    a = p[0].second;
    BOOST_CHECK_EQUAL( a.size(), 1 );
    BOOST_CHECK( a.find("bar") != a.end() );
    BOOST_CHECK_EQUAL( a.find("bar")->second, "" );

    p.clear();
    it = extract_parameters(s7.begin(),s7.end(),p);
    BOOST_CHECK( it == s7.end() );
    BOOST_CHECK_EQUAL( p.size(), 2 );
    BOOST_CHECK( p[0].first == "foo" );
    a = p[0].second;
    BOOST_CHECK_EQUAL( a.size(), 1 );
    BOOST_CHECK( a.find("baz") != a.end() );
    BOOST_CHECK_EQUAL( a.find("baz")->second, "" );
    BOOST_CHECK( p[1].first == "bar" );
    a = p[1].second;
    BOOST_CHECK_EQUAL( a.size(), 0 );

    p.clear();
    it = extract_parameters(s8.begin(),s8.end(),p);
    BOOST_CHECK( it == s8.end() );
    BOOST_CHECK_EQUAL( p.size(), 1 );
    BOOST_CHECK( p[0].first == "foo" );
    a = p[0].second;
    BOOST_CHECK_EQUAL( a.size(), 2 );
    BOOST_CHECK( a.find("bar") != a.end() );
    BOOST_CHECK_EQUAL( a.find("bar")->second, "" );
    BOOST_CHECK( a.find("baz") != a.end() );
    BOOST_CHECK_EQUAL( a.find("baz")->second, "" );

    p.clear();
    it = extract_parameters(s9.begin(),s9.end(),p);
    BOOST_CHECK( it == s9.end() );
    BOOST_CHECK_EQUAL( p.size(), 1 );
    BOOST_CHECK( p[0].first == "foo" );
    a = p[0].second;
    BOOST_CHECK_EQUAL( a.size(), 1 );
    BOOST_CHECK( a.find("bar") != a.end() );
    BOOST_CHECK_EQUAL( a.find("bar")->second, "baz" );

    p.clear();
    it = extract_parameters(s10.begin(),s10.end(),p);
    BOOST_CHECK( it == s10.end() );
    BOOST_CHECK_EQUAL( p.size(), 1 );
    BOOST_CHECK( p[0].first == "foo" );
    a = p[0].second;
    BOOST_CHECK_EQUAL( a.size(), 2 );
    BOOST_CHECK( a.find("bar") != a.end() );
    BOOST_CHECK_EQUAL( a.find("bar")->second, "baz" );
    BOOST_CHECK( a.find("boo") != a.end() );
    BOOST_CHECK_EQUAL( a.find("boo")->second, "" );

    p.clear();
    it = extract_parameters(s11.begin(),s11.end(),p);
    BOOST_CHECK( it == s11.end() );
    BOOST_CHECK_EQUAL( p.size(), 2 );
    BOOST_CHECK( p[0].first == "foo" );
    a = p[0].second;
    BOOST_CHECK_EQUAL( a.size(), 2 );
    BOOST_CHECK( a.find("bar") != a.end() );
    BOOST_CHECK_EQUAL( a.find("bar")->second, "baz" );
    BOOST_CHECK( a.find("boo") != a.end() );
    BOOST_CHECK_EQUAL( a.find("boo")->second, "" );
    a = p[1].second;
    BOOST_CHECK_EQUAL( a.size(), 0 );

    p.clear();
    it = extract_parameters(s12.begin(),s12.end(),p);
    BOOST_CHECK( it == s12.end() );
    BOOST_CHECK_EQUAL( p.size(), 1 );
    BOOST_CHECK( p[0].first == "foo" );
    a = p[0].second;
    BOOST_CHECK_EQUAL( a.size(), 1 );
    BOOST_CHECK( a.find("bar") != a.end() );
    BOOST_CHECK_EQUAL( a.find("bar")->second, "a b c" );

    p.clear();
    it = extract_parameters(s13.begin(),s13.end(),p);
    BOOST_CHECK( it == s13.end() );
    BOOST_CHECK_EQUAL( p.size(), 1 );
    BOOST_CHECK( p[0].first == "foo" );
    a = p[0].second;
    BOOST_CHECK_EQUAL( a.size(), 1 );
    BOOST_CHECK( a.find("bar") != a.end() );
    BOOST_CHECK_EQUAL( a.find("bar")->second, "a \"b\" c" );
}

BOOST_AUTO_TEST_CASE( strip_lws ) {
    std::string test1 = "foo";
    std::string test2 = " foo ";
    std::string test3 = "foo ";
    std::string test4 = " foo";
    std::string test5 = "    foo     ";
    std::string test6 = "  \r\n  foo     ";
    std::string test7 = "  \t  foo     ";
    std::string test8 = "  \t       ";
    std::string test9 = " \n\r";

    BOOST_CHECK_EQUAL( websocketpp::http::parser::strip_lws(test1), "foo" );
    BOOST_CHECK_EQUAL( websocketpp::http::parser::strip_lws(test2), "foo" );
    BOOST_CHECK_EQUAL( websocketpp::http::parser::strip_lws(test3), "foo" );
    BOOST_CHECK_EQUAL( websocketpp::http::parser::strip_lws(test4), "foo" );
    BOOST_CHECK_EQUAL( websocketpp::http::parser::strip_lws(test5), "foo" );
    BOOST_CHECK_EQUAL( websocketpp::http::parser::strip_lws(test6), "foo" );
    BOOST_CHECK_EQUAL( websocketpp::http::parser::strip_lws(test7), "foo" );
    BOOST_CHECK_EQUAL( websocketpp::http::parser::strip_lws(test8), "" );
    BOOST_CHECK_EQUAL( websocketpp::http::parser::strip_lws(test9), "" );
}

BOOST_AUTO_TEST_CASE( case_insensitive_headers ) {
    websocketpp::http::parser::parser r;

    r.replace_header("foo","bar");

    BOOST_CHECK_EQUAL( r.get_header("foo"), "bar" );
    BOOST_CHECK_EQUAL( r.get_header("FOO"), "bar" );
    BOOST_CHECK_EQUAL( r.get_header("Foo"), "bar" );
}

BOOST_AUTO_TEST_CASE( case_insensitive_headers_overwrite ) {
    websocketpp::http::parser::parser r;

    r.replace_header("foo","bar");

    BOOST_CHECK_EQUAL( r.get_header("foo"), "bar" );
    BOOST_CHECK_EQUAL( r.get_header("Foo"), "bar" );

    r.replace_header("Foo","baz");

    BOOST_CHECK_EQUAL( r.get_header("foo"), "baz" );
    BOOST_CHECK_EQUAL( r.get_header("Foo"), "baz" );

    r.remove_header("FoO");

    BOOST_CHECK_EQUAL( r.get_header("foo"), "" );
    BOOST_CHECK_EQUAL( r.get_header("Foo"), "" );
}

BOOST_AUTO_TEST_CASE( blank_consume ) {
    websocketpp::http::parser::request r;

    std::string raw;

    bool exception = false;

    try {
        r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK( r.ready() == false );
}

BOOST_AUTO_TEST_CASE( blank_request ) {
    websocketpp::http::parser::request r;

    std::string raw = "\r\n\r\n";

    bool exception = false;

    try {
        r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == true );
    BOOST_CHECK( r.ready() == false );
}

BOOST_AUTO_TEST_CASE( bad_request_no_host ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\n\r\n";

    bool exception = false;

    try {
        r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == true );
    BOOST_CHECK( r.ready() == false );
}

BOOST_AUTO_TEST_CASE( basic_request ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK( pos == 41 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK( r.get_version() == "HTTP/1.1" );
    BOOST_CHECK( r.get_method() == "GET" );
    BOOST_CHECK( r.get_uri() == "/" );
    BOOST_CHECK( r.get_header("Host") == "www.example.com" );
}

BOOST_AUTO_TEST_CASE( basic_request_with_body ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost: www.example.com\r\nContent-Length: 5\r\n\r\nabcdef";

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 65 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.1" );
    BOOST_CHECK_EQUAL( r.get_method(), "GET" );
    BOOST_CHECK_EQUAL( r.get_uri(), "/" );
    BOOST_CHECK_EQUAL( r.get_header("Host"), "www.example.com" );
    BOOST_CHECK_EQUAL( r.get_header("Content-Length"), "5" );
    BOOST_CHECK_EQUAL( r.get_body(), "abcde" );
}

BOOST_AUTO_TEST_CASE( basic_request_with_body_split ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost: www.example.com\r\nContent-Length: 6\r\n\r\nabc";
    std::string raw2 = "def";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
        pos += r.consume(raw2.c_str(),raw2.size());
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 66 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.1" );
    BOOST_CHECK_EQUAL( r.get_method(), "GET" );
    BOOST_CHECK_EQUAL( r.get_uri(), "/" );
    BOOST_CHECK_EQUAL( r.get_header("Host"), "www.example.com" );
    BOOST_CHECK_EQUAL( r.get_header("Content-Length"), "6" );
    BOOST_CHECK_EQUAL( r.get_body(), "abcdef" );
}



BOOST_AUTO_TEST_CASE( trailing_body_characters ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\na";

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK( pos == 41 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK( r.get_version() == "HTTP/1.1" );
    BOOST_CHECK( r.get_method() == "GET" );
    BOOST_CHECK( r.get_uri() == "/" );
    BOOST_CHECK( r.get_header("Host") == "www.example.com" );
}

BOOST_AUTO_TEST_CASE( trailing_body_characters_beyond_max_lenth ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
    raw.append(websocketpp::http::max_header_size,'*');

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK( pos == 41 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK( r.get_version() == "HTTP/1.1" );
    BOOST_CHECK( r.get_method() == "GET" );
    BOOST_CHECK( r.get_uri() == "/" );
    BOOST_CHECK( r.get_header("Host") == "www.example.com" );
}

BOOST_AUTO_TEST_CASE( basic_split1 ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\n";
    std::string raw2 = "Host: www.example.com\r\n\r\na";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
        pos += r.consume(raw2.c_str(),raw2.size());
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK( pos == 41 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK( r.get_version() == "HTTP/1.1" );
    BOOST_CHECK( r.get_method() == "GET" );
    BOOST_CHECK( r.get_uri() == "/" );
    BOOST_CHECK( r.get_header("Host") == "www.example.com" );
}

BOOST_AUTO_TEST_CASE( basic_split2 ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost: www.example.com\r";
    std::string raw2 = "\n\r\na";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
        pos += r.consume(raw2.c_str(),raw2.size());
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK( pos == 41 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK( r.get_version() == "HTTP/1.1" );
    BOOST_CHECK( r.get_method() == "GET" );
    BOOST_CHECK( r.get_uri() == "/" );
    BOOST_CHECK( r.get_header("Host") == "www.example.com" );
}

BOOST_AUTO_TEST_CASE( max_header_len ) {
    websocketpp::http::parser::request r;

    std::string raw(websocketpp::http::max_header_size-1,'*');
    raw += "\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
    } catch (const websocketpp::http::exception& e) {
        if (e.m_error_code == websocketpp::http::status_code::request_header_fields_too_large) {
            exception = true;
        }
    }

    BOOST_CHECK( exception == true );
}

BOOST_AUTO_TEST_CASE( max_header_len_split ) {
    websocketpp::http::parser::request r;

    std::string raw(websocketpp::http::max_header_size-1,'*');
    std::string raw2(2,'*');

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
        pos += r.consume(raw2.c_str(),raw2.size());
    } catch (const websocketpp::http::exception& e) {
        if (e.m_error_code == websocketpp::http::status_code::request_header_fields_too_large) {
            exception = true;
        }
    }

    BOOST_CHECK( exception == true );
}

BOOST_AUTO_TEST_CASE( max_body_len ) {
    websocketpp::http::parser::request r;
    
    r.set_max_body_size(5);

    std::string raw = "GET / HTTP/1.1\r\nHost: www.example.com\r\nContent-Length: 6\r\n\r\nabcdef";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
    } catch (websocketpp::http::exception const & e) {
        exception = true;
        BOOST_CHECK_EQUAL(e.m_error_code,websocketpp::http::status_code::request_entity_too_large);
    }

    BOOST_CHECK_EQUAL(r.get_max_body_size(),5);
    BOOST_CHECK( exception == true );
}

BOOST_AUTO_TEST_CASE( firefox_full_request ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost: localhost:5000\r\nUser-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.7; rv:10.0) Gecko/20100101 Firefox/10.0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Language: en-us,en;q=0.5\r\nAccept-Encoding: gzip, deflate\r\nConnection: keep-alive, Upgrade\r\nSec-WebSocket-Version: 8\r\nSec-WebSocket-Origin: http://zaphoyd.com\r\nSec-WebSocket-Key: pFik//FxwFk0riN4ZiPFjQ==\r\nPragma: no-cache\r\nCache-Control: no-cache\r\nUpgrade: websocket\r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK( pos == 482 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK( r.get_version() == "HTTP/1.1" );
    BOOST_CHECK( r.get_method() == "GET" );
    BOOST_CHECK( r.get_uri() == "/" );
    BOOST_CHECK( r.get_header("Host") == "localhost:5000" );
    BOOST_CHECK( r.get_header("User-Agent") == "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.7; rv:10.0) Gecko/20100101 Firefox/10.0" );
    BOOST_CHECK( r.get_header("Accept") == "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8" );
    BOOST_CHECK( r.get_header("Accept-Language") == "en-us,en;q=0.5" );
    BOOST_CHECK( r.get_header("Accept-Encoding") == "gzip, deflate" );
    BOOST_CHECK( r.get_header("Connection") == "keep-alive, Upgrade" );
    BOOST_CHECK( r.get_header("Sec-WebSocket-Version") == "8" );
    BOOST_CHECK( r.get_header("Sec-WebSocket-Origin") == "http://zaphoyd.com" );
    BOOST_CHECK( r.get_header("Sec-WebSocket-Key") == "pFik//FxwFk0riN4ZiPFjQ==" );
    BOOST_CHECK( r.get_header("Pragma") == "no-cache" );
    BOOST_CHECK( r.get_header("Cache-Control") == "no-cache" );
    BOOST_CHECK( r.get_header("Upgrade") == "websocket" );
}

BOOST_AUTO_TEST_CASE( bad_method ) {
    websocketpp::http::parser::request r;

    std::string raw = "GE]T / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";

    bool exception = false;

    try {
        r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == true );
}

BOOST_AUTO_TEST_CASE( bad_header_name ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHo]st: www.example.com\r\n\r\n";

    bool exception = false;

    try {
        r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == true );
}

BOOST_AUTO_TEST_CASE( old_http_version ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.0\r\nHost: www.example.com\r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 41 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.0" );
    BOOST_CHECK_EQUAL( r.get_method(), "GET" );
    BOOST_CHECK_EQUAL( r.get_uri(), "/" );
    BOOST_CHECK_EQUAL( r.get_header("Host"), "www.example.com" );
}

BOOST_AUTO_TEST_CASE( new_http_version1 ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.12\r\nHost: www.example.com\r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 42 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.12" );
    BOOST_CHECK_EQUAL( r.get_method(), "GET" );
    BOOST_CHECK_EQUAL( r.get_uri(), "/" );
    BOOST_CHECK_EQUAL( r.get_header("Host"), "www.example.com" );
}

BOOST_AUTO_TEST_CASE( new_http_version2 ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/12.12\r\nHost: www.example.com\r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 43 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/12.12" );
    BOOST_CHECK_EQUAL( r.get_method(), "GET" );
    BOOST_CHECK_EQUAL( r.get_uri(), "/" );
    BOOST_CHECK_EQUAL( r.get_header("Host"), "www.example.com" );
}

/* commented out due to not being implemented yet

BOOST_AUTO_TEST_CASE( new_http_version3 ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTPS/12.12\r\nHost: www.example.com\r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == true );
}*/

BOOST_AUTO_TEST_CASE( header_whitespace1 ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost:  www.example.com \r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 43 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.1" );
    BOOST_CHECK_EQUAL( r.get_method(), "GET" );
    BOOST_CHECK_EQUAL( r.get_uri(), "/" );
    BOOST_CHECK_EQUAL( r.get_header("Host"), "www.example.com" );
}

BOOST_AUTO_TEST_CASE( header_whitespace2 ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost:www.example.com\r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 40 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.1" );
    BOOST_CHECK_EQUAL( r.get_method(), "GET" );
    BOOST_CHECK_EQUAL( r.get_uri(), "/" );
    BOOST_CHECK_EQUAL( r.get_header("Host"), "www.example.com" );
}

BOOST_AUTO_TEST_CASE( header_aggregation ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost: www.example.com\r\nFoo: bar\r\nFoo: bat\r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos = r.consume(raw.c_str(),raw.size());
    } catch (...) {
        exception = true;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 61 );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.1" );
    BOOST_CHECK_EQUAL( r.get_method(), "GET" );
    BOOST_CHECK_EQUAL( r.get_uri(), "/" );
    BOOST_CHECK_EQUAL( r.get_header("Foo"), "bar, bat" );
}

BOOST_AUTO_TEST_CASE( wikipedia_example_response ) {
    websocketpp::http::parser::response r;

    std::string raw = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=\r\nSec-WebSocket-Protocol: chat\r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 159 );
    BOOST_CHECK( r.headers_ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.1" );
    BOOST_CHECK_EQUAL( r.get_status_code(), websocketpp::http::status_code::switching_protocols );
    BOOST_CHECK_EQUAL( r.get_status_msg(), "Switching Protocols" );
    BOOST_CHECK_EQUAL( r.get_header("Upgrade"), "websocket" );
    BOOST_CHECK_EQUAL( r.get_header("Connection"), "Upgrade" );
    BOOST_CHECK_EQUAL( r.get_header("Sec-WebSocket-Accept"), "HSmrc0sMlYUkAGmm5OPpG2HaGWk=" );
    BOOST_CHECK_EQUAL( r.get_header("Sec-WebSocket-Protocol"), "chat" );
}

BOOST_AUTO_TEST_CASE( wikipedia_example_response_trailing ) {
    websocketpp::http::parser::response r;

    std::string raw = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=\r\nSec-WebSocket-Protocol: chat\r\n\r\n";
    raw += "a";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 159 );
    BOOST_CHECK( r.headers_ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.1" );
    BOOST_CHECK_EQUAL( r.get_status_code(), websocketpp::http::status_code::switching_protocols );
    BOOST_CHECK_EQUAL( r.get_status_msg(), "Switching Protocols" );
    BOOST_CHECK_EQUAL( r.get_header("Upgrade"), "websocket" );
    BOOST_CHECK_EQUAL( r.get_header("Connection"), "Upgrade" );
    BOOST_CHECK_EQUAL( r.get_header("Sec-WebSocket-Accept"), "HSmrc0sMlYUkAGmm5OPpG2HaGWk=" );
    BOOST_CHECK_EQUAL( r.get_header("Sec-WebSocket-Protocol"), "chat" );
}

BOOST_AUTO_TEST_CASE( wikipedia_example_response_trailing_large ) {
    websocketpp::http::parser::response r;

    std::string raw = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=\r\nSec-WebSocket-Protocol: chat\r\n\r\n";
    raw.append(websocketpp::http::max_header_size,'*');

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 159 );
    BOOST_CHECK( r.headers_ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.1" );
    BOOST_CHECK_EQUAL( r.get_status_code(), websocketpp::http::status_code::switching_protocols );
    BOOST_CHECK_EQUAL( r.get_status_msg(), "Switching Protocols" );
    BOOST_CHECK_EQUAL( r.get_header("Upgrade"), "websocket" );
    BOOST_CHECK_EQUAL( r.get_header("Connection"), "Upgrade" );
    BOOST_CHECK_EQUAL( r.get_header("Sec-WebSocket-Accept"), "HSmrc0sMlYUkAGmm5OPpG2HaGWk=" );
    BOOST_CHECK_EQUAL( r.get_header("Sec-WebSocket-Protocol"), "chat" );
}

BOOST_AUTO_TEST_CASE( response_with_non_standard_lws ) {
    websocketpp::http::parser::response r;

    std::string raw = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept:HSmrc0sMlYUkAGmm5OPpG2HaGWk=\r\nSec-WebSocket-Protocol: chat\r\n\r\n";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 158 );
    BOOST_CHECK( r.headers_ready() );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.1" );
    BOOST_CHECK_EQUAL( r.get_status_code(), websocketpp::http::status_code::switching_protocols );
    BOOST_CHECK_EQUAL( r.get_status_msg(), "Switching Protocols" );
    BOOST_CHECK_EQUAL( r.get_header("Upgrade"), "websocket" );
    BOOST_CHECK_EQUAL( r.get_header("Connection"), "Upgrade" );
    BOOST_CHECK_EQUAL( r.get_header("Sec-WebSocket-Accept"), "HSmrc0sMlYUkAGmm5OPpG2HaGWk=" );
    BOOST_CHECK_EQUAL( r.get_header("Sec-WebSocket-Protocol"), "chat" );
}

BOOST_AUTO_TEST_CASE( plain_http_response ) {
    websocketpp::http::parser::response r;

    std::string raw = "HTTP/1.1 200 OK\r\nDate: Thu, 10 May 2012 11:59:25 GMT\r\nServer: Apache/2.2.21 (Unix) mod_ssl/2.2.21 OpenSSL/0.9.8r DAV/2 PHP/5.3.8 with Suhosin-Patch\r\nLast-Modified: Tue, 30 Mar 2010 17:41:28 GMT\r\nETag: \"16799d-55-4830823a78200\"\r\nAccept-Ranges: bytes\r\nContent-Length: 85\r\nVary: Accept-Encoding\r\nContent-Type: text/html\r\n\r\n<!doctype html>\n<html>\n<head>\n<title>Thor</title>\n</head>\n<body> \n<p>Thor</p>\n</body>";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(raw.c_str(),raw.size());
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK( exception == false );
    BOOST_CHECK_EQUAL( pos, 405 );
    BOOST_CHECK( r.headers_ready() == true );
    BOOST_CHECK( r.ready() == true );
    BOOST_CHECK_EQUAL( r.get_version(), "HTTP/1.1" );
    BOOST_CHECK_EQUAL( r.get_status_code(), websocketpp::http::status_code::ok );
    BOOST_CHECK_EQUAL( r.get_status_msg(), "OK" );
    BOOST_CHECK_EQUAL( r.get_header("Date"), "Thu, 10 May 2012 11:59:25 GMT" );
    BOOST_CHECK_EQUAL( r.get_header("Server"), "Apache/2.2.21 (Unix) mod_ssl/2.2.21 OpenSSL/0.9.8r DAV/2 PHP/5.3.8 with Suhosin-Patch" );
    BOOST_CHECK_EQUAL( r.get_header("Last-Modified"), "Tue, 30 Mar 2010 17:41:28 GMT" );
    BOOST_CHECK_EQUAL( r.get_header("ETag"), "\"16799d-55-4830823a78200\"" );
    BOOST_CHECK_EQUAL( r.get_header("Accept-Ranges"), "bytes" );
    BOOST_CHECK_EQUAL( r.get_header("Content-Length"), "85" );
    BOOST_CHECK_EQUAL( r.get_header("Vary"), "Accept-Encoding" );
    BOOST_CHECK_EQUAL( r.get_header("Content-Type"), "text/html" );
    BOOST_CHECK_EQUAL( r.get_body(), "<!doctype html>\n<html>\n<head>\n<title>Thor</title>\n</head>\n<body> \n<p>Thor</p>\n</body>" );
}

BOOST_AUTO_TEST_CASE( parse_istream ) {
    websocketpp::http::parser::response r;

    std::stringstream s;

    s << "HTTP/1.1 200 OK\r\nDate: Thu, 10 May 2012 11:59:25 GMT\r\nServer: Apache/2.2.21 (Unix) mod_ssl/2.2.21 OpenSSL/0.9.8r DAV/2 PHP/5.3.8 with Suhosin-Patch\r\nLast-Modified: Tue, 30 Mar 2010 17:41:28 GMT\r\nETag: \"16799d-55-4830823a78200\"\r\nAccept-Ranges: bytes\r\nContent-Length: 85\r\nVary: Accept-Encoding\r\nContent-Type: text/html\r\n\r\n<!doctype html>\n<html>\n<head>\n<title>Thor</title>\n</head>\n<body> \n<p>Thor</p>\n</body>";

    bool exception = false;
    size_t pos = 0;

    try {
        pos += r.consume(s);
    } catch (std::exception &e) {
        exception = true;
        std::cout << e.what() << std::endl;
    }

    BOOST_CHECK_EQUAL( exception, false );
    BOOST_CHECK_EQUAL( pos, 405 );
    BOOST_CHECK_EQUAL( r.headers_ready(), true );
    BOOST_CHECK_EQUAL( r.ready(), true );
}

BOOST_AUTO_TEST_CASE( write_request_basic ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\n\r\n";

    r.set_version("HTTP/1.1");
    r.set_method("GET");
    r.set_uri("/");

    BOOST_CHECK_EQUAL( r.raw(), raw );
}

BOOST_AUTO_TEST_CASE( write_request_with_header ) {
    websocketpp::http::parser::request r;

    std::string raw = "GET / HTTP/1.1\r\nHost: http://example.com\r\n\r\n";

    r.set_version("HTTP/1.1");
    r.set_method("GET");
    r.set_uri("/");
    r.replace_header("Host","http://example.com");

    BOOST_CHECK_EQUAL( r.raw(), raw );
}

BOOST_AUTO_TEST_CASE( write_request_with_body ) {
    websocketpp::http::parser::request r;

    std::string raw = "POST / HTTP/1.1\r\nContent-Length: 48\r\nContent-Type: application/x-www-form-urlencoded\r\nHost: http://example.com\r\n\r\nlicenseID=string&content=string&paramsXML=string";

    r.set_version("HTTP/1.1");
    r.set_method("POST");
    r.set_uri("/");
    r.replace_header("Host","http://example.com");
    r.replace_header("Content-Type","application/x-www-form-urlencoded");
    r.set_body("licenseID=string&content=string&paramsXML=string");

    BOOST_CHECK_EQUAL( r.raw(), raw );
}
