#pragma once
#include <fc/log/appender.hpp>

namespace dfuse { namespace dm  {

   class log_appender : public fc::appender
   {
       public:
            log_appender( const fc::variant& args );
            log_appender();

            virtual ~log_appender();
            void initialize( boost::asio::io_service& io_service ) {
                 setbuf(stdout, NULL);
            }

            virtual void log( const fc::log_message& m );

       private:
            class impl;
            std::unique_ptr<impl> my;
   };

}}

