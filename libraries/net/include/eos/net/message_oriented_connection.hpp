/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <fc/network/tcp_socket.hpp>
#include <eos/net/message.hpp>

namespace eos { namespace net {

  namespace detail { class message_oriented_connection_impl; }

  class message_oriented_connection;

  /** receives incoming messages from a message_oriented_connection object */
  class message_oriented_connection_delegate 
  {
  public:
    virtual void on_message(message_oriented_connection* originating_connection, const message& received_message) = 0;
    virtual void on_connection_closed(message_oriented_connection* originating_connection) = 0;
  };

  /** uses a secure socket to create a connection that reads and writes a stream of `fc::net::message` objects */
  class message_oriented_connection
  {
     public:
       message_oriented_connection(message_oriented_connection_delegate* delegate = nullptr);
       ~message_oriented_connection();
       fc::tcp_socket& get_socket();

       void accept();
       void bind(const fc::ip::endpoint& local_endpoint);
       void connect_to(const fc::ip::endpoint& remote_endpoint);

       void send_message(const message& message_to_send);
       void close_connection();
       void destroy_connection();

       uint64_t       get_total_bytes_sent() const;
       uint64_t       get_total_bytes_received() const;
       fc::time_point get_last_message_sent_time() const;
       fc::time_point get_last_message_received_time() const;
       fc::time_point get_connection_time() const;
       fc::sha512     get_shared_secret() const;
     private:
       std::unique_ptr<detail::message_oriented_connection_impl> my;
  };
  typedef std::shared_ptr<message_oriented_connection> message_oriented_connection_ptr;

} } // eos::net
