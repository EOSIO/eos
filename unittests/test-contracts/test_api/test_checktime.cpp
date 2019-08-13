#include <vector>

#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/print.h>

#include "test_api.hpp"

void test_checktime::checktime_pass() {
   int p = 0;
   for ( int i = 0; i < 10000; i++ )
      p += i;

   eosio::print(p);
}


void test_checktime::checktime_failure() {
   volatile unsigned long long bound{}; // `volatile' necessary to prevent loop optimization
   read_action_data( (char*)&bound, sizeof(bound) );

   int p = 0;
   for ( unsigned long long i = 0; i < bound; i++ )
      for ( unsigned long long j = 0; j < bound; j++ )
         p += i+j+bound;

   eosio::print(p);
}

constexpr size_t size = 20000000;

void test_checktime::checktime_sha1_failure() {
   char* ptr = new char[size];
   capi_checksum160 res;
   sha1( ptr, size, &res );
}

void test_checktime::checktime_assert_sha1_failure() {
   char* ptr = new char[size];
   capi_checksum160 res;
   assert_sha1( ptr, size, &res );
}

void test_checktime::checktime_sha256_failure() {
   char* ptr = new char[size];
   capi_checksum256 res;
   sha256( ptr, size, &res );
}

void test_checktime::checktime_assert_sha256_failure() {
   char* ptr = new char[size];
   capi_checksum256 res;
   assert_sha256( ptr, size, &res );
}

void test_checktime::checktime_sha512_failure() {
   char* ptr = new char[size];
   capi_checksum512 res;
   sha512( ptr, size, &res );
}

void test_checktime::checktime_assert_sha512_failure() {
   char* ptr = new char[size];
   capi_checksum512 res;
   assert_sha512( ptr, size, &res );
}

void test_checktime::checktime_ripemd160_failure() {
   char* ptr = new char[size];
   capi_checksum160 res;
   ripemd160( ptr, size, &res );
}

void test_checktime::checktime_assert_ripemd160_failure() {
   char* ptr = new char[size];
   capi_checksum160 res;
   assert_ripemd160( ptr, size, &res );
}
