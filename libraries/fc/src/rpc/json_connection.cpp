#include <fc/rpc/json_connection.hpp>
#include <fc/io/json.hpp>
#include <boost/unordered_map.hpp>
#include <fc/thread/thread.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/log/logger.hpp>
#include <string>

namespace fc { namespace rpc {

   namespace detail
   {
      class json_connection_impl 
      {
         public:
            json_connection_impl( fc::buffered_istream_ptr&& in, fc::buffered_ostream_ptr&& out )
            :_in(fc::move(in)),_out(fc::move(out)),_eof(false),_next_id(0),_logger("json_connection"){}

            fc::buffered_istream_ptr                                              _in;
            fc::buffered_ostream_ptr                                              _out;

            fc::future<void>                                                      _done;
            fc::future<void>                                                      _handle_message_future;
            bool                                                                  _eof;

            uint64_t                                                              _next_id;
            boost::unordered_map<uint64_t, fc::promise<variant>::ptr>             _awaiting;
            boost::unordered_map<std::string, json_connection::method>            _methods;
            boost::unordered_map<std::string, json_connection::named_param_method> _named_param_methods;

            fc::mutex                                                             _write_mutex;
            std::function<void(fc::exception_ptr)>                                _on_close;

            logger                                                                _logger;

            void send_result( variant id, variant result )
            {
               ilog( "send: {\"id\": ${i}, \"result\": ${r}}", ("i",id)("r",result) );
               {
                 fc::scoped_lock<fc::mutex> lock(_write_mutex);
                 *_out << "{\"id\":";
                 json::to_stream( *_out, id  );
                 *_out << ",\"result\":";
                 json::to_stream( *_out, result);
                 *_out << "}\n";
                 _out->flush();
               }
            }
            void send_error( variant id, fc::exception& e )
            {
               ilog( "send: {\"id\": ${i}, \"error\":{\"message\": ${what},\"code\":0,\"data\":${data}}}",
                     ("i",id)("what",e.what())("data", e) );
               {
                 fc::scoped_lock<fc::mutex> lock(_write_mutex);
                 *_out << "{\"id\":";
                 json::to_stream( *_out, id  );
                 *_out << ",\"error\":{\"message\":";
                 json::to_stream( *_out, fc::string(e.what()) );
                 *_out <<",\"code\":0,\"data\":";
                 json::to_stream( *_out, variant(e));
                 *_out << "}}\n";
                 _out->flush();
               }
               //wlog(  "exception: ${except}", ("except", variant(e)) );
            }

