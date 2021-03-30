#include <fc/time.hpp>

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <deque>

namespace eosio {

/*
 * A transactional_amqp_publisher manages a connection to a single AMQP server that publishes messages to an exchange
 * in a reliable manner: connections to the AMQP server are retried, publishing occurs in a transaction,
 * publish blocks until AMQP acks transactional group of messages.
 *
 * on_fatal_error is called if AMQP does not ack transactional group of messages in the specified time. Called from
 * AMQP thread or calling thread.
 *
 * Be aware that the transactional_amqp_publisher will NOT create the exchange. It requires the exchange
 * to already exist. Publishing to a non-existing exchange will effectively block as the transaction will not be
 * ack'ed.
 */

class transactional_amqp_publisher {
   public:
      // Called from amqp thread when publish messages are not ack
      using error_callback_t = std::function<void(const std::string& err)>;

      /// Create a reliable queue to the given server publishing to the given exchange
      /// \param server_url server url as amqp://...
      /// \param exchange the exchange to publish to
      /// \param time_out time to wait on transactional ack from AMQP before calling on_fatal_error
      /// \param on_fatal_error called from AMQP does not ack transaction in time_out time
      transactional_amqp_publisher(const std::string& server_url, const std::string& exchange,
                                   const fc::microseconds& time_out, error_callback_t on_fatal_error);

      /// Publish messages. May be called from any thread except internal thread (do not call from on_fatal_error)
      /// All calls should be from the same thread or at the very least no two calls should be performed concurrently.
      /// Blocks until AMQP ack of success transactional commit or on_fatal_error if timeout waiting on ack.
      /// \param queue set of messages to send in one transaction <routing_key, message_data>
      void publish_messages_raw(std::deque<std::pair<std::string, std::vector<char>>>&& queue);

      /// Publish message directly to AMQP queue. May be called from any thread.
      /// Bypasses all transactional mechanisms and just does a simple AMQP publish. Does not queue or retry or block.
      /// \param routing_key if empty() uses class provided default routing_key
      /// \param data message to send
      /// \param on_error() call from AMQP thread if unable to directly publish (e.g. not currently connected)
      void publish_message_direct(const std::string& routing_key, std::vector<char> data, error_callback_t on_error);

      ~transactional_amqp_publisher();

   private:
      std::unique_ptr<struct transactional_amqp_publisher_impl> my;
};

}