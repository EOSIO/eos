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
#define BOOST_TEST_MODULE transport_iostream_connection
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <cstring>
#include <string>

#include <websocketpp/common/memory.hpp>

#include <websocketpp/error.hpp>
#include <websocketpp/transport/iostream/connection.hpp>

// Policies
#include <websocketpp/concurrency/basic.hpp>
#include <websocketpp/logger/basic.hpp>

struct config {
    typedef websocketpp::concurrency::basic concurrency_type;
    typedef websocketpp::log::basic<concurrency_type,
        websocketpp::log::elevel> elog_type;
    typedef websocketpp::log::basic<concurrency_type,
        websocketpp::log::alevel> alog_type;
};

typedef websocketpp::transport::iostream::connection<config> iostream_con;

using websocketpp::transport::iostream::error::make_error_code;

struct stub_con : public iostream_con {
    typedef stub_con type;
    typedef websocketpp::lib::shared_ptr<type> ptr;
    typedef iostream_con::timer_ptr timer_ptr;

    stub_con(bool is_server, config::alog_type & a, config::elog_type & e)
        : iostream_con(is_server,a,e)
        // Set the error to a known code that is unused by the library
        // This way we can easily confirm that the handler was run at all.
        , ec(websocketpp::error::make_error_code(websocketpp::error::test))
        , indef_read_total(0)
    {}

    /// Get a shared pointer to this component
    ptr get_shared() {
        return websocketpp::lib::static_pointer_cast<type>(iostream_con::get_shared());
    }

    void write(std::string msg) {
        iostream_con::async_write(
            msg.data(),
            msg.size(),
            websocketpp::lib::bind(
                &stub_con::handle_op,
                type::get_shared(),
                websocketpp::lib::placeholders::_1
            )
        );
    }

    void write(std::vector<websocketpp::transport::buffer> & bufs) {
        iostream_con::async_write(
            bufs,
            websocketpp::lib::bind(
                &stub_con::handle_op,
                type::get_shared(),
                websocketpp::lib::placeholders::_1
            )
        );
    }

    void async_read_at_least(size_t num_bytes, char *buf, size_t len)
    {
        iostream_con::async_read_at_least(
            num_bytes,
            buf,
            len,
            websocketpp::lib::bind(
                &stub_con::handle_op,
                type::get_shared(),
                websocketpp::lib::placeholders::_1
            )
        );
    }

    void handle_op(websocketpp::lib::error_code const & e) {
        ec = e;
    }

    void async_read_indef(size_t num_bytes, char *buf, size_t len)
    {
        indef_read_size = num_bytes;
        indef_read_buf = buf;
        indef_read_len = len;

        indef_read();
    }

    void indef_read() {
        iostream_con::async_read_at_least(
            indef_read_size,
            indef_read_buf,
            indef_read_len,
            websocketpp::lib::bind(
                &stub_con::handle_indef,
                type::get_shared(),
                websocketpp::lib::placeholders::_1,
                websocketpp::lib::placeholders::_2
            )
        );
    }

    void handle_indef(websocketpp::lib::error_code const & e, size_t amt_read) {
        ec = e;
        indef_read_total += amt_read;

        indef_read();
    }

    void shutdown() {
        iostream_con::async_shutdown(
            websocketpp::lib::bind(
                &stub_con::handle_async_shutdown,
                type::get_shared(),
                websocketpp::lib::placeholders::_1
            )
        );
    }

    void handle_async_shutdown(websocketpp::lib::error_code const & e) {
        ec = e;
    }

    websocketpp::lib::error_code ec;
    size_t indef_read_size;
    char * indef_read_buf;
    size_t indef_read_len;
    size_t indef_read_total;
};

// Stubs
config::alog_type alogger;
config::elog_type elogger;

BOOST_AUTO_TEST_CASE( const_methods ) {
    iostream_con::ptr con(new iostream_con(true,alogger,elogger));

    BOOST_CHECK( !con->is_secure() );
    BOOST_CHECK_EQUAL( con->get_remote_endpoint(), "iostream transport" );
}

