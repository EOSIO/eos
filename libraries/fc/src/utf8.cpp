#include "fc/utf8.hpp"

#include "utf8/checked.h"
#include "utf8/core.h"

#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>

namespace fc {

    inline constexpr char hex_digits[] = "0123456789abcdef";

    bool is_utf8( const std::string& str )
    {
       return utf8::is_valid( str.begin(), str.end() );
    }

   // tweaked utf8::find_invalid that also considers provided range as invalid
   // @param invalid_range, indicates additional invalid values
   // @return [iterator to found invalid char, the value found if in range of provided pair invalid_range otherwise UINT32_MAX]
   template <typename octet_iterator>
   std::pair<octet_iterator, uint32_t> find_invalid(octet_iterator start, octet_iterator end,
                                                const std::pair<uint32_t, uint32_t>& invalid_range)
   {
      FC_ASSERT( invalid_range.first <= invalid_range.second );
      octet_iterator result = start;
      uint32_t value = UINT32_MAX;
      while( result != end ) {
         octet_iterator itr = result;
         utf8::internal::utf_error err_code = utf8::internal::validate_next( result, end, value );
         if( err_code != utf8::internal::UTF8_OK )
            return {result, UINT32_MAX};
         if( value >= invalid_range.first && value <= invalid_range.second )
            return {itr, value};
      }
      return {result, UINT32_MAX};
   }


   bool is_valid_utf8( const std::string_view& str ) {
      const auto invalid_range = std::make_pair<uint32_t, uint32_t>(0x80, 0x9F);
      auto [itr, v] = find_invalid( str.begin(), str.end(), invalid_range );
      return itr == str.end();
   }

   // escape 0x80-0x9F C1 control characters
   string prune_invalid_utf8( const std::string_view& str ) {
      const auto invalid_range = std::make_pair<uint32_t, uint32_t>(0x80, 0x9F);
      auto [itr, v] = find_invalid( str.begin(), str.end(), invalid_range );
      if( itr == str.end() ) return std::string( str );

      string result;
      auto escape = [&result](uint32_t v) { // v is [0x80-0x9F]
         result += "\\u00";
         result += hex_digits[v >> 4u];
         result += hex_digits[v & 15u];
      };

      result = string( str.begin(), itr );
      if( v != UINT32_MAX ) escape(v);
      while( itr != str.end() ) {
         ++itr;
         auto start = itr;
         std::tie(itr, v) = find_invalid( start, str.end(), invalid_range );
         result += string( start, itr );
         if( v != UINT32_MAX ) escape(v);
      }
      return result;
   }

   void decodeUtf8(const std::string& input, std::wstring* storage)
   {
     FC_ASSERT(storage != nullptr);

     utf8::utf8to32(input.begin(), input.end(), std::back_inserter(*storage));
   }

   void encodeUtf8(const std::wstring& input, std::string* storage)
   {
     FC_ASSERT(storage != nullptr);

     utf8::utf32to8(input.begin(), input.end(), std::back_inserter(*storage));
   }

} ///namespace fc
