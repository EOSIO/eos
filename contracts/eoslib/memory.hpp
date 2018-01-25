/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eoslib/memory.h>
#include <eoslib/print.hpp>

#include <unistd.h>

extern "C" {

void* malloc(size_t size);

void* realloc(void* ptr, size_t size);

void free(void* ptr);

}
