/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/types.h>

extern "C" {
    uint32_t is_valid_utf8_str( const char* str, uint32_t len);
}
