#include <eosio/amqp_trx_plugin/amqp_trx_plugin.hpp>
#include <eosio/amqp_trace_plugin/amqp_handler.hpp>
#include <eosio/amqp_trace_plugin/amqp_trace_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/thread_utils.hpp>

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
   std::atomic<uint32_t> trx_in_progress_size{0};

   // called from amqp thread
   bool consume_message( const char* buf, size_t s ) {
      try {
         fc::datastream<const char*> ds( buf, s );
         fc::unsigned_int which;
         fc::raw::unpack(ds, which);
         if( which == fc::unsigned_int(transaction_msg::tag<chain::packed_transaction_v0>::value) ) {
            chain::packed_transaction_v0 v0;
            fc::raw::unpack(ds, v0);
            auto ptr = std::make_shared<chain::packed_transaction>( std::move( v0 ), true );
            handle_message( std::move( ptr ) );
         } else if ( which == fc::unsigned_int(transaction_msg::tag<chain::packed_transaction>::value) ) {
            auto ptr = std::make_shared<chain::packed_transaction>();
            fc::raw::unpack(ds, *ptr);
            handle_message( std::move( ptr ) );
         } else {
            FC_THROW_EXCEPTION( fc::out_of_range_exception, "Invalid which ${w} for consume of transaction_type message", ("w", which) );
         }
         return true;
      } FC_LOG_AND_DROP()
      return false;
   }

private:

   // called from amqp thread
   void handle_message( chain::packed_transaction_ptr trx ) {
      const auto& tid = trx->id();
      dlog( "received packed_transaction ${id}", ("id", tid) );

      auto trx_in_progress = trx_in_progress_size.load();
      if( trx_in_progress > def_max_trx_in_progress_size ) {
         wlog( "Dropping trx, too many trx in progress ${s} bytes", ("s", trx_in_progress) );
         if( trace_plug ) {
            std::string err = "Dropped trx, too many trx in progress " + std::to_string( trx_in_progress ) + " bytes";
            trace_plug->publish_error( trx->id().str(), chain::tx_resource_exhaustion::code_enum::code_value, std::move(err) );
         }
         return;
      }

      trx_in_progress_size += trx->get_estimated_size();
      app().post( priority::medium_low, [my=shared_from_this(), trx{std::move(trx)}]() {
         my->chain_plug->accept_transaction( trx,
            [my, trx](const fc::static_variant<fc::exception_ptr, chain::transaction_trace_ptr>& result) mutable {
               // publish to trace plugin as execptions are not reported via controller signal applied_transaction
               if( result.contains<chain::exception_ptr>() ) {
                  auto& eptr = result.get<chain::exception_ptr>();
                  if( my->trace_plug ) {
                     my->trace_plug->publish_error( trx->id().str(), eptr->code(), eptr->to_string() );
                  }
                  dlog( "accept_transaction ${id} exception: ${e}", ("id", trx->id())("e", eptr->to_string()) );
               } else {
                  dlog( "accept_transaction ${id}", ("id", trx->id()) );
               }
               my->trx_in_progress_size -= trx->get_estimated_size();

            } );
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
}

void amqp_trx_plugin::plugin_initialize(const variables_map& options) {
   try {
      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT( my->chain_plug, chain::missing_chain_plugin_exception, "chain_plugin required" );

      my->trace_plug = app().find_plugin<amqp_trace_plugin>(); // optional

      EOS_ASSERT( options.count("amqp-trx-address"), chain::plugin_config_exception, "amqp-trx-address required" );
      my->amqp_trx_address = options.at("amqp-trx-address").as<std::string>();
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

      my->amqp_trx.emplace( my->amqp_trx_address, "trx",
            [](const std::string& err) {
               elog( "amqp error: ${e}", ("e", err) );
               app().quit();
            },
            [&]( const char* buf, size_t s ) {
               if( app().is_quiting() ) return false;
               return my->consume_message( buf, s );
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

      dlog( "exit amqp_trx_plugin" );
   }
   FC_CAPTURE_AND_RETHROW()
}

void amqp_trx_plugin::handle_sighup() {
}

} // namespace eosio
