#include <string>
#include <memory>
#include <vector>
#include <functional>

#include <fc/io/raw.hpp>

namespace eosio {

/*
 * A reliable_amqp_publisher manages a connection to a single AMQP server that publishes messages to an exchange
 * in a reliable manner: connections to the AMQP server are retried, publishing occurs in a transaction,
 * and unconfirmed messages are kept in a queue that is persisted to disk on destruction.
 *
 * Be aware that the reliable_amqp_publisher will NOT create the exchange. It requires the exchange
 * to already exist. Publishing to a non-existing exchange will effectively block
 * reliable_amqp_publisher's queue until the exchange exists because the queue will be unable to make progress.
 *
 * At a large enough unconfirmed queue depth (currently on the order of 1 million entries) reliable_amqp_publisher
 * will call on_fatal_error() but continue queuing messages.
 */

class reliable_amqp_publisher {
   public:
      // Called from amqp thread when unconfirmed queue depth about to be exceeded.
      using error_callback_t = std::function<void(const std::string& err)>;
      // Called on successful ack from AMQP and all queued messages have been ack'ed
      using amqp_ack_callback_t = std::function<void()>;

      /// Create a reliable queue to the given server publishing to the given exchange
      /// \param server_url server url as amqp://...
      /// \param exchange the exchange to publish to
      /// \param routing_key on published messages, used if no routing_key provided for publish_message.. calls
      /// \param unconfirmed_path path to save/load unconfirmed message to be tried again after stop/start
      /// \param on_fatal_error called from AMQP thread when unconfirmed queue depth is exceeded
      /// \param on_amqp_ack called from AMQP thread on successful ack from AMQP and all queued messages have been ack'ed
      /// \param message_id optional message id to send with each message
      reliable_amqp_publisher(const std::string& server_url, const std::string& exchange, const std::string& routing_key,
                              const boost::filesystem::path& unconfirmed_path,
                              error_callback_t on_fatal_error, amqp_ack_callback_t on_amqp_ack = nullptr,
                              const std::optional<std::string>& message_id = {});

      /// Publish a message. May be called from any thread.
      /// \param t serializable object
      template<typename T>
      void publish_message(const T& t) {
         std::vector<char> v = fc::raw::pack(t);
         publish_message_raw(std::move(v));
      }

      /// \param data message to send
      void publish_message_raw(std::vector<char>&& data);

      /// \param routing_key if empty() uses class provided default routing_key
      /// \param correlation_id if not empty() sets as correlation id of the message envelope
      /// \param data message to send
      void publish_message_raw(const std::string& routing_key, const std::string& correlation_id, std::vector<char>&& data);

      /// Publish messages. May be called from any thread.
      /// \param queue set of messages to send in one transaction <routing_key, message_data>
      void publish_messages_raw(std::deque<std::pair<std::string, std::vector<char>>>&& queue);

      /// Publish message directly to AMQP queue
      /// Bypasses all reliable mechanisms and just does a simple AMQP publish. Does not queue or retry.
      /// \param routing_key if empty() uses class provided default routing_key
      /// \param correlation_id if not empty() sets as correlation id of the message envelope
      /// \param data message to send
      /// \param on_error() call from AMQP thread if unable to directly publish (e.g. not currently connected)
      void publish_message_direct(const std::string& routing_key, const std::string& correlation_id,
                                  std::vector<char> data, error_callback_t on_error);

      /// reliable_amqp_publisher runs its own thread. In some cases it may be desirable to skip a needless thread jump
      ///  when performing work. This method will allow submission of work to reliable_amqp_publisher's thread.
      ///  To ensure proper shutdown semantics, the function passed should call publish_message() before returning. That is,
      ///  it shouldn't call another post_on_io_context() or attempt to defer more work.
      void post_on_io_context(std::function<void()>&& f);

      ~reliable_amqp_publisher();

   private:
      std::unique_ptr<struct reliable_amqp_publisher_impl> my;
};

}