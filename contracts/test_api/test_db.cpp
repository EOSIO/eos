#include <eoslib/types.hpp>
#include <eoslib/message.hpp>
#include <eoslib/db.h>

#include "test_api.hpp"

#pragma pack(push, 1)
struct TestModel {
   AccountName   name;
   unsigned char age;
   uint64_t      phone;
};

struct TestModelV2 : TestModel {
  TestModelV2() : new_field(0) {}
  uint64_t new_field;
};

 struct OrderID {
    AccountName name    = 0;
    uint64_t    number  = 0;
 };

struct TestModel128x2 {
  OrderID   id;
  uint128_t price;
  uint64_t  extra;
};
#pragma pack(pop)

extern "C" {
  void my_memset(void *vptr, unsigned char val, unsigned int size) {
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
  
  my_memset(&alice, 0, sizeof(TestModel));

  WASM_ASSERT(alice.name == 0 && alice.age == 0 && alice.phone == 0, "my_memset");

  alice.name = N(alice);

  res = load_i64(currentCode(), currentCode(), N(test_table), &alice, sizeof(TestModel));
  WASM_ASSERT(res == sizeof(TestModel) && alice.age == 20 && alice.phone == 4234622, "alice error 1");

  alice.age = 21;
  alice.phone = 1234;
  
  res = store_i64(currentCode(), N(test_table), &alice, sizeof(TestModel));
  WASM_ASSERT(res == 0, "store alice 2" );

  my_memset(&alice, 0, sizeof(TestModel));
  alice.name = N(alice);
  
  res = load_i64(currentCode(), currentCode(), N(test_table), &alice, sizeof(TestModel));
  WASM_ASSERT(res == sizeof(TestModel) && alice.age == 21 && alice.phone == 1234, "alice error 2");

  my_memset(&bob, 0, sizeof(TestModel));
  bob.name = N(bob);

  my_memset(&carol, 0, sizeof(TestModel));
  carol.name = N(carol);

  my_memset(&dave, 0, sizeof(TestModel));
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

  my_memset(&alicev2, 0, sizeof(TestModelV2));
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

// extern "C" {
//   unsigned int store_set_in_table(uint64_t table_name);
// };

unsigned int store_set_in_table(uint64_t table_name)
{

  uint32_t res = 0;
  
  TestModel128x2 alice0{{N(alice),0}, 500, N(alice0)};
  TestModel128x2 alice1{{N(alice),1}, 400, N(alice1)};
  TestModel128x2 alice2{{N(alice),2}, 300, N(alice2)};

  res = store_i128i128(currentCode(),  table_name, &alice0,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store alice0" );

  res = store_i128i128(currentCode(),  table_name, &alice1,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store alice1" );

  res = store_i128i128(currentCode(),  table_name, &alice2,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store alice2" );

  TestModel128x2 bob0{{N(bob), 0}, 1, N(bob0)};
  TestModel128x2 bob1{{N(bob), 1}, 2, N(bob1)};
  TestModel128x2 bob2{{N(bob), 2}, 3, N(bob2)};
  TestModel128x2 bob3{{N(bob), 3}, 4, N(bob3)};

  res = store_i128i128(currentCode(),  table_name, &bob0,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store bob0" );

  res = store_i128i128(currentCode(),  table_name, &bob1,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store bob1" );

  res = store_i128i128(currentCode(),  table_name, &bob2,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store bob2" );

  res = store_i128i128(currentCode(),  table_name, &bob3,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store bob3" );

  TestModel128x2 carol0{{N(carol), 0}, 900, N(carol0)};
  TestModel128x2 carol1{{N(carol), 1}, 800, N(carol1)};
  TestModel128x2 carol2{{N(carol), 2}, 700, N(carol2)};
  TestModel128x2 carol3{{N(carol), 3}, 600, N(carol3)};

  res = store_i128i128(currentCode(),  table_name, &carol0,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store carol0" );

  res = store_i128i128(currentCode(),  table_name, &carol1,  sizeof(TestModel128x2));
  eos::print("Store carol1 ", res, "\n");
  WASM_ASSERT(res > 0, "store carol1" );
  eos::print("Store carol1 ", res, " ", carol1.id.name, " ", carol1.id.number, " ", carol1.price, " ", carol1.extra, "\n");


  res = store_i128i128(currentCode(),  table_name, &carol2,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store carol2" );

  res = store_i128i128(currentCode(),  table_name, &carol3,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store carol3" );

  TestModel128x2 dave0{{N(dave) ,0}, 8, N(dave0)};
  TestModel128x2 dave1{{N(dave) ,1}, 7, N(dave1)};
  TestModel128x2 dave2{{N(dave) ,2}, 5, N(dave2)};
  TestModel128x2 dave3{{N(dave) ,3}, 4, N(dave3)};

  res = store_i128i128(currentCode(),  table_name, &dave0,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store dave0" );

  res = store_i128i128(currentCode(),  table_name, &dave1,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store dave1" );

  res = store_i128i128(currentCode(),  table_name, &dave2,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store dave2" );

  res = store_i128i128(currentCode(),  table_name, &dave3,  sizeof(TestModel128x2));
  WASM_ASSERT(res > 0, "store dave3" );

  return WASM_TEST_PASS;
}

unsigned int test_db::key_i128i128_general() {

  uint32_t res = 0;

  if(store_set_in_table(N(table1)) != WASM_TEST_PASS)
    return WASM_TEST_FAIL;

  // if(store_set_in_table(N(table2)) != WASM_TEST_PASS)
  //   return WASM_TEST_FAIL;

  // if(store_set_in_table(N(table3)) != WASM_TEST_PASS)
  //   return WASM_TEST_FAIL;

  TestModel128x2 tmp;
  my_memset(&tmp, 0, sizeof(TestModel128x2));
  tmp.id.name   = N(carol);
  tmp.id.number = 1;
  tmp.price     = 800;

  res = load_primary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );

  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.id.name == N(carol) &&
               tmp.id.number == 1 &&
               tmp.price == 800 &&
               tmp.extra == N(carol1),
              "carol1 primary load");

  my_memset(&tmp, 0, sizeof(TestModel128x2));
  tmp.price = 4; //TestModel128x2 dave3{{N(dave) ,3}, 4, N(dave3)};
                 //TestModel128x2 bob3{{N(bob), 3}, 4, N(bob3)};
  
  res = load_secondary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  //eos::print("SECOND -> ", res, " ", tmp.id.name, " ", tmp.id.number, " ", tmp.price, " ", tmp.extra, "\n");
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.id.name == N(bob) &&
               tmp.id.number == 3 &&
               tmp.price == 4 &&
               tmp.extra == N(bob3),
              "bob3 secondary load");
  
  return WASM_TEST_PASS;
}
