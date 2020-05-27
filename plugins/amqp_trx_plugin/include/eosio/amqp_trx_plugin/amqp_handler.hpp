#pragma once
#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>
#include <amqpcpp/linux_tcp.h>

#include <fc/log/logger.hpp>

namespace eosio {

class amqp_handler : public AMQP::LibBoostAsioHandler {
public:
   explicit amqp_handler( boost::asio::io_service& io_service ) : AMQP::LibBoostAsioHandler( io_service ) {}

   void onError( AMQP::TcpConnection* connection, const char* message ) override {
      elog( "amqp connection failed: ${m}", ("m", message) );
   }

   uint16_t onNegotiate( AMQP::TcpConnection* connection, uint16_t interval ) override {
      return 0; // disable heartbeats
   }
};

class amqp {
   fc::logger logger_;
   std::unique_ptr<amqp_handler> handler_;
   std::unique_ptr<AMQP::TcpConnection> connection_;
   std::unique_ptr<AMQP::TcpChannel> channel_;
   std::string name_;

public:
   amqp( fc::logger logger, boost::asio::io_service& io_service, const std::string& address, std::string name )
         : logger_(std::move( logger ) )
         , name_( std::move( name ) )
   {
      AMQP::Address amqp_address( address );
      fc_dlog( logger_, "Connecting to AMQP address ${a} - Queue: ${q}...",
               ("a", std::string( amqp_address ))( "q", name_ ) );

      handler_ = std::make_unique<amqp_handler>( io_service );
      connection_ = std::make_unique<AMQP::TcpConnection>( handler_.get(), amqp_address );
      channel_ = std::make_unique<AMQP::TcpChannel>( connection_.get() );
      declare_queue();
   }

   void publish( const string& exchange, const std::string& correlation_id, const char* data, size_t data_size ) {
      AMQP::Envelope env( data, data_size );
      env.setCorrelationID( correlation_id );
      channel_->publish( exchange, name_, env, 0 );
   }

   auto& consume() { return channel_->consume( name_ ); }

   void ack( uint64_t delivery_tag ) {
      channel_->ack( delivery_tag );
   }

   void reject( uint64_t delivery_tag ) {
      channel_->reject( delivery_tag );
   }

private:
   void declare_queue() {
      auto& queue = channel_->declareQueue( name_, AMQP::durable );
      queue.onSuccess( [this]( const std::string& name, uint32_t messagecount, uint32_t consumercount ) {
         fc_dlog( logger_, "AMQP Connected Successfully!\n Queue ${q} - Messages: ${mc} - Consumers: ${cc}",
                  ("q", name)( "mc", messagecount )( "cc", consumercount ) );
      } );
      queue.onError( [this]( const char* error_message ) {
         std::string err = "AMQP Queue error: " + std::string( error_message );
         fc_elog( logger_, err );
         app().quit();
      } );
   }
};

} // namespace eosio
