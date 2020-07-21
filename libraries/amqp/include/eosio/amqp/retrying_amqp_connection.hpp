#pragma once

#include <amqpcpp.h>

#include <boost/asio.hpp>

#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>

/*
 * A retrying_amqp_connection manages a connection to an AMQP server that will retry the connection
 * on failure. Most users should consider single_channel_retrying_amqp_connection instead, which additionally
 * manages a single channel.
 */

namespace eosio {
struct retrying_amqp_connection {
   using connection_ready_callback_t = std::function<void(AMQP::Connection*)>;
   using connection_failed_callback_t = std::function<void()>;

   /// \param io_context a strand is created on this io_context for all asio operatoins
   /// \param address AMQP address to connect to
   /// \param ready a callback when the AMQP connection has been established
   /// \param failed a callback when the AMQP connection has failed after being established; should no longer use the AMQP::Connection* after this callback
   /// \param logger logger to send logging to
   retrying_amqp_connection(boost::asio::io_context& io_context, const AMQP::Address& address, connection_ready_callback_t ready,
                            connection_failed_callback_t failed, fc::logger logger = fc::logger::get());

   const AMQP::Address& address() const;

   boost::asio::io_context::strand& strand();

   ~retrying_amqp_connection();

private:
   struct impl;
   std::unique_ptr<impl> my;
};

struct single_channel_retrying_amqp_connection {
   using channel_ready_callback_t = std::function<void(AMQP::Channel*)>;
   using failed_callback_t = std::function<void()>;

   /// \param io_context a strand is created on this io_context for all asio operatoins
   /// \param address AMQP address to connect to
   /// \param ready a callback when the AMQP channel has been established, do NOT set the .onError() for the passed AMQP::Channel
   /// \param failed a callback when the AMQP channel has failed after being established; should no longer use the AMQP::Channel* within or after this callback
   /// \param logger logger to send logging to
   single_channel_retrying_amqp_connection(boost::asio::io_context& io_context, const AMQP::Address& address, channel_ready_callback_t ready,
                                           failed_callback_t failed, fc::logger logger = fc::logger::get());

   const AMQP::Address& address() const;

   ~single_channel_retrying_amqp_connection();

private:
   struct impl;
   std::unique_ptr<impl> my;
};

}
