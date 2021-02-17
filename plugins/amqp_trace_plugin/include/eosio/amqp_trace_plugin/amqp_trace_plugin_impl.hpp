#pragma once
#include <eosio/amqp/amqp_handler.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/transaction.hpp>

namespace eosio {

struct amqp_trace_plugin_impl : std::enable_shared_from_this<amqp_trace_plugin_impl> {

   std::optional<amqp> amqp_trace;

   std::string amqp_trace_address;
   std::string amqp_trace_exchange;
   bool started = false;

public:

   // called from any thread
   void publish_error( std::string tid, int64_t error_code, std::string error_message );

   // called on application thread
   void on_applied_transaction(const chain::transaction_trace_ptr& trace, const chain::packed_transaction_ptr& t);

private:

   // called from application thread
   void publish_result( const chain::packed_transaction_ptr& trx, const chain::transaction_trace_ptr& trace );
};

} // namespace eosio
