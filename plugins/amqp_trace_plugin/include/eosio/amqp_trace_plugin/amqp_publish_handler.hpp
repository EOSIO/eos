#pragma once
#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>
#include <amqpcpp/linux_tcp.h>
#include <string>

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

class amqp_publish {
public:
   using on_error_t = std::function<void(const std::string& err)>;

   amqp_publish( boost::asio::io_service& io_service, const std::string& address, std::string name, on_error_t on_err )
         : name_( std::move( name ) )
         , on_error( std::move(on_err) )
   {
      AMQP::Address amqp_address( address );
      ilog( "Connecting to AMQP address ${a} - Queue: ${q}...", ("a", std::string( amqp_address ))( "q", name_ ) );

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

private:
   void declare_queue() {
      auto& queue = channel_->declareQueue( name_, AMQP::durable );
      queue.onSuccess( []( const std::string& name, uint32_t messagecount, uint32_t consumercount ) {
         dlog( "AMQP Connected Successfully!\n Queue ${q} - Messages: ${mc} - Consumers: ${cc}",
                  ("q", name)( "mc", messagecount )( "cc", consumercount ) );
      } );
      queue.onError( [on_err=on_error]( const char* error_message ) {
         on_err( error_message );
      } );
   }

private:
   std::unique_ptr<amqp_handler> handler_;
   std::unique_ptr<AMQP::TcpConnection> connection_;
   std::unique_ptr<AMQP::TcpChannel> channel_;
   std::string name_;
   on_error_t on_error;
};

} // namespace eosio
