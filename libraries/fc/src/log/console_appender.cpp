#include <fc/log/console_appender.hpp>
#include <fc/log/log_message.hpp>
#include <fc/string.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>
#ifndef WIN32
#include <unistd.h>
#endif
#define COLOR_CONSOLE 1
#include "console_defines.h"
#include <fc/exception/exception.hpp>
#include <iomanip>
#include <mutex>
#include <sstream>


namespace fc {

   class console_appender::impl {
   public:
     config                      cfg;
     color::type                 lc[log_level::off+1];
     bool                        use_syslog_header{getenv("JOURNAL_STREAM") != nullptr};
#ifdef WIN32
     HANDLE                      console_handle;
#endif
   };

   console_appender::console_appender( const variant& args )
   :my(new impl)
   {
      configure( args.as<config>() );
   }

   console_appender::console_appender( const config& cfg )
   :my(new impl)
   {
      configure( cfg );
   }
   console_appender::console_appender()
   :my(new impl){}


   void console_appender::configure( const config& console_appender_config )
   { try {
#ifdef WIN32
      my->console_handle = INVALID_HANDLE_VALUE;
#endif
      my->cfg = console_appender_config;
#ifdef WIN32
         if (my->cfg.stream = stream::std_error)
           my->console_handle = GetStdHandle(STD_ERROR_HANDLE);
         else if (my->cfg.stream = stream::std_out)
           my->console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

         for( int i = 0; i < log_level::off+1; ++i )
            my->lc[i] = color::console_default;
         for( auto itr = my->cfg.level_colors.begin(); itr != my->cfg.level_colors.end(); ++itr )
            my->lc[itr->level] = itr->color;
   } FC_CAPTURE_AND_RETHROW( (console_appender_config) ) }

   console_appender::~console_appender() {}

   #ifdef WIN32
   static WORD
   #else
   static const char*
   #endif
   get_console_color(console_appender::color::type t ) {
      switch( t ) {
         case console_appender::color::red: return CONSOLE_RED;
         case console_appender::color::green: return CONSOLE_GREEN;
         case console_appender::color::brown: return CONSOLE_BROWN;
         case console_appender::color::blue: return CONSOLE_BLUE;
         case console_appender::color::magenta: return CONSOLE_MAGENTA;
         case console_appender::color::cyan: return CONSOLE_CYAN;
         case console_appender::color::white: return CONSOLE_WHITE;
         case console_appender::color::console_default:
         default:
            return CONSOLE_DEFAULT;
      }
   }

   string fixed_size( size_t s, const string& str ) {
      if( str.size() == s ) return str;
      if( str.size() > s ) return str.substr( 0, s );
      string tmp = str;
      tmp.append( s - str.size(), ' ' );
      return tmp;
   }

   void console_appender::log( const log_message& m ) {
      //fc::string message = fc::format_string( m.get_format(), m.get_data() );
      //fc::variant lmsg(m);

      FILE* out = stream::std_error ? stderr : stdout;

      //fc::string fmt_str = fc::format_string( cfg.format, mutable_variant_object(m.get_context())( "message", message)  );

      const log_context context = m.get_context();
      std::string file_line = context.get_file().substr( 0, 22 );
      file_line += ':';
      file_line += fixed_size(  6, fc::to_string( context.get_line_number() ) );

      std::string line;
      line.reserve( 256 );
      if(my->use_syslog_header) {
         switch(m.get_context().get_log_level()) {
            case log_level::error:
               line += "<3>";
               break;
            case log_level::warn:
               line += "<4>";
               break;
            case log_level::info:
               line += "<6>";
               break;
            case log_level::debug:
               line += "<7>";
               break;
         }
      }

      line += fixed_size(  5, context.get_log_level().to_string() ); line += ' ';
      // use now() instead of context.get_timestamp() because log_message construction can include user provided long running calls
      line += string( time_point::now() ); line += ' ';
      line += fixed_size(  9, context.get_thread_name() ); line += ' ';
      line += fixed_size( 29, file_line ); line += ' ';

      auto me = context.get_method();
      // strip all leading scopes...
      if( me.size() ) {
         uint32_t p = 0;
         for( uint32_t i = 0;i < me.size(); ++i ) {
             if( me[i] == ':' ) p = i;
         }

         if( me[p] == ':' ) ++p;
         line += fixed_size( 20, context.get_method().substr( p, 20 ) ); line += ' ';
      }

      line += "] ";
      std::string message = fc::format_string( m.get_format(), m.get_data() );
      message.erase( std::remove(message.begin(), message.end(), '\r'), message.end() );
      line += message;

      print( line, my->lc[context.get_log_level()] );

      fprintf( out, "\n" );

      if( my->cfg.flush ) fflush( out );
   }

   void console_appender::print( const std::string& text, color::type text_color )
   {
      FILE* out = stream::std_error ? stderr : stdout;

      #ifdef WIN32
         if (my->console_handle != INVALID_HANDLE_VALUE)
           SetConsoleTextAttribute(my->console_handle, get_console_color(text_color));
      #else
         if(isatty(fileno(out))) fprintf( out, "%s", get_console_color( text_color ) );
      #endif

      if( text.size() )
         fprintf( out, "%s", text.c_str() ); //fmt_str.c_str() );

      #ifdef WIN32
      if (my->console_handle != INVALID_HANDLE_VALUE)
        SetConsoleTextAttribute(my->console_handle, CONSOLE_DEFAULT);
      #else
      if(isatty(fileno(out))) fprintf( out, "%s", CONSOLE_DEFAULT );
      #endif

      if( my->cfg.flush ) fflush( out );
   }

}
