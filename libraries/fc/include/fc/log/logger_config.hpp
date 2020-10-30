#pragma once
#include <fc/log/logger.hpp>
#include <fc/log/appender.hpp>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace fc {
   class path;
   struct appender_config {
      appender_config(const string& name = "",
                      const string& type = "",
                      variant args = variant()) :
        name(name),
        type(type),
        args(fc::move(args)),
        enabled(true)
      {}
      string   name;
      string   type;
      variant  args;
      bool     enabled;
   };

   struct logger_config {
      logger_config(const fc::string& name = ""):name(name),enabled(true),additivity(false){}
      string                           name;
      ostring                          parent;
      /// if not set, then parents level is used.
      fc::optional<log_level>          level;
      bool                             enabled;
      /// if any appenders are sepecified, then parent's appenders are not set.
      bool                             additivity;
      std::vector<string>              appenders;
   };

   struct logging_config {
      static logging_config default_config();
      std::vector<string>          includes;
      std::vector<appender_config> appenders;
      std::vector<logger_config>   loggers;
   };

   struct log_config {

      template<typename T>
      static bool register_appender(const fc::string& type) {
         return register_appender( type, std::make_shared<detail::appender_factory_impl<T>>() );
      }

      static bool register_appender( const fc::string& type, const appender_factory::ptr& f );

      static logger get_logger( const fc::string& name );
      static void update_logger( const fc::string& name, logger& log );

      static void initialize_appenders( boost::asio::io_service& ios );

      static bool configure_logging( const logging_config& l );

      static uint64_t get_next_unique_id();

   private:
      static log_config& get();

      friend class logger;

      std::mutex                                               log_mutex;
      std::unordered_map<std::string, appender_factory::ptr>   appender_factory_map;
      std::unordered_map<std::string, appender::ptr>           appender_map;
      std::unordered_map<std::string, logger>                  logger_map;
      uint64_t                                                 next_id = 0;
   };

   void configure_logging( const fc::path& log_config );
   bool configure_logging( const logging_config& l );

   void set_os_thread_name( const string& name );
   void set_thread_name( const string& name );
   const string& get_thread_name();
}

#include <fc/reflect/reflect.hpp>
FC_REFLECT( fc::appender_config, (name)(type)(args)(enabled) )
FC_REFLECT( fc::logger_config, (name)(parent)(level)(enabled)(additivity)(appenders) )
FC_REFLECT( fc::logging_config, (includes)(appenders)(loggers) )
