#include <eosio/amqp_trx_plugin/amqp_trx_plugin.hpp>
#include <eosio/amqp_trx_plugin/fifo_trx_processing_queue.hpp>
#include <eosio/amqp/amqp_handler.hpp>
#include <eosio/amqp_trace_plugin/amqp_trace_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/thread_utils.hpp>

#include <fc/log/trace.hpp>

#include <boost/signals2/connection.hpp>

namespace {

static appbase::abstract_plugin& amqp_trx_plugin_ = appbase::app().register_plugin<eosio::amqp_trx_plugin>();

enum class ack_mode {
   received,
   executed,
   in_block
};

std::istream& operator>>(std::istream& in, ack_mode& m) {
   std::string s;
   in >> s;
   if( s == "received" )
      m = ack_mode::received;
   else if( s == "executed" )
      m = ack_mode::executed;
   else if( s == "in_block" )
      m = ack_mode::in_block;
   else
      in.setstate( std::ios_base::failbit );
   return in;
}

std::ostream& operator<<(std::ostream& osm, ack_mode m) {
   if( m == ack_mode::received )
      osm << "received";
   else if( m == ack_mode::executed )
      osm << "executed";
   else if( m == ack_mode::in_block )
      osm << "in_block";
   return osm;
}

} // anonymous

namespace eosio {

using boost::signals2::scoped_connection;

struct amqp_trx_plugin_impl : std::enable_shared_from_this<amqp_trx_plugin_impl> {

   chain_plugin* chain_plug = nullptr;
   amqp_trace_plugin* trace_plug = nullptr;
   std::optional<amqp> amqp_trx;

   std::string amqp_trx_address;
   ack_mode acked = ack_mode::executed;
   std::map<uint32_t, eosio::amqp::delivery_tag_t> tracked_delivery_tags; // block, highest delivery_tag for block
   uint32_t trx_processing_queue_size = 1000;
   bool allow_speculative_execution = false;
   std::shared_ptr<fifo_trx_processing_queue<producer_plugin>> trx_queue_ptr;

   std::optional<scoped_connection> block_start_connection;
   std::optional<scoped_connection> block_abort_connection;
   std::optional<scoped_connection> accepted_block_connection;


   // called from amqp thread
   void consume_message( const eosio::amqp::delivery_tag_t& delivery_tag, const char* buf, size_t s ) {
      try {
         fc::datastream<const char*> ds( buf, s );
         fc::unsigned_int which;
         fc::raw::unpack(ds, which);
         if( which == fc::unsigned_int(transaction_msg::tag<chain::packed_transaction_v0>::value) ) {
            chain::packed_transaction_v0 v0;
            fc::raw::unpack(ds, v0);
            auto ptr = std::make_shared<chain::packed_transaction>( std::move( v0 ), true );
            handle_message( delivery_tag, std::move( ptr ) );
         } else if ( which == fc::unsigned_int(transaction_msg::tag<chain::packed_transaction>::value) ) {
            auto ptr = std::make_shared<chain::packed_transaction>();
            fc::raw::unpack(ds, *ptr);
            handle_message( delivery_tag, std::move( ptr ) );
         } else {
            FC_THROW_EXCEPTION( fc::out_of_range_exception, "Invalid which ${w} for consume of transaction_type message", ("w", which) );
         }
         if( acked == ack_mode::received ) {
            amqp_trx->ack( delivery_tag );
         }
         return;
      } FC_LOG_AND_DROP()

      amqp_trx->reject( delivery_tag );
   }

   void on_block_start( uint32_t bn ) {
      trx_queue_ptr->on_block_start();
   }

   void on_block_abort( uint32_t bn ) {
      trx_queue_ptr->on_block_stop();
   }

