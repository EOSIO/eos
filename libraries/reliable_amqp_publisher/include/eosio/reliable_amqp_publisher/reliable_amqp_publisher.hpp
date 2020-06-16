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
 * At a large enough uncomfirmed queue depth (currently on the order of 1 million entries) reliable_amqp_publisher
 * will start dropping messages after logging an error.
 */

class reliable_amqp_publisher {
   public:
      /// Create a reliable queue to the given server publishing to the given exchange
      /// \param server_url server url as amqp://...
      /// \param exchange the exchange to publish to
      /// \param routing_key on published messages
      /// \param unconfirmed_path path to save/load unconfirmed message to be tried again after stop/start
      /// \param message_id optional message id to send with each message
      reliable_amqp_publisher(const std::string& server_url, const std::string& exchange, const std::string& routing_key,
                              const boost::filesystem::path& unconfirmed_path, const std::optional<std::string>& message_id = {});

      /// Publish a message. May be called from any thread.
      /// \param t serializable object
      template<typename T>
      void publish_message(const T& t) {
         std::vector<char> v = fc::raw::pack(t);
         publish_message_raw(std::move(v));
      }

      void publish_message_raw(std::vector<char>&& data);

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