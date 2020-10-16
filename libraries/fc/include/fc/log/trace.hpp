#pragma once

#include <fc/scoped_exit.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <optional>

/// Simple wrappers for gelf-zipkin tracing, see gelf_zipkin_appender
namespace fc {
   inline std::string gelf_zipkin_logger_name = "gelf_zipkin";

   struct zipkin_span {
      explicit zipkin_span( std::string name, const fc::log_context& lc, uint64_t parent_id = 0 )
            : id( log_config::get_next_unique_id() )
            , parent_id( parent_id )
            , name( std::move(name) )
            , log_context( lc )
            {}

      ~zipkin_span() {
         try {
            auto log = fc::logger::get( gelf_zipkin_logger_name );
            if( log.is_enabled( fc::log_level::debug ) ) {
               data["zipkin.id"] = id;
               data["zipkin.name"] = std::move(name);
               if( parent_id != 0 ) {
                  data["zipkin.traceId"] = parent_id;
                  data["zipkin.parentId"] = parent_id;
               } else {
                  data["zipkin.traceId"] = id;
               }
               log.log( fc::log_message( log_context, "", std::move( data ) ) );
            }
         } catch( ... ) {}
      }

      template<typename T>
      void add_tag( const std::string& key, T&& var ) {
         data( key, std::forward<T>( var ) );
      }

      const uint64_t id;
      // zipkin traceId and parentId are same since only allowing trace with span children
      const uint64_t parent_id;
      std::string name;
      fc::log_context log_context;
      fc::mutable_variant_object data;
   };

   struct zipkin_trace : public zipkin_span {
      using zipkin_span::zipkin_span;
      std::optional<zipkin_span> create_span(std::string name, const fc::log_context& lc) {
         return zipkin_span{ std::move(name), lc, id };
      }
   };

} // fc

/// @param TRACE_STR const char* identifier for trace
/// @return implementation defined type RAII object that submits trace on exit of scope
#define fc_create_trace( TRACE_STR ) \
      ::fc::logger::get(::fc::gelf_zipkin_logger_name).is_enabled( ::fc::log_level::debug ) ? \
        ::std::optional<::fc::zipkin_trace>( ::std::in_place, (TRACE_STR), FC_LOG_CONTEXT( debug ) ) \
        : ::std::optional<::fc::zipkin_trace>{};

/// @param TRACE_VARNAME variable returned from fc_create_trace
/// @param SPAN_STR const char* indentifier
/// @return implementation defined type RAII object that submits span on exit of scope
#define fc_create_span( TRACE_VARNAME, SPAN_STR ) \
      ( (TRACE_VARNAME) && ::fc::logger::get(::fc::gelf_zipkin_logger_name).is_enabled( ::fc::log_level::debug ) ) ? \
        (TRACE_VARNAME)->create_span( (SPAN_STR), FC_LOG_CONTEXT( debug ) ) \
        : ::std::optional<::fc::zipkin_span>{};

/// @param SPAN_VARNAME variable returned from fc_create_span
/// @param TAG_KEY_STR const char* key
/// @param TAG_VALUE any fc::variant type
#define fc_add_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE) \
  FC_MULTILINE_MACRO_BEGIN \
    if( (SPAN_VARNAME) && ::fc::logger::get(::fc::gelf_zipkin_logger_name).is_enabled( ::fc::log_level::debug ) ) \
       (SPAN_VARNAME)->add_tag( (TAG_KEY_STR), (TAG_VALUE) ); \
  FC_MULTILINE_MACRO_END

/// @param SPAN_VARNAME variable returned from fc_create_span
/// @param TAG_KEY_STR const char* key
/// @param TAG_VALUE_STR std::string value
#define fc_add_str_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE_STR) \
  fc_add_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE_STR);