   void on_accepted_block( const chain::block_state_ptr& bsp ) {
      if( acked == ack_mode::in_block ) {
         const auto& entry = tracked_delivery_tags.find( bsp->block_num );
         if( entry != tracked_delivery_tags.end() ) {
            amqp_trx->ack( entry->second, true );
            tracked_delivery_tags.erase( entry );
         }
      }
      trx_queue_ptr->on_block_stop();
   }

private:

   // called from amqp thread
   void handle_message( const eosio::amqp::delivery_tag_t& delivery_tag, chain::packed_transaction_ptr trx ) {
      const auto& tid = trx->id();
      dlog( "received packed_transaction ${id}", ("id", tid) );

      auto trx_trace = fc_create_trace_with_id("Transaction", tid);
      auto trx_span = fc_create_span(trx_trace, "AMQP Received");
      fc_add_tag(trx_span, "trx_id", tid);

      trx_queue_ptr->push( trx,
                [my=shared_from_this(), token=trx_trace.get_token(), delivery_tag, trx](const fc::static_variant<fc::exception_ptr, chain::transaction_trace_ptr>& result) {
            auto trx_span = fc_create_span_from_token(token, "Processed");
            fc_add_tag(trx_span, "trx_id", trx->id());

            // publish to trace plugin as exceptions are not reported via controller signal applied_transaction
            if( result.contains<chain::exception_ptr>() ) {
               auto& eptr = result.get<chain::exception_ptr>();
               if( my->trace_plug ) {
                  my->trace_plug->publish_error( trx->id().str(), eptr->code(), eptr->to_string() );
               }
               fc_add_tag(trx_span, "error", eptr->to_string());
               dlog( "accept_transaction ${id} exception: ${e}", ("id", trx->id())("e", eptr->to_string()) );
               if( my->acked == ack_mode::executed || my->acked == ack_mode::in_block ) { // ack immediately on failure
                  my->amqp_trx->ack( delivery_tag );
               }
            } else {
               auto& trace = result.get<chain::transaction_trace_ptr>();
               fc_add_tag(trx_span, "block_num", trace->block_num);
               fc_add_tag(trx_span, "block_time", trace->block_time.to_time_point());
               fc_add_tag(trx_span, "elapsed", trace->elapsed.count());
               if( trace->receipt ) {
                  fc_add_tag(trx_span, "status", std::string(trace->receipt->status));
               }
               if( trace->except ) {
                  fc_add_tag(trx_span, "error", trace->except->to_string());
               }
               dlog( "accept_transaction ${id}", ("id", trx->id()) );
               if( my->acked == ack_mode::executed ) {
                  my->amqp_trx->ack( delivery_tag );
               } else if ( my->acked == ack_mode::in_block ) {
                  my->tracked_delivery_tags[trace->block_num] = delivery_tag;
               }
            }
         } );
   }
};

amqp_trx_plugin::amqp_trx_plugin()
: my(std::make_shared<amqp_trx_plugin_impl>()) {
   app().register_config_type<ack_mode>();
}

amqp_trx_plugin::~amqp_trx_plugin() {}

void amqp_trx_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto op = cfg.add_options();
   op("amqp-trx-address", bpo::value<std::string>(),
      "AMQP address: Format: amqp://USER:PASSWORD@ADDRESS:PORT\n"
      "Will consume from 'trx' queue.");
   op("amqp-trx-queue-size", bpo::value<uint32_t>()->default_value(my->trx_processing_queue_size),
      "The maximum number of transactions to pull from the AMQP queue at any given time.");
   op("amqp-trx-speculative-execution", bpo::bool_switch()->default_value(false),
      "Allow non-ordered speculative execution of transactions");
   op("amqp-trx-ack-mode", bpo::value<ack_mode>()->default_value(ack_mode::in_block),
      "AMQP ack when 'received' from AMQP, when 'executed', or when 'in_block' is produced that contains trx.\n"
      "Options: received, executed, in_block");
}

