#pragma once
#include <fc/log/appender.hpp>
#include <fc/log/logger.hpp>
#include <vector>

namespace fc 
{
   class console_appender final : public appender 
   {
       public:
            struct color 
            {
                enum type {
                   red,
                   green,
                   brown,
                   blue,
                   magenta,
                   cyan,
                   white,
                   console_default,
                };
            };

            struct stream { enum type { std_out, std_error }; };

            struct level_color 
            {
               level_color( log_level l=log_level::all, 
                            color::type c=color::console_default )
               :level(l),color(c){}

               log_level                         level;
               console_appender::color::type     color;
            };

            struct config 
            {
               config()
               :format( "${timestamp} ${thread_name} ${context} ${file}:${line} ${method} ${level}]  ${message}" ),
                stream(console_appender::stream::std_error),flush(true){}

               fc::string                         format;
               console_appender::stream::type     stream;
               std::vector<level_color>           level_colors;
               bool                               flush;
            };


            console_appender( const variant& args );
            console_appender( const config& cfg );
            console_appender();

            ~console_appender();
            void initialize( boost::asio::io_service& io_service ) {}
            virtual void log( const log_message& m );
            
            void print( const std::string& text_to_print, 
                        color::type text_color = color::console_default );

            void configure( const config& cfg );

       private:
            class impl;
            std::unique_ptr<impl> my;
   };
} // namespace fc

#include <fc/reflect/reflect.hpp>
FC_REFLECT_ENUM( fc::console_appender::stream::type, (std_out)(std_error) )
FC_REFLECT_ENUM( fc::console_appender::color::type, (red)(green)(brown)(blue)(magenta)(cyan)(white)(console_default) )
FC_REFLECT( fc::console_appender::level_color, (level)(color) )
FC_REFLECT( fc::console_appender::config, (format)(stream)(level_colors)(flush) )
