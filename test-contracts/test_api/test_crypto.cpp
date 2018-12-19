/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>

#include "test_api.hpp"

#define WASM_TEST_FAIL 1

static const char test1[] = "abc";
static const unsigned char test1_ok_1[] = {
   0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81,
   0x6a, 0xba, 0x3e, 0x25, 0x71, 0x78, 0x50,
   0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d
};

static const unsigned char test1_ok_256[] = { 
  0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
  0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
  0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
  0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
};

static const unsigned char test1_ok_512[] = {
   0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 
   0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31, 
   0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2, 
   0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a, 
   0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8, 
   0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd, 
   0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e, 
   0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f
};

static const unsigned char test1_ok_ripe[] = {
   0x8e, 0xb2, 0x08, 0xf7, 0xe0, 0x5d, 0x98, 0x7a, 
   0x9b, 0x04, 0x4a, 0x8e, 0x98, 0xc6, 0xb0, 0x87, 
   0xf1, 0x5a, 0x0b, 0xfc
};

const char test2[] = "";
static const unsigned char test2_ok_1[] = {
   0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b,
   0x0d, 0x32, 0x55, 0xbf, 0xef, 0x95, 0x60,
   0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09
};

const unsigned char test2_ok_256[] = { 
  0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
  0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
  0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
  0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
};

const unsigned char test2_ok_512[] = {
   0xcf, 0x83, 0xe1, 0x35, 0x7e, 0xef, 0xb8, 0xbd, 
   0xf1, 0x54, 0x28, 0x50, 0xd6, 0x6d, 0x80, 0x07, 
   0xd6, 0x20, 0xe4, 0x05, 0x0b, 0x57, 0x15, 0xdc, 
   0x83, 0xf4, 0xa9, 0x21, 0xd3, 0x6c, 0xe9, 0xce, 
   0x47, 0xd0, 0xd1, 0x3c, 0x5d, 0x85, 0xf2, 0xb0, 
   0xff, 0x83, 0x18, 0xd2, 0x87, 0x7e, 0xec, 0x2f, 
   0x63, 0xb9, 0x31, 0xbd, 0x47, 0x41, 0x7a, 0x81, 
   0xa5, 0x38, 0x32, 0x7a, 0xf9, 0x27, 0xda, 0x3e
};

const unsigned char test2_ok_ripe[] = {
   0x9c, 0x11, 0x85, 0xa5, 0xc5, 0xe9, 0xfc, 0x54, 
   0x61, 0x28, 0x08, 0x97, 0x7e, 0xe8, 0xf5, 0x48, 
   0xb2, 0x25, 0x8d, 0x31
};