void amqp_trx_plugin::plugin_initialize(const variables_map& options) {
   try {
      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT( my->chain_plug, chain::missing_chain_plugin_exception, "chain_plugin required" );

      my->trace_plug = app().find_plugin<amqp_trace_plugin>(); // optional

      EOS_ASSERT( options.count("amqp-trx-address"), chain::plugin_config_exception, "amqp-trx-address required" );
      my->amqp_trx_address = options.at("amqp-trx-address").as<std::string>();

      my->acked = options.at("amqp-trx-ack-mode").as<ack_mode>();

      my->trx_processing_queue_size = options.at("amqp-trx-queue-size").as<uint32_t>();
      my->allow_speculative_execution = options.at("amqp-trx-speculative-execution").as<bool>();

      EOS_ASSERT( my->acked != ack_mode::in_block || !my->allow_speculative_execution, chain::plugin_config_exception,
                  "amqp-trx-ack-mode = in_block not supported with amqp-trx-speculative-execution" );

      my->chain_plug->enable_accept_transactions();
   }
   FC_LOG_AND_RETHROW()
}

void amqp_trx_plugin::plugin_startup() {
   handle_sighup();
   try {

      if( !my->trace_plug ||
          ( my->trace_plug->get_state() != abstract_plugin::initialized &&
            my->trace_plug->get_state() != abstract_plugin::started ) ) {
         dlog( "running without amqp_trace_plugin" );
         my->trace_plug = nullptr;
      } else {
         // always want trace plugin running if specified so traces can be published
         my->trace_plug->plugin_startup();
      }

      ilog( "Starting amqp_trx_plugin" );

      auto* prod_plugin = app().find_plugin<producer_plugin>();
      EOS_ASSERT( prod_plugin, chain::plugin_config_exception, "producer_plugin required" ); // should not be possible
      EOS_ASSERT( my->allow_speculative_execution || prod_plugin->has_producers(), chain::plugin_config_exception,
                  "Must be a producer to run without amqp-trx-speculative-execution" );

      auto& chain = my->chain_plug->chain();
      my->trx_queue_ptr =
            std::make_shared<fifo_trx_processing_queue<producer_plugin>>( chain.get_chain_id(),
                                                                          chain.configured_subjective_signature_length_limit(),
                                                                          my->allow_speculative_execution,
                                                                          chain.get_thread_pool(),
                                                                          prod_plugin,
                                                                          my->trx_processing_queue_size );

      my->block_start_connection.emplace(chain.block_start.connect( [this]( uint32_t bn ){ my->on_block_start( bn ); } ));
      my->block_abort_connection.emplace(chain.block_abort.connect( [this]( uint32_t bn ){ my->on_block_abort( bn ); } ));
      my->accepted_block_connection.emplace(chain.accepted_block.connect( [this]( const auto& bsp ){ my->on_accepted_block( bsp ); } ));

      my->trx_queue_ptr->run();

      my->amqp_trx.emplace( my->amqp_trx_address, "trx",
            [](const std::string& err) {
               elog( "amqp error: ${e}", ("e", err) );
               app().quit();
            },
            [&]( const eosio::amqp::delivery_tag_t& delivery_tag, const char* buf, size_t s ) {
               if( app().is_quiting() ) return; // leave non-ack
               my->consume_message( delivery_tag, buf, s );
            }
      );

   } catch( ... ) {
      // always want plugin_shutdown even on exception
      plugin_shutdown();
      if( my->trace_plug )
         my->trace_plug->plugin_shutdown();
      throw;
   }
}

void amqp_trx_plugin::plugin_shutdown() {
   try {
      dlog( "shutdown.." );

      if( my->amqp_trx ) {
         my->amqp_trx->stop();
      }

      if( my->trx_queue_ptr ) {
         my->trx_queue_ptr->stop();
      }

      dlog( "exit amqp_trx_plugin" );
   }
   FC_CAPTURE_AND_RETHROW()
}

void amqp_trx_plugin::handle_sighup() {
}

} // namespace eosio
