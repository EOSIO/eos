#pragma once

#include <fc/log/logger.hpp>
#include <string>
#include <type_traits>

namespace fc {
/// Active Object that sends zipkin messages in JSON format
/// https://zipkin.io/zipkin-api/
///
/// Span contains following data
///     uint64_t    traceId   - unique id for trace, all children spans shared same id
///     std::string name      - logical operation, should have low cardinality
///     uint64_t    parentId  - The parent span id, or absent if root span
///     uint64_t    id        - unique id for this span
///     int64_t     timestamp - epoch microseconds of start of span
///     int64_t     duration  - microseconds of span
///
/// Enable zipkin by calling zipkin_config::init() from main thread on startup.
/// Use macros defined in trace.hpp.

class zipkin;

class sha256;

class zipkin_config {
public:
   /// Thread safe only if init() called from main thread before spawning of any threads
   static bool is_enabled() { return get_zipkin_() != nullptr; }

   /// Not thread safe, call from main thread before spawning any threads that might use zipkin.
   /// @param url the url endpoint of zipkin server. e.g. http://127.0.0.1:9411/api/v2/spans
   /// @param service_name the service name to include in each zipkin span
   /// @param timeout_us the timeout in microseconds for each http call (9 consecutive failures and zipkin is disabled)
   static void init( const std::string& url, const std::string& service_name, uint32_t timeout_us );

   /// Thread safe only if init() called from main thread before spawning of any threads
   /// @throw assert_exception if called before init()
   static zipkin& get_zipkin();

   /// Thread safe only if init() called from main thread before spawning of any threads
   /// @throw assert_exception if called before init()
   static void shutdown();

   /// Starts with a random id and increments on each call, will not return 0
   static uint64_t get_next_unique_id();

private:
   /// Provide access to initialized zipkin endpoint
   /// Thread safe as long as init() called correctly
   /// @return nullptr if init() not called
   static zipkin* get_zipkin_() { return get().zip.get(); };

   static zipkin_config& get();

private:
   std::unique_ptr<zipkin> zip;
};

struct zipkin_span {
   explicit zipkin_span( std::string name, uint64_t parent_id = 0 )
         : data( std::move( name ), parent_id ) {}

   explicit zipkin_span( uint64_t id, std::string name, uint64_t parent_id = 0 )
         : data( id, std::move( name ), parent_id ) {}

   zipkin_span( const zipkin_span& ) = delete;
   zipkin_span& operator=( const zipkin_span& ) = delete;
   zipkin_span& operator=( zipkin_span&& ) = delete;

   zipkin_span( zipkin_span&& rhs ) noexcept
         : data( std::move( rhs.data ) ) {
      rhs.data.id = 0;
   }

   ~zipkin_span();

   void add_tag( const std::string& key, const std::string& var ) {
      // zipkin tags are required to be strings
      data.tags( key, var );
   }

   void add_tag( const std::string& key, const char* var ) {
      // zipkin tags are required to be strings
      data.tags( key, var );
   }

   void add_tag( const std::string& key, bool v ) {
      // zipkin tags are required to be strings
      data.tags( key, v ? "true" : "false" );
   }

   template<typename T>
   std::enable_if_t<std::is_arithmetic_v<std::remove_reference_t<T>>, void>
   add_tag( const std::string& key, T&& var ) {
      data.tags( key, std::to_string( std::forward<T>( var ) ) );
   }

   template<typename T>
   std::enable_if_t<!std::is_arithmetic_v<std::remove_reference_t<T>>, void>
   add_tag( const std::string& key, T&& var ) {
      data.tags( key, (std::string) var );
   }

   struct token {
      friend zipkin_span;
      friend struct zipkin_trace;
      friend struct optional_trace;
      constexpr explicit operator bool() const noexcept { return id != 0; }
   private:
      explicit token( uint64_t id )
            : id( id ) {}
      uint64_t id;
   };

   token get_token() const { return token{data.id}; };

   static uint64_t to_id( const fc::sha256& id );

   template<typename T>
   static uint64_t to_id( const T& id ) {
      static_assert( std::is_same_v<decltype( id.data() ), const uint64_t*>, "expected uint64_t" );
      return id.data()[3];
   }

   struct span_data {
      explicit span_data( std::string name, uint64_t parent_id = 0 )
            : id( zipkin_config::get_next_unique_id() ), parent_id( parent_id ),
              start( time_point::now() ), name( std::move( name ) ) {}

      explicit span_data( uint64_t id, std::string name, uint64_t parent_id = 0 )
            : id( id ), parent_id( parent_id ), start( time_point::now() ), name( std::move( name ) ) {}

      span_data( const span_data& ) = delete;
      span_data& operator=( const span_data& ) = delete;
      span_data& operator=( span_data&& ) = delete;
      span_data( span_data&& rhs ) = default;

      uint64_t id;
      // zipkin traceId and parentId are same (when parent_id set) since only allowing trace with span children.
      // Not currently supporting spans with children, only trace with children spans.
      const uint64_t parent_id;
      const fc::time_point start;
      fc::time_point stop;
      std::string name;
      fc::mutable_variant_object tags;
   };

   span_data data;
};

struct zipkin_trace : public zipkin_span {
   using zipkin_span::zipkin_span;

   [[nodiscard]] std::optional<zipkin_span> create_span( std::string name ) const {
      return zipkin_span{std::move( name ), get_token().id};
   }

   [[nodiscard]] static std::optional<zipkin_span>
   create_span_from_token( zipkin_span::token token, std::string name ) {
      return zipkin_span{std::move( name ), token.id};
   }
};

struct optional_trace {
   std::optional<zipkin_trace> opt;

   constexpr explicit operator bool() const noexcept { return opt.has_value(); }

   [[nodiscard]] zipkin_span::token get_token() const {
      return opt ? opt->get_token() : zipkin_span::token( 0 );
   }
};

class zipkin {
public:
   zipkin( const std::string& url, const std::string& service_name, uint32_t timeout_us );

   /// finishes logging all queued up spans
   ~zipkin() = default;

   /// Starts with a random id and increments on each call, will not return 0
   uint64_t get_next_unique_id();

   // finish logging all queued up spans, not restartable
   void shutdown();

   // Logs zipkin json via http on separate thread
   void log( zipkin_span::span_data&& span );

private:
   class impl;
   std::unique_ptr<impl> my;
};

} // namespace fc

