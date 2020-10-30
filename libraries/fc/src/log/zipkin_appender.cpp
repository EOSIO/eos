#include <fc/exception/exception.hpp>
#include <fc/log/zipkin_appender.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/network/http/http_client.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/variant.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/bitutil.hpp>
#include <fc/variant.hpp>

#include <boost/asio.hpp>

#include <thread>

namespace fc {

class zipkin_appender::impl {
public:
   static constexpr uint32_t max_consecutive_errors = 9;

   config cfg;
   http_client http;
   std::atomic<uint32_t> consecutive_errors = 0;
   std::optional<url> endpoint;
   std::thread thread;
   boost::asio::io_context ctx;
   boost::asio::strand<boost::asio::io_context::executor_type> work_strand = boost::asio::make_strand(ctx);
   boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard = boost::asio::make_work_guard(ctx);

   explicit impl( config c ) :
         cfg( std::move( c ) ) {
   }

   void init();

   void log( const log_message& message );

   ~impl();
};

void zipkin_appender::impl::init() {
   thread = std::thread( [this]() {
      fc::set_os_thread_name( "zipkin" );
      while( true ) {
         try {
            ctx.run();
            break;
         } FC_LOG_AND_DROP();
      }
   } );
}

zipkin_appender::impl::~impl() {
   try {
      work_guard.reset(); // drain the queue
      thread.join();
   } catch (...) {}
}

zipkin_appender::zipkin_appender( const variant& args ) :
      my( new impl( args.as<config>() ) ) {
}

void zipkin_appender::initialize( boost::asio::io_service& io_service ) {
   // called holding the log_config log_mutex
   if( my )
      my->init();
}

void zipkin_appender::shutdown() {
   // called holding the log_config log_mutex
   my.reset();
}

fc::variant create_zipkin_variant( const log_message& message, const std::string& service_name ) {
   // https://zipkin.io/zipkin-api/
   //   std::string traceId;  // [a-f0-9]{16,32} unique id for trace, all children spans shared same id
   //   std::string name;     // logical operation, should have low cardinality
   //   std::string parentId; // The parent span id, or absent if root span
   //   std::string id        // a-f0-9]{16}
   //   int64_t     timestamp // epoch microseconds of start of span
   //   int64_t     duration  // microseconds of span

   log_context context = message.get_context();
   fc::mutable_variant_object d = message.get_data();

   fc::mutable_variant_object mvo;
   auto i = d.find( "zipkin.traceId" );
   if( i != d.end() && i->value().is_uint64() ) {
      uint64_t v = i->value().as_uint64();
      mvo( "traceId", fc::to_hex( reinterpret_cast<const char*>(&v), sizeof( v ) ) );
      d.erase( "zipkin.traceId" );
   } else {
      FC_THROW_EXCEPTION( parse_error_exception, "zipkin.traceId required" );
   }
   i = d.find( "zipkin.id" );
   if( i != d.end() && i->value().is_uint64() ) {
      uint64_t v = i->value().as_uint64();
      mvo( "id", fc::to_hex( reinterpret_cast<const char*>(&v), sizeof( v ) ) );
      d.erase( "zipkin.id" );
   } else {
      FC_THROW_EXCEPTION( parse_error_exception, "zipkin.id required" );
   }
   i = d.find( "zipkin.parentId" );
   if( i != d.end() && i->value().is_uint64() ) {
      uint64_t v = i->value().as_uint64();
      if( v != 0 )
         mvo( "parentId", fc::to_hex( reinterpret_cast<const char*>(&v), sizeof( v ) ) );
      d.erase( "zipkin.parentId" );
   }
   i = d.find( "zipkin.name" );
   if( i != d.end() ) {
      mvo( "name", i->value() );
      d.erase( "zipkin.name" );
   }
   i = d.find( "zipkin.timestamp" );
   if( i != d.end() ) {
      mvo( "timestamp", i->value() );
      d.erase( "zipkin.timestamp" );
   }
   i = d.find( "zipkin.duration" );
   if( i != d.end() ) {
      mvo( "duration", i->value() );
      d.erase( "zipkin.duration" );
   }

   mvo( "localEndpoint", fc::variant_object( "serviceName", service_name ) );

   mvo( "tags", d );

   // /api/v2/spans takes an array of spans
   fc::variants result;
   result.emplace_back( mvo );

   return result;
}

void zipkin_appender::log( const log_message& message ) {
   // called holding the log_config log_mutex
   if( !my || my->consecutive_errors > my->max_consecutive_errors )
      return;

   boost::asio::post(my->work_strand, [this, message]() {
      my->log( message );
   });
}

void zipkin_appender::impl::log( const log_message& message ) {
   if( consecutive_errors > max_consecutive_errors )
      return;

   try {
      auto deadline = fc::time_point::now() + fc::microseconds( cfg.timeout_us );
      if( !endpoint ) {
         endpoint = url( cfg.endpoint + cfg.path );
         std::cout << "info: connecting to zipkin: " << (std::string)*endpoint << std::endl;
      }

      http.post_sync( *endpoint, create_zipkin_variant( message, cfg.service_name ), deadline );

      consecutive_errors = 0;
      return;
   } catch( const fc::exception& e ) {
      std::cerr << "warn: unable to connect to zipkin: " << (cfg.endpoint + cfg.path)
                << ", error: " << e.to_detail_string() << std::endl;
   } catch( const std::exception& e ) {
      std::cerr << "warn: unable to connect to zipkin: " << (cfg.endpoint + cfg.path)
                << ", error: " << e.what() << std::endl;
   } catch( ... ) {
      std::cerr << "warn: unable to connect to zipkin: " << (cfg.endpoint + cfg.path)
                << ", error: unknown" << std::endl;
   }
   ++consecutive_errors;
}

} // fc
