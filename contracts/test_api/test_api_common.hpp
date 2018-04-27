/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#ifdef DISABLE_EOSLIB_SERIALIZE
#include <eosiolib/serialize.hpp>
#undef EOSLIB_SERIALIZE
#define EOSLIB_SERIALIZE(...)
#endif

static constexpr unsigned int DJBH(const char* cp)
{
  unsigned int hash = 5381;
  while (*cp)
      hash = 33 * hash ^ (unsigned char) *cp++;
  return hash;
}

static constexpr unsigned long long WASM_TEST_ACTION(const char* cls, const char* method)
{
  return static_cast<unsigned long long>(DJBH(cls)) << 32 | static_cast<unsigned long long>(DJBH(method));
}

#pragma pack(push, 1)
struct dummy_action {
   static uint64_t get_name() {
      return N(dummy_action);
   }
   static uint64_t get_account() {
      return N(testapi);
   }

  char a; //1
  uint64_t b; //8
  int32_t  c; //4

  EOSLIB_SERIALIZE( dummy_action, (a)(b)(c) )
};

struct u128_action {
  unsigned __int128  values[3]; //16*3

  EOSLIB_SERIALIZE( u128_action, (values) )
};

struct cf_action {
   static uint64_t get_name() {
      return N(cf_action);
   }
   static uint64_t get_account() {
      return N(testapi);
   }

   uint32_t       payload = 100;
   uint32_t       cfd_idx = 0; // context free data index

   EOSLIB_SERIALIZE( cf_action, (payload)(cfd_idx) )
};
#pragma pack(pop)

static_assert( sizeof(dummy_action) == 13 , "unexpected packing" );
static_assert( sizeof(u128_action) == 16*3 , "unexpected packing" );

#define DUMMY_ACTION_DEFAULT_A 0x45
#define DUMMY_ACTION_DEFAULT_B 0xab11cd1244556677
#define DUMMY_ACTION_DEFAULT_C 0x7451ae12

struct invalid_access_action {
   uint64_t code;
   uint64_t val;
   uint32_t index;
   bool store;

   EOSLIB_SERIALIZE( invalid_access_action, (code)(val)(index)(store) )
};
