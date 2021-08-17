#pragma once
#include <eosio/amqp/reliable_amqp_publisher.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/transaction.hpp>

namespace eosio {

struct amqp_trace_plugin_impl : std::enable_shared_from_this<amqp_trace_plugin_impl> {

   enum class reliable_mode {
      exit,
      log,
      queue
   };

   std::optional<reliable_amqp_publisher> amqp_trace;

   std::string amqp_trace_address;
   std::string amqp_trace_queue_name;
   std::string amqp_trace_exchange;
   reliable_mode pub_reliable_mode = reliable_mode::queue;

public:

   // called from any thread
   void publish_error( std::string routing_key, std::string correlation_id, int64_t error_code, std::string error_message );

   // called on application thread
   void on_applied_transaction(const chain::transaction_trace_ptr& trace, const chain::packed_transaction_ptr& t);

   // called from any thread
   void publish_result( std::string routing_key, std::string correlation_id, std::string block_uuid,
                        const chain::packed_transaction_ptr& trx, const chain::transaction_trace_ptr& trace );

   // called from any thread
   void publish_block_uuid( std::string routing_key, std::string block_uuid, const chain::block_id_type& block_id );
};

std::istream& operator>>(std::istream& in, amqp_trace_plugin_impl::reliable_mode& m);
std::ostream& operator<<(std::ostream& osm, amqp_trace_plugin_impl::reliable_mode m);

} // namespace eosio
