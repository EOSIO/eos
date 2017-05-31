#include "fc/utf8.hpp"

#include "utf8/checked.h"
#include "utf8/core.h"
#include "utf8/unchecked.h"
#include <websocketpp/utf8_validator.hpp>

#include <assert.h>
#include <fc/log/logger.hpp>
#include <iostream>

namespace fc {

   bool is_utf8( const std::string& str )
   {
      auto itr = utf8::find_invalid(str.begin(), str.end()); 
      return utf8::is_valid( str.begin(), str.end() );
   }

   string prune_invalid_utf8( const string& str ) {
      string result;

      auto itr = utf8::find_invalid(str.begin(), str.end()); 
      if( itr == str.end() ) return str;

      result = string( str.begin(), itr );
      while( itr != str.end() ) {
         ++itr;
         auto start = itr;
         itr = utf8::find_invalid( start, str.end()); 
         result += string( start, itr );
      }
      return result;
   }

   void decodeUtf8(const std::string& input, std::wstring* storage)
   {
     assert(storage != nullptr);

     utf8::utf8to32(input.begin(), input.end(), std::back_inserter(*storage));
   }

   void encodeUtf8(const std::wstring& input, std::string* storage)
   {
     assert(storage != nullptr);

     utf8::utf32to8(input.begin(), input.end(), std::back_inserter(*storage));
   }

} ///namespace fc


