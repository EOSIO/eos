#pragma once
#include <fc/variant.hpp>
#include <fc/filesystem.hpp>
#include <fc/time.hpp>
#include <fc/utility.hpp>
#include <fc/exception/exception.hpp>

#define DEFAULT_MAX_RECURSION_DEPTH 200

namespace fc
{
   using std::ostream;

   /**
    *  Provides interface for json serialization.
    *
    *  json strings are always UTF8
    */
   class json
   {
      public:
         enum class parse_type : unsigned char
         {
            legacy_parser         = 0,
            strict_parser         = 1,
            relaxed_parser        = 2,
            legacy_parser_with_string_doubles = 3
         };
         enum class output_formatting : unsigned char
         {
            stringify_large_ints_and_doubles = 0,
            legacy_generator = 1
         };
         using yield_function_t = fc::optional_delegate<void(std::ostream&)>;
         static constexpr uint64_t max_length_limit = std::numeric_limits<uint64_t>::max();
         static constexpr size_t escape_string_yeild_check_count = 128;
         static ostream& to_stream( ostream& out, const fc::string&, const yield_function_t& yield );
         static ostream& to_stream( ostream& out, const variant& v, const yield_function_t& yield, const output_formatting format = output_formatting::stringify_large_ints_and_doubles );
         static ostream& to_stream( ostream& out, const variants& v, const yield_function_t& yield, const output_formatting format = output_formatting::stringify_large_ints_and_doubles );
         static ostream& to_stream( ostream& out, const variant_object& v, const yield_function_t& yield, const output_formatting format = output_formatting::stringify_large_ints_and_doubles );
         static ostream& to_stream( ostream& out, const variant& v, const fc::time_point& deadline, const output_formatting format = output_formatting::stringify_large_ints_and_doubles, const uint64_t max_len = max_length_limit );

         static variant  from_string( const string& utf8_str, const parse_type ptype = parse_type::legacy_parser, uint32_t max_depth = DEFAULT_MAX_RECURSION_DEPTH );
         static variants variants_from_string( const string& utf8_str, const parse_type ptype = parse_type::legacy_parser, uint32_t max_depth = DEFAULT_MAX_RECURSION_DEPTH );
         static string   to_string( const variant& v, const yield_function_t& yield, const output_formatting format = output_formatting::stringify_large_ints_and_doubles);
         static string   to_pretty_string( const variant& v, const yield_function_t& yield, const output_formatting format = output_formatting::stringify_large_ints_and_doubles );

         static bool     is_valid( const std::string& json_str, const parse_type ptype = parse_type::legacy_parser, const uint32_t max_depth = DEFAULT_MAX_RECURSION_DEPTH );

         template<typename T>
         static bool     save_to_file( const T& v, const fc::path& fi, const bool pretty = true, const output_formatting format = output_formatting::stringify_large_ints_and_doubles )
         {
            return save_to_file( variant(v), fi, pretty, format );
         }

         static bool     save_to_file( const variant& v, const fc::path& fi, const bool pretty = true, const output_formatting format = output_formatting::stringify_large_ints_and_doubles );
         static variant  from_file( const fc::path& p, const parse_type ptype = parse_type::legacy_parser, const uint32_t max_depth = DEFAULT_MAX_RECURSION_DEPTH );

         template<typename T>
         static T from_file( const fc::path& p, const parse_type ptype = parse_type::legacy_parser, const uint32_t max_depth = DEFAULT_MAX_RECURSION_DEPTH )
         {
            return json::from_file(p, ptype, max_depth).as<T>();
         }

         template<typename T>
         static string   to_string( const T& v, const fc::time_point& deadline, const output_formatting format = output_formatting::stringify_large_ints_and_doubles, const uint64_t max_len = max_length_limit )
         {
            const auto yield = [&](std::ostream& os) {
               FC_CHECK_DEADLINE(deadline);
               FC_ASSERT(os.tellp() <= max_len);
            };
            return to_string( variant(v), yield, format );
         }

         template<typename T>
         static string   to_pretty_string( const T& v, const fc::time_point& deadline = fc::time_point::maximum(), const output_formatting format = output_formatting::stringify_large_ints_and_doubles, const uint64_t max_len = max_length_limit )
         {
            const auto yield = [&](std::ostream& os) {
               FC_CHECK_DEADLINE(deadline);
               FC_ASSERT( os.tellp() <= max_len );
            };
            return to_pretty_string( variant(v), yield, format );
         }

         template<typename T>
         static bool save_to_file( const T& v, const std::string& p, const bool pretty = true, const output_formatting format = output_formatting::stringify_large_ints_and_doubles )
         {
            return save_to_file( variant(v), fc::path(p), pretty, format );
         }
   };

   void escape_string( const string& str, std::ostream& os, const json::yield_function_t& yield );
} // fc

#undef DEFAULT_MAX_RECURSION_DEPTH
