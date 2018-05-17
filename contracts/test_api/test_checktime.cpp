/**
 * @file
 * @copyright defined in enumivo/LICENSE.txt
 */

#include <enumivolib/enumivo.hpp>
#include "test_api.hpp"

void test_checktime::checktime_pass() {
   int p = 0;
   for ( int i = 0; i < 10000; i++ )
      p += i;

   enumivo::print(p);
}

void test_checktime::checktime_failure() {
   int p = 0;
   for ( unsigned long long i = 0; i < 10000000000000000000ULL; i++ )
      for ( unsigned long long j = 0; i < 10000000000000000000ULL; i++ )
         p += i+j;


   enumivo::print(p);
}
