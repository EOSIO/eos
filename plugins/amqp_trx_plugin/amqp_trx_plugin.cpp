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

constexpr auto def_max_trx_in_progress_size = 100*1024*1024; // 100 MB

} // anonymous

namespace eosio {

using boost::signals2::scoped_connection;

struct amqp_trx_plugin_impl : std::enable_shared_from_this<amqp_trx_plugin_impl> {

   chain_plugin* chain_plug = nullptr;
   amqp_trace_plugin* trace_plug = nullptr;
   std::optional<amqp> amqp_trx;

   std::string amqp_trx_address;
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
         return;
      } FC_LOG_AND_DROP()

      amqp_trx->reject( delivery_tag );
   }

private:

   // called from amqp thread
   void handle_message( const eosio::amqp::delivery_tag_t& delivery_tag, chain::packed_transaction_ptr trx ) {
      const auto& tid = trx->id();
      dlog( "received packed_transaction ${id}", ("id", tid) );

      auto trx_trace = fc_create_trace("Transaction");
      auto trx_span = fc_create_span(trx_trace, "AMQP Received");
      fc_add_str_tag(trx_span, "trx_id", tid.str());

      trx_queue_ptr->push( trx,
                [my=shared_from_this(), delivery_tag, trx](const fc::static_variant<fc::exception_ptr, chain::transaction_trace_ptr>& result) {
            my->amqp_trx->ack( delivery_tag );

            auto trx_trace = fc_create_trace("Transaction");
            auto trx_span = fc_create_span(trx_trace, "Processed");
            fc_add_str_tag(trx_span, "trx_id", trx->id().str());


            // publish to trace plugin as exceptions are not reported via controller signal applied_transaction
            if( result.contains<chain::exception_ptr>() ) {
               auto& eptr = result.get<chain::exception_ptr>();
               if( my->trace_plug ) {
                  my->trace_plug->publish_error( trx->id().str(), eptr->code(), eptr->to_string() );
               }
               fc_add_str_tag(trx_span, "error", eptr->to_string());
               dlog( "accept_transaction ${id} exception: ${e}", ("id", trx->id())("e", eptr->to_string()) );
            } else {
               auto& trace = result.get<chain::transaction_trace_ptr>();
               dlog( "accept_transaction ${id}", ("id", trx->id()) );
               fc_add_str_tag(trx_span, "block_num", std::to_string(trace->block_num));
               fc_add_str_tag(trx_span, "block_time", std::string(trace->block_time.to_time_point()));
               fc_add_str_tag(trx_span, "elapsed", std::to_string(trace->elapsed.count()));
               if( trace->receipt ) {
                  fc_add_str_tag(trx_span, "status", std::string(trace->receipt->status));
               }
               if( trace->except ) {
                  fc_add_str_tag(trx_span, "error", trace->except->to_string());
               }
            }
         } );
   }
};

amqp_trx_plugin::amqp_trx_plugin()
: my(std::make_shared<amqp_trx_plugin_impl>()) {}

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
}

void amqp_trx_plugin::plugin_initialize(const variables_map& options) {
   try {
      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT( my->chain_plug, chain::missing_chain_plugin_exception, "chain_plugin required" );

      my->trace_plug = app().find_plugin<amqp_trace_plugin>(); // optional

      EOS_ASSERT( options.count("amqp-trx-address"), chain::plugin_config_exception, "amqp-trx-address required" );
      my->amqp_trx_address = options.at("amqp-trx-address").as<std::string>();

      my->trx_processing_queue_size = options.at("amqp-trx-queue-size").as<uint32_t>();
      my->allow_speculative_execution = options.at("amqp-trx-speculative-execution").as<bool>();
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

      my->block_start_connection.emplace(chain.block_start.connect( [this]( uint32_t bn ){ my->trx_queue_ptr->on_block_start(); } ));
      my->block_abort_connection.emplace(chain.block_abort.connect( [this]( uint32_t bn ){ my->trx_queue_ptr->on_block_stop(); } ));
      my->accepted_block_connection.emplace(chain.accepted_block.connect( [this]( const auto& bsp ){ my->trx_queue_ptr->on_block_stop(); } ));

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
