/**
 + *  @file
 + *  @copyright defined in eos/LICENSE.txt
 + */
 #pragma once
 #include <eoslib/types.h>

 extern "C" {

    uint32_t uint_digit_len( uint64_t val);
    uint32_t int_digit_len( int64_t val);

    void uint_to_char_arr( uint64_t input, uint32_t digit_len, char* output);
    void int_to_char_arr( int64_t input, uint32_t digit_len, char* output);

}