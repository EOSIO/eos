#include <eosio/chain/deep_mind.hpp>

namespace eosio { namespace chain {

   fc::logging_config add_deep_mind_logger(fc::logging_config config) {
      config.appenders.push_back(
         fc::appender_config( "deep-mind", "dmlog" )
      );

      fc::logger_config dmlc;
      dmlc.name = "deep-mind";
      dmlc.level = fc::log_level::debug;
      dmlc.enabled = true;
      dmlc.appenders.push_back("deep-mind");

      config.loggers.push_back( dmlc );

      fc::configure_logging(config);

      return config;
   }

   void set_dmlog_appender_stdout_unbuffered() {
      // The actual `fc::dmlog_appender` implementation that is currently used by deep mind
      // logger is using `stdout` to prints it's log line out. Deep mind logging outputs
      // massive amount of data out of the process, which can lead under pressure to some
      // of the system calls (i.e. `fwrite`) to fail abruptly without fully writing the
      // entire line.
      //
      // Recovering from errors on a buffered (line or full) and continuing retrying write
      // is merely impossible to do right, because the buffer is actually held by the
      // underlying `libc` implementation nor the operation system.
      //
      // To ensure good functionalities of deep mind tracer, the `stdout` is made unbuffered
      // and the actual `fc::dmlog_appender` deals with retry when facing error, enabling a much
      // more robust deep mind output.
      //
      // Changing the standard `stdout` behavior from buffered to unbuffered can is disruptive
      // and can lead to weird scenarios in the logging process if `stdout` is used there too.
      //
      // In a future version, the `fc::dmlog_appender` implementation will switch to a `FIFO` file
      // approach, which will remove the dependency on `stdout` and hence this call.
      //
      // For the time being, when `deep-mind = true` is activated, we set `stdout` here to
      // be an unbuffered I/O stream.
      setbuf(stdout, NULL);
   }

} } /// namespace eosio::chain
