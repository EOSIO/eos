#pragma once
#include <fc/io/buffered_iostream.hpp>
#include <fc/variant_object.hpp>
#include <fc/thread/future.hpp>
#include <fc/log/logger.hpp>
#include <functional>

namespace fc { namespace rpc  {

   namespace detail { class json_connection_impl; }

   /**
    * @brief Implements JSON-RPC 2.0 over a set of io streams
    *
    * Each JSON RPC message is expected to be on its own line, violators
    * will be prosecuted to the fullest extent of the law.
    */
   class json_connection
   {
      public:
         typedef std::function<variant(const variants&)>       method;
         typedef std::function<variant(const variant_object&)> named_param_method;

         json_connection( fc::buffered_istream_ptr in, fc::buffered_ostream_ptr out );
         ~json_connection();

         /**
          *   Starts processing messages from input
          */
         future<void> exec();

         bool is_open();
         void close();

         void set_on_disconnected_callback(std::function<void(fc::exception_ptr)> callback);

         logger get_logger()const;
         void   set_logger( const logger& l );

         /**
          * @name server interface
          *
          * Adding methods to the interface allows the remote side
          * to call them.
          */
         ///@{ 
         void add_method( const fc::string& name, method );
         void add_named_param_method( const fc::string& name, named_param_method );
         void remove_method( const fc::string& name );
         //@}

         /**
          * @name client interface
          */
         ///@{
         void notice( const fc::string& method );
         void notice( const fc::string& method, const variants& args );
         void notice( const fc::string& method, const variant_object& named_args );

         /// args will be handled as named params
         future<variant> async_call( const fc::string& method, 
                                     const variant_object& args );

         future<variant> async_call( const fc::string& method, mutable_variant_object args );

         /// Sending in an array of variants will be handled as positional arguments
         future<variant> async_call( const fc::string& method, 
                                     const variants& args );

         future<variant> async_call( const fc::string& method );

         future<variant> async_call( const fc::string& method, 
                                     const variant& a1 );

         future<variant> async_call( const fc::string& method, 
                                     const variant& a1, 
                                     const variant& a2 );

         future<variant> async_call( const fc::string& method, 
                                     const variant& a1, 
                                     const variant& a2, 
                                     const variant& a3 );

         future<variant> async_call( const fc::string& method, 
                                     const variant& a1, 
                                     const variant& a2, 
                                     const variant& a3, 
                                     const variant& a4 );

         future<variant> async_call( const fc::string& method, 
                                     const variant& a1, 
                                     const variant& a2, 
                                     const variant& a3, 
                                     const variant& a4, 
                                     const variant& a5 );

         future<variant> async_call( const fc::string& method, 
                                     const variant& a1, 
                                     const variant& a2, 
                                     const variant& a3, 
                                     const variant& a4, 
                                     const variant& a5, 
                                     const variant& a6 );

         future<variant> async_call( const fc::string& method, 
                                     const variant& a1, 
                                     const variant& a2, 
                                     const variant& a3, 
                                     const variant& a4, 
                                     const variant& a5, 
                                     const variant& a6,
                                     const variant& a7 
                                     );

         future<variant> async_call( const fc::string& method, 
                                     const variant& a1, 
                                     const variant& a2, 
                                     const variant& a3, 
                                     const variant& a4, 
                                     const variant& a5, 
                                     const variant& a6,
                                     const variant& a7, 
                                     const variant& a8 
                                     );
         future<variant> async_call( const fc::string& method, 
                                     const variant& a1, 
                                     const variant& a2, 
                                     const variant& a3, 
                                     const variant& a4, 
                                     const variant& a5, 
                                     const variant& a6,
                                     const variant& a7, 
                                     const variant& a8, 
                                     const variant& a9
                                     );
         future<variant> async_call( const fc::string& method, 
                                     const variant& a1, 
                                     const variant& a2, 
                                     const variant& a3, 
                                     const variant& a4, 
                                     const variant& a5, 
                                     const variant& a6,
                                     const variant& a7, 
                                     const variant& a8, 
                                     const variant& a9, 
                                     const variant& a10 
                                     );

         template<typename Result>
         Result call( const fc::string& method,
                               const variants& args,
                               microseconds timeout = microseconds::maximum())
         {
             return async_call( method, args ).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                               const variant& a1, 
                               const variant& a2, 
                               const variant& a3,
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2, a3 ).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                               const variant& a1, 
                               const variant& a2, 
                               const variant& a3,
                               const variant& a4,
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2, a3, a4).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                               const variant& a1, 
                               const variant& a2, 
                               const variant& a3,
                               const variant& a4,
                               const variant& a5,
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2, a3, a4, a5).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                               const variant& a1, 
                               const variant& a2, 
                               const variant& a3,
                               const variant& a4,
                               const variant& a5,
                               const variant& a6,
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2, a3, a4, a5, a6).wait(timeout).as<Result>();
         }
         template<typename Result>
         Result call( const fc::string& method, 
                               const variant& a1, 
                               const variant& a2, 
                               const variant& a3,
                               const variant& a4,
                               const variant& a5,
                               const variant& a6,
                               const variant& a7,
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2, a3, a4, a5, a6, a7).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                               const variant& a1, 
                               const variant& a2, 
                               const variant& a3,
                               const variant& a4,
                               const variant& a5,
                               const variant& a6,
                               const variant& a7,
                               const variant& a8,
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2, a3, a4, a5, a6, a7, a8).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                               const variant& a1, 
                               const variant& a2, 
                               const variant& a3,
                               const variant& a4,
                               const variant& a5,
                               const variant& a6,
                               const variant& a7,
                               const variant& a8,
                               const variant& a9,
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2, a3, a4, a5, a6, a7, a8, a9).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                               const variant& a1, 
                               const variant& a2, 
                               const variant& a3,
                               const variant& a4,
                               const variant& a5,
                               const variant& a6,
                               const variant& a7,
                               const variant& a8,
                               const variant& a9,
                               const variant& a10,
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                               const variant& a1, 
                               const variant& a2, 
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2 ).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                               const variant& a1, 
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1 ).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                               variant_object a1, 
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, fc::move(a1) ).wait(timeout).as<Result>();
         }
         template<typename Result>
         Result call( const fc::string& method, 
                               mutable_variant_object a1, 
                               microseconds timeout = microseconds::maximum())
         {
            return async_call( method, variant_object( fc::move(a1) ) ).wait(timeout).as<Result>();
         }


         template<typename Result>
         Result call( const fc::string& method, microseconds timeout = microseconds::maximum() )
         {
            return async_call( method ).wait(timeout).as<Result>();
         }

         /// Sending in a variant_object will be issued as named parameters
         variant call( const fc::string& method, const variant_object& named_args );
         ///@}
         
      private:
         std::unique_ptr<detail::json_connection_impl> my;
   };
   typedef std::shared_ptr<json_connection> json_connection_ptr;

}} // fc::rpc



