#include <eoslib/types.hpp>
#include <eoslib/message.hpp>
#include <eoslib/db.h>

#include "test_api.hpp"

#pragma pack(push, 1)
struct TestModel
{
   AccountName   name;
   unsigned char age;
   uint64_t      phone;
};

struct TestModelV2 : TestModel
{
  TestModelV2() : new_field(0) {}

  uint64_t new_field;
};
#pragma pack(pop)



extern "C" {
  void memset(void *vptr, unsigned char val, unsigned int size) {
    char *ptr = (char *)vptr;
    while(size--) { *(ptr++)=val; }
  }
}

unsigned int test_db::key_i64_general() {
  uint32_t res = 0;

  TestModel alice{ N(alice), 20, 4234622};
  TestModel bob  { N(bob),   15, 11932435};
  TestModel carol{ N(carol), 30, 545342453};
  TestModel dave { N(dave),  46, 6535354};

  res = store_i64(currentCode(),  N(test_table), &dave,  sizeof(TestModel));
  WASM_ASSERT(res != 0, "store dave" );

  res = store_i64(currentCode(), N(test_table), &carol, sizeof(TestModel));
  WASM_ASSERT(res != 0, "store carol" );

  res = store_i64(currentCode(),   N(test_table), &bob,   sizeof(TestModel));
  WASM_ASSERT(res != 0, "store bob" );

  res = store_i64(currentCode(), N(test_table), &alice, sizeof(TestModel));
  WASM_ASSERT(res != 0, "store alice" );
  
  memset(&alice, 0, sizeof(TestModel));

  WASM_ASSERT(alice.name == 0 && alice.age == 0 && alice.phone == 0, "memset");

  alice.name = N(alice);

  res = load_i64(currentCode(), currentCode(), N(test_table), &alice, sizeof(TestModel));
  WASM_ASSERT(res == sizeof(TestModel) && alice.age == 20 && alice.phone == 4234622, "alice error 1");

  alice.age = 21;
  alice.phone = 1234;
  
  res = store_i64(currentCode(), N(test_table), &alice, sizeof(TestModel));
  WASM_ASSERT(res == 0, "store alice 2" );

  memset(&alice, 0, sizeof(TestModel));
  alice.name = N(alice);
  
  res = load_i64(currentCode(), currentCode(), N(test_table), &alice, sizeof(TestModel));
  WASM_ASSERT(res == sizeof(TestModel) && alice.age == 21 && alice.phone == 1234, "alice error 2");

  memset(&bob, 0, sizeof(TestModel));
  bob.name = N(bob);

  memset(&carol, 0, sizeof(TestModel));
  carol.name = N(carol);

  memset(&dave, 0, sizeof(TestModel));
  dave.name = N(dave);

  res = load_i64(currentCode(), currentCode(), N(test_table), &bob, sizeof(TestModel));
  WASM_ASSERT(res == sizeof(TestModel) && bob.age == 15 && bob.phone == 11932435, "bob error 1");

  res = load_i64(currentCode(), currentCode(), N(test_table), &carol, sizeof(TestModel));
  WASM_ASSERT(res == sizeof(TestModel) && carol.age == 30 && carol.phone == 545342453, "carol error 1");

  res = load_i64(currentCode(), currentCode(), N(test_table), &dave, sizeof(TestModel));
  WASM_ASSERT(res == sizeof(TestModel) && dave.age == 46 && dave.phone == 6535354, "dave error 1");

  res = load_i64(currentCode(), N(other_code), N(test_table), &alice, sizeof(TestModel));
  WASM_ASSERT(res == -1, "other_code");

  res = load_i64(currentCode(), currentCode(), N(other_table), &alice, sizeof(TestModel));
  WASM_ASSERT(res == -1, "other_table");


  TestModelV2 alicev2;
  alicev2.name = N(alice);

  res = load_i64(currentCode(), currentCode(), N(test_table), &alicev2, sizeof(TestModelV2));
  WASM_ASSERT(res == sizeof(TestModel) && alicev2.age == 21 && alicev2.phone == 1234, "alicev2 load");

  alicev2.new_field = 66655444;
  res = store_i64(currentCode(), N(test_table), &alicev2, sizeof(TestModelV2));
  WASM_ASSERT(res == 0, "store alice 3" );

  memset(&alicev2, 0, sizeof(TestModelV2));
  alicev2.name = N(alice);

  res = load_i64(currentCode(), currentCode(), N(test_table), &alicev2, sizeof(TestModelV2));
  WASM_ASSERT(res == sizeof(TestModelV2) && alicev2.age == 21 && alicev2.phone == 1234 && alicev2.new_field == 66655444, "alice model v2");

  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_remove_all() {
  
  uint32_t res = 0;

  res = remove_i64(currentCode(), N(test_table), N(alice));
  WASM_ASSERT(res == 1, "remove alice");
  res = remove_i64(currentCode(),   N(test_table), N(bob));
  WASM_ASSERT(res == 1, "remove bob");
  res = remove_i64(currentCode(), N(test_table), N(carol));
  WASM_ASSERT(res == 1, "remove carol");
  res = remove_i64(currentCode(),  N(test_table), N(dave));
  WASM_ASSERT(res == 1, "remove dave");


  res = remove_i64(currentCode(), N(test_table), N(alice));
  WASM_ASSERT(res == 0, "remove alice 2");
  res = remove_i64(currentCode(),   N(test_table), N(bob));
  WASM_ASSERT(res == 0, "remove bob 2");
  res = remove_i64(currentCode(), N(test_table), N(carol));
  WASM_ASSERT(res == 0, "remove carol 2");
  res = remove_i64(currentCode(),  N(test_table), N(dave));
  WASM_ASSERT(res == 0, "remove dave 2");
  
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_small_load() {
  uint64_t dummy = 0;
  load_i64(currentCode(), currentCode(), N(just_uint64), &dummy, sizeof(uint64_t)-1);
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_small_store() {
  uint64_t dummy = 0;
  store_i64(currentCode(), N(just_uint64), &dummy, sizeof(uint64_t)-1);
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_store_scope() {
  uint64_t dummy = 0;
  store_i64(currentCode(), N(just_uint64), &dummy, sizeof(uint64_t));
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_remove_scope() {
  uint64_t dummy = 0;
  store_i64(currentCode(), N(just_uint64), &dummy, sizeof(uint64_t));
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_not_found() {
  uint64_t dummy = 1000;

  auto res = load_i64(currentCode(), currentCode(), N(just_uint64), &dummy, sizeof(uint64_t));
  WASM_ASSERT(res == -1, "i64_not_found load");

  res = remove_i64(currentCode(),  N(just_uint64), dummy);
  WASM_ASSERT(res == 0, "i64_not_found remove");
  return WASM_TEST_PASS;
}