static const char test3[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
static const unsigned char test3_ok_1[] = {
   0x84, 0x98, 0x3e, 0x44, 0x1c, 0x3b, 0xd2,
   0x6e, 0xba, 0xae, 0x4a, 0xa1, 0xf9, 0x51,
   0x29, 0xe5, 0xe5, 0x46, 0x70, 0xf1
};

static const unsigned char test3_ok_256[] = { 
  0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
  0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
  0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
  0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1
};

static const unsigned char test3_ok_512[] = {
   0x20, 0x4a, 0x8f, 0xc6, 0xdd, 0xa8, 0x2f, 0x0a, 
   0x0c, 0xed, 0x7b, 0xeb, 0x8e, 0x08, 0xa4, 0x16, 
   0x57, 0xc1, 0x6e, 0xf4, 0x68, 0xb2, 0x28, 0xa8, 
   0x27, 0x9b, 0xe3, 0x31, 0xa7, 0x03, 0xc3, 0x35, 
   0x96, 0xfd, 0x15, 0xc1, 0x3b, 0x1b, 0x07, 0xf9, 
   0xaa, 0x1d, 0x3b, 0xea, 0x57, 0x78, 0x9c, 0xa0, 
   0x31, 0xad, 0x85, 0xc7, 0xa7, 0x1d, 0xd7, 0x03, 
   0x54, 0xec, 0x63, 0x12, 0x38, 0xca, 0x34, 0x45
};

static const unsigned char test3_ok_ripe[] = {
   0x12, 0xa0, 0x53, 0x38, 0x4a, 0x9c, 0x0c, 0x88, 
   0xe4, 0x05, 0xa0, 0x6c, 0x27, 0xdc, 0xf4, 0x9a, 
   0xda, 0x62, 0xeb, 0x2b
};

static const char test4[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
static const unsigned char test4_ok_1[] = {
   0xa4, 0x9b, 0x24, 0x46, 0xa0, 0x2c, 0x64,
   0x5b, 0xf4, 0x19, 0xf9, 0x95, 0xb6, 0x70,
   0x91, 0x25, 0x3a, 0x04, 0xa2, 0x59
};

static const unsigned char test4_ok_256[] = {
  0xcf, 0x5b, 0x16, 0xa7, 0x78, 0xaf, 0x83, 0x80,
  0x03, 0x6c, 0xe5, 0x9e, 0x7b, 0x04, 0x92, 0x37,
  0x0b, 0x24, 0x9b, 0x11, 0xe8, 0xf0, 0x7a, 0x51,
  0xaf, 0xac, 0x45, 0x03, 0x7a, 0xfe, 0xe9, 0xd1
};

static const unsigned char test4_ok_512[] = {
   0x8e, 0x95, 0x9b, 0x75, 0xda, 0xe3, 0x13, 0xda, 
   0x8c, 0xf4, 0xf7, 0x28, 0x14, 0xfc, 0x14, 0x3f, 
   0x8f, 0x77, 0x79, 0xc6, 0xeb, 0x9f, 0x7f, 0xa1, 
   0x72, 0x99, 0xae, 0xad, 0xb6, 0x88, 0x90, 0x18, 
   0x50, 0x1d, 0x28, 0x9e, 0x49, 0x00, 0xf7, 0xe4, 
   0x33, 0x1b, 0x99, 0xde, 0xc4, 0xb5, 0x43, 0x3a, 
   0xc7, 0xd3, 0x29, 0xee, 0xb6, 0xdd, 0x26, 0x54, 
   0x5e, 0x96, 0xe5, 0x5b, 0x87, 0x4b, 0xe9, 0x09
};

static const unsigned char test4_ok_ripe[] = {
   0x6f, 0x3f, 0xa3, 0x9b, 0x6b, 0x50, 0x3c, 0x38, 
   0x4f, 0x91, 0x9a, 0x49, 0xa7, 0xaa, 0x5c, 0x2c, 
   0x08, 0xbd, 0xfb, 0x45
};

static const char test5[] = "message digest";
static const unsigned char test5_ok_1[] = {
   0xc1, 0x22, 0x52, 0xce, 0xda, 0x8b, 0xe8,
   0x99, 0x4d, 0x5f, 0xa0, 0x29, 0x0a, 0x47,
   0x23, 0x1c, 0x1d, 0x16, 0xaa, 0xe3
};

static const unsigned char test5_ok_256[] = { 
  0xf7, 0x84, 0x6f, 0x55, 0xcf, 0x23, 0xe1, 0x4e,
  0xeb, 0xea, 0xb5, 0xb4, 0xe1, 0x55, 0x0c, 0xad,
  0x5b, 0x50, 0x9e, 0x33, 0x48, 0xfb, 0xc4, 0xef,
  0xa3, 0xa1, 0x41, 0x3d, 0x39, 0x3c, 0xb6, 0x50
};

static const unsigned char test5_ok_512[] = {
   0x10, 0x7d, 0xbf, 0x38, 0x9d, 0x9e, 0x9f, 0x71, 
   0xa3, 0xa9, 0x5f, 0x6c, 0x05, 0x5b, 0x92, 0x51, 
   0xbc, 0x52, 0x68, 0xc2, 0xbe, 0x16, 0xd6, 0xc1, 
   0x34, 0x92, 0xea, 0x45, 0xb0, 0x19, 0x9f, 0x33, 
   0x09, 0xe1, 0x64, 0x55, 0xab, 0x1e, 0x96, 0x11, 
   0x8e, 0x8a, 0x90, 0x5d, 0x55, 0x97, 0xb7, 0x20, 
   0x38, 0xdd, 0xb3, 0x72, 0xa8, 0x98, 0x26, 0x04, 
   0x6d, 0xe6, 0x66, 0x87, 0xbb, 0x42, 0x0e, 0x7c
};

static const unsigned char test5_ok_ripe[] = {
   0x5d, 0x06, 0x89, 0xef, 0x49, 0xd2, 0xfa, 0xe5, 
   0x72, 0xb8, 0x81, 0xb1, 0x23, 0xa8, 0x5f, 0xfa,
   0x21, 0x59, 0x5f, 0x36
};

extern "C" {
  uint32_t my_strlen( const char *str ) {
    uint32_t len = 0;
    while(str[len]) ++len;
    return len;
  }

  bool my_memcmp( void *s1, void *s2, uint32_t n )
  {
    unsigned char *c1 = (unsigned char *)s1;
    unsigned char *c2 = (unsigned char *)s2;
    for (uint32_t i = 0; i < n; ++i) {
      if ( c1[i] != c2[i] ) {
        return false;
      }
    }
    return true;
  }

}

struct sig_hash_key {
   capi_checksum256 hash;
   capi_public_key pk;
   capi_signature sig;
};

void test_crypto::test_recover_key_assert_true() {
   sig_hash_key sh;
   read_action_data( (char*)&sh, sizeof(sh) );
   assert_recover_key( &sh.hash, (const char*)&sh.sig, sizeof(sh.sig), (const char*)&sh.pk, sizeof(sh.pk) );
}

void test_crypto::test_recover_key_assert_false() {
   sig_hash_key sh;
   read_action_data( (char*)&sh, sizeof(sh) );
   assert_recover_key( &sh.hash, (const char*)&sh.sig, sizeof(sh.sig), (const char*)&sh.pk, sizeof(sh.pk) );
   eosio_assert( false, "should have thrown an error" );
}

void test_crypto::test_recover_key() {
   sig_hash_key sh;
   read_action_data( (char*)&sh, sizeof(sh) );
   capi_public_key pk;
   recover_key( &sh.hash, (const char*)&sh.sig, sizeof(sh.sig), pk.data, sizeof(pk) );
   for ( uint32_t i=0; i < sizeof(pk); i++ )
      if ( pk.data[i] != sh.pk.data[i] )
         eosio_assert( false, "public key does not match" );
}

void test_crypto::test_sha1() {
   eosio::checksum160 tmp;

   tmp = eosio::sha1( test1, my_strlen(test1) );
   eosio_assert( my_memcmp((void *)test1_ok_1, &tmp, sizeof(eosio::checksum160)), "sha1 test1" );

   tmp = eosio::sha1( test3, my_strlen(test3) );
   eosio_assert( my_memcmp((void *)test3_ok_1, &tmp, sizeof(eosio::checksum160)), "sha1 test3" );

   tmp = eosio::sha1( test4, my_strlen(test4) );
   eosio_assert( my_memcmp((void *)test4_ok_1, &tmp, sizeof(eosio::checksum160)), "sha1 test4" );

   tmp = eosio::sha1( test5, my_strlen(test5) );
   eosio_assert( my_memcmp((void *)test5_ok_1, &tmp, sizeof(eosio::checksum160)), "sha1 test5" );
}

void test_crypto::test_sha256() {
  eosio::checksum256 tmp;

  tmp = eosio::sha256( test1, my_strlen(test1) );
  eosio_assert( my_memcmp((void *)test1_ok_256, &tmp, sizeof(eosio::checksum256)), "sha256 test1" );

  tmp = eosio::sha256( test3, my_strlen(test3) );
  eosio_assert( my_memcmp((void *)test3_ok_256, &tmp, sizeof(eosio::checksum256)), "sha256 test3" );

  tmp = eosio::sha256( test4, my_strlen(test4) );
  eosio_assert( my_memcmp((void *)test4_ok_256, &tmp, sizeof(eosio::checksum256)), "sha256 test4" );

  tmp = eosio::sha256( test5, my_strlen(test5) );
  eosio_assert( my_memcmp((void *)test5_ok_256, &tmp, sizeof(eosio::checksum256)), "sha256 test5" );
}

void test_crypto::test_sha512() {
  eosio::checksum512 tmp;

  tmp = eosio::sha512( test1, my_strlen(test1) );
  eosio_assert( my_memcmp((void *)test1_ok_512, &tmp, sizeof(eosio::checksum512)), "sha512 test1" );

  tmp = eosio::sha512( test3, my_strlen(test3) );
  eosio_assert( my_memcmp((void *)test3_ok_512, &tmp, sizeof(eosio::checksum512)), "sha512 test3" );

  tmp = eosio::sha512( test4, my_strlen(test4) );
  eosio_assert( my_memcmp((void *)test4_ok_512, &tmp, sizeof(eosio::checksum512)), "sha512 test4" );

  tmp = eosio::sha512( test5, my_strlen(test5) );
  eosio_assert( my_memcmp((void *)test5_ok_512, &tmp, sizeof(eosio::checksum512)), "sha512 test5" );
}

void test_crypto::test_ripemd160() {
  eosio::checksum160 tmp;

  tmp = eosio::ripemd160( test1, my_strlen(test1) );
  eosio_assert( my_memcmp((void *)test1_ok_ripe, &tmp, sizeof(eosio::checksum160)), "ripemd160 test1" );

  tmp = eosio::ripemd160( test3, my_strlen(test3) );
  eosio_assert( my_memcmp((void *)test3_ok_ripe, &tmp, sizeof(eosio::checksum160)), "ripemd160 test3" );

  tmp = eosio::ripemd160( test4, my_strlen(test4) );
  eosio_assert( my_memcmp((void *)test4_ok_ripe, &tmp, sizeof(eosio::checksum160)), "ripemd160 test4" );

  tmp = eosio::ripemd160( test5, my_strlen(test5) );
  eosio_assert( my_memcmp((void *)test5_ok_ripe, &tmp, sizeof(eosio::checksum160)), "ripemd160 test5" );
}

void test_crypto::sha256_null() {
  eosio::checksum256 tmp;
  tmp = eosio::sha256( nullptr, 100);
  eosio_assert( false, "should've thrown an error" );
}

void test_crypto::sha1_no_data() {
  eosio::checksum160 tmp;

  tmp = eosio::sha1( test2, my_strlen(test2) );
  eosio_assert( my_memcmp((void *)test2_ok_1, &tmp, sizeof(eosio::checksum160)), "sha1 test2" );
}

void test_crypto::sha256_no_data() {
  eosio::checksum256 tmp;

  tmp = eosio::sha256( test2, my_strlen(test2) );
  eosio_assert( my_memcmp((void *)test2_ok_256, &tmp, sizeof(eosio::checksum256)), "sha256 test2" );
}

void test_crypto::sha512_no_data() {
  eosio::checksum512 tmp;

  tmp = eosio::sha512( test2, my_strlen(test2) );
  eosio_assert( my_memcmp((void *)test2_ok_512, &tmp, sizeof(eosio::checksum512)), "sha512 test2" );
}

void test_crypto::ripemd160_no_data() {
  eosio::checksum160 tmp;

  tmp = eosio::ripemd160( test2, my_strlen(test2) );
  eosio_assert( my_memcmp((void *)test2_ok_ripe, &tmp, sizeof(eosio::checksum160)), "ripemd160 test2" );
}


void test_crypto::assert_sha256_false() {
  eosio::checksum256 tmp;

  tmp = eosio::sha256( test1, my_strlen(test1) );
  tmp.data()[0] ^= (uint64_t)(-1);
  assert_sha256( test1, my_strlen(test1), tmp );
   
  eosio_assert( false, "should have failed" );
}

void test_crypto::assert_sha256_true() {
  eosio::checksum256 tmp;

  tmp = eosio::sha256( test1, my_strlen(test1) );
  assert_sha256( test1, my_strlen(test1), tmp );

  tmp = eosio::sha256( test3, my_strlen(test3) );
  assert_sha256( test3, my_strlen(test3), tmp );

  tmp = eosio::sha256( test4, my_strlen(test4) );
  assert_sha256( test4, my_strlen(test4), tmp );

  tmp = eosio::sha256( test5, my_strlen(test5) );
  assert_sha256( test5, my_strlen(test5), tmp );
}

void test_crypto::assert_sha1_false() {
  eosio::checksum160 tmp;

  tmp = eosio::sha1( test1, my_strlen(test1) );
  tmp.data()[0] ^= (uint64_t)(-1);
  assert_sha1( test1, my_strlen(test1), tmp );
   
  eosio_assert( false, "should have failed" );
}


void test_crypto::assert_sha1_true() {
  eosio::checksum160 tmp;

  tmp = eosio::sha1( test1, my_strlen(test1) );
  assert_sha1( test1, my_strlen(test1), tmp );

  tmp = eosio::sha1( test3, my_strlen(test3) );
  assert_sha1( test3, my_strlen(test3), tmp );

  tmp = eosio::sha1( test4, my_strlen(test4) );
  assert_sha1( test4, my_strlen(test4), tmp );

  tmp = eosio::sha1( test5, my_strlen(test5) );
  assert_sha1( test5, my_strlen(test5), tmp );
}

void test_crypto::assert_sha512_false() { 
  eosio::checksum512 tmp;

  tmp = eosio::sha512( test1, my_strlen(test1) );
  tmp.data()[0] ^= (uint64_t)(-1);
  assert_sha512( test1, my_strlen(test1), tmp );
   
  eosio_assert(false, "should have failed");
}


void test_crypto::assert_sha512_true() {
  eosio::checksum512 tmp;

  tmp = eosio::sha512( test1, my_strlen(test1) );
  assert_sha512( test1, my_strlen(test1), tmp );

  tmp = eosio::sha512( test3, my_strlen(test3) );
  assert_sha512( test3, my_strlen(test3), tmp );

  tmp = eosio::sha512( test4, my_strlen(test4) );
  assert_sha512( test4, my_strlen(test4), tmp );

  tmp = eosio::sha512( test5, my_strlen(test5) );
  assert_sha512( test5, my_strlen(test5), tmp );
}

void test_crypto::assert_ripemd160_false() {
  eosio::checksum160 tmp;

  tmp = eosio::ripemd160( test1, my_strlen(test1) );
  tmp.data()[0] ^= (uint64_t)(-1);
  assert_ripemd160( test1, my_strlen(test1), tmp );
   
  eosio_assert( false, "should have failed" );
}


void test_crypto::assert_ripemd160_true() {
  eosio::checksum160 tmp;

  tmp = eosio::ripemd160( test1, my_strlen(test1) );
  assert_ripemd160( test1, my_strlen(test1), tmp );

  tmp = eosio::ripemd160( test3, my_strlen(test3) );
  assert_ripemd160( test3, my_strlen(test3), tmp );

  tmp = eosio::ripemd160( test4, my_strlen(test4) );
  assert_ripemd160( test4, my_strlen(test4), tmp );

  tmp = eosio::ripemd160( test5, my_strlen(test5) );
  assert_ripemd160( test5, my_strlen(test5), tmp );
}