            void handle_message( const variant_object& obj )
            {
              wlog(  "recv: ${msg}", ("msg", obj) );
               fc::exception_ptr eptr;
               try 
               {
                  auto m = obj.find("method");
                  auto i = obj.find("id");
                  if( m != obj.end() )
                  {
                     fc::exception except;
                     bool exception_caught = false;
                     try
                     {
                        auto p = obj.find("params");
                        variant result;
                        if( p == obj.end() )
                        {
                           auto pmi = _methods.find(m->value().as_string());
                           auto nmi = _named_param_methods.find(m->value().as_string());
                           if( pmi != _methods.end()  )
                           {
                               result = pmi->second( variants() );
                           }
                           else if( nmi != _named_param_methods.end() )
                           {
                               result = nmi->second( variant_object() );
                           }
                           else // invalid method
                           {
                              FC_THROW_EXCEPTION( exception, "Invalid Method '${method}'", ("method",m->value().as_string()));
                           }
                        }
                        else if( p->value().is_array() )
                        {
                           auto pmi = _methods.find(m->value().as_string());
                           if( pmi != _methods.end()  )
                           {
                               result = pmi->second( p->value().get_array() );
                           }
                           else // invalid method / param combo
                           {
                              FC_THROW_EXCEPTION( exception, "Invalid method or params  '${method}'", 
                                                  ("method",m->value().as_string()));
                           }
                        
                        }
                        else if( p->value().is_object() )
                        {
                           auto nmi = _named_param_methods.find(m->value().as_string());
                           if( nmi != _named_param_methods.end() )
                           {
                               result = nmi->second( p->value().get_object() );
                           }
                           else // invalid method / param combo?
                           {
                              FC_THROW_EXCEPTION( exception, "Invalid method or params  '${method}'", 
                                                  ("method",m->value().as_string()));
                           }
                        }
                        else // invalid params
                        {
                            FC_THROW_EXCEPTION( exception, "Invalid Params for method ${method}", 
                                                    ("method",m->value().as_string()));
                        }
                        if( i != obj.end() )
                        {
                           send_result( i->value(), result );
                        }
                     }
                     catch ( fc::exception& e )
                     {
                        exception_caught = true;
                        except = e;
                     }
                     if( exception_caught && i != obj.end() )
                        send_error( i->value(), except );
                     else
                        fc_wlog( _logger, "json rpc exception: ${exception}", ("exception",except) );
                  }
                  else if( i != obj.end() ) //handle any received JSON response
                  {
                     uint64_t id = i->value().as_int64();
                     auto await = _awaiting.find(id);
                     if( await != _awaiting.end() )
                     {
                        auto r = obj.find("result");
                        auto e = obj.find("error");
                        if( r != obj.end() ) //if regular result response
                        {
                           await->second->set_value( r->value() ); 
                        }
                        else if( e != obj.end() ) //if error response
                        {
                          fc::exception_ptr eptr;
                          try
                          {
                             auto err = e->value().get_object();
                             auto data = err.find( "data" );
                             if( data != err.end() )
                             {
                                //wlog(  "exception: ${except}", ("except", data->value() ) );
                                await->second->set_exception( data->value().as<exception>().dynamic_copy_exception() );  
                             }
                             else
                                await->second->set_exception( exception_ptr(new FC_EXCEPTION( exception, "${error}", ("error",e->value()) ) ) );
                          } 
                          catch ( fc::exception& e )
                          {
                            elog( "Error parsing exception: ${e}", ("e", e.to_detail_string() ) );
                            eptr = e.dynamic_copy_exception();
                          }
                          if( eptr ) await->second->set_exception( eptr );
                        }
                        else // id found without error, result, nor method field
                        {
                           fc_wlog( _logger, "no error or result specified in '${message}'", ("message",obj) );
                        }
                     }
                  }
                  else // no method nor request id... invalid message
                  {
                    
                  }
               } 
               catch ( fc::exception& e ) // catch all other errors...
               {
                  fc_elog( _logger, "json rpc exception: ${exception}", ("exception",e ));
                  elog( "json rpc exception: ${exception}", ("exception",e ));
                  eptr = e.dynamic_copy_exception();
               }
               if( eptr ) { close(eptr); }
            }

            void read_loop()
            {
               fc::exception_ptr eptr;
               try 
               {
                  fc::string line;
                  while( !_done.canceled() )
                  {
                      variant v = json::from_stream(*_in);
                      ///ilog( "input: ${in}", ("in", v ) );
                      //wlog(  "recv: ${line}", ("line", line) );
                      _handle_message_future = fc::async([=](){ handle_message(v.get_object()); }, "json_connection handle_message");
                  } 
               } 
               catch ( eof_exception& eof ) 
               { 
                  _eof = true; 
                  eptr = eof.dynamic_copy_exception();
               }
               catch ( exception& e )
               {
                  eptr = e.dynamic_copy_exception();
               }
               catch ( ... )
               {
                  eptr = fc::exception_ptr(new FC_EXCEPTION( unhandled_exception, "json connection read error" ));
               }
               if( eptr ) close( eptr );
            }

            void close( fc::exception_ptr e )
            {
               wlog( "close ${reason}", ("reason", e->to_detail_string() ) );
               if( _on_close )
                  _on_close(e);
               for( auto itr = _awaiting.begin(); itr != _awaiting.end(); ++itr )
               {
                  itr->second->set_exception( e->dynamic_copy_exception() );
               }
            }
      };
   }//namespace detail

