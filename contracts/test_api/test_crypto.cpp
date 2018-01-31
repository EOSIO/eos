/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eos.hpp>
#include <eosiolib/crypto.h>

#include "test_api.hpp"

static const char test1[] = "abc";
static const unsigned char test1_ok[] = { 
  0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
  0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
  0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
  0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
};

const char test2[] = "";
const unsigned char test2_ok[] = { 
  0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
  0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
  0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
  0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
};

static const char test3[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
static const unsigned char test3_ok[] = { 
  0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
  0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
  0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
  0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1
};

static const char test4[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
static const unsigned char test4_ok[] = {
  0xcf, 0x5b, 0x16, 0xa7, 0x78, 0xaf, 0x83, 0x80,
  0x03, 0x6c, 0xe5, 0x9e, 0x7b, 0x04, 0x92, 0x37,
  0x0b, 0x24, 0x9b, 0x11, 0xe8, 0xf0, 0x7a, 0x51,
  0xaf, 0xac, 0x45, 0x03, 0x7a, 0xfe, 0xe9, 0xd1
};

static const char test5[] = "message digest";
static const unsigned char test5_ok[] = { 
  0xf7, 0x84, 0x6f, 0x55, 0xcf, 0x23, 0xe1, 0x4e,
  0xeb, 0xea, 0xb5, 0xb4, 0xe1, 0x55, 0x0c, 0xad,
  0x5b, 0x50, 0x9e, 0x33, 0x48, 0xfb, 0xc4, 0xef,
  0xa3, 0xa1, 0x41, 0x3d, 0x39, 0x3c, 0xb6, 0x50
};

extern "C" {
  uint32_t my_strlen(const char *str) {
    uint32_t len = 0;
    while(str[len]) ++len;
    return len;
  }

  bool my_memcmp(void *s1, void *s2, uint32_t n)
  {
    unsigned char *c1 = (unsigned char *)s1;
    unsigned char *c2 = (unsigned char *)s2;
    for (uint32_t i = 0; i < n; ++i) {
      if (c1[i] != c2[i]) {
        return false;
      }
    }
    return true;
  }

}

unsigned int test_crypto::test_sha256() {

  checksum tmp;

  sha256( (char *)test1, my_strlen(test1), &tmp );
  WASM_ASSERT( my_memcmp((void *)test1_ok, &tmp, sizeof(checksum)), "sha256 test1" );

  sha256( (char *)test3, my_strlen(test3), &tmp );
  WASM_ASSERT( my_memcmp((void *)test3_ok, &tmp, sizeof(checksum)), "sha256 test3" );

  sha256( (char *)test4, my_strlen(test4), &tmp );
  WASM_ASSERT( my_memcmp((void *)test4_ok, &tmp, sizeof(checksum)), "sha256 test4" );

  sha256( (char *)test5, my_strlen(test5), &tmp );
  WASM_ASSERT( my_memcmp((void *)test5_ok, &tmp, sizeof(checksum)), "sha256 test5" );

  return WASM_TEST_PASS;
}

unsigned int test_crypto::sha256_no_data() {

  checksum tmp;

  sha256( (char *)test2, my_strlen(test2), &tmp );
  WASM_ASSERT( my_memcmp((void *)test2_ok, &tmp, sizeof(checksum)), "sha256 test2" );

  return WASM_TEST_PASS;
}

unsigned int test_crypto::asert_sha256_false() {
  
  checksum tmp;

  sha256( (char *)test1, my_strlen(test1), &tmp );
  tmp.hash[0] ^= (uint64_t)(-1);
  assert_sha256( (char *)test1, my_strlen(test1), &tmp);
  
  return WASM_TEST_FAIL;
}

unsigned int test_crypto::asert_sha256_true() {
  
  checksum tmp;

  sha256( (char *)test1, my_strlen(test1), &tmp );
  assert_sha256( (char *)test1, my_strlen(test1), &tmp);

  sha256( (char *)test3, my_strlen(test3), &tmp );
  assert_sha256( (char *)test3, my_strlen(test3), &tmp);

  sha256( (char *)test4, my_strlen(test4), &tmp );
  assert_sha256( (char *)test4, my_strlen(test4), &tmp);

  sha256( (char *)test5, my_strlen(test5), &tmp );
  assert_sha256( (char *)test5, my_strlen(test5), &tmp);

  return WASM_TEST_PASS;
}

unsigned int test_crypto::asert_no_data() {
  
  checksum *tmp = (checksum*)test2_ok;
  assert_sha256( (char *)test2, my_strlen(test2), tmp);

  return WASM_TEST_FAIL;
}