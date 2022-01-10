#include <eosio/amqp_trx_plugin/amqp_trace_types.hpp>
#include <eosio/amqp_trx_plugin/amqp_trace_plugin_impl.hpp>
#include <eosio/state_history/type_convert.hpp>
#include <eosio/for_each_field.hpp>
#include <eosio/to_bin.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <appbase/application.hpp>

namespace eosio {

std::istream& operator>>(std::istream& in, amqp_trace_plugin_impl::reliable_mode& m) {
   std::string s;
   in >> s;
   if( s == "exit" )
      m = amqp_trace_plugin_impl::reliable_mode::exit;
   else if( s == "log" )
      m = amqp_trace_plugin_impl::reliable_mode::log;
   else if( s == "queue" )
      m = amqp_trace_plugin_impl::reliable_mode::queue;
   else
      in.setstate( std::ios_base::failbit );
   return in;
}

std::ostream& operator<<(std::ostream& osm, amqp_trace_plugin_impl::reliable_mode m) {
   if( m == amqp_trace_plugin_impl::reliable_mode::exit )
      osm << "exit";
   else if( m == amqp_trace_plugin_impl::reliable_mode::log )
      osm << "log";
   else if( m == amqp_trace_plugin_impl::reliable_mode::queue )
      osm << "queue";
   return osm;
}

// Can be called from any thread except reliable_amqp_publisher thread
void amqp_trace_plugin_impl::publish_error( std::string routing_key, std::string correlation_id, int64_t error_code, std::string error_message ) {
   try {
      // reliable_amqp_publisher ensures that any post_on_io_context() is called before its dtor returns
      amqp_trace->post_on_io_context(
            [&amqp_trace = *amqp_trace, mode=pub_reliable_mode, rk{std::move(routing_key)},
             cid{std::move(correlation_id)}, error_code, em{std::move(error_message)}]() mutable {
         transaction_trace_msg msg{transaction_trace_exception{error_code}};
         std::get<transaction_trace_exception>( msg ).error_message = std::move( em );
         std::vector<char> buf = convert_to_bin( msg );
         if( mode == reliable_mode::queue) {
            amqp_trace.publish_message_raw( rk, cid, std::move( buf ) );
         } else {
            amqp_trace.publish_message_direct( rk, cid, std::move( buf ),
                                               [mode]( const std::string& err ) {
                                                  elog( "AMQP direct message error: ${e}", ("e", err) );
                                                  if( mode == reliable_mode::exit )
                                                     appbase::app().quit();
                                               } );
         }
      });
   }
   FC_LOG_AND_DROP()
}

// called from application thread
void amqp_trace_plugin_impl::publish_result( std::string routing_key,
                                             std::string correlation_id,
                                             std::string block_uuid,
                                             const chain::packed_transaction_ptr& trx,
                                             const chain::transaction_trace_ptr& trace ) {
   try {
      // reliable_amqp_publisher ensures that any post_on_io_context() is called before its dtor returns
      amqp_trace->post_on_io_context(
            [&amqp_trace = *amqp_trace, trx, trace,
             rk=std::move(routing_key), cid=std::move(correlation_id), uuid=std::move(block_uuid),
             mode=pub_reliable_mode]() mutable {
         if( !trace->except ) {
            dlog( "chain accepted transaction, bcast ${id}", ("id", trace->id) );
         } else {
            dlog( "trace except : ${m}", ("m", trace->except->to_string()) );
         }
         transaction_trace_msg msg{ transaction_trace_message{ std::move(uuid), eosio::state_history::convert( *trace ) } };
         std::vector<char> buf = convert_to_bin( msg );
         if( mode == reliable_mode::queue) {
            amqp_trace.publish_message_raw( rk, cid, std::move( buf ) );
         } else {
            amqp_trace.publish_message_direct( rk, cid, std::move( buf ),
                                               [mode]( const std::string& err ) {
                                                  elog( "AMQP direct message error: ${e}", ("e", err) );
                                                  if( mode == reliable_mode::exit )
                                                     appbase::app().quit();
                                               } );
         }
      });
   }
   FC_LOG_AND_DROP()
}

void amqp_trace_plugin_impl::publish_block_uuid( std::string routing_key,
                                                 std::string block_uuid,
                                                 const chain::block_id_type& block_id ) {
   try {
      // reliable_amqp_publisher ensures that any post_on_io_context() is called before its dtor returns
      amqp_trace->post_on_io_context(
            [&amqp_trace = *amqp_trace,
                  rk=std::move(routing_key), uuid=std::move(block_uuid), block_id, mode=pub_reliable_mode]() mutable {
               transaction_trace_msg msg{ block_uuid_message{ std::move(uuid), eosio::state_history::convert( block_id ) } };
               std::vector<char> buf = convert_to_bin( msg );
               if( mode == reliable_mode::queue) {
                  amqp_trace.publish_message_raw( rk, {}, std::move( buf ) );
               } else {
                  amqp_trace.publish_message_direct( rk, {}, std::move( buf ),
                                                     [mode]( const std::string& err ) {
                                                        elog( "AMQP direct message error: ${e}", ("e", err) );
                                                        if( mode == reliable_mode::exit )
                                                           appbase::app().quit();
                                                     } );
               }
            });
   }
   FC_LOG_AND_DROP()
}


} // namespace eosio
