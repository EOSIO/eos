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
#define BOOST_TEST_MODULE hybi_13_processor
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>

#include <websocketpp/processors/hybi13.hpp>

#include <websocketpp/http/request.hpp>
#include <websocketpp/http/response.hpp>
#include <websocketpp/message_buffer/message.hpp>
#include <websocketpp/message_buffer/alloc.hpp>
#include <websocketpp/random/none.hpp>

#include <websocketpp/extensions/permessage_deflate/disabled.hpp>
#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

struct stub_config {
    typedef websocketpp::http::parser::request request_type;
    typedef websocketpp::http::parser::response response_type;

    typedef websocketpp::message_buffer::message
        <websocketpp::message_buffer::alloc::con_msg_manager> message_type;
    typedef websocketpp::message_buffer::alloc::con_msg_manager<message_type>
        con_msg_manager_type;

    typedef websocketpp::random::none::int_generator<uint32_t> rng_type;

    struct permessage_deflate_config {
        typedef stub_config::request_type request_type;
    };

    typedef websocketpp::extensions::permessage_deflate::disabled
        <permessage_deflate_config> permessage_deflate_type;

    static const size_t max_message_size = 16000000;
    static const bool enable_extensions = false;
};

struct stub_config_ext {
    typedef websocketpp::http::parser::request request_type;
    typedef websocketpp::http::parser::response response_type;

    typedef websocketpp::message_buffer::message
        <websocketpp::message_buffer::alloc::con_msg_manager> message_type;
    typedef websocketpp::message_buffer::alloc::con_msg_manager<message_type>
        con_msg_manager_type;

    typedef websocketpp::random::none::int_generator<uint32_t> rng_type;

    struct permessage_deflate_config {
        typedef stub_config_ext::request_type request_type;
    };

    typedef websocketpp::extensions::permessage_deflate::enabled
        <permessage_deflate_config> permessage_deflate_type;

    static const size_t max_message_size = 16000000;
    static const bool enable_extensions = true;
};

typedef stub_config::con_msg_manager_type con_msg_manager_type;
typedef stub_config::message_type::ptr message_ptr;

// Set up a structure that constructs new copies of all of the support structure
// for using connection processors
struct processor_setup {
    processor_setup(bool server)
      : msg_manager(new con_msg_manager_type())
      , p(false,server,msg_manager,rng) {}

    websocketpp::lib::error_code ec;
    con_msg_manager_type::ptr msg_manager;
    stub_config::rng_type rng;
    stub_config::request_type req;
    stub_config::response_type res;
    websocketpp::processor::hybi13<stub_config> p;
};

struct processor_setup_ext {
    processor_setup_ext(bool server)
      : msg_manager(new con_msg_manager_type())
      , p(false,server,msg_manager,rng) {}

    websocketpp::lib::error_code ec;
    con_msg_manager_type::ptr msg_manager;
    stub_config::rng_type rng;
    stub_config::request_type req;
    stub_config::response_type res;
    websocketpp::processor::hybi13<stub_config_ext> p;
};

BOOST_AUTO_TEST_CASE( exact_match ) {
    processor_setup env(true);

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";

    env.req.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(env.req));
    BOOST_CHECK_EQUAL(websocketpp::processor::get_websocket_version(env.req), env.p.get_version());
    BOOST_CHECK(!env.p.validate_handshake(env.req));

    websocketpp::uri_ptr u;

    BOOST_CHECK_NO_THROW( u = env.p.get_uri(env.req) );

    BOOST_CHECK_EQUAL(u->get_secure(), false);
    BOOST_CHECK_EQUAL(u->get_host(), "www.example.com");
    BOOST_CHECK_EQUAL(u->get_resource(), "/");
    BOOST_CHECK_EQUAL(u->get_port(), websocketpp::uri_default_port);

    env.p.process_handshake(env.req,"",env.res);

    BOOST_CHECK_EQUAL(env.res.get_header("Connection"), "upgrade");
    BOOST_CHECK_EQUAL(env.res.get_header("Upgrade"), "websocket");
    BOOST_CHECK_EQUAL(env.res.get_header("Sec-WebSocket-Accept"), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

BOOST_AUTO_TEST_CASE( non_get_method ) {
    processor_setup env(true);

    std::string handshake = "POST / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: foo\r\n\r\n";

    env.req.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(env.req));
    BOOST_CHECK_EQUAL(websocketpp::processor::get_websocket_version(env.req), env.p.get_version());
    BOOST_CHECK( env.p.validate_handshake(env.req) == websocketpp::processor::error::invalid_http_method );
}

