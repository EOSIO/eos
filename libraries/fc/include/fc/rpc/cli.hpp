#pragma once
#include <fc/io/stdio.hpp>
#include <fc/io/json.hpp>
#include <fc/io/buffered_iostream.hpp>
#include <fc/io/sstream.hpp>
#include <fc/rpc/api_connection.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>

namespace fc { namespace rpc {

   /**
    *  Provides a simple wrapper for RPC calls to a given interface.
    */
   class cli : public api_connection
   {
      public:
         ~cli();

         virtual variant send_call( api_id_type api_id, string method_name, variants args = variants() );
         virtual variant send_callback( uint64_t callback_id, variants args = variants() );
         virtual void    send_notice( uint64_t callback_id, variants args = variants() );

         void start();
         void stop();
         void wait();
         void format_result( const string& method, std::function<string(variant,const variants&)> formatter);

         virtual void getline( const fc::string& prompt, fc::string& line );

         void set_prompt( const string& prompt );

      private:
         void run();

         std::string _prompt = ">>>";
         std::map<string,std::function<string(variant,const variants&)> > _result_formatters;
         fc::future<void> _run_complete;
   };
} } 