   json_connection::json_connection( fc::buffered_istream_ptr in, fc::buffered_ostream_ptr out )
   :my( new detail::json_connection_impl(fc::move(in),fc::move(out)) )
   {}

   json_connection::~json_connection()
   {
      close();
   }

   fc::future<void> json_connection::exec()
   {
      if( my->_done.valid() )
      {
         FC_THROW_EXCEPTION( assert_exception, "start should only be called once" );
      }
      return my->_done = fc::async( [=](){ my->read_loop(); }, "json_connection read_loop" );
   }

   void json_connection::close()
   {
      try
      {
         if( my->_handle_message_future.valid() && !my->_handle_message_future.ready() )
            my->_handle_message_future.cancel_and_wait(__FUNCTION__);
         if( my->_done.valid() && !my->_done.ready() )
         {
            my->_done.cancel("json_connection is destructing");
            my->_out->close();
            my->_done.wait();
         }
      }
      catch ( fc::canceled_exception& ){} // expected exception
      catch ( fc::eof_exception& ){} // expected exception
      catch ( fc::exception& e )
      {
         // unhandled, unexpected exception cannot throw from destructor, so log it.
         wlog( "${exception}", ("exception",e.to_detail_string()) );
      }
   }

   void json_connection::set_on_disconnected_callback(std::function<void (exception_ptr)> callback)
   {
      my->_on_close = callback;
   }

   void json_connection::add_method( const fc::string& name, method m )
   {
      ilog( "add method ${name}", ("name",name) );
      my->_methods.emplace(std::pair<std::string,method>(name,fc::move(m)));
   }
   void json_connection::add_named_param_method( const fc::string& name, named_param_method m )
   {
      ilog( "add named param method ${name}", ("name",name) );
      my->_named_param_methods.emplace(std::pair<std::string,named_param_method>(name,fc::move(m)));
   }
   void json_connection::remove_method( const fc::string& name )
   {
      my->_methods.erase(name);
      my->_named_param_methods.erase(name);
   }
   void json_connection::notice( const fc::string& method, const variants& args )
   {
      fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
      *my->_out << "{\"method\":";
      json::to_stream( *my->_out, method );
      if( args.size() )
      {
         *my->_out << ",\"params\":";
         fc::json::to_stream( *my->_out, args );
         *my->_out << "}\n";
      }
      else
      {
         *my->_out << ",\"params\":[]}\n";
      }
   }
   void json_connection::notice( const fc::string& method, const variant_object& named_args )
   {
      {
        fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
        *my->_out << "{\"method\":";
        json::to_stream( *my->_out, method );
        *my->_out << ",\"params\":";
        fc::json::to_stream( *my->_out, named_args );
        *my->_out << "}\n";
        my->_out->flush();
      }
   }
   void json_connection::notice( const fc::string& method )
   {
      {
        fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
        *my->_out << "{\"method\":";
        json::to_stream( *my->_out, method );
        *my->_out << "}\n";
        my->_out->flush();
      }
   }


