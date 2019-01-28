/**
 * @file
 * @copyright defined in eos/LICENSE
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/print.h>
#include "test_api.hpp"

#include <vector>

void test_checktime::checktime_pass() {
   int p = 0;
   for ( int i = 0; i < 10000; i++ )
      p += i;

   eosio::print(p);
}

void test_checktime::checktime_failure() {
   int p = 0;
   for ( unsigned long long i = 0; i < 10000000000000000000ULL; i++ )
      for ( unsigned long long j = 0; i < 10000000000000000000ULL; i++ )
         p += i+j;


   eosio::print(p);
}

constexpr size_t size = 20000000;

void test_checktime::checktime_sha1_failure() {
   char* ptr = new char[size];
   checksum160 res;
   sha1( ptr, size, &res );
}

void test_checktime::checktime_assert_sha1_failure() {
   char* ptr = new char[size];
   checksum160 res;
   assert_sha1( ptr, size, &res );
}

void test_checktime::checktime_sha256_failure() {
   char* ptr = new char[size];
   checksum256 res;
   sha256( ptr, size, &res );
}

void test_checktime::checktime_assert_sha256_failure() {
   char* ptr = new char[size];
   checksum256 res;
   assert_sha256( ptr, size, &res );
}

void test_checktime::checktime_sha512_failure() {
   char* ptr = new char[size];
   checksum512 res;
   sha512( ptr, size, &res );
}

void test_checktime::checktime_assert_sha512_failure() {
   char* ptr = new char[size];
   checksum512 res;
   assert_sha512( ptr, size, &res );
}

void test_checktime::checktime_ripemd160_failure() {
   char* ptr = new char[size];
   checksum160 res;
   ripemd160( ptr, size, &res );
}

void test_checktime::checktime_assert_ripemd160_failure() {
   char* ptr = new char[size];
   checksum160 res;
   assert_ripemd160( ptr, size, &res );
}
