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

struct TestModelV3 : TestModelV2 {
  uint64_t another_field;
};

struct TestModel128x2 {
  uint128_t number;
  uint128_t price;
  uint64_t  extra;
  uint64_t  table_name;
};

struct TestModel128x2_V2 : TestModel128x2 {
  uint64_t  new_field;
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

  //fill with different ages in adjacent tables
  dave.age=123;  store_i64(currentCode(), N(test_tabld), &dave,  sizeof(TestModel));
  dave.age=124;  store_i64(currentCode(), N(test_tablf), &dave,  sizeof(TestModel));
  carol.age=125; store_i64(currentCode(), N(test_tabld), &carol, sizeof(TestModel));
  carol.age=126; store_i64(currentCode(), N(test_tablf), &carol, sizeof(TestModel));
  bob.age=127;   store_i64(currentCode(), N(test_tabld), &bob,   sizeof(TestModel));
  bob.age=128;   store_i64(currentCode(), N(test_tablf), &bob,   sizeof(TestModel));
  alice.age=129; store_i64(currentCode(), N(test_tabld), &alice, sizeof(TestModel));
  alice.age=130; store_i64(currentCode(), N(test_tablf), &alice, sizeof(TestModel));

  TestModel tmp;

  res = front_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "front_i64 1");
  my_memset(&tmp, 0, sizeof(TestModel));

  res = back_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "front_i64 2");

  res = previous_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(carol) && tmp.age == 30 && tmp.phone == 545342453, "carol previous");
  
  res = previous_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(bob) && tmp.age == 15 && tmp.phone == 11932435, "bob previous");

  res = previous_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "alice previous");

  res = previous_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == -1, "previous null");

  res = next_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(bob) && tmp.age == 15 && tmp.phone == 11932435, "bob next");

  res = next_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(carol) && tmp.age == 30 && tmp.phone == 545342453, "carol next");

  res = next_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "dave next");

  res = next_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == -1, "next null");

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

  my_memset(&tmp, 0, sizeof(TestModel));
  tmp.name = N(bob);
  res = lower_bound_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(bob), "lower_bound_i64 bob" );

  my_memset(&tmp, 0, sizeof(TestModel));
  tmp.name = N(boc);
  res = lower_bound_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(carol), "lower_bound_i64 carol" );

  my_memset(&tmp, 0, sizeof(TestModel));
  tmp.name = N(dave);
  res = lower_bound_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(uint64_t) );
  WASM_ASSERT(res == sizeof(uint64_t) && tmp.name == N(dave), "lower_bound_i64 dave" );

  my_memset(&tmp, 0, sizeof(TestModel));
  tmp.name = N(davf);
  res = lower_bound_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(uint64_t) );
  WASM_ASSERT(res == -1, "lower_bound_i64 fail" );

  my_memset(&tmp, 0, sizeof(TestModel));
  tmp.name = N(alice);
  res = upper_bound_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.age == 15 && tmp.name == N(bob), "upper_bound_i64 bob" );

  my_memset(&tmp, 0, sizeof(TestModel));
  tmp.name = N(dave);
  res = upper_bound_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == -1, "upper_bound_i64 dave" );

  TestModelV3 tmp2;
  tmp2.name = N(alice);

  res = load_i64(currentCode(), currentCode(), N(test_table), &tmp2, sizeof(TestModelV3));
  WASM_ASSERT(res == sizeof(TestModelV2) && 
              tmp2.age == 21 && 
              tmp2.phone == 1234 &&
              tmp2.new_field == 66655444,
              "load4update");

  tmp2.another_field = 221122;
  res = update_i64(currentCode(), N(test_table), &tmp2, sizeof(TestModelV3));
  WASM_ASSERT(res == 1, "update_i64");

  res = load_i64(currentCode(), currentCode(), N(test_table), &tmp2, sizeof(TestModelV3));
  WASM_ASSERT(res == sizeof(TestModelV3) && 
              tmp2.age == 21 && 
              tmp2.phone == 1234 &&
              tmp2.new_field == 66655444 &&
              tmp2.another_field == 221122,
              "load4update");

  tmp2.age = 11;
  res = update_i64(currentCode(), N(test_table), &tmp2,  sizeof(uint64_t)+1 );
  WASM_ASSERT(res == 1, "update_i64 small");

  res = load_i64(currentCode(), currentCode(), N(test_table), &tmp2, sizeof(TestModelV3));
  WASM_ASSERT(res == sizeof(TestModelV3) && 
              tmp2.age == 11 && 
              tmp2.phone == 1234 &&
              tmp2.new_field == 66655444 &&
              tmp2.another_field == 221122,
              "load_i64 update_i64");


  //Remove dummy records
  uint64_t tables[] { N(test_tabld), N(test_tablf) };
  for(auto& t : tables) {
    while( front_i64( currentCode(), currentCode(), t, &tmp, sizeof(TestModel) ) != -1 ) {
      remove_i64(currentCode(), t, &tmp);
    }
  }

  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_remove_all() {
  
  uint32_t res = 0;
  uint64_t key;

  key = N(alice);
  res = remove_i64(currentCode(), N(test_table), &key);
  WASM_ASSERT(res == 1, "remove alice");
  
  key = N(bob);
  res = remove_i64(currentCode(),   N(test_table), &key);
  WASM_ASSERT(res == 1, "remove bob");
  
  key = N(carol);
  res = remove_i64(currentCode(), N(test_table), &key);
  WASM_ASSERT(res == 1, "remove carol");
  
  key = N(dave);
  res = remove_i64(currentCode(),  N(test_table), &key);
  WASM_ASSERT(res == 1, "remove dave");

  TestModel tmp;
  res = front_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == -1, "front_i64 remove");

  res = back_i64( currentCode(), currentCode(), N(test_table), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == -1, "back_i64_i64 remove");
  
  key = N(alice);
  res = remove_i64(currentCode(), N(test_table), &key);
  WASM_ASSERT(res == 0, "remove alice 1");
  
  key = N(bob);
  res = remove_i64(currentCode(),   N(test_table), &key);
  WASM_ASSERT(res == 0, "remove bob 1");
  
  key = N(carol);
  res = remove_i64(currentCode(), N(test_table), &key);
  WASM_ASSERT(res == 0, "remove carol 1");
  
  key = N(dave);
  res = remove_i64(currentCode(),  N(test_table), &key);
  WASM_ASSERT(res == 0, "remove dave 1");

 
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

  res = remove_i64(currentCode(),  N(just_uint64), &dummy);
  WASM_ASSERT(res == 0, "i64_not_found remove");
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_front_back() {
  
  uint32_t res = 0;

  TestModel dave { N(dave),  46, 6535354};
  TestModel carol{ N(carol), 30, 545342453};
  store_i64(currentCode(), N(b), &dave,  sizeof(TestModel));
  store_i64(currentCode(), N(b), &carol, sizeof(TestModel));

  TestModel bob  { N(bob),   15, 11932435};
  TestModel alice{ N(alice), 20, 4234622};
  store_i64(currentCode(), N(a), &bob, sizeof(TestModel));
  store_i64(currentCode(), N(a), &alice,  sizeof(TestModel));

  TestModel tmp;

  my_memset(&tmp, 0, sizeof(TestModel));
  res = front_i64( currentCode(), currentCode(), N(a), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "key_i64_front 1");

  my_memset(&tmp, 0, sizeof(TestModel));
  res = back_i64( currentCode(), currentCode(), N(a), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(bob) && tmp.age == 15 && tmp.phone == 11932435, "key_i64_front 2");

  my_memset(&tmp, 0, sizeof(TestModel));
  res = front_i64( currentCode(), currentCode(), N(b), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(carol) && tmp.age == 30 && tmp.phone == 545342453, "key_i64_front 3");

  my_memset(&tmp, 0, sizeof(TestModel));
  res = back_i64( currentCode(), currentCode(), N(b), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "key_i64_front 4");

  uint64_t key = N(carol);
  remove_i64(currentCode(), N(b), &key);

  my_memset(&tmp, 0, sizeof(TestModel));
  res = front_i64( currentCode(), currentCode(), N(b), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "key_i64_front 5");

  my_memset(&tmp, 0, sizeof(TestModel));
  res = back_i64( currentCode(), currentCode(), N(b), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "key_i64_front 6");

  my_memset(&tmp, 0, sizeof(TestModel));
  res = front_i64( currentCode(), currentCode(), N(a), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "key_i64_front 7");

  my_memset(&tmp, 0, sizeof(TestModel));
  res = back_i64( currentCode(), currentCode(), N(a), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(bob) && tmp.age == 15 && tmp.phone == 11932435, "key_i64_front 8");

  key = N(dave);
  remove_i64(currentCode(), N(b), &key);
  
  res = front_i64( currentCode(), currentCode(), N(b), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == -1, "key_i64_front 9");
  res = back_i64( currentCode(), currentCode(), N(b), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == -1, "key_i64_front 10");

  key = N(bob);
  remove_i64(currentCode(), N(a), &key);

  my_memset(&tmp, 0, sizeof(TestModel));
  res = front_i64( currentCode(), currentCode(), N(a), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "key_i64_front 11");

  my_memset(&tmp, 0, sizeof(TestModel));
  res = back_i64( currentCode(), currentCode(), N(a), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == sizeof(TestModel) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "key_i64_front 12");

  key = N(alice);
  remove_i64(currentCode(), N(a), &key);

  res = front_i64( currentCode(), currentCode(), N(a), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == -1, "key_i64_front 13");
  res = back_i64( currentCode(), currentCode(), N(a), &tmp, sizeof(TestModel) );
  WASM_ASSERT(res == -1, "key_i64_front 14");

  return WASM_TEST_PASS;
}

