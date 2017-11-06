/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <libintl.h>
#include <fc/variant.hpp>

namespace eosio { namespace client { namespace localize {
   #if !defined(_)
   #define _(str) str
   #endif

   #define localized(str, ...) localized_with_variant((str), fc::mutable_variant_object() __VA_ARGS__ )

   inline auto localized_with_variant( const char* raw_fmt, const fc::variant_object& args) {
      return fc::format_string(::gettext(raw_fmt), args);
   }
}}}