BOOST_AUTO_TEST_CASE( write_before_output_method_set ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    con->write("foo");
    BOOST_CHECK( con->ec == make_error_code(websocketpp::transport::iostream::error::output_stream_required) );

    std::vector<websocketpp::transport::buffer> bufs;
    con->write(bufs);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::transport::iostream::error::output_stream_required) );
}

BOOST_AUTO_TEST_CASE( async_write_ostream ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    std::stringstream output;

    con->register_ostream(&output);

    con->write("foo");

    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( output.str(), "foo" );
}

websocketpp::lib::error_code write_handler(std::string & o, websocketpp::connection_hdl, char const * buf, size_t len) {
    o += std::string(buf,len);
    return websocketpp::lib::error_code();
}

websocketpp::lib::error_code vector_write_handler(std::string & o, websocketpp::connection_hdl, std::vector<websocketpp::transport::buffer> const & bufs) {
    std::vector<websocketpp::transport::buffer>::const_iterator it;
    for (it = bufs.begin(); it != bufs.end(); it++) {
        o += std::string((*it).buf, (*it).len);
    }

    return websocketpp::lib::error_code();
}

websocketpp::lib::error_code write_handler_error(websocketpp::connection_hdl, char const *, size_t) {
    return make_error_code(websocketpp::transport::error::general);
}

BOOST_AUTO_TEST_CASE( async_write_handler ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));
    std::string output;

    con->set_write_handler(websocketpp::lib::bind(
        &write_handler,
        websocketpp::lib::ref(output),
        websocketpp::lib::placeholders::_1,
        websocketpp::lib::placeholders::_2,
        websocketpp::lib::placeholders::_3
    ));
    con->write("foo");
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL(output, "foo");
}

BOOST_AUTO_TEST_CASE( async_write_handler_error ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    con->set_write_handler(&write_handler_error);
    con->write("foo");
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::transport::error::general) );
}

BOOST_AUTO_TEST_CASE( async_write_vector_0_ostream ) {
    std::stringstream output;

    stub_con::ptr con(new stub_con(true,alogger,elogger));
    con->register_ostream(&output);

    std::vector<websocketpp::transport::buffer> bufs;

    con->write(bufs);

    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( output.str(), "" );
}

BOOST_AUTO_TEST_CASE( async_write_vector_0_write_handler ) {
    std::string output;

    stub_con::ptr con(new stub_con(true,alogger,elogger));

    con->set_write_handler(websocketpp::lib::bind(
        &write_handler,
        websocketpp::lib::ref(output),
        websocketpp::lib::placeholders::_1,
        websocketpp::lib::placeholders::_2,
        websocketpp::lib::placeholders::_3
    ));

    std::vector<websocketpp::transport::buffer> bufs;

    con->write(bufs);

    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( output, "" );
}

BOOST_AUTO_TEST_CASE( async_write_vector_1_ostream ) {
    std::stringstream output;

    stub_con::ptr con(new stub_con(true,alogger,elogger));
    con->register_ostream(&output);

    std::vector<websocketpp::transport::buffer> bufs;

    std::string foo = "foo";

    bufs.push_back(websocketpp::transport::buffer(foo.data(),foo.size()));

    con->write(bufs);

    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( output.str(), "foo" );
}

BOOST_AUTO_TEST_CASE( async_write_vector_1_write_handler ) {
    std::string output;

    stub_con::ptr con(new stub_con(true,alogger,elogger));
    con->set_write_handler(websocketpp::lib::bind(
        &write_handler,
        websocketpp::lib::ref(output),
        websocketpp::lib::placeholders::_1,
        websocketpp::lib::placeholders::_2,
        websocketpp::lib::placeholders::_3
    ));

    std::vector<websocketpp::transport::buffer> bufs;

    std::string foo = "foo";

    bufs.push_back(websocketpp::transport::buffer(foo.data(),foo.size()));

    con->write(bufs);

    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( output, "foo" );
}

