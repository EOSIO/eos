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
      transaction_trace_msg msg{transaction_trace_exception{error_code}};
      std::get<transaction_trace_exception>( msg ).error_message = std::move( error_message );
      auto buf = convert_to_bin( msg );
      amqp_trace->publish( amqp_trace_exchange, tid, std::move( buf ) );
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
      if( !trace->except ) {
         dlog( "chain accepted transaction, bcast ${id}", ("id", trace->id) );
      } else {
         dlog( "trace except : ${m}", ("m", trace->except->to_string()) );
      }
      amqp_trace->publish( amqp_trace_exchange, trx->id(), [trace]() {
         transaction_trace_msg msg{ eosio::state_history::convert( *trace ) };
         return convert_to_bin( msg );
      } );
   }
   FC_LOG_AND_DROP()
}

} // namespace eosio