BOOST_AUTO_TEST_CASE( old_http_version ) {
    processor_setup env(true);

    std::string handshake = "GET / HTTP/1.0\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: foo\r\n\r\n";

    env.req.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK(websocketpp::processor::is_websocket_handshake(env.req));
    BOOST_CHECK_EQUAL(websocketpp::processor::get_websocket_version(env.req), env.p.get_version());
    BOOST_CHECK_EQUAL( env.p.validate_handshake(env.req), websocketpp::processor::error::invalid_http_version );
}

BOOST_AUTO_TEST_CASE( missing_handshake_key1 ) {
    processor_setup env(true);

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\n\r\n";

    env.req.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK( websocketpp::processor::is_websocket_handshake(env.req) );
    BOOST_CHECK_EQUAL( websocketpp::processor::get_websocket_version(env.req), env.p.get_version() );
    BOOST_CHECK_EQUAL( env.p.validate_handshake(env.req), websocketpp::processor::error::missing_required_header );
}

BOOST_AUTO_TEST_CASE( missing_handshake_key2 ) {
    processor_setup env(true);

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\n\r\n";

    env.req.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK( websocketpp::processor::is_websocket_handshake(env.req) );
    BOOST_CHECK_EQUAL( websocketpp::processor::get_websocket_version(env.req), env.p.get_version() );
    BOOST_CHECK_EQUAL( env.p.validate_handshake(env.req), websocketpp::processor::error::missing_required_header );
}

BOOST_AUTO_TEST_CASE( bad_host ) {
    processor_setup env(true);

    std::string handshake = "GET / HTTP/1.1\r\nHost: www.example.com:70000\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: foo\r\n\r\n";

    env.req.consume(handshake.c_str(),handshake.size());

    BOOST_CHECK( websocketpp::processor::is_websocket_handshake(env.req) );
    BOOST_CHECK_EQUAL( websocketpp::processor::get_websocket_version(env.req), env.p.get_version() );
    BOOST_CHECK( !env.p.validate_handshake(env.req) );
    BOOST_CHECK( !env.p.get_uri(env.req)->get_valid() );
}

// FRAME TESTS TO DO
//
// unmasked, 0 length, binary
// 0x82 0x00
//
// masked, 0 length, binary
// 0x82 0x80
//
// unmasked, 0 length, text
// 0x81 0x00
//
// masked, 0 length, text
// 0x81 0x80

BOOST_AUTO_TEST_CASE( frame_empty_binary_unmasked ) {
    uint8_t frame[2] = {0x82, 0x00};

    // all in one chunk
    processor_setup env1(false);

    size_t ret1 = env1.p.consume(frame,2,env1.ec);

    BOOST_CHECK_EQUAL( ret1, 2 );
    BOOST_CHECK( !env1.ec );
    BOOST_CHECK_EQUAL( env1.p.ready(), true );

    // two separate chunks
    processor_setup env2(false);

    BOOST_CHECK_EQUAL( env2.p.consume(frame,1,env2.ec), 1 );
    BOOST_CHECK( !env2.ec );
    BOOST_CHECK_EQUAL( env2.p.ready(), false );

    BOOST_CHECK_EQUAL( env2.p.consume(frame+1,1,env2.ec), 1 );
    BOOST_CHECK( !env2.ec );
    BOOST_CHECK_EQUAL( env2.p.ready(), true );
}

BOOST_AUTO_TEST_CASE( frame_small_binary_unmasked ) {
    processor_setup env(false);

    uint8_t frame[4] = {0x82, 0x02, 0x2A, 0x2A};

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( env.p.consume(frame,4,env.ec), 4 );
    BOOST_CHECK( !env.ec );
    BOOST_CHECK_EQUAL( env.p.ready(), true );

    message_ptr foo = env.p.get_message();

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( foo->get_payload(), "**" );

}

BOOST_AUTO_TEST_CASE( frame_extended_binary_unmasked ) {
    processor_setup env(false);

    uint8_t frame[130] = {0x82, 0x7E, 0x00, 0x7E};
    frame[0] = 0x82;
    frame[1] = 0x7E;
    frame[2] = 0x00;
    frame[3] = 0x7E;
    std::fill_n(frame+4,126,0x2A);

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( env.p.consume(frame,130,env.ec), 130 );
    BOOST_CHECK( !env.ec );
    BOOST_CHECK_EQUAL( env.p.ready(), true );

    message_ptr foo = env.p.get_message();

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( foo->get_payload().size(), 126 );
}