BOOST_AUTO_TEST_CASE( async_write_vector_2_ostream ) {
    std::stringstream output;

    stub_con::ptr con(new stub_con(true,alogger,elogger));
    con->register_ostream(&output);

    std::vector<websocketpp::transport::buffer> bufs;

    std::string foo = "foo";
    std::string bar = "bar";

    bufs.push_back(websocketpp::transport::buffer(foo.data(),foo.size()));
    bufs.push_back(websocketpp::transport::buffer(bar.data(),bar.size()));

    con->write(bufs);

    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( output.str(), "foobar" );
}

BOOST_AUTO_TEST_CASE( async_write_vector_2_write_handler ) {
    std::string output;

    stub_con::ptr con(new stub_con(true,alogger,elogger));
    con->set_write_handler(websocketpp::lib::bind(
        &write_handler,
        websocketpp::lib::ref(output),
        websocketpp::lib::placeholders::_1,
        websocketpp::lib::placeholders::_2,
        websocketpp::lib::placeholders::_3
    ));

    std::vector<websocketpp::transport::buffer> bufs;

    std::string foo = "foo";
    std::string bar = "bar";

    bufs.push_back(websocketpp::transport::buffer(foo.data(),foo.size()));
    bufs.push_back(websocketpp::transport::buffer(bar.data(),bar.size()));

    con->write(bufs);

    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( output, "foobar" );
}

BOOST_AUTO_TEST_CASE( async_write_vector_2_vector_write_handler ) {
    std::string output;

    stub_con::ptr con(new stub_con(true,alogger,elogger));
    con->set_vector_write_handler(websocketpp::lib::bind(
        &vector_write_handler,
        websocketpp::lib::ref(output),
        websocketpp::lib::placeholders::_1,
        websocketpp::lib::placeholders::_2
    ));

    std::vector<websocketpp::transport::buffer> bufs;

    std::string foo = "foo";
    std::string bar = "bar";

    bufs.push_back(websocketpp::transport::buffer(foo.data(),foo.size()));
    bufs.push_back(websocketpp::transport::buffer(bar.data(),bar.size()));

    con->write(bufs);

    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( output, "foobar" );
}

BOOST_AUTO_TEST_CASE( async_read_at_least_too_much ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    char buf[10];

    con->async_read_at_least(11,buf,10);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::transport::iostream::error::invalid_num_bytes) );
}

BOOST_AUTO_TEST_CASE( async_read_at_least_double_read ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    char buf[10];

    con->async_read_at_least(5,buf,10);
    con->async_read_at_least(5,buf,10);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::transport::iostream::error::double_read) );
}

BOOST_AUTO_TEST_CASE( async_read_at_least ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    char buf[10];

    memset(buf,'x',10);

    con->async_read_at_least(5,buf,10);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );

    std::stringstream channel;
    channel << "abcd";
    channel >> *con;
    BOOST_CHECK_EQUAL( channel.tellg(), -1 );
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );

    std::stringstream channel2;
    channel2 << "e";
    channel2 >> *con;
    BOOST_CHECK_EQUAL( channel2.tellg(), -1 );
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "abcdexxxxx" );

    std::stringstream channel3;
    channel3 << "f";
    channel3 >> *con;
    BOOST_CHECK_EQUAL( channel3.tellg(), 0 );
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "abcdexxxxx" );
    con->async_read_at_least(1,buf+5,5);
    channel3 >> *con;
    BOOST_CHECK_EQUAL( channel3.tellg(), -1 );
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "abcdefxxxx" );
}

BOOST_AUTO_TEST_CASE( async_read_at_least2 ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    char buf[10];

    memset(buf,'x',10);

    con->async_read_at_least(5,buf,5);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );

    std::stringstream channel;
    channel << "abcdefg";
    channel >> *con;
    BOOST_CHECK_EQUAL( channel.tellg(), 5 );
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "abcdexxxxx" );

    con->async_read_at_least(1,buf+5,5);
    channel >> *con;
    BOOST_CHECK_EQUAL( channel.tellg(), -1 );
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "abcdefgxxx" );
}

void timer_callback_stub(websocketpp::lib::error_code const &) {}

BOOST_AUTO_TEST_CASE( set_timer ) {
   stub_con::ptr con(new stub_con(true,alogger,elogger));

    stub_con::timer_ptr tp = con->set_timer(1000,timer_callback_stub);

    BOOST_CHECK( !tp );
}

