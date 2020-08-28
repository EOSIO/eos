
#include <fc/rpc/http_api.hpp>

namespace fc { namespace rpc {

http_api_connection::~http_api_connection()
{
}

http_api_connection::http_api_connection()
{
   _rpc_state.add_method( "call", [this]( const variants& args ) -> variant
   {
      // TODO: This logic is duplicated between http_api_connection and websocket_api_connection
      // it should be consolidated into one place instead of copy-pasted
      FC_ASSERT( args.size() == 3 && args[2].is_array() );
      api_id_type api_id;
      if( args[0].is_string() )
      {
         variants subargs;
         subargs.push_back( args[0] );
         variant subresult = this->receive_call( 1, "get_api_by_name", subargs );
         api_id = subresult.as_uint64();
      }
      else
         api_id = args[0].as_uint64();

      return this->receive_call(
         api_id,
         args[1].as_string(),
         args[2].get_array() );
   } );

   _rpc_state.add_method( "notice", [this]( const variants& args ) -> variant
   {
      FC_ASSERT( args.size() == 2 && args[1].is_array() );
      this->receive_notice(
         args[0].as_uint64(),
         args[1].get_array() );
      return variant();
   } );

   _rpc_state.add_method( "callback", [this]( const variants& args ) -> variant
   {
      FC_ASSERT( args.size() == 2 && args[1].is_array() );
      this->receive_callback(
         args[0].as_uint64(),
         args[1].get_array() );
      return variant();
   } );

   _rpc_state.on_unhandled( [&]( const std::string& method_name, const variants& args )
   {
      return this->receive_call( 0, method_name, args );
   } );
}

variant http_api_connection::send_call(
   api_id_type api_id,
   string method_name,
   variants args /* = variants() */ )
{
   // HTTP has no way to do this, so do nothing
   return variant();
}

variant http_api_connection::send_callback(
   uint64_t callback_id,
   variants args /* = variants() */ )
{
   // HTTP has no way to do this, so do nothing
   return variant();
}

void http_api_connection::send_notice(
   uint64_t callback_id,
   variants args /* = variants() */ )
{
   // HTTP has no way to do this, so do nothing
   return;
}

void http_api_connection::on_request( const fc::http::request& req, const fc::http::server::response& resp )
{
   // this must be called by outside HTTP server's on_request method
   std::string resp_body;
   http::reply::status_code resp_status;

   auto handle_error = [&](const auto& e)
   {
      resp_status = http::reply::InternalServerError;
      resp_body = "";
      wdump((e.to_detail_string()));
   };

   try
   {
      resp.add_header( "Content-Type", "application/json" );
      std::string req_body( req.body.begin(), req.body.end() );
      auto var = fc::json::from_string( req_body );
      const auto& var_obj = var.get_object();

      if( var_obj.contains( "method" ) )
      {
         auto call = var.as<fc::rpc::request>();
         auto handle_error_inner = [&](const auto& e)
         {
            resp_body = fc::json::to_string( fc::rpc::response( *call.id, error_object{ 1, e.to_detail_string(), fc::variant(e)} ) );
            resp_status = http::reply::InternalServerError;}
         };

         try
         {
            auto result = _rpc_state.local_call( call.method, call.params );
            resp_body = fc::json::to_string( fc::rpc::response( *call.id, result ) );
            resp_status = http::reply::OK;
         }
         catch ( const std::bad_alloc& ) 
         {
            throw;
         } 
         catch ( const boost::interprocess::bad_alloc& ) 
         {
            throw;
         } 
         catch ( const fc::exception& e )
         {
            handle_error_inner(e);
         }
         catch ( const std::exception& e )
         {
            handle_error_inner(fc::std_exception_wrapper::from_current_exception(e));
         }
      }
      else
      {
         resp_status = http::reply::BadRequest;
         resp_body = "";
      }
   }
   catch ( const std::bad_alloc& ) 
   {
     throw;
   } 
   catch ( const boost::interprocess::bad_alloc& ) 
   {
     throw;
   } 
   catch ( const fc::exception& e )
   {
      handle_error(e);
   }
   catch ( const std::exception& e )
   {
      handle_error(fc::std_exception_wrapper::from_current_exception(e));
   }

   try
   {
      resp.set_status( resp_status );
      resp.set_length( resp_body.length() );
      resp.write( resp_body.c_str(), resp_body.length() );
   }
   catch ( const std::bad_alloc& ) 
   {
     throw;
   } 
   catch ( const boost::interprocess::bad_alloc& ) 
   {
     throw;
   } 
   catch( const fc::exception& e )
   {
      wdump((e.to_detail_string()));
   }
   catch ( const std::exception& e )
   {
      wdump((fc::std_exception_wrapper::from_current_exception(e).to_detail_string()));
   }
   return;
}

} } // namespace fc::rpc
