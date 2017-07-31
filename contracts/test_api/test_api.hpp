#pragma once

#define WASM_TEST_ERROR_CODE *((unsigned int *)((1<<16) - 2*sizeof(unsigned int)))
#define WASM_TEST_ERROR_MESSAGE *((unsigned int *)((1<<16) - 1*sizeof(unsigned int)))

#define WASM_TEST_FAIL 0xDEADBEEF
#define WASM_TEST_PASS 0xB0CABACA

#define WASM_ASSERT(m, message) if(!(m)) { WASM_TEST_ERROR_MESSAGE=(unsigned int)message; return WASM_TEST_FAIL; }

typedef unsigned long long u64;
#define WASM_TEST_HANDLER(CLASS, METHOD) \
  if( u32(action>>32) == DJBH(#CLASS) && u32(action) == DJBH(#METHOD) ) { \
     WASM_TEST_ERROR_CODE = CLASS::METHOD(); \
     return; \
  }

#define WASM_TEST_HANDLER_EX(CLASS, METHOD) \
  if( u32(action>>32) == DJBH(#CLASS) && u32(action) == DJBH(#METHOD) ) { \
     WASM_TEST_ERROR_CODE = CLASS::METHOD(code, action); \
     return; \
  }

typedef unsigned int u32;
static constexpr u32 DJBH(const char* cp)
{
  u32 hash = 5381;
  while (*cp)
      hash = 33 * hash ^ (unsigned char) *cp++;
  return hash;
}

#pragma pack(push, 1)
struct dummy_message {
  char a; //1
  unsigned long long b; //8
  int  c; //4
};

struct u128_msg {
  unsigned __int128  values[3]; //16*3
};
#pragma pack(pop)

static_assert( sizeof(dummy_message) == 13 , "unexpected packing" );
static_assert( sizeof(u128_msg) == 16*3 , "unexpected packing" );

struct test_types {
  static unsigned int types_size();
  static unsigned int char_to_symbol();
  static unsigned int string_to_name();
  static unsigned int name_class();
};

struct test_print {
  static unsigned int test_prints();
  static unsigned int test_printi();
  static unsigned int test_printi128();
  static unsigned int test_printn();
};

#define DUMMY_MESSAGE_DEFAULT_A 0x45
#define DUMMY_MESSAGE_DEFAULT_B 0xab11cd1244556677
#define DUMMY_MESSAGE_DEFAULT_C 0x7451ae12

struct test_message {

  static unsigned int read_message();
  static unsigned int read_message_to_0();
  static unsigned int read_message_to_64k();
  static unsigned int require_notice(unsigned long long code, unsigned long long action);
  static unsigned int require_auth();
  static unsigned int assert_false();
  static unsigned int assert_true();
  static unsigned int now();

};

struct test_math {
  static unsigned int test_multeq_i128();
  static unsigned int test_diveq_i128();
  static unsigned int test_diveq_i128_by_0();
};

struct test_db {
   static unsigned int key_i64_general();
   static unsigned int key_i64_remove_all();
   static unsigned int key_i64_small_load();
   static unsigned int key_i64_small_store();
   static unsigned int key_i64_store_scope();
   static unsigned int key_i64_remove_scope();
   static unsigned int key_i64_not_found();

   static unsigned int key_i128i128_general();
};

