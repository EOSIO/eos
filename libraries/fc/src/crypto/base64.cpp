#include <fc/crypto/base64.hpp>
#include <fc/exception/exception.hpp>
#include <ctype.h>
/* 
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

namespace fc {

static constexpr char base64_chars[] =
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 "abcdefghijklmnopqrstuvwxyz"
                 "0123456789+/";
static constexpr char base64url_chars[] =
                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 "abcdefghijklmnopqrstuvwxyz"
                 "0123456789-_";

static_assert(sizeof(base64_chars) == sizeof(base64url_chars), "base64 and base64url must have the same amount of chars");

static inline void throw_on_nonbase64(unsigned char c, const char* const b64_chars) {
  FC_ASSERT(isalnum(c) || (c == b64_chars[sizeof(base64_chars)-3]) || (c == b64_chars[sizeof(base64_chars)-2]), "encountered non-base64 character");
}

std::string base64_encode_impl(unsigned char const* bytes_to_encode, unsigned int in_len, const char* const b64_chars) {

  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += b64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += b64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
   return base64_encode_impl(bytes_to_encode, in_len, base64_chars);
}

std::string base64_encode( const std::string& enc ) {
  char const* s = enc.c_str();
  return base64_encode( (unsigned char const*)s, enc.size() );
}

std::string base64url_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
   return base64_encode_impl(bytes_to_encode, in_len, base64url_chars);
}

std::string base64url_encode( const std::string& enc ) {
  char const* s = enc.c_str();
  return base64url_encode( (unsigned char const*)s, enc.size() );
}

std::string base64_decode_impl(std::string const& encoded_string, const char* const b64_chars) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && encoded_string[in_] != '=') {
    throw_on_nonbase64(encoded_string[in_], b64_chars);
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = strchr(b64_chars, char_array_4[i]) - b64_chars;

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = strchr(b64_chars, char_array_4[j]) - b64_chars;

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

std::string base64_decode(std::string const& encoded_string) {
   return base64_decode_impl(encoded_string, base64_chars);
}

std::string base64url_decode(std::string const& encoded_string) {
   return base64_decode_impl(encoded_string, base64url_chars);
}

} // namespace fc

