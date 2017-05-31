#include <fc/rpc/state.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>

namespace fc { namespace rpc {
state::~state()
{
   close();
}

void state::add_method( const fc::string& name, method m )
{
   _methods.emplace(std::pair<std::string,method>(name,fc::move(m)));
}

void state::remove_method( const fc::string& name )
{
   _methods.erase(name);
}

variant state::local_call( const string& method_name, const variants& args )
{
   auto method_itr = _methods.find(method_name);
   if( method_itr == _methods.end() && _unhandled )
      return _unhandled( method_name, args );
   FC_ASSERT( method_itr != _methods.end(), "Unknown Method: ${name}", ("name",method_name) );
   return method_itr->second(args);
}

void  state::handle_reply( const response& response )
{
   auto await = _awaiting.find( response.id );
   FC_ASSERT( await != _awaiting.end(), "Unknown Response ID: ${id}", ("id",response.id)("response",response) );
   if( response.result ) 
      await->second->set_value( *response.result );
   else if( response.error )
   {
      await->second->set_exception( std::make_exception_ptr( FC_EXCEPTION( exception, "${error}", ("error",response.error->message)("data",response) ) ) );
   }
   else
      await->second->set_value( fc::variant() );
   _awaiting.erase(await);
}

request state::start_remote_call( const string& method_name, variants args )
{
   request request{ _next_id++, method_name, std::move(args) };
   _awaiting[*request.id].reset( new boost::fibers::promise<variant>() );
   return request;
}
variant state::wait_for_response( uint64_t request_id )
{
   auto itr = _awaiting.find(request_id);
   FC_ASSERT( itr != _awaiting.end() );
   auto fut = itr->second->get_future();
   return fut.get();
}
void state::close()
{
   for( auto& item : _awaiting )
      item.second->set_exception( std::make_exception_ptr(  FC_EXCEPTION( eof_exception, "connection closed" )) );
   _awaiting.clear();
}
void state::on_unhandled( const std::function<variant(const string&, const variants&)>& unhandled )
{
   _unhandled = unhandled;
}

} }  // namespace fc::rpc