unsigned int store_set_in_table(uint64_t table_name)
{

  uint32_t res = 0;
  
  TestModel128x2 alice0{0, 500, N(alice0), table_name};
  TestModel128x2 alice1{1, 400, N(alice1), table_name};
  TestModel128x2 alice2{2, 300, N(alice2), table_name};
  TestModel128x2 alice22{2, 200, N(alice22), table_name};

  res = store_i128i128(currentCode(),  table_name, &alice0,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store alice0" );

  res = store_i128i128(currentCode(),  table_name, &alice1,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store alice1" );

  res = store_i128i128(currentCode(),  table_name, &alice2,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store alice2" );

  res = store_i128i128(currentCode(),  table_name, &alice22,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store alice22" );

  TestModel128x2 bob0{10, 1, N(bob0), table_name};
  TestModel128x2 bob1{11, 2, N(bob1), table_name};
  TestModel128x2 bob2{12, 3, N(bob2), table_name};
  TestModel128x2 bob3{13, 4, N(bob3), table_name};

  res = store_i128i128(currentCode(),  table_name, &bob0,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store bob0" );

  res = store_i128i128(currentCode(),  table_name, &bob1,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store bob1" );

  res = store_i128i128(currentCode(),  table_name, &bob2,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store bob2" );

  res = store_i128i128(currentCode(),  table_name, &bob3,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store bob3" );

  TestModel128x2 carol0{20, 900, N(carol0), table_name};
  TestModel128x2 carol1{21, 800, N(carol1), table_name};
  TestModel128x2 carol2{22, 700, N(carol2), table_name};
  TestModel128x2 carol3{23, 600, N(carol3), table_name};

  res = store_i128i128(currentCode(),  table_name, &carol0,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store carol0" );

  res = store_i128i128(currentCode(),  table_name, &carol1,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store carol1" );

  res = store_i128i128(currentCode(),  table_name, &carol2,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store carol2" );

  res = store_i128i128(currentCode(),  table_name, &carol3,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store carol3" );

  TestModel128x2 dave0{30, 8, N(dave0), table_name};
  TestModel128x2 dave1{31, 7, N(dave1), table_name};
  TestModel128x2 dave2{32, 5, N(dave2), table_name};
  TestModel128x2 dave3{33, 4, N(dave3), table_name};

  res = store_i128i128(currentCode(),  table_name, &dave0,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store dave0" );

  res = store_i128i128(currentCode(),  table_name, &dave1,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store dave1" );

  res = store_i128i128(currentCode(),  table_name, &dave2,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store dave2" );

  res = store_i128i128(currentCode(),  table_name, &dave3,  sizeof(TestModel128x2));
  WASM_ASSERT(res == 1, "store dave3" );

  return WASM_TEST_PASS;
}

unsigned int test_db::key_i128i128_general() {

  uint32_t res = 0;

  if(store_set_in_table(N(table0)) != WASM_TEST_PASS)
    return WASM_TEST_FAIL;

  if(store_set_in_table(N(table1)) != WASM_TEST_PASS)
    return WASM_TEST_FAIL;

  if(store_set_in_table(N(table2)) != WASM_TEST_PASS)
    return WASM_TEST_FAIL;

  TestModel128x2 tmp;
  my_memset(&tmp, 0, sizeof(TestModel128x2));
  tmp.number = 21;

  res = load_primary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );

  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.price == 800 &&
               tmp.extra == N(carol1) &&
               tmp.table_name == N(table1),
              "carol1 primary load");

  my_memset(&tmp, 0, sizeof(TestModel128x2));
  tmp.price = 4;
  
  res = load_secondary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 13 &&
               tmp.price == 4 &&
               tmp.extra == N(bob3) &&
               tmp.table_name == N(table1),
              "bob3 secondary load");

  res = front_primary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 0 &&
               tmp.price == 500 &&
               tmp.extra == N(alice0) &&
               tmp.table_name == N(table1),
              "front primary load");
  
  res = previous_primary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT(res == -1, "previous primary fail");

  res = next_primary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 1 &&
               tmp.price == 400 &&
               tmp.extra == N(alice1) &&
               tmp.table_name == N(table1),
              "next primary ok");

  res = front_secondary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 10 &&
               tmp.price == 1 &&
               tmp.extra == N(bob0) &&
               tmp.table_name == N(table1),
              "front secondary ok");
  
  res = previous_secondary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT(res == -1, "previous secondary fail");

  res = next_secondary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 11 &&
               tmp.price == 2 &&
               tmp.extra == N(bob1) &&
               tmp.table_name == N(table1),
              "next secondary ok");

  res = back_primary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 33 &&
               tmp.price == 4 &&
               tmp.extra == N(dave3) &&
               tmp.table_name == N(table1),
              "back primary ok");
  
  res = next_primary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT(res == -1, "next primary fail");

  res = previous_primary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 32 &&
               tmp.price == 5 &&
               tmp.extra == N(dave2) &&
               tmp.table_name == N(table1),
              "previous primary ok");

  res = back_secondary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 20 &&
               tmp.price == 900 &&
               tmp.extra == N(carol0) &&
               tmp.table_name == N(table1),
              "back secondary ok");
  
  res = next_secondary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT(res == -1, "next secondary fail");

  res = previous_secondary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 21 &&
               tmp.price == 800 &&
               tmp.extra == N(carol1) &&
               tmp.table_name == N(table1),
              "previous secondary ok");

  tmp.number = 1;

  res = lower_bound_primary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 1 &&
               tmp.price == 400 &&
               tmp.extra == N(alice1) &&
               tmp.table_name == N(table1),
              "lb primary ok");

  res = upper_bound_primary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 2 &&
               tmp.price == 200 &&
               tmp.extra == N(alice22) &&
               tmp.table_name == N(table1),
              "ub primary ok");

  tmp.price = 800;

  res = lower_bound_secondary_i128i128( currentCode(), currentCode(), N(table1), &tmp, sizeof(TestModel128x2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp.number == 21 &&
               tmp.price == 800 &&
               tmp.extra == N(carol1) &&
               tmp.table_name == N(table1),
              "lb secondary ok");

  TestModel128x2_V2 tmp2;
  tmp2.price = 800;

  res = upper_bound_secondary_i128i128( currentCode(), currentCode(), N(table1), &tmp2, sizeof(TestModel128x2_V2) );
  WASM_ASSERT( res == sizeof(TestModel128x2) &&
               tmp2.number == 20 &&
               tmp2.price == 900 &&
               tmp2.extra == N(carol0) &&
               tmp2.table_name == N(table1),
              "ub secondary ok");
  
  tmp2.new_field = 123456;
  res = update_i128i128(currentCode(), N(table1), &tmp2, sizeof(TestModel128x2_V2));
  WASM_ASSERT( res == 1, "update_i128i128 ok");

  my_memset(&tmp2, 0, sizeof(TestModel128x2_V2));
  tmp2.number = 20;

  res = load_primary_i128i128(currentCode(), currentCode(), N(table1), &tmp2, sizeof(TestModel128x2_V2));
  WASM_ASSERT( res == sizeof(TestModel128x2_V2) &&
               tmp2.number == 20 &&
               tmp2.price == 900 &&
               tmp2.extra == N(carol0) &&
               tmp2.table_name == N(table1) &&
               tmp2.new_field == 123456,
              "lp update_i128i128 ok");

  tmp2.extra = N(xxxxx);
  res = update_i128i128(currentCode(), N(table1), &tmp2, sizeof(uint128_t)*2+sizeof(uint64_t));
  WASM_ASSERT( res == 1, "update_i128i128 small ok");

  res = load_primary_i128i128(currentCode(), currentCode(), N(table1), &tmp2, sizeof(TestModel128x2_V2));
  WASM_ASSERT( res == sizeof(TestModel128x2_V2) &&
               tmp2.number == 20 &&
               tmp2.price == 900 &&
               tmp2.extra == N(xxxxx) &&
               tmp2.table_name == N(table1) &&
               tmp2.new_field == 123456,
              "lp small update_i128i128 ok");

  return WASM_TEST_PASS;
}

  //eos::print("xxxx ", res, " ", tmp2.name, " ", uint64_t(tmp2.age), " ", tmp2.phone, " ", tmp2.new_field, "\n");