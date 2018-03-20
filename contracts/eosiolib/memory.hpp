/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/memory.h>
#include <eosiolib/print.hpp>

#include <unistd.h>

#ifndef EOSIO_NATIVE_CONTRACT_COMPILATION
extern "C" {

void* malloc(size_t size);

void* calloc(size_t count, size_t size);

void* realloc(void* ptr, size_t size);

void free(void* ptr);

}
#endif