BOOST_AUTO_TEST_CASE( frame_jumbo_binary_unmasked ) {
    processor_setup env(false);

    uint8_t frame[130] = {0x82, 0x7E, 0x00, 0x7E};
    std::fill_n(frame+4,126,0x2A);

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( env.p.consume(frame,130,env.ec), 130 );
    BOOST_CHECK( !env.ec );
    BOOST_CHECK_EQUAL( env.p.ready(), true );

    message_ptr foo = env.p.get_message();

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( foo->get_payload().size(), 126 );
}

BOOST_AUTO_TEST_CASE( control_frame_too_large ) {
    processor_setup env(false);

    uint8_t frame[130] = {0x88, 0x7E, 0x00, 0x7E};
    std::fill_n(frame+4,126,0x2A);

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_GT( env.p.consume(frame,130,env.ec), 0 );
    BOOST_CHECK_EQUAL( env.ec, websocketpp::processor::error::control_too_big  );
    BOOST_CHECK_EQUAL( env.p.ready(), false );
}

BOOST_AUTO_TEST_CASE( rsv_bits_used ) {
    uint8_t frame[3][2] = {{0x90, 0x00},
                           {0xA0, 0x00},
                           {0xC0, 0x00}};

    for (int i = 0; i < 3; i++) {
        processor_setup env(false);

        BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
        BOOST_CHECK_GT( env.p.consume(frame[i],2,env.ec), 0 );
        BOOST_CHECK_EQUAL( env.ec, websocketpp::processor::error::invalid_rsv_bit  );
        BOOST_CHECK_EQUAL( env.p.ready(), false );
    }
}


BOOST_AUTO_TEST_CASE( reserved_opcode_used ) {
    uint8_t frame[10][2] = {{0x83, 0x00},
                            {0x84, 0x00},
                            {0x85, 0x00},
                            {0x86, 0x00},
                            {0x87, 0x00},
                            {0x8B, 0x00},
                            {0x8C, 0x00},
                            {0x8D, 0x00},
                            {0x8E, 0x00},
                            {0x8F, 0x00}};

    for (int i = 0; i < 10; i++) {
        processor_setup env(false);

        BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
        BOOST_CHECK_GT( env.p.consume(frame[i],2,env.ec), 0 );
        BOOST_CHECK_EQUAL( env.ec, websocketpp::processor::error::invalid_opcode  );
        BOOST_CHECK_EQUAL( env.p.ready(), false );
    }
}

BOOST_AUTO_TEST_CASE( fragmented_control_message ) {
    processor_setup env(false);

    uint8_t frame[2] = {0x08, 0x00};

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_GT( env.p.consume(frame,2,env.ec), 0 );
    BOOST_CHECK_EQUAL( env.ec, websocketpp::processor::error::fragmented_control );
    BOOST_CHECK_EQUAL( env.p.ready(), false );
}

