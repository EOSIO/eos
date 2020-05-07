#include <fc/log/dmlog_appender.hpp>
#include <fc/log/log_message.hpp>
#include <fc/string.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>
#ifndef WIN32
#include <unistd.h>
#endif
#include <boost/asio/io_context.hpp>
#include <boost/thread/mutex.hpp>
#include <fc/exception/exception.hpp>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace fc {
   class dmlog_appender::impl {
      public:
         bool is_stopped;
         boost::asio::io_service* io_service;
         boost::mutex log_mutex;
   };

   dmlog_appender::dmlog_appender( const variant& args )
   :my(new impl){}

   dmlog_appender::dmlog_appender()
   :my(new impl){}

   dmlog_appender::~dmlog_appender() {}

   void dmlog_appender::initialize( boost::asio::io_service& io_service ) {
      my->io_service = &io_service;
   }

   void dmlog_appender::log( const log_message& m ) {
      FILE* out = stdout;

      string message = format_string( "DMLOG " + m.get_format() + "\n", m.get_data() );
      std::unique_lock<boost::mutex> lock(my->log_mutex);
      if (my->is_stopped) {
         // It might happen that `io_server->stop` was called due to printing errors but did not take
         // effect just yet. So if we are stopped, print a line that we are terminated and return.
         fprintf(out, "DMLOG FPRINTF_FAILURE_TERMINATED\n");
         return;
      }

      int retries = 0;
      auto remaining_size = message.size();
      auto message_ptr = message.c_str();
      while (true) {
         auto written = fwrite(message_ptr, sizeof(char), remaining_size, out);
         if (written == remaining_size) {
            break;
         }

         fprintf(stderr, "DMLOG FPRINTF_FAILED failed written=%lu remaining=%lu %d %s\n", written, remaining_size, ferror(out), strerror(errno));

         if (retries++ > 5) {
            fprintf(stderr, "DMLOG FPRINTF_FAILURE_TERMINATED\n");
            my->io_service->stop();
            my->is_stopped = true;

            return;
         }

         message_ptr = &message_ptr[written];
         remaining_size -= written;

         // We don't `fflush`, rather we made `stdout` unbuffered with `setbuf`. This way we have
         // atomic error handling and retry logic up here ^^.
      }
   }
}