BOOST_AUTO_TEST_CASE( async_read_at_least_read_some ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    char buf[10];
    memset(buf,'x',10);

    con->async_read_at_least(5,buf,5);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );

    char input[10] = "abcdefg";
    BOOST_CHECK_EQUAL(con->read_some(input,5), 5);
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "abcdexxxxx" );

    BOOST_CHECK_EQUAL(con->read_some(input+5,2), 0);
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "abcdexxxxx" );

    con->async_read_at_least(1,buf+5,5);
    BOOST_CHECK_EQUAL(con->read_some(input+5,2), 2);
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "abcdefgxxx" );
}

BOOST_AUTO_TEST_CASE( async_read_at_least_read_some_indef ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    char buf[20];
    memset(buf,'x',20);

    con->async_read_indef(5,buf,5);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );

    // here we expect to return early from read some because the outstanding
    // read was for 5 bytes and we were called with 10.
    char input[11] = "aaaaabbbbb";
    BOOST_CHECK_EQUAL(con->read_some(input,10), 5);
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "aaaaaxxxxx" );
    BOOST_CHECK_EQUAL( con->indef_read_total, 5 );

    // A subsequent read should read 5 more because the indef read refreshes
    // itself. The new read will start again at the beginning of the buffer.
    BOOST_CHECK_EQUAL(con->read_some(input+5,5), 5);
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "bbbbbxxxxx" );
    BOOST_CHECK_EQUAL( con->indef_read_total, 10 );
}

BOOST_AUTO_TEST_CASE( async_read_at_least_read_all ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    char buf[20];
    memset(buf,'x',20);

    con->async_read_indef(5,buf,5);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );

    char input[11] = "aaaaabbbbb";
    BOOST_CHECK_EQUAL(con->read_all(input,10), 10);
    BOOST_CHECK( !con->ec );
    BOOST_CHECK_EQUAL( std::string(buf,10), "bbbbbxxxxx" );
    BOOST_CHECK_EQUAL( con->indef_read_total, 10 );
}

BOOST_AUTO_TEST_CASE( eof_flag ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));
    char buf[10];
    con->async_read_at_least(5,buf,5);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );
    con->eof();
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::transport::error::eof) );
}

BOOST_AUTO_TEST_CASE( fatal_error_flag ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));
    char buf[10];
    con->async_read_at_least(5,buf,5);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );
    con->fatal_error();
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::transport::error::pass_through) );
}

BOOST_AUTO_TEST_CASE( shutdown ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );
    con->shutdown();
    BOOST_CHECK_EQUAL( con->ec, websocketpp::lib::error_code() );
}

websocketpp::lib::error_code sd_handler(websocketpp::connection_hdl) {
    return make_error_code(websocketpp::transport::error::general);
}

BOOST_AUTO_TEST_CASE( shutdown_handler ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    con->set_shutdown_handler(&sd_handler);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );
    con->shutdown();
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::transport::error::general) );
}

BOOST_AUTO_TEST_CASE( shared_pointer_memory_cleanup ) {
    stub_con::ptr con(new stub_con(true,alogger,elogger));

    BOOST_CHECK_EQUAL(con.use_count(), 1);

    char buf[10];
    memset(buf,'x',10);
    con->async_read_at_least(5,buf,5);
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::error::test) );
    BOOST_CHECK_EQUAL(con.use_count(), 2);

    char input[10] = "foo";
    con->read_some(input,3);
    BOOST_CHECK_EQUAL(con.use_count(), 2);

    con->read_some(input,2);
    BOOST_CHECK_EQUAL( std::string(buf,10), "foofoxxxxx" );
    BOOST_CHECK_EQUAL(con.use_count(), 1);

    con->async_read_at_least(5,buf,5);
    BOOST_CHECK_EQUAL(con.use_count(), 2);

    con->eof();
    BOOST_CHECK_EQUAL( con->ec, make_error_code(websocketpp::transport::error::eof) );
    BOOST_CHECK_EQUAL(con.use_count(), 1);
}

