#pragma once
#include <fc/io/buffered_iostream.hpp>
#include <fc/variant_object.hpp>
#include <fc/thread/future.hpp>
#include <fc/log/logger.hpp>
#include <functional>

namespace fc { namespace rpc  {

   namespace detail { class variant_connection_impl; }

   /**
    * @brief Implements JSON-RPC 2.0 over a set of io streams
    *
    * Each JSON RPC message is expected to be on its own line, violators
    * will be prosecuted to the fullest extent of the law.
    */
   class variant_connection
   {
      public:
         typedef std::function<variant(const variants&)>       method;
         typedef std::function<variant(const variant_object&)> named_param_method;

         variant_connection( fc::variant_stream::ptr in, fc::variant_stream::ptr out );
         ~variant_connection();

         /**
          *   Starts processing messages from input
          */
         future<void> exec();

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
         future<fc::variant> async_call( const fc::string& method, 
                                         const variant_object& args );

         future<fc::variant> async_call( const fc::string& method, mutable_variant_object args );

         /// Sending in an array of variants will be handled as positional arguments
         future<fc::variant> async_call( const fc::string& method, 
                                         const variants& args );

         future<fc::variant> async_call( const fc::string& method );

         future<fc::variant> async_call( const fc::string& method, 
                                         const fc::variant& a1 );

         future<fc::variant> async_call( const fc::string& method, 
                                         const fc::variant& a1, 
                                         const fc::variant& a2 );

         future<fc::variant> async_call( const fc::string& method, 
                                         const fc::variant& a1, 
                                         const fc::variant& a2, 
                                         const fc::variant& a3 );

         template<typename Result>
         Result call( const fc::string& method, 
                      const fc::variant& a1, 
                      const fc::variant& a2, 
                      const fc::variant& a3,
                      microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2, a3 ).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                      const fc::variant& a1, 
                      const fc::variant& a2, 
                      microseconds timeout = microseconds::maximum())
         {
            return async_call( method, a1, a2 ).wait(timeout).as<Result>();
         }

         template<typename Result>
         Result call( const fc::string& method, 
                      const fc::variant& a1, 
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
         std::unique_ptr<detail::variant_connection_impl> my;
   };
   typedef std::shared_ptr<variant_connection> variant_connection_ptr;

}} // fc::rpc



