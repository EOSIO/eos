/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/types.h>

extern "C" {
    /**
        *  Aborts processing of this message and unwinds all pending changes if the given string is not utf8
        *  @brief Aborts processing of this message and unwinds all pending changes
        *  @param str - string to be tested
        *  @param len - length of the string to be tested
        *  @param err_msg_cstr - a null terminated message to explain the reason for failure
    */
    void assert_is_utf8( const char* str, uint32_t len, const char* err_msg_cstr );
    
}
