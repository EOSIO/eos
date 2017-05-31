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
#define BOOST_TEST_MODULE frame
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <string>

#include <websocketpp/frame.hpp>
#include <websocketpp/utilities.hpp>

using namespace websocketpp;

BOOST_AUTO_TEST_CASE( basic_bits ) {
    frame::basic_header h1(0x00,0x00); // all false
    frame::basic_header h2(0xF0,0x80); // all true

    // Read Values
    BOOST_CHECK( frame::get_fin(h1) == false );
    BOOST_CHECK( frame::get_rsv1(h1) == false );
    BOOST_CHECK( frame::get_rsv2(h1) == false );
    BOOST_CHECK( frame::get_rsv3(h1) == false );
    BOOST_CHECK( frame::get_masked(h1) == false );

    BOOST_CHECK( frame::get_fin(h2) == true );
    BOOST_CHECK( frame::get_rsv1(h2) == true );
    BOOST_CHECK( frame::get_rsv2(h2) == true );
    BOOST_CHECK( frame::get_rsv3(h2) == true );
    BOOST_CHECK( frame::get_masked(h2) == true );

    // Set Values
    frame::set_fin(h1,true);
    BOOST_CHECK( h1.b0 == 0x80 );

    frame::set_rsv1(h1,true);
    BOOST_CHECK( h1.b0 == 0xC0 );

    frame::set_rsv2(h1,true);
    BOOST_CHECK( h1.b0 == 0xE0 );

    frame::set_rsv3(h1,true);
    BOOST_CHECK( h1.b0 == 0xF0 );

    frame::set_masked(h1,true);
    BOOST_CHECK( h1.b1 == 0x80 );
}

BOOST_AUTO_TEST_CASE( basic_constructors ) {
    // Read Values
    frame::basic_header h1(frame::opcode::TEXT,12,true,false);
    BOOST_CHECK( frame::get_opcode(h1) == frame::opcode::TEXT );
    BOOST_CHECK( frame::get_basic_size(h1) == 12 );
    BOOST_CHECK( frame::get_fin(h1) == true );
    BOOST_CHECK( frame::get_rsv1(h1) == false );
    BOOST_CHECK( frame::get_rsv2(h1) == false );
    BOOST_CHECK( frame::get_rsv3(h1) == false );
    BOOST_CHECK( frame::get_masked(h1) == false );

    frame::basic_header h2(frame::opcode::BINARY,0,false,false,false,true);
    BOOST_CHECK( frame::get_opcode(h2) == frame::opcode::BINARY );
    BOOST_CHECK( frame::get_basic_size(h2) == 0 );
    BOOST_CHECK( frame::get_fin(h2) == false );
    BOOST_CHECK( frame::get_rsv1(h2) == false );
    BOOST_CHECK( frame::get_rsv2(h2) == true );
    BOOST_CHECK( frame::get_rsv3(h2) == false );
    BOOST_CHECK( frame::get_masked(h2) == false );
}

BOOST_AUTO_TEST_CASE( basic_size ) {
    frame::basic_header h1(0x00,0x00); // length 0
    frame::basic_header h2(0x00,0x01); // length 1
    frame::basic_header h3(0x00,0x7D); // length 125
    frame::basic_header h4(0x00,0x7E); // length 126
    frame::basic_header h5(0x00,0x7F); // length 127
    frame::basic_header h6(0x00,0x80); // length 0, mask bit set

    BOOST_CHECK( frame::get_basic_size(h1) == 0 );
    BOOST_CHECK( frame::get_basic_size(h2) == 1 );
    BOOST_CHECK( frame::get_basic_size(h3) == 125 );
    BOOST_CHECK( frame::get_basic_size(h4) == 126 );
    BOOST_CHECK( frame::get_basic_size(h5) == 127 );
    BOOST_CHECK( frame::get_basic_size(h6) == 0 );

    /*frame::set_basic_size(h1,1);
    BOOST_CHECK( h1.b1 == 0x01 );

    frame::set_basic_size(h1,125);
    BOOST_CHECK( h1.b1 == 0x7D );

    frame::set_basic_size(h1,126);
    BOOST_CHECK( h1.b1 == 0x7E );

    frame::set_basic_size(h1,127);
    BOOST_CHECK( h1.b1 == 0x7F );

    frame::set_basic_size(h1,0);
    BOOST_CHECK( h1.b1 == 0x00 );*/
}

