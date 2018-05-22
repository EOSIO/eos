#include <fc/exception/exception.hpp>
#include <boost/exception/all.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/json.hpp>

#include <iostream>

namespace fc
{
   FC_REGISTER_EXCEPTIONS( (timeout_exception)
                           (file_not_found_exception)
                           (parse_error_exception)
                           (invalid_arg_exception)
                           (invalid_operation_exception)
                           (key_not_found_exception)
                           (bad_cast_exception)
                           (out_of_range_exception)
                           (canceled_exception)
                           (assert_exception)
                           (eof_exception)
                           (unknown_host_exception)
                           (null_optional)
                           (udt_exception)
                           (aes_exception)
                           (overflow_exception)
                           (underflow_exception)
                           (divide_by_zero_exception)
                         )

   namespace detail
   {
      class exception_impl
      {
         public:
            std::string     _name;
            std::string     _what;
            int64_t         _code;
            log_messages    _elog;
      };
   }
   exception::exception( log_messages&& msgs, int64_t code,
                                    const std::string& name_value,
                                    const std::string& what_value )
   :my( new detail::exception_impl() )
   {
      my->_code = code;
      my->_what = what_value;
      my->_name = name_value;
      my->_elog = fc::move(msgs);
   }

   exception::exception(
      const log_messages& msgs,
      int64_t code,
      const std::string& name_value,
      const std::string& what_value )
   :my( new detail::exception_impl() )
   {
      my->_code = code;
      my->_what = what_value;
      my->_name = name_value;
      my->_elog = msgs;
   }

   unhandled_exception::unhandled_exception( log_message&& m, std::exception_ptr e )
   :exception( fc::move(m) )
   {
      _inner = e;
   }
   unhandled_exception::unhandled_exception( const exception& r )
   :exception(r)
   {
   }
   unhandled_exception::unhandled_exception( log_messages m )
   :exception()
   { my->_elog = fc::move(m); }

   std::exception_ptr unhandled_exception::get_inner_exception()const { return _inner; }

   NO_RETURN void     unhandled_exception::dynamic_rethrow_exception()const
   {
      if( !(_inner == std::exception_ptr()) ) std::rethrow_exception( _inner );
      else { fc::exception::dynamic_rethrow_exception(); }
   }

   std::shared_ptr<exception> unhandled_exception::dynamic_copy_exception()const
   {
      auto e = std::make_shared<unhandled_exception>( *this );
      e->_inner = _inner;
      return e;
   }

   exception::exception( int64_t code,
                         const std::string& name_value,
                         const std::string& what_value )
   :my( new detail::exception_impl() )
   {
      my->_code = code;
      my->_what = what_value;
      my->_name = name_value;
   }

   exception::exception( log_message&& msg,
                         int64_t code,
                         const std::string& name_value,
                         const std::string& what_value )
   :my( new detail::exception_impl() )
   {
      my->_code = code;
      my->_what = what_value;
      my->_name = name_value;
      my->_elog.push_back( fc::move( msg ) );
   }
   exception::exception( const exception& c )
   :my( new detail::exception_impl(*c.my) )
   { }
   exception::exception( exception&& c )
   :my( fc::move(c.my) ){}

   const char*  exception::name()const throw() { return my->_name.c_str(); }
   const char*  exception::what()const throw() { return my->_what.c_str(); }
   int64_t      exception::code()const throw() { return my->_code;         }

   exception::~exception(){}

   void to_variant( const exception& e, variant& v )
   {
      v = mutable_variant_object( "code", e.code() )
                                ( "name", e.name() )
                                ( "message", e.what() )
                                ( "stack", e.get_log() );

   }
   void          from_variant( const variant& v, exception& ll )
   {
      auto obj = v.get_object();
      if( obj.contains( "stack" ) )
         ll.my->_elog =  obj["stack"].as<log_messages>();
      if( obj.contains( "code" ) )
         ll.my->_code = obj["code"].as_int64();
      if( obj.contains( "name" ) )
         ll.my->_name = obj["name"].as_string();
      if( obj.contains( "message" ) )
         ll.my->_what = obj["message"].as_string();
   }

   const log_messages&   exception::get_log()const { return my->_elog; }
   void                  exception::append_log( log_message m )
   {
      my->_elog.emplace_back( fc::move(m) );
   }

