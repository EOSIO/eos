/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <fc/exception/exception.hpp>

namespace eosio { namespace client { namespace help {
   bool print_recognized_errors(const fc::exception& e, const bool verbose_errors);
   bool print_help_text(const fc::exception& e);
}}}