BOOST_AUTO_TEST_CASE( basic_header_length ) {
    frame::basic_header h1(0x82,0x00); // short binary frame, unmasked
    frame::basic_header h2(0x82,0x80); // short binary frame, masked
    frame::basic_header h3(0x82,0x7E); // medium binary frame, unmasked
    frame::basic_header h4(0x82,0xFE); // medium binary frame, masked
    frame::basic_header h5(0x82,0x7F); // jumbo binary frame, unmasked
    frame::basic_header h6(0x82,0xFF); // jumbo binary frame, masked

    BOOST_CHECK( frame::get_header_len(h1) == 2);
    BOOST_CHECK( frame::get_header_len(h2) == 6);
    BOOST_CHECK( frame::get_header_len(h3) == 4);
    BOOST_CHECK( frame::get_header_len(h4) == 8);
    BOOST_CHECK( frame::get_header_len(h5) == 10);
    BOOST_CHECK( frame::get_header_len(h6) == 14);
}

BOOST_AUTO_TEST_CASE( basic_opcode ) {
    frame::basic_header h1(0x00,0x00);

    BOOST_CHECK( is_control(frame::opcode::CONTINUATION) == false);
    BOOST_CHECK( is_control(frame::opcode::TEXT) == false);
    BOOST_CHECK( is_control(frame::opcode::BINARY) == false);
    BOOST_CHECK( is_control(frame::opcode::CLOSE) == true);
    BOOST_CHECK( is_control(frame::opcode::PING) == true);
    BOOST_CHECK( is_control(frame::opcode::PONG) == true);

    BOOST_CHECK( frame::get_opcode(h1) == frame::opcode::CONTINUATION );
}

