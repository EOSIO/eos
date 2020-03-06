#include <dfuse/dm/log_appender.hpp>
#include <fc/log/log_message.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/string.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>
#ifndef WIN32
#include <unistd.h>
#endif
#include <boost/thread/mutex.hpp>
#include <fc/exception/exception.hpp>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace dfuse { namespace dm {
   // static bool reg_deep_mind_appender = fc::log_config::register_appender<log_appender>( "deep-mind" );

   class log_appender::impl {
      public:
         boost::mutex log_mutex;
   };

   log_appender::log_appender( const fc::variant& args )
   :my(new impl){}

   log_appender::log_appender()
   :my(new impl){}

   log_appender::~log_appender() {}

   void log_appender::log( const fc::log_message& m ) {
      FILE* out = stdout;

      fc::string message = fc::format_string( "DMLOG " + m.get_format() + "\n", m.get_data() );
      std::unique_lock<boost::mutex> lock(my->log_mutex);

      int retries = 0;
      auto remaining_size = message.size();
      auto message_ptr = message.c_str();
      while (true) {
         auto written = fwrite(message_ptr, sizeof(char), remaining_size, out);
         if (written == remaining_size) {
            break;
         }

         int errval = ferror(out);
         fprintf(stderr, "DMLOG FPRINTF_FAILED failed written=%lu remaining=%lu %d %d %s\n", written, remaining_size, ferror(out), errval, strerror(errno));

         if (retries++ > 5) {
            fprintf(stderr, "DMLOG FPRINTF_FAILED enough is enough\n");
            exit(1);
         }

         message_ptr = &message_ptr[written];
         remaining_size -= written;

         // We don't `fflush`, rather we made `stdout` unbuffered with `setbuf`. This way we have
         // atomic error handling and retry logic up here ^^.
      }
   }
}}

