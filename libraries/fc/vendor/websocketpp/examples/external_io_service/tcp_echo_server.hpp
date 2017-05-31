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
 */
 
/**
 * TCP Echo Server
 *
 * This file defines a simple TCP Echo Server. It is adapted from the Asio
 * example: cpp03/echo/async_tcp_echo_server.cpp
 */ 

#include <websocketpp/common/asio.hpp>
#include <websocketpp/common/memory.hpp>
#include <websocketpp/common/functional.hpp>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

namespace asio = websocketpp::lib::asio;

struct tcp_echo_session : websocketpp::lib::enable_shared_from_this<tcp_echo_session> {
    typedef websocketpp::lib::shared_ptr<tcp_echo_session> ptr;
    
    tcp_echo_session(asio::io_service & service) : m_socket(service) {}

    void start() {
        m_socket.async_read_some(asio::buffer(m_buffer, sizeof(m_buffer)),
            websocketpp::lib::bind(
                &tcp_echo_session::handle_read, shared_from_this(), _1, _2));
    }
    
    void handle_read(const asio::error_code & ec, size_t transferred) {
        if (!ec) {
            asio::async_write(m_socket,
                asio::buffer(m_buffer, transferred),
                    bind(&tcp_echo_session::handle_write, shared_from_this(), _1));
        }
    }
    
    void handle_write(const asio::error_code & ec) {
        if (!ec) {
            m_socket.async_read_some(asio::buffer(m_buffer, sizeof(m_buffer)),
                bind(&tcp_echo_session::handle_read, shared_from_this(), _1, _2));
        }
    }

    asio::ip::tcp::socket m_socket;
    char m_buffer[1024];
};

struct tcp_echo_server {
    tcp_echo_server(asio::io_service & service, short port)
        : m_service(service)
        , m_acceptor(service, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), port))
    {
        this->start_accept();
    }
    
    void start_accept() {
        tcp_echo_session::ptr new_session(new tcp_echo_session(m_service));
        m_acceptor.async_accept(new_session->m_socket,
            bind(&tcp_echo_server::handle_accept, this, new_session, _1));
    }
    
    void handle_accept(tcp_echo_session::ptr new_session, const asio::error_code & ec) {
        if (!ec) {
            new_session->start();
        }
        start_accept();
    }

    asio::io_service & m_service;
    asio::ip::tcp::acceptor m_acceptor;
};