BOOST_AUTO_TEST_CASE( extended_header_basics ) {
    frame::extended_header h1;
    uint8_t h1_solution[12] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    frame::extended_header h2(uint16_t(255));
    uint8_t h2_solution[12] = {0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    frame::extended_header h3(uint16_t(256),htonl(0x8040201));
    uint8_t h3_solution[12] = {0x01, 0x00, 0x08, 0x04, 0x02, 0x01,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    frame::extended_header h4(uint64_t(0x0807060504030201LL));
    uint8_t h4_solution[12] = {0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
                               0x02, 0x01, 0x00, 0x00, 0x00, 0x00};

    frame::extended_header h5(uint64_t(0x0807060504030201LL),htonl(0x8040201));
    uint8_t h5_solution[12] = {0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
                               0x02, 0x01, 0x08, 0x04, 0x02, 0x01};

    BOOST_CHECK( std::equal(h1_solution,h1_solution+12,h1.bytes) );
    BOOST_CHECK( std::equal(h2_solution,h2_solution+12,h2.bytes) );
    BOOST_CHECK( std::equal(h3_solution,h3_solution+12,h3.bytes) );
    BOOST_CHECK( std::equal(h4_solution,h4_solution+12,h4.bytes) );
    BOOST_CHECK( std::equal(h5_solution,h5_solution+12,h5.bytes) );
}

BOOST_AUTO_TEST_CASE( extended_header_extractors ) {
    frame::basic_header h1(0x00,0x7E);
    frame::extended_header e1(uint16_t(255));
    BOOST_CHECK( get_extended_size(e1) == 255 );
    BOOST_CHECK( get_payload_size(h1,e1) == 255 );
    BOOST_CHECK( get_masking_key_offset(h1) == 2 );
    BOOST_CHECK( get_masking_key(h1,e1).i == 0 );

    frame::basic_header h2(0x00,0x7F);
    frame::extended_header e2(uint64_t(0x0807060504030201LL));
    BOOST_CHECK( get_jumbo_size(e2) == 0x0807060504030201LL );
    BOOST_CHECK( get_payload_size(h2,e2) == 0x0807060504030201LL );
    BOOST_CHECK( get_masking_key_offset(h2) == 8 );
    BOOST_CHECK( get_masking_key(h2,e2).i == 0 );

    frame::basic_header h3(0x00,0xFE);
    frame::extended_header e3(uint16_t(255),0x08040201);
    BOOST_CHECK( get_extended_size(e3) == 255 );
    BOOST_CHECK( get_payload_size(h3,e3) == 255 );
    BOOST_CHECK( get_masking_key_offset(h3) == 2 );
    BOOST_CHECK( get_masking_key(h3,e3).i == 0x08040201 );

    frame::basic_header h4(0x00,0xFF);
    frame::extended_header e4(uint64_t(0x0807060504030201LL),0x08040201);
    BOOST_CHECK( get_jumbo_size(e4) == 0x0807060504030201LL );
    BOOST_CHECK( get_payload_size(h4,e4) == 0x0807060504030201LL );
    BOOST_CHECK( get_masking_key_offset(h4) == 8 );
    BOOST_CHECK( get_masking_key(h4,e4).i == 0x08040201 );

    frame::basic_header h5(0x00,0x7D);
    frame::extended_header e5;
    BOOST_CHECK( get_payload_size(h5,e5) == 125 );
}

BOOST_AUTO_TEST_CASE( header_preparation ) {
    frame::basic_header h1(0x81,0xFF); //
    frame::extended_header e1(uint64_t(0xFFFFFLL),htonl(0xD5FB70EE));
    std::string p1 = prepare_header(h1, e1);
    uint8_t s1[14] = {0x81, 0xFF,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF,
                     0xD5, 0xFB, 0x70, 0xEE};

    BOOST_CHECK( p1.size() == 14);
    BOOST_CHECK( std::equal(p1.begin(),p1.end(),reinterpret_cast<char*>(s1)) );

    frame::basic_header h2(0x81,0x7E); //
    frame::extended_header e2(uint16_t(255));
    std::string p2 = prepare_header(h2, e2);
    uint8_t s2[4] = {0x81, 0x7E, 0x00, 0xFF};

    BOOST_CHECK( p2.size() == 4);
    BOOST_CHECK( std::equal(p2.begin(),p2.end(),reinterpret_cast<char*>(s2)) );
}

BOOST_AUTO_TEST_CASE( prepare_masking_key ) {
    frame::masking_key_type key;

    key.i = htonl(0x12345678);

    if (sizeof(size_t) == 8) {
        BOOST_CHECK(
            frame::prepare_masking_key(key) == lib::net::_htonll(0x1234567812345678LL)
        );
    } else {
        BOOST_CHECK( frame::prepare_masking_key(key) == htonl(0x12345678) );
    }
}

BOOST_AUTO_TEST_CASE( prepare_masking_key2 ) {
    frame::masking_key_type key;

    key.i = htonl(0xD5FB70EE);

    // One call
    if (sizeof(size_t) == 8) {
        BOOST_CHECK(
            frame::prepare_masking_key(key) == lib::net::_htonll(0xD5FB70EED5FB70EELL)
        );
    } else {
        BOOST_CHECK( frame::prepare_masking_key(key) == htonl(0xD5FB70EE) );
    }
}

// TODO: figure out a way to run/test both 4 and 8 byte versions.
BOOST_AUTO_TEST_CASE( circshift ) {
    /*if (sizeof(size_t) == 8) {
        size_t test = 0x0123456789abcdef;

        BOOST_CHECK( frame::circshift_prepared_key(test,0) == 0x0123456789abcdef);
        BOOST_CHECK( frame::circshift_prepared_key(test,1) == 0xef0123456789abcd);
        BOOST_CHECK( frame::circshift_prepared_key(test,2) == 0xcdef0123456789ab);
        BOOST_CHECK( frame::circshift_prepared_key(test,3) == 0xabcdef0123456789);
    } else {
        size_t test = 0x01234567;

        BOOST_CHECK( frame::circshift_prepared_key(test,0) == 0x01234567);
        BOOST_CHECK( frame::circshift_prepared_key(test,1) == 0x67012345);
        BOOST_CHECK( frame::circshift_prepared_key(test,2) == 0x45670123);
        BOOST_CHECK( frame::circshift_prepared_key(test,3) == 0x23456701);
    }*/
}

BOOST_AUTO_TEST_CASE( block_byte_mask ) {
    uint8_t input[15] = {0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00};

    uint8_t output[15];

    uint8_t masked[15] = {0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02};

    frame::masking_key_type key;
    key.c[0] = 0x00;
    key.c[1] = 0x01;
    key.c[2] = 0x02;
    key.c[3] = 0x03;

    byte_mask(input,input+15,output,key);

    BOOST_CHECK( std::equal(output,output+15,masked) );
}

BOOST_AUTO_TEST_CASE( block_byte_mask_inplace ) {
    uint8_t buffer[15] = {0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00};

    uint8_t masked[15] = {0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02};

    frame::masking_key_type key;
    key.c[0] = 0x00;
    key.c[1] = 0x01;
    key.c[2] = 0x02;
    key.c[3] = 0x03;

    byte_mask(buffer,buffer+15,key);

    BOOST_CHECK( std::equal(buffer,buffer+15,masked) );
}

BOOST_AUTO_TEST_CASE( block_word_mask ) {
    uint8_t input[15] =  {0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00};

    uint8_t output[15];

    uint8_t masked[15] = {0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02};

    frame::masking_key_type key;
    key.c[0] = 0x00;
    key.c[1] = 0x01;
    key.c[2] = 0x02;
    key.c[3] = 0x03;

    word_mask_exact(input,output,15,key);

    BOOST_CHECK( std::equal(output,output+15,masked) );
}

BOOST_AUTO_TEST_CASE( block_word_mask_inplace ) {
    uint8_t buffer[15] = {0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00};

    uint8_t masked[15] = {0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02};

    frame::masking_key_type key;
    key.c[0] = 0x00;
    key.c[1] = 0x01;
    key.c[2] = 0x02;
    key.c[3] = 0x03;

    word_mask_exact(buffer,15,key);

    BOOST_CHECK( std::equal(buffer,buffer+15,masked) );
}

BOOST_AUTO_TEST_CASE( continuous_word_mask ) {
    uint8_t input[16];
    uint8_t output[16];

    uint8_t masked[16] = {0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x00};

    frame::masking_key_type key;
    key.c[0] = 0x00;
    key.c[1] = 0x01;
    key.c[2] = 0x02;
    key.c[3] = 0x03;

    // One call
    size_t pkey,pkey_temp;
    pkey = frame::prepare_masking_key(key);
    std::fill_n(input,16,0x00);
    std::fill_n(output,16,0x00);
    frame::word_mask_circ(input,output,15,pkey);
    BOOST_CHECK( std::equal(output,output+16,masked) );

    // calls not split on word boundaries
    pkey = frame::prepare_masking_key(key);
    std::fill_n(input,16,0x00);
    std::fill_n(output,16,0x00);

    pkey_temp = frame::word_mask_circ(input,output,7,pkey);
    BOOST_CHECK( std::equal(output,output+7,masked) );
    BOOST_CHECK( pkey_temp == frame::circshift_prepared_key(pkey,3) );

    pkey_temp = frame::word_mask_circ(input+7,output+7,8,pkey_temp);
    BOOST_CHECK( std::equal(output,output+16,masked) );
    BOOST_CHECK_EQUAL( pkey_temp, frame::circshift_prepared_key(pkey,3) );
}

BOOST_AUTO_TEST_CASE( continuous_byte_mask ) {
    uint8_t input[16];
    uint8_t output[16];

    uint8_t masked[16] = {0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x00};

    frame::masking_key_type key;
    key.c[0] = 0x00;
    key.c[1] = 0x01;
    key.c[2] = 0x02;
    key.c[3] = 0x03;

    // One call
    size_t pkey,pkey_temp;
    pkey = frame::prepare_masking_key(key);
    std::fill_n(input,16,0x00);
    std::fill_n(output,16,0x00);
    frame::byte_mask_circ(input,output,15,pkey);
    BOOST_CHECK( std::equal(output,output+16,masked) );

    // calls not split on word boundaries
    pkey = frame::prepare_masking_key(key);
    std::fill_n(input,16,0x00);
    std::fill_n(output,16,0x00);

    pkey_temp = frame::byte_mask_circ(input,output,7,pkey);
    BOOST_CHECK( std::equal(output,output+7,masked) );
    BOOST_CHECK( pkey_temp == frame::circshift_prepared_key(pkey,3) );

    pkey_temp = frame::byte_mask_circ(input+7,output+7,8,pkey_temp);
    BOOST_CHECK( std::equal(output,output+16,masked) );
    BOOST_CHECK_EQUAL( pkey_temp, frame::circshift_prepared_key(pkey,3) );
}

BOOST_AUTO_TEST_CASE( continuous_word_mask_inplace ) {
    uint8_t buffer[16];

    uint8_t masked[16] = {0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x00};

    frame::masking_key_type key;
    key.c[0] = 0x00;
    key.c[1] = 0x01;
    key.c[2] = 0x02;
    key.c[3] = 0x03;

    // One call
    size_t pkey,pkey_temp;
    pkey = frame::prepare_masking_key(key);
    std::fill_n(buffer,16,0x00);
    frame::word_mask_circ(buffer,15,pkey);
    BOOST_CHECK( std::equal(buffer,buffer+16,masked) );

    // calls not split on word boundaries
    pkey = frame::prepare_masking_key(key);
    std::fill_n(buffer,16,0x00);

    pkey_temp = frame::word_mask_circ(buffer,7,pkey);
    BOOST_CHECK( std::equal(buffer,buffer+7,masked) );
    BOOST_CHECK_EQUAL( pkey_temp, frame::circshift_prepared_key(pkey,3) );

    pkey_temp = frame::word_mask_circ(buffer+7,8,pkey_temp);
    BOOST_CHECK( std::equal(buffer,buffer+16,masked) );
    BOOST_CHECK_EQUAL( pkey_temp, frame::circshift_prepared_key(pkey,3) );
}

BOOST_AUTO_TEST_CASE( continuous_byte_mask_inplace ) {
    uint8_t buffer[16];

    uint8_t masked[16] = {0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x03,
                          0x00, 0x01, 0x02, 0x00};

    frame::masking_key_type key;
    key.c[0] = 0x00;
    key.c[1] = 0x01;
    key.c[2] = 0x02;
    key.c[3] = 0x03;

    // One call
    size_t pkey,pkey_temp;
    pkey = frame::prepare_masking_key(key);
    std::fill_n(buffer,16,0x00);
    frame::byte_mask_circ(buffer,15,pkey);
    BOOST_CHECK( std::equal(buffer,buffer+16,masked) );

    // calls not split on word boundaries
    pkey = frame::prepare_masking_key(key);
    std::fill_n(buffer,16,0x00);

    pkey_temp = frame::byte_mask_circ(buffer,7,pkey);
    BOOST_CHECK( std::equal(buffer,buffer+7,masked) );
    BOOST_CHECK_EQUAL( pkey_temp, frame::circshift_prepared_key(pkey,3) );

    pkey_temp = frame::byte_mask_circ(buffer+7,8,pkey_temp);
    BOOST_CHECK( std::equal(buffer,buffer+16,masked) );
    BOOST_CHECK_EQUAL( pkey_temp, frame::circshift_prepared_key(pkey,3) );
}

BOOST_AUTO_TEST_CASE( continuous_word_mask2 ) {
    uint8_t buffer[12] = {0xA6, 0x15, 0x97, 0xB9,
                          0x81, 0x50, 0xAC, 0xBA,
                          0x9C, 0x1C, 0x9F, 0xF4};

    uint8_t unmasked[12] = {0x48, 0x65, 0x6C, 0x6C,
                            0x6F, 0x20, 0x57, 0x6F,
                            0x72, 0x6C, 0x64, 0x21};

    frame::masking_key_type key;
    key.c[0] = 0xEE;
    key.c[1] = 0x70;
    key.c[2] = 0xFB;
    key.c[3] = 0xD5;

    // One call
    size_t pkey;
    pkey = frame::prepare_masking_key(key);
    frame::word_mask_circ(buffer,12,pkey);
    BOOST_CHECK( std::equal(buffer,buffer+12,unmasked) );
}