   future<variant> json_connection::async_call( const fc::string& method, const variants& args )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         if( args.size() )
         {
            *my->_out << ",\"params\":";
            fc::json::to_stream( *my->_out, args );
            *my->_out << "}\n";
         }
         else
         {
            *my->_out << ",\"params\":[]}\n";
         }
         my->_out->flush();
      }
      return my->_awaiting[id];
   }

   future<variant> json_connection::async_call( const fc::string& method, const variant& a1 )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":[";
         fc::json::to_stream( *my->_out, a1 );
         *my->_out << "]}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }
   future<variant> json_connection::async_call( const fc::string& method, const variant& a1, const variant& a2 )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":[";
         fc::json::to_stream( *my->_out, a1 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a2 );
         *my->_out << "]}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }
   future<variant> json_connection::async_call( const fc::string& method, const variant& a1, const variant& a2, const variant& a3 )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":[";
         fc::json::to_stream( *my->_out, a1 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a2 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a3 );
         *my->_out << "]}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }

   future<variant> json_connection::async_call( const fc::string& method, const variant& a1, const variant& a2, const variant& a3, const variant& a4 )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":[";
         fc::json::to_stream( *my->_out, a1 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a2 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a3 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a4 );
         *my->_out << "]}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }

   future<variant> json_connection::async_call( const fc::string& method, const variant& a1, const variant& a2, const variant& a3, const variant& a4, const variant& a5 )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":[";
         fc::json::to_stream( *my->_out, a1 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a2 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a3 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a4 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a5 );
         *my->_out << "]}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }

   future<variant> json_connection::async_call( const fc::string& method, const variant& a1, const variant& a2, const variant& a3, const variant& a4, const variant& a5, const variant& a6 )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":[";
         fc::json::to_stream( *my->_out, a1 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a2 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a3 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a4 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a5 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a6 );
         *my->_out << "]}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }
   future<variant> json_connection::async_call( const fc::string& method, const variant& a1, const variant& a2, const variant& a3, const variant& a4, const variant& a5, const variant& a6, const variant& a7 )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":[";
         fc::json::to_stream( *my->_out, a1 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a2 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a3 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a4 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a5 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a6 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a7 );
         *my->_out << "]}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }
   future<variant> json_connection::async_call( const fc::string& method, 
                                                const variant& a1, 
                                                const variant& a2, 
                                                const variant& a3, 
                                                const variant& a4, 
                                                const variant& a5, 
                                                const variant& a6, 
                                                const variant& a7, 
                                                const variant& a8 )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":[";
         fc::json::to_stream( *my->_out, a1 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a2 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a3 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a4 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a5 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a6 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a7 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a8 );
         *my->_out << "]}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }
   future<variant> json_connection::async_call( const fc::string& method, 
                                                const variant& a1, 
                                                const variant& a2, 
                                                const variant& a3, 
                                                const variant& a4, 
                                                const variant& a5, 
                                                const variant& a6, 
                                                const variant& a7, 
                                                const variant& a8, 
                                                const variant& a9 )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":[";
         fc::json::to_stream( *my->_out, a1 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a2 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a3 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a4 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a5 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a6 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a7 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a8 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a9 );
         *my->_out << "]}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }
   future<variant> json_connection::async_call( const fc::string& method, 
                                                const variant& a1, 
                                                const variant& a2, 
                                                const variant& a3, 
                                                const variant& a4, 
                                                const variant& a5, 
                                                const variant& a6, 
                                                const variant& a7, 
                                                const variant& a8, 
                                                const variant& a9, 
                                                const variant& a10 )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );

      {
         fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":[";
         fc::json::to_stream( *my->_out, a1 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a2 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a3 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a4 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a5 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a6 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a7 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a8 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a9 );
         *my->_out << ",";
         fc::json::to_stream( *my->_out, a10 );
         *my->_out << "]}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }

   future<variant> json_connection::async_call( const fc::string& method, mutable_variant_object named_args )
   {
        return async_call( method, variant_object( fc::move(named_args) ) );
   }
   future<variant> json_connection::async_call( const fc::string& method, const variant_object& named_args )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );
      fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
      {
         *my->_out << "{\"id\":";
         *my->_out << id;
         *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
         *my->_out << ",\"params\":";
         fc::json::to_stream( *my->_out, named_args );
         *my->_out << "}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }
   future<variant> json_connection::async_call( const fc::string& method )
   {
      auto id = my->_next_id++;
      my->_awaiting[id] = fc::promise<variant>::ptr( new fc::promise<variant>("json_connection::async_call") );
      fc::scoped_lock<fc::mutex> lock(my->_write_mutex);
      {
        *my->_out << "{\"id\":";
        *my->_out << id;
        *my->_out << ",\"method\":";
         json::to_stream( *my->_out, method );
        *my->_out << "}\n";
         my->_out->flush();
      }
      return my->_awaiting[id];
   }

   logger json_connection::get_logger()const
   {
      return my->_logger;
   }

   void   json_connection::set_logger( const logger& l )
   {
      my->_logger = l;
   }

}}
