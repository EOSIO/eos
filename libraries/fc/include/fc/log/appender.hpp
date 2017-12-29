#pragma once
#include <fc/any.hpp>
#include <fc/shared_ptr.hpp>
#include <fc/string.hpp>

#if BOOST_VERSION >= 106600
namespace boost { namespace asio { class io_context; typedef io_context io_service; } }
#else
namespace boost { namespace asio { class io_service; } }
#endif

namespace fc {
   class appender;
   class log_message;
   class variant;

   class appender_factory : public fc::retainable {
      public:
       typedef fc::shared_ptr<appender_factory> ptr;

       virtual ~appender_factory(){};
       virtual fc::shared_ptr<appender> create( const variant& args ) = 0;
   };

   namespace detail {
      template<typename T>
      class appender_factory_impl : public appender_factory {
        public:
           virtual fc::shared_ptr<appender> create( const variant& args ) {
              return fc::shared_ptr<appender>(new T(args));
           }
      };
   }

   class appender : public fc::retainable {
      public:
         typedef fc::shared_ptr<appender> ptr;

         template<typename T>
         static bool register_appender(const fc::string& type) {
            return register_appender( type, new detail::appender_factory_impl<T>() );
         }

         static appender::ptr create( const fc::string& name, const fc::string& type, const variant& args  );
         static appender::ptr get( const fc::string& name );
         static bool          register_appender( const fc::string& type, const appender_factory::ptr& f );

         virtual void initialize( boost::asio::io_service& io_service ) = 0;
         virtual void log( const log_message& m ) = 0;
   };
}
