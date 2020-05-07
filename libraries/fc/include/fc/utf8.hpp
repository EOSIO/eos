#ifndef __UTF8_HPP
#define __UTF8_HPP

#include <string>

/// This file contains general purpose utilities related to UTF-8 <-> Unicode conversions

namespace fc
{

   std::string prune_invalid_utf8( const std::string& str );

   bool is_utf8( const std::string& str );

   /** Decodes utf 8 std::string into unicode string.
       @param input   - input string to be decoded and stored in 'storage'
       @param storage - buffer for converted text. Cannot be nullptr.
   */
   void decodeUtf8(const std::string& input, std::wstring* storage);
     
   /** Encodes given wide (unicode) string into UTF-8 representation.
       @param input - input string to be encoded and stored in 'storage'
       @param storage - buffer for converted text. Cannot be nullptr.
   */
   void encodeUtf8(const std::wstring& input, std::string* storage);

} /// namespace fc

#endif ///__UTF8_HPP

