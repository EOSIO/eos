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
#define BOOST_TEST_MODULE message
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>

#include <websocketpp/message_buffer/message.hpp>

template <typename message>
struct stub {
    typedef websocketpp::lib::weak_ptr<stub> weak_ptr;
    typedef websocketpp::lib::shared_ptr<stub> ptr;

    stub() : recycled(false) {}

    bool recycle(message *) {
        this->recycled = true;
        return false;
    }

    bool recycled;
};

BOOST_AUTO_TEST_CASE( basic_size_check ) {
    typedef websocketpp::message_buffer::message<stub> message_type;
    typedef stub<message_type> stub_type;

    stub_type::ptr s(new stub_type());
    message_type::ptr msg(new message_type(s,websocketpp::frame::opcode::TEXT,500));

    BOOST_CHECK(msg->get_payload().capacity() >= 500);
}

BOOST_AUTO_TEST_CASE( recycle ) {
    typedef websocketpp::message_buffer::message<stub> message_type;
    typedef stub<message_type> stub_type;

    stub_type::ptr s(new stub_type());
    message_type::ptr msg(new message_type(s,websocketpp::frame::opcode::TEXT,500));

    BOOST_CHECK(s->recycled == false);
    BOOST_CHECK(msg->recycle() == false);
    BOOST_CHECK(s->recycled == true);
}

