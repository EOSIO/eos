#pragma once

#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/crypto/sha256.hpp>
#include <optional>
#include <type_traits>

/// Simple wrappers for zipkin tracing, see zipkin_appender
namespace fc {
   inline const std::string zipkin_logger_name = "zipkin";

   struct zipkin_span {
      explicit zipkin_span( std::string name, const fc::log_context& lc, uint64_t parent_id = 0 )
            : id( log_config::get_next_unique_id() )
            , parent_id( parent_id )
            , start( time_point::now() )
            , name( std::move(name) )
            , log_context( lc )
            {}

      explicit zipkin_span( uint64_t id, std::string name, const fc::log_context& lc, uint64_t parent_id = 0 )
            : id( id )
            , parent_id( parent_id )
            , start( time_point::now() )
            , name( std::move(name) )
            , log_context( lc )
            {}

      zipkin_span( const zipkin_span& ) = delete;
      zipkin_span& operator=( const zipkin_span& ) = delete;
      zipkin_span& operator=( zipkin_span&& ) = delete;

      zipkin_span( zipkin_span&& rhs ) noexcept
            : id( rhs.id )
            , parent_id( rhs.parent_id )
            , start( rhs.start )
            , name( std::move( rhs.name ) )
            , log_context( std::move( rhs.log_context ) )
            , data( std::move( rhs.data ) )
            {
               rhs.id = 0;
            }

      ~zipkin_span() {
         if( id == 0 )
            return;
         try {
            auto log = fc::logger::get( zipkin_logger_name );
            if( log.is_enabled( fc::log_level::debug ) ) {
               data["zipkin.id"] = id;
               static_assert( std::is_same<decltype(start.time_since_epoch().count()), int64_t>::value, "Expect to store int64_t");
               data["zipkin.timestamp"] = start.time_since_epoch().count();
               static_assert( std::is_same<decltype((time_point::now() - start).count()), int64_t>::value, "Expect to store int64_t");
               data["zipkin.duration"] = (time_point::now() - start).count();
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

      void add_tag( const std::string& key, const std::string& var ) {
         // zipkin tags are required to be strings
         data( key, var );
      }
      void add_tag( const std::string& key, const char* var ) {
         // zipkin tags are required to be strings
         data( key, var );
      }
      void add_tag( const std::string& key, bool v ) {
         // zipkin tags are required to be strings
         data( key, v ? "true" : "false" );
      }
      template<typename T>
      std::enable_if_t<std::is_arithmetic_v<std::remove_reference_t<T>>, void>
      add_tag( const std::string& key, T&& var ) {
         data( key, std::to_string( std::forward<T>( var ) ) );
      }
      template<typename T>
      std::enable_if_t<!std::is_arithmetic_v<std::remove_reference_t<T>>, void>
      add_tag( const std::string& key, T&& var ) {
         data( key, (std::string) var );
      }

      struct token {
         friend zipkin_span;
         friend struct zipkin_trace;
         friend struct optional_trace;
         constexpr explicit operator bool() const noexcept { return id != 0; }
      private:
         explicit token(uint64_t id)
         : id(id) {}
         uint64_t id;
      };

      token get_token() const { return token{id}; };

      static uint64_t to_id( const fc::sha256& id ) {
         // avoid 0 since id of 0 is used as a flag
         return id._hash[3] == 0 ? 1 : id._hash[3];
      }

      template<typename T>
      static uint64_t to_id( const T& id ) {
         static_assert( std::is_same_v<decltype(id.data()), const uint64_t*>, "expected uint64_t" );
         return id.data()[3];
      }

   private:
      uint64_t id;
      // zipkin traceId and parentId are same (when parent_id set) since only allowing trace with span children.
      // Not currently supporting spans with children, only trace with children spans.
      const uint64_t parent_id;
      const fc::time_point start;
      std::string name;
      fc::log_context log_context;
      fc::mutable_variant_object data;
   };

   struct zipkin_trace : public zipkin_span {
      using zipkin_span::zipkin_span;
      [[nodiscard]] std::optional<zipkin_span> create_span(std::string name, const fc::log_context& lc) const {
         return zipkin_span{ std::move(name), lc, get_token().id };
      }
      [[nodiscard]] static std::optional<zipkin_span> create_span_from_token( zipkin_span::token token, std::string name, const fc::log_context& lc) {
         return zipkin_span{ std::move(name), lc, token.id };
      }
   };

   struct optional_trace {
      std::optional<zipkin_trace> opt;
      constexpr explicit operator bool() const noexcept { return opt.has_value(); }
      [[nodiscard]] zipkin_span::token get_token() const {
         return opt ? opt->get_token() : zipkin_span::token(0);
      }
   };

} // fc

/// @param TRACE_STR const char* identifier for trace
/// @return implementation defined type RAII object that submits trace on exit of scope
#define fc_create_trace( TRACE_STR ) \
      ::fc::logger::get(::fc::zipkin_logger_name).is_enabled( ::fc::log_level::debug ) ? \
        ::fc::optional_trace{ ::std::optional<::fc::zipkin_trace>( ::std::in_place, (TRACE_STR), FC_LOG_CONTEXT( debug ) ) } \
        : ::fc::optional_trace{};

/// @param TRACE_STR const char* identifier for trace
/// @param TRACE_ID fc::sha256 id to use
/// @return implementation defined type RAII object that submits trace on exit of scope
#define fc_create_trace_with_id( TRACE_STR, TRACE_ID ) \
      ::fc::logger::get(::fc::zipkin_logger_name).is_enabled( ::fc::log_level::debug ) ? \
        ::fc::optional_trace{ ::std::optional<::fc::zipkin_trace>( ::std::in_place, ::fc::zipkin_span::to_id(TRACE_ID), (TRACE_STR), FC_LOG_CONTEXT( debug ) ) } \
        : ::fc::optional_trace{};

/// @param TRACE_VARNAME variable returned from fc_create_trace
/// @param SPAN_STR const char* indentifier
/// @return implementation defined type RAII object that submits span on exit of scope
#define fc_create_span( TRACE_VARNAME, SPAN_STR ) \
      ( (TRACE_VARNAME) && ::fc::logger::get(::fc::zipkin_logger_name).is_enabled( ::fc::log_level::debug ) ) ? \
        (TRACE_VARNAME).opt->create_span( (SPAN_STR), FC_LOG_CONTEXT( debug ) ) \
        : ::std::optional<::fc::zipkin_span>{};

/// @param TRACE_TOKEN variable returned from trace.get_token()
/// @param SPAN_STR const char* indentifier
/// @return implementation defined type RAII object that submits span on exit of scope
#define fc_create_span_from_token( TRACE_TOKEN, SPAN_STR ) \
      ( (TRACE_TOKEN) && ::fc::logger::get(::fc::zipkin_logger_name).is_enabled( ::fc::log_level::debug ) ) ? \
        ::fc::zipkin_trace::create_span_from_token( (TRACE_TOKEN), (SPAN_STR), FC_LOG_CONTEXT( debug ) ) \
        : ::std::optional<::fc::zipkin_span>{};

/// @param SPAN_VARNAME variable returned from fc_create_span
/// @param TAG_KEY_STR string key
/// @param TAG_VALUE string value
#define fc_add_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE_STR) \
  FC_MULTILINE_MACRO_BEGIN \
    if( (SPAN_VARNAME) && ::fc::logger::get(::fc::zipkin_logger_name).is_enabled( ::fc::log_level::debug ) ) \
       (SPAN_VARNAME)->add_tag( (TAG_KEY_STR), (TAG_VALUE_STR) ); \
  FC_MULTILINE_MACRO_END