   /**
    *   Generates a detailed string including file, line, method,
    *   and other information that is generally only useful for
    *   developers.
    */
   string exception::to_detail_string( log_level ll  )const
   {
      std::stringstream ss;
      try {
         try {
            ss << variant( my->_code ).as_string();
         } catch( std::bad_alloc& ) {
            throw;
         } catch( ... ) {
            ss << "<- exception in to_detail_string.";
         }
         ss << " " << my->_name << ": " << my->_what << "\n";
         for( auto itr = my->_elog.begin(); itr != my->_elog.end(); ) {
            try {
               ss << itr->get_message() << "\n"; //fc::format_string( itr->get_format(), itr->get_data() ) <<"\n";
               ss << "    " << json::to_string( itr->get_data()) << "\n";
               ss << "    " << itr->get_context().to_string();
               ++itr;
            } catch( std::bad_alloc& ) {
               throw;
            } catch( ... ) {
               ss << "<- exception in to_detail_string.";
            }
            if( itr != my->_elog.end()) ss << "\n";
         }
      } catch( std::bad_alloc& ) {
         throw;
      } catch( ... ) {
         ss << "<- exception in to_detail_string.\n";
      }
      return ss.str();
   }

   /**
    *   Generates a user-friendly error report.
    */
   string exception::to_string( log_level ll   )const
   {
      std::stringstream ss;
      try {
         ss << my->_what;
         try {
            ss << " (" << variant( my->_code ).as_string() << ")\n";
         } catch( std::bad_alloc& ) {
            throw;
         } catch( ... ) {
            ss << "<- exception in to_string.\n";
         }
         for( auto itr = my->_elog.begin(); itr != my->_elog.end(); ++itr ) {
            try {
               ss << fc::format_string( itr->get_format(), itr->get_data()) << "\n";
               //      ss << "    " << itr->get_context().to_string() <<"\n";
            } catch( std::bad_alloc& ) {
               throw;
            } catch( ... ) {
               ss << "<- exception in to_string.\n";
            }
         }
         return ss.str();
      } catch( std::bad_alloc& ) {
         throw;
      } catch( ... ) {
         ss << "<- exception in to_string.\n";
      }
   }

   /**
    *   Generates a user-friendly error report.
    */
   string exception::top_message( )const
   {
      for( auto itr = my->_elog.begin(); itr != my->_elog.end(); ++itr )
      {
         auto s = fc::format_string( itr->get_format(), itr->get_data() );
         if (!s.empty()) {
            return s;
         }
      }
      return string();
   }

   void NO_RETURN exception_factory::rethrow( const exception& e )const
   {
      auto itr = _registered_exceptions.find( e.code() );
      if( itr != _registered_exceptions.end() )
         itr->second->rethrow( e );
      throw e;
   }
   /**
    * Rethrows the exception restoring the proper type based upon
    * the error code.  This is used to propagate exception types
    * across conversions to/from JSON
    */
   NO_RETURN void  exception::dynamic_rethrow_exception()const
   {
      exception_factory::instance().rethrow( *this );
   }

   exception_ptr exception::dynamic_copy_exception()const
   {
       return std::make_shared<exception>(*this);
   }

   fc::string except_str()
   {
       return boost::current_exception_diagnostic_information();
   }

   void throw_bad_enum_cast( int64_t i, const char* e )
   {
      FC_THROW_EXCEPTION( bad_cast_exception,
                          "invalid index '${key}' in enum '${enum}'",
                          ("key",i)("enum",e) );
   }
   void throw_bad_enum_cast( const char* k, const char* e )
   {
      FC_THROW_EXCEPTION( bad_cast_exception,
                          "invalid name '${key}' in enum '${enum}'",
                          ("key",k)("enum",e) );
   }

   bool assert_optional(bool is_valid )
   {
      if( !is_valid )
         throw null_optional();
      return true;
   }
   exception& exception::operator=( const exception& copy )
   {
      *my = *copy.my;
      return *this;
   }

   exception& exception::operator=( exception&& copy )
   {
      my = std::move(copy.my);
      return *this;
   }

   void record_assert_trip(
      const char* filename,
      uint32_t lineno,
      const char* expr
      )
   {
      fc::mutable_variant_object assert_trip_info =
         fc::mutable_variant_object()
         ("source_file", filename)
         ("source_lineno", lineno)
         ("expr", expr)
         ;
      /* TODO: restore this later
      std::cout
         << "FC_ASSERT triggered:  "
         << fc::json::to_string( assert_trip_info ) << "\n";
         */
      return;
   }

   bool enable_record_assert_trip = false;

} // fc
