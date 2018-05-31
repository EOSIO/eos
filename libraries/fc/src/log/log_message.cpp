#include <fc/log/log_message.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant.hpp>
#include <fc/time.hpp>
#include <fc/filesystem.hpp>
#include <fc/io/json.hpp>

namespace fc
{
   const string& get_thread_name();
   namespace detail
   {
      class log_context_impl
      {
         public:
            log_level level;
            string       file;
            uint64_t     line;
            string       method;
            string       thread_name;
            string       task_name;
            string       hostname;
            string       context;
            time_point   timestamp;
      };

      class log_message_impl
      {
         public:
            log_message_impl( log_context&& ctx )
            :context( std::move(ctx) ){}
            log_message_impl(){}

            log_context     context;
            string          format;
            variant_object  args;
      };
   }



   log_context::log_context()
   :my( std::make_shared<detail::log_context_impl>() ){}

   log_context::log_context( log_level ll, const char* file, uint64_t line, 
                                            const char* method )
   :my( std::make_shared<detail::log_context_impl>() )
   {
      my->level       = ll;
      my->file        = fc::path(file).filename().generic_string(); // TODO truncate filename
      my->line        = line;
      my->method      = method;
      my->timestamp   = time_point::now();
      my->thread_name = fc::get_thread_name();
   }

   log_context::log_context( const variant& v, uint32_t max_depth )
   :my( std::make_shared<detail::log_context_impl>() )
   {
       auto obj = v.get_object();
       my->level        = obj["level"].as<log_level>(max_depth);
       my->file         = obj["file"].as_string();
       my->line         = obj["line"].as_uint64();
       my->method       = obj["method"].as_string();
       my->hostname     = obj["hostname"].as_string();
       my->thread_name  = obj["thread_name"].as_string();
       if (obj.contains("task_name"))
         my->task_name    = obj["task_name"].as_string();
       my->timestamp    = obj["timestamp"].as<time_point>(max_depth);
       if( obj.contains( "context" ) )
           my->context      = obj["context"].as<string>(max_depth);
   }

   fc::string log_context::to_string()const
   {
      return my->thread_name + "  " + my->file + ":" + fc::to_string(my->line) + " " + my->method;

   }

   void log_context::append_context( const fc::string& s )
   {
        if (!my->context.empty())
          my->context += " -> ";
        my->context += s;
   }

   log_context::~log_context(){}


   void to_variant( const log_context& l, variant& v, uint32_t max_depth)
   { 
      v = l.to_variant(max_depth);
   }

   void from_variant( const variant& l, log_context& c, uint32_t max_depth)
   { 
      c = log_context(l, max_depth); 
   }

   void from_variant( const variant& l, log_message& c, uint32_t max_depth)
   { 
      c = log_message(l, max_depth);
   }
   void to_variant( const log_message& m, variant& v, uint32_t max_depth)
   {
        v = m.to_variant(max_depth);
   }

   void  to_variant( log_level e, variant& v, uint32_t )
   {
      switch( e )
      {
        case log_level::all: 
           v = "all";
           return;
        case log_level::debug:
           v = "debug";
           return;
        case log_level::info:
           v = "info";
           return;
        case log_level::warn:
           v = "warn";
           return;
        case log_level::error:
           v = "error";
           return;
        case log_level::off:
           v = "off";
           return;
      }
   }
   void from_variant( const variant& v, log_level& e, uint32_t )
   {
      try 
      {
        if( v.as_string() == "all" ) e = log_level::all;
        else if( v.as_string() == "debug" ) e = log_level::debug;
        else if( v.as_string() == "info" ) e = log_level::info;
        else if( v.as_string() == "warn" ) e = log_level::warn;
        else if( v.as_string() == "error" ) e = log_level::error;
        else if( v.as_string() == "off" ) e = log_level::off;
        else FC_THROW_EXCEPTION( bad_cast_exception, "Failed to cast from Variant to log_level" );
      } FC_RETHROW_EXCEPTIONS( error, 
                                   "Expected 'all|debug|info|warn|error|off', but got '${variant}'",
                                   ("variant",v) );
   }



   string     log_context::get_file()const       { return my->file; }
   uint64_t   log_context::get_line_number()const { return my->line; }
   string     log_context::get_method()const     { return my->method; }
   string     log_context::get_thread_name()const { return my->thread_name; }
   string     log_context::get_task_name()const { return my->task_name; }
   string     log_context::get_host_name()const   { return my->hostname; }
   time_point  log_context::get_timestamp()const  { return my->timestamp; }
   log_level  log_context::get_log_level()const{ return my->level;   }
   string     log_context::get_context()const   { return my->context; }


   variant log_context::to_variant(uint32_t max_depth)const
   {
      mutable_variant_object o;
              o( "level",        variant(my->level, max_depth - 1)      )
               ( "file",         my->file                )
               ( "line",         my->line                )
               ( "method",       my->method              )
               ( "hostname",     my->hostname            )
               ( "thread_name",  my->thread_name         )
               ( "timestamp",    variant(my->timestamp, max_depth - 1)  );

      if( my->context.size() ) 
         o( "context",      my->context             );

      return o;
   }

   log_message::~log_message(){}
   log_message::log_message()
   :my( std::make_shared<detail::log_message_impl>() ){}

   log_message::log_message( log_context ctx, std::string format, variant_object args )
      :my( std::make_shared<detail::log_message_impl>(std::move(ctx)) )
   {
      my->format  = std::move(format);
      my->args    = std::move(args);
   }

   log_message::log_message( const variant& v, uint32_t max_depth )
   :my( std::make_shared<detail::log_message_impl>( log_context( v.get_object()["context"], max_depth ) ) )
   {
      my->format = v.get_object()["format"].as_string();
      my->args   = v.get_object()["data"].get_object();
   }

   variant log_message::to_variant(uint32_t max_depth)const
   {
      return limited_mutable_variant_object(max_depth)
                          ( "context", my->context )
                          ( "format",  my->format )
                          ( "data",    my->args   );
   }

   log_context          log_message::get_context()const { return my->context; }
   string              log_message::get_format()const  { return my->format;  }
   variant_object log_message::get_data()const    { return my->args;    }

   string        log_message::get_message()const
   {
      return format_string( my->format, my->args );
   }


} // fc

