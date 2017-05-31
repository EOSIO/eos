#include <fc/rpc/bstate.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>

namespace fc { namespace rpc {
bstate::~bstate()
{
   close();
}

void bstate::add_method( const fc::string& name, method m )
{
   _methods.emplace(std::pair<std::string,method>(name,fc::move(m)));
}

void bstate::remove_method( const fc::string& name )
{
   _methods.erase(name);
}

result_type bstate::local_call( const string& method_name, const params_type& args )
{
   auto method_itr = _methods.find(method_name);
   if( method_itr == _methods.end() && _unhandled )
      return _unhandled( method_name, args );
   FC_ASSERT( method_itr != _methods.end(), "Unknown Method: ${name}", ("name",method_name) );
   return method_itr->second(args);
}

void  bstate::handle_reply( const bresponse& bresponse )
{
   auto await = _awaiting.find( bresponse.id );
   FC_ASSERT( await != _awaiting.end(), "Unknown Response ID: ${id}", ("id",bresponse.id)("bresponse",bresponse) );
   if( bresponse.result ) 
      await->second->set_value( *bresponse.result );
   else if( bresponse.error )
   {
      await->second->set_exception( std::make_exception_ptr( FC_EXCEPTION( exception, "${error}", ("error",bresponse.error->message)("data",bresponse) ) ) );
   }
   else
      await->second->set_value( params_type() );
   _awaiting.erase(await);
}

brequest bstate::start_remote_call( const string& method_name, params_type args )
{
   brequest brequest{ _next_id++, method_name, std::move(args) };
   _awaiting[*brequest.id].reset( new boost::fibers::promise<result_type>() );
   return brequest;
}
result_type bstate::wait_for_response( uint64_t request_id )
{
   auto itr = _awaiting.find(request_id);
   FC_ASSERT( itr != _awaiting.end() );
   auto fut = itr->second->get_future();
   return fut.get();
}
void bstate::close()
{
   for( auto& item : _awaiting )
      item.second->set_exception( std::make_exception_ptr(  FC_EXCEPTION( eof_exception, "connection closed" )) );
   _awaiting.clear();
}
void bstate::on_unhandled( const std::function<result_type(const string&, const params_type&)>& unhandled )
{
   _unhandled = unhandled;
}

} }  // namespace fc::rpc
