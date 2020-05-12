
#include <fc/rpc/websocket_api.hpp>

namespace fc { namespace rpc {

websocket_api_connection::~websocket_api_connection()
{
}

websocket_api_connection::websocket_api_connection( fc::http::websocket_connection& c )
   : _connection(c)
{
   _rpc_state.add_method( "call", [this]( const variants& args ) -> variant
   {
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
      this->receive_notice( args[0].as_uint64(), args[1].get_array() );
      return variant();
   } );

   _rpc_state.add_method( "callback", [this]( const variants& args ) -> variant
   {
      FC_ASSERT( args.size() == 2 && args[1].is_array() );
      this->receive_callback( args[0].as_uint64(), args[1].get_array() );
      return variant();
   } );

   _rpc_state.on_unhandled( [&]( const std::string& method_name, const variants& args )
   {
      return this->receive_call( 0, method_name, args );
   } );

   _connection.on_message_handler( [&]( const std::string& msg ){ on_message(msg,true); } );
   _connection.on_http_handler( [&]( const std::string& msg ){ return on_message(msg,false); } );
   _connection.closed.connect( [this](){ closed(); } );
}

variant websocket_api_connection::send_call(
   api_id_type api_id,
   string method_name,
   variants args /* = variants() */ )
{
   auto request = _rpc_state.start_remote_call(  "call", {api_id, std::move(method_name), std::move(args) } );
   _connection.send_message( fc::json::to_string(request) );
   return _rpc_state.wait_for_response( *request.id );
}

variant websocket_api_connection::send_callback(
   uint64_t callback_id,
   variants args /* = variants() */ )
{
   auto request = _rpc_state.start_remote_call( "callback", {callback_id, std::move(args) } );
   _connection.send_message( fc::json::to_string(request) );
   return _rpc_state.wait_for_response( *request.id );
}

void websocket_api_connection::send_notice(
   uint64_t callback_id,
   variants args /* = variants() */ )
{
   fc::rpc::request req{ optional<uint64_t>(), "notice", {callback_id, std::move(args)}};
   _connection.send_message( fc::json::to_string(req) );
}

std::string websocket_api_connection::on_message(
   const std::string& message,
   bool send_message /* = true */ )
{
   wdump((message));
   try
   {
      auto var = fc::json::from_string(message);
      const auto& var_obj = var.get_object();
      if( var_obj.contains( "method" ) )
      {
         auto call = var.as<fc::rpc::request>();
         exception_ptr optexcept;
         try
         {
            auto result = _rpc_state.local_call( call.method, call.params );
            if( call.id )
            {
               auto reply = fc::json::to_string( response( *call.id, result ) );
               if( send_message )
                  _connection.send_message( reply );
               return reply;
            }
         }
         catch ( const fc::exception& e )
         {
            if( call.id )
            {
               optexcept = e.dynamic_copy_exception();
            }
         }
         if( optexcept ) {

               auto reply = fc::json::to_string( response( *call.id,  error_object{ 1, optexcept->to_detail_string(), fc::variant(*optexcept)}  ) );
               if( send_message )
                  _connection.send_message( reply );

               return reply;
         }
      }
      else
      {
         auto reply = var.as<fc::rpc::response>();
         _rpc_state.handle_reply( reply );
      }
   }
   catch ( const fc::exception& e )
   {
      wdump((e.to_detail_string()));
      return e.to_detail_string();
   }
   return string();
}

} } // namespace fc::rpc
