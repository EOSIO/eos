#include <eosio/amqp_trace_plugin/amqp_trace_types.hpp>
#include <eosio/amqp_trace_plugin/amqp_trace_plugin_impl.hpp>
#include <eosio/state_history/type_convert.hpp>
#include <eosio/for_each_field.hpp>
#include <eosio/to_bin.hpp>

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>

namespace eosio {

// called from any thread
void amqp_trace_plugin_impl::publish_error( std::string tid, int64_t error_code, std::string error_message ) {
   try {
      // reliable_amqp_publisher ensures that any post_on_io_context() is called before its dtor returns
      amqp_trace->post_on_io_context([&amqp_trace = *amqp_trace, tid{std::move(tid)}, error_code, em{std::move(error_message)}]() mutable {
         transaction_trace_msg msg{transaction_trace_exception{error_code}};
         std::get<transaction_trace_exception>( msg ).error_message = std::move( em );
         std::vector<char> buf = convert_to_bin( msg );
         amqp_trace.publish_message_direct(std::string(), tid, std::move(buf));
      });
   }
   FC_LOG_AND_DROP()
}

// called on application thread
void amqp_trace_plugin_impl::on_applied_transaction( const chain::transaction_trace_ptr& trace,
                                                     const chain::packed_transaction_ptr& t ) {
   try {
      publish_result( t, trace );
   }
   FC_LOG_AND_DROP()
}

// called from application thread
void amqp_trace_plugin_impl::publish_result( const chain::packed_transaction_ptr& trx,
                                             const chain::transaction_trace_ptr& trace ) {
   try {
      // reliable_amqp_publisher ensures that any post_on_io_context() is called before its dtor returns
      amqp_trace->post_on_io_context([&amqp_trace = *amqp_trace, trx, trace]() {
         if( !trace->except ) {
            dlog( "chain accepted transaction, bcast ${id}", ("id", trace->id) );
         } else {
            dlog( "trace except : ${m}", ("m", trace->except->to_string()) );
         }
         transaction_trace_msg msg{ eosio::state_history::convert( *trace ) };
         std::vector<char> buf = convert_to_bin( msg );
         amqp_trace.publish_message_direct(std::string(), trx->id(), std::move(buf));
      });
   }
   FC_LOG_AND_DROP()
}

} // namespace eosio