BOOST_AUTO_TEST_CASE( fragmented_binary_message ) {
    processor_setup env0(false);
    processor_setup env1(false);

    uint8_t frame0[6] = {0x02, 0x01, 0x2A, 0x80, 0x01, 0x2A};
    uint8_t frame1[8] = {0x02, 0x01, 0x2A, 0x89, 0x00, 0x80, 0x01, 0x2A};

    // read fragmented message in one chunk
    BOOST_CHECK_EQUAL( env0.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( env0.p.consume(frame0,6,env0.ec), 6 );
    BOOST_CHECK( !env0.ec );
    BOOST_CHECK_EQUAL( env0.p.ready(), true );
    BOOST_CHECK_EQUAL( env0.p.get_message()->get_payload(), "**" );

    // read fragmented message in two chunks
    BOOST_CHECK_EQUAL( env0.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( env0.p.consume(frame0,3,env0.ec), 3 );
    BOOST_CHECK( !env0.ec );
    BOOST_CHECK_EQUAL( env0.p.ready(), false );
    BOOST_CHECK_EQUAL( env0.p.consume(frame0+3,3,env0.ec), 3 );
    BOOST_CHECK( !env0.ec );
    BOOST_CHECK_EQUAL( env0.p.ready(), true );
    BOOST_CHECK_EQUAL( env0.p.get_message()->get_payload(), "**" );

    // read fragmented message with control message in between
    BOOST_CHECK_EQUAL( env0.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( env0.p.consume(frame1,8,env0.ec), 5 );
    BOOST_CHECK( !env0.ec );
    BOOST_CHECK_EQUAL( env0.p.ready(), true );
    BOOST_CHECK_EQUAL( env0.p.get_message()->get_opcode(), websocketpp::frame::opcode::PING);
    BOOST_CHECK_EQUAL( env0.p.consume(frame1+5,3,env0.ec), 3 );
    BOOST_CHECK( !env0.ec );
    BOOST_CHECK_EQUAL( env0.p.ready(), true );
    BOOST_CHECK_EQUAL( env0.p.get_message()->get_payload(), "**" );

    // read lone continuation frame
    BOOST_CHECK_EQUAL( env0.p.get_message(), message_ptr() );
    BOOST_CHECK_GT( env0.p.consume(frame0+3,3,env0.ec), 0);
    BOOST_CHECK_EQUAL( env0.ec, websocketpp::processor::error::invalid_continuation );

    // read two start frames in a row
    BOOST_CHECK_EQUAL( env1.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( env1.p.consume(frame0,3,env1.ec), 3);
    BOOST_CHECK( !env1.ec );
    BOOST_CHECK_GT( env1.p.consume(frame0,3,env1.ec), 0);
    BOOST_CHECK_EQUAL( env1.ec, websocketpp::processor::error::invalid_continuation );
}

BOOST_AUTO_TEST_CASE( unmasked_client_frame ) {
    processor_setup env(true);

    uint8_t frame[2] = {0x82, 0x00};

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_GT( env.p.consume(frame,2,env.ec), 0 );
    BOOST_CHECK_EQUAL( env.ec, websocketpp::processor::error::masking_required );
    BOOST_CHECK_EQUAL( env.p.ready(), false );
}

BOOST_AUTO_TEST_CASE( masked_server_frame ) {
    processor_setup env(false);

    uint8_t frame[8] = {0x82, 0x82, 0xFF, 0xFF, 0xFF, 0xFF, 0xD5, 0xD5};

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_GT( env.p.consume(frame,8,env.ec), 0 );
    BOOST_CHECK_EQUAL( env.ec, websocketpp::processor::error::masking_forbidden );
    BOOST_CHECK_EQUAL( env.p.ready(), false );
}

BOOST_AUTO_TEST_CASE( frame_small_binary_masked ) {
    processor_setup env(true);

    uint8_t frame[8] = {0x82, 0x82, 0xFF, 0xFF, 0xFF, 0xFF, 0xD5, 0xD5};

    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( env.p.consume(frame,8,env.ec), 8 );
    BOOST_CHECK( !env.ec );
    BOOST_CHECK_EQUAL( env.p.ready(), true );
    BOOST_CHECK_EQUAL( env.p.get_message()->get_payload(), "**" );
}

BOOST_AUTO_TEST_CASE( masked_fragmented_binary_message ) {
    processor_setup env(true);

    uint8_t frame0[14] = {0x02, 0x81, 0xAB, 0x23, 0x98, 0x45, 0x81,
                         0x80, 0x81, 0xB8, 0x34, 0x12, 0xFF, 0x92};

    // read fragmented message in one chunk
    BOOST_CHECK_EQUAL( env.p.get_message(), message_ptr() );
    BOOST_CHECK_EQUAL( env.p.consume(frame0,14,env.ec), 14 );
    BOOST_CHECK( !env.ec );
    BOOST_CHECK_EQUAL( env.p.ready(), true );
    BOOST_CHECK_EQUAL( env.p.get_message()->get_payload(), "**" );
}

BOOST_AUTO_TEST_CASE( prepare_data_frame ) {
    processor_setup env(true);

    message_ptr in = env.msg_manager->get_message();
    message_ptr out = env.msg_manager->get_message();
    message_ptr invalid;

    // empty pointers arguements should return sane error
    BOOST_CHECK_EQUAL( env.p.prepare_data_frame(invalid,invalid), websocketpp::processor::error::invalid_arguments );

    BOOST_CHECK_EQUAL( env.p.prepare_data_frame(in,invalid), websocketpp::processor::error::invalid_arguments );

    BOOST_CHECK_EQUAL( env.p.prepare_data_frame(invalid,out), websocketpp::processor::error::invalid_arguments );

    // test valid opcodes
    // control opcodes should return an error, data ones shouldn't
    for (int i = 0; i < 0xF; i++) {
        in->set_opcode(websocketpp::frame::opcode::value(i));

        env.ec = env.p.prepare_data_frame(in,out);

        if (websocketpp::frame::opcode::is_control(in->get_opcode())) {
            BOOST_CHECK_EQUAL( env.ec, websocketpp::processor::error::invalid_opcode );
        } else {
            BOOST_CHECK_NE( env.ec, websocketpp::processor::error::invalid_opcode );
        }
    }


    //in.set_payload("foo");

    //e = prepare_data_frame(in,out);


}

BOOST_AUTO_TEST_CASE( single_frame_message_too_large ) {
    processor_setup env(true);
    
    env.p.set_max_message_size(3);
    
    uint8_t frame0[10] = {0x82, 0x84, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01};

    // read message that is one byte too large
    BOOST_CHECK_EQUAL( env.p.consume(frame0,10,env.ec), 6 );
    BOOST_CHECK_EQUAL( env.ec, websocketpp::processor::error::message_too_big );
}

BOOST_AUTO_TEST_CASE( multiple_frame_message_too_large ) {
    processor_setup env(true);
    
    env.p.set_max_message_size(4);
    
    uint8_t frame0[8] = {0x02, 0x82, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};
    uint8_t frame1[9] = {0x80, 0x83, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01};

    // read first message frame with size under the limit
    BOOST_CHECK_EQUAL( env.p.consume(frame0,8,env.ec), 8 );
    BOOST_CHECK( !env.ec );
    
    // read second message frame that puts the size over the limit
    BOOST_CHECK_EQUAL( env.p.consume(frame1,9,env.ec), 6 );
    BOOST_CHECK_EQUAL( env.ec, websocketpp::processor::error::message_too_big );
}



BOOST_AUTO_TEST_CASE( client_handshake_request ) {
    processor_setup env(false);

    websocketpp::uri_ptr u(new websocketpp::uri("ws://localhost/"));

    env.p.client_handshake_request(env.req,u, std::vector<std::string>());

    BOOST_CHECK_EQUAL( env.req.get_method(), "GET" );
    BOOST_CHECK_EQUAL( env.req.get_version(), "HTTP/1.1");
    BOOST_CHECK_EQUAL( env.req.get_uri(), "/");

    BOOST_CHECK_EQUAL( env.req.get_header("Host"), "localhost");
    BOOST_CHECK_EQUAL( env.req.get_header("Sec-WebSocket-Version"), "13");
    BOOST_CHECK_EQUAL( env.req.get_header("Connection"), "Upgrade");
    BOOST_CHECK_EQUAL( env.req.get_header("Upgrade"), "websocket");
}

// TODO:
// test cases
// - adding headers
// - adding Upgrade header
// - adding Connection header
// - adding Sec-WebSocket-Version, Sec-WebSocket-Key, or Host header
// - other Sec* headers?
// - User Agent header?

// Origin support
// Subprotocol requests

//websocketpp::uri_ptr u(new websocketpp::uri("ws://localhost/"));
    //env.p.client_handshake_request(env.req,u);

BOOST_AUTO_TEST_CASE( client_handshake_response_404 ) {
    processor_setup env(false);

    std::string res = "HTTP/1.1 404 Not Found\r\n\r\n";
    env.res.consume(res.data(),res.size());

    BOOST_CHECK_EQUAL( env.p.validate_server_handshake_response(env.req,env.res), websocketpp::processor::error::invalid_http_status );
}

BOOST_AUTO_TEST_CASE( client_handshake_response_no_upgrade ) {
    processor_setup env(false);

    std::string res = "HTTP/1.1 101 Switching Protocols\r\n\r\n";
    env.res.consume(res.data(),res.size());

    BOOST_CHECK_EQUAL( env.p.validate_server_handshake_response(env.req,env.res), websocketpp::processor::error::missing_required_header );
}

BOOST_AUTO_TEST_CASE( client_handshake_response_no_connection ) {
    processor_setup env(false);

    std::string res = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: foo, wEbsOckEt\r\n\r\n";
    env.res.consume(res.data(),res.size());

    BOOST_CHECK_EQUAL( env.p.validate_server_handshake_response(env.req,env.res), websocketpp::processor::error::missing_required_header );
}

BOOST_AUTO_TEST_CASE( client_handshake_response_no_accept ) {
    processor_setup env(false);

    std::string res = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: foo, wEbsOckEt\r\nConnection: bar, UpGrAdE\r\n\r\n";
    env.res.consume(res.data(),res.size());

    BOOST_CHECK_EQUAL( env.p.validate_server_handshake_response(env.req,env.res), websocketpp::processor::error::missing_required_header );
}

BOOST_AUTO_TEST_CASE( client_handshake_response ) {
    processor_setup env(false);

    env.req.append_header("Sec-WebSocket-Key", "dGhlIHNhbXBsZSBub25jZQ==");

    std::string res = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: foo, wEbsOckEt\r\nConnection: bar, UpGrAdE\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n\r\n";
    env.res.consume(res.data(),res.size());

    BOOST_CHECK( !env.p.validate_server_handshake_response(env.req,env.res) );
}

BOOST_AUTO_TEST_CASE( extensions_disabled ) {
    processor_setup env(true);

    env.req.replace_header("Sec-WebSocket-Extensions","");

    std::pair<websocketpp::lib::error_code,std::string> neg_results;
    neg_results = env.p.negotiate_extensions(env.req);

    BOOST_CHECK_EQUAL( neg_results.first, websocketpp::processor::error::extensions_disabled );
    BOOST_CHECK_EQUAL( neg_results.second, "" );
}

BOOST_AUTO_TEST_CASE( extension_negotiation_blank ) {
    processor_setup_ext env(true);

    env.req.replace_header("Sec-WebSocket-Extensions","");

    std::pair<websocketpp::lib::error_code,std::string> neg_results;
    neg_results = env.p.negotiate_extensions(env.req);

    BOOST_CHECK( !neg_results.first );
    BOOST_CHECK_EQUAL( neg_results.second, "" );
}

BOOST_AUTO_TEST_CASE( extension_negotiation_unknown ) {
    processor_setup_ext env(true);

    env.req.replace_header("Sec-WebSocket-Extensions","foo");

    std::pair<websocketpp::lib::error_code,std::string> neg_results;
    neg_results = env.p.negotiate_extensions(env.req);

    BOOST_CHECK( !neg_results.first );
    BOOST_CHECK_EQUAL( neg_results.second, "" );
}

BOOST_AUTO_TEST_CASE( extract_subprotocols_empty ) {
    processor_setup env(true);
    std::vector<std::string> subps;

    BOOST_CHECK( !env.p.extract_subprotocols(env.req,subps) );
    BOOST_CHECK_EQUAL( subps.size(), 0 );
}

BOOST_AUTO_TEST_CASE( extract_subprotocols_one ) {
    processor_setup env(true);
    std::vector<std::string> subps;

    env.req.replace_header("Sec-WebSocket-Protocol","foo");

    BOOST_CHECK( !env.p.extract_subprotocols(env.req,subps) );
    BOOST_REQUIRE_EQUAL( subps.size(), 1 );
    BOOST_CHECK_EQUAL( subps[0], "foo" );
}

BOOST_AUTO_TEST_CASE( extract_subprotocols_multiple ) {
    processor_setup env(true);
    std::vector<std::string> subps;

    env.req.replace_header("Sec-WebSocket-Protocol","foo,bar");

    BOOST_CHECK( !env.p.extract_subprotocols(env.req,subps) );
    BOOST_REQUIRE_EQUAL( subps.size(), 2 );
    BOOST_CHECK_EQUAL( subps[0], "foo" );
    BOOST_CHECK_EQUAL( subps[1], "bar" );
}

BOOST_AUTO_TEST_CASE( extract_subprotocols_invalid) {
    processor_setup env(true);
    std::vector<std::string> subps;

    env.req.replace_header("Sec-WebSocket-Protocol","foo,bar,,,,");

    BOOST_CHECK_EQUAL( env.p.extract_subprotocols(env.req,subps), websocketpp::processor::error::make_error_code(websocketpp::processor::error::subprotocol_parse_error) );
    BOOST_CHECK_EQUAL( subps.size(), 0 );
}

BOOST_AUTO_TEST_CASE( extension_negotiation_permessage_deflate ) {
    processor_setup_ext env(true);

    env.req.replace_header("Sec-WebSocket-Extensions",
        "permessage-deflate; client_max_window_bits");

    std::pair<websocketpp::lib::error_code,std::string> neg_results;
    neg_results = env.p.negotiate_extensions(env.req);

    BOOST_CHECK( !neg_results.first );
    BOOST_CHECK_EQUAL( neg_results.second, "permessage-deflate" );
}

