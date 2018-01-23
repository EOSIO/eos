/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eoslib/memory.hpp>

typedef struct {
   long quot; /* Quotient.  */
   long rem;  /* Remainder. */
} ldiv_t;

typedef struct {
   long long quot; /* Quotient.  */
   long long rem;  /* Remainder. */
} lldiv_t;
   
extern "C" {

   void abort();

   long labs(long i);

   long long llabs(long long i);

   ldiv_t ldiv(long numer, long denom);

   lldiv_t lldiv(long long numer, long long denom);

   int posix_memalign(void** memptr, size_t alignment, size_t size);

}

inline void* malloc(size_t size)
{
   return eosio::malloc(size);
}

inline void* realloc(void* ptr, size_t size)
{
   return eosio::realloc(ptr, size);
}

inline void free(void* ptr)
{
   return eosio::free(ptr);
}
