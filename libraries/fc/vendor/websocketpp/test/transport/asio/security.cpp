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
#define BOOST_TEST_MODULE transport_asio_base
#include <boost/test/unit_test.hpp>

#include <iostream>

#include <websocketpp/common/type_traits.hpp>

#include <websocketpp/transport/asio/security/none.hpp>
#include <websocketpp/transport/asio/security/tls.hpp>

template <typename base>
struct dummy_con : public base {
	websocketpp::lib::error_code test() {
		return this->translate_ec(websocketpp::lib::asio::error_code());
	}
};

BOOST_AUTO_TEST_CASE( translated_ec_none ) {
    dummy_con<websocketpp::transport::asio::basic_socket::connection> tscon;

	// If the current configuration settings result in the library error type and the asio
	// error type being the same, then the code should pass through natively. Otherwise
	// we should get a generic pass through error.
	if(websocketpp::lib::is_same<websocketpp::lib::error_code,websocketpp::lib::asio::error_code>::value) {
    	BOOST_CHECK_EQUAL( tscon.test(), websocketpp::lib::error_code() );
    } else {
    	BOOST_CHECK_EQUAL( tscon.test(), websocketpp::transport::error::make_error_code(websocketpp::transport::error::pass_through) );
    }
}

BOOST_AUTO_TEST_CASE( translated_ec_tls ) {
    dummy_con<websocketpp::transport::asio::tls_socket::connection> tscon;

	// If the current configuration settings result in the library error type and the asio
	// error type being the same, then the code should pass through natively. Otherwise
	// we should get a generic pass through error.
	if(websocketpp::lib::is_same<websocketpp::lib::error_code,websocketpp::lib::asio::error_code>::value) {
    	BOOST_CHECK_EQUAL( tscon.test(), websocketpp::lib::error_code() );
    } else {
    	BOOST_CHECK_EQUAL( tscon.test(), websocketpp::transport::error::make_error_code(websocketpp::transport::error::pass_through) );
    }
}
