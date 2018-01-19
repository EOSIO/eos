/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

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
}
