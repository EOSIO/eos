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
#define BOOST_TEST_MODULE basic_log
#include <boost/test/unit_test.hpp>

#include <string>

#include <websocketpp/logger/basic.hpp>
#include <websocketpp/concurrency/none.hpp>
#include <websocketpp/concurrency/basic.hpp>

typedef websocketpp::log::basic<websocketpp::concurrency::basic,websocketpp::log::alevel> basic_access_log_type;

BOOST_AUTO_TEST_CASE( is_token_char ) {
    typedef websocketpp::log::basic<websocketpp::concurrency::none,websocketpp::log::elevel> error_log;

    error_log elog;

    BOOST_CHECK( elog.static_test(websocketpp::log::elevel::info ) == true );
    BOOST_CHECK( elog.static_test(websocketpp::log::elevel::warn ) == true );
    BOOST_CHECK( elog.static_test(websocketpp::log::elevel::rerror ) == true );
    BOOST_CHECK( elog.static_test(websocketpp::log::elevel::fatal ) == true );

    elog.set_channels(websocketpp::log::elevel::info);

    elog.write(websocketpp::log::elevel::info,"Information");
    elog.write(websocketpp::log::elevel::warn,"A warning");
    elog.write(websocketpp::log::elevel::rerror,"A error");
    elog.write(websocketpp::log::elevel::fatal,"A critical error");
}

BOOST_AUTO_TEST_CASE( access_clear ) {
    typedef websocketpp::log::basic<websocketpp::concurrency::none,websocketpp::log::alevel> access_log;

    std::stringstream out;
    access_log logger(0xffffffff,&out);

    // clear all channels
    logger.clear_channels(0xffffffff);

    // writes shouldn't happen
    logger.write(websocketpp::log::alevel::devel,"devel");
    //std::cout << "|" << out.str() << "|" << std::endl;
    BOOST_CHECK( out.str().size() == 0 );
}

BOOST_AUTO_TEST_CASE( basic_concurrency ) {
    typedef websocketpp::log::basic<websocketpp::concurrency::basic,websocketpp::log::alevel> access_log;

    std::stringstream out;
    access_log logger(0xffffffff,&out);

    logger.set_channels(0xffffffff);

    logger.write(websocketpp::log::alevel::devel,"devel");
    //std::cout << "|" << out.str() << "|" << std::endl;
    BOOST_CHECK( out.str().size() > 0 );
}


BOOST_AUTO_TEST_CASE( copy_constructor ) {
    std::stringstream out;

    basic_access_log_type logger1(0xffffffff,&out);
    basic_access_log_type logger2(logger1);

    logger2.set_channels(0xffffffff);
    logger2.write(websocketpp::log::alevel::devel,"devel");
    BOOST_CHECK( out.str().size() > 0 );
}

#ifdef _WEBSOCKETPP_MOVE_SEMANTICS_
BOOST_AUTO_TEST_CASE( move_constructor ) {
    std::stringstream out;

    basic_access_log_type logger1(0xffffffff,&out);
    basic_access_log_type logger2(std::move(logger1));

    logger2.set_channels(0xffffffff);
    logger2.write(websocketpp::log::alevel::devel,"devel");
    BOOST_CHECK( out.str().size() > 0 );
}

// Emplace requires move assignment, which logger doesn't support right now
// due to const members. This is pretty irritating and will probably result in
// the const members being removed. For now though this test will fail to
// compile
/*BOOST_AUTO_TEST_CASE( emplace ) {
    std::stringstream out1;
    std::stringstream out2;

    std::vector<basic_access_log_type> v;

    v.emplace_back(websocketpp::log::level(0xffffffff),&out1);
    v.emplace_back(websocketpp::log::level(0xffffffff),&out2);

    v[0].set_channels(0xffffffff);
    v[1].set_channels(0xffffffff);
    v[0].write(websocketpp::log::alevel::devel,"devel");
    v[1].write(websocketpp::log::alevel::devel,"devel");
    BOOST_CHECK( out1.str().size() > 0 );
    BOOST_CHECK( out2.str().size() > 0 );
}*/
#endif // #ifdef _WEBSOCKETPP_MOVE_SEMANTICS_

// As long as there are const member variables these can't exist
// These remain commented as they are useful for testing the deleted operators
/*BOOST_AUTO_TEST_CASE( copy_assign ) {
    basic_access_log_type logger1;
    basic_access_log_type logger2;

    logger2 = logger1;
}

BOOST_AUTO_TEST_CASE( move_assign ) {
    basic_access_log_type logger1;
    basic_access_log_type logger2;

    logger2 = std::move(logger1);
}*/
