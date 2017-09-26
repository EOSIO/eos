#include <eoslib/types.hpp>
#include <eoslib/message.hpp>
#include <eoslib/db.h>
#include <eoslib/db.hpp>
#include "test_api.hpp"

int primary[11]      = {0,1,2,3,4,5,6,7,8,9,10};
int secondary[11]    = {7,0,1,3,6,9,2,4,5,10,8};
int tertiary[11]     = {0,1,2,3,4,5,6,7,8,9,10};

int primary_lb[11]   = {0,0,0,3,3,3,6,7,7,9,9};
int secondary_lb[11] = {0,0,2,0,2,2,0,7,8,0,2};
int tertiary_lb[11]  = {0,1,2,3,4,5,6,7,8,9,10};

int primary_ub[11]   = {3,3,3,6,6,6,7,9,9,-1,-1};
int secondary_ub[11] = {2,2,8,2,8,8,2,0,-1,2,8};
int tertiary_ub[11]  = {1,2,3,4,5,6,7,8,9,10,-1};

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

struct TestModel3xi64 {
  uint64_t a;
  uint64_t b;
  uint64_t c;
  uint64_t table;
};

struct TestModel3xi64_V2 : TestModel3xi64 {
  uint64_t new_field;
};

#pragma pack(pop)

#define STRLEN(s) my_strlen(s)

extern "C" {
  void my_memset(void *vptr, unsigned char val, unsigned int size) {
    char *ptr = (char *)vptr;
    while(size--) { *(ptr++)=val; }
  }
  uint32_t my_strlen(const char *str);
  bool my_memcmp(void *s1, void *s2, uint32_t n);
}

unsigned int test_db::key_str_table() {

    const char* keys[] = { "alice", "bob", "carol", "dave" };
    const char* vals[] = { "data1", "data2", "data3", "data4" };

    const char* atr[]  = { "atr", "atr", "atr", "atr" };
    const char* ztr[]  = { "ztr", "ztr", "ztr", "ztr" };
    
    VarTable<N(tester), N(tester), N(atr), char*> StringTableAtr;
    VarTable<N(tester), N(tester), N(ztr), char*> StringTableZtr;
    VarTable<N(tester), N(tester), N(str), char*> StringTableStr;

    uint32_t res = 0;

    // fill some data in contiguous tables
    for( int ii = 0; ii < 4; ++ii ) {
        res = StringTableAtr.store( (char*)keys[ii], STRLEN(keys[ii]), (char*)atr[ii], STRLEN(atr[ii]) );
        WASM_ASSERT( res != 0, "atr" );
    
        res = StringTableZtr.store( (char*)keys[ii], STRLEN(keys[ii]), (char*)ztr[ii], STRLEN(ztr[ii]) );
        WASM_ASSERT(res != 0, "ztr" );
    }

    char tmp[64];
    
    res = StringTableStr.store ((char *)keys[0], STRLEN(keys[0]), (char *)vals[0], STRLEN(vals[0]));
    WASM_ASSERT(res != 0, "store alice" );

    res = StringTableStr.store((char *)keys[1], STRLEN(keys[1]), (char *)vals[1], STRLEN(vals[1]) );
    WASM_ASSERT(res != 0, "store bob" );

    res = StringTableStr.store((char *)keys[2], STRLEN(keys[2]), (char *)vals[2], STRLEN(vals[2]) );
    WASM_ASSERT(res != 0, "store carol" );

    res = StringTableStr.store((char *)keys[3], STRLEN(keys[3]), (char *)vals[3], STRLEN(vals[3]) );
    WASM_ASSERT(res != 0, "store dave" );

    res = StringTableStr.load((char *)keys[0], STRLEN(keys[0]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "load alice");

    res = StringTableStr.load((char *)keys[1], STRLEN(keys[1]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "load bob");

    res = StringTableStr.load((char *)keys[2], STRLEN(keys[2]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "load carol");

    res = StringTableStr.load((char *)keys[3], STRLEN(keys[3]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "load dave");

    res = StringTableStr.previous((char *)keys[3], STRLEN(keys[3]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "back carol");

    res = StringTableStr.previous((char *)keys[2], STRLEN(keys[2]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "back dave");

    res = StringTableStr.previous((char *)keys[1], STRLEN(keys[1]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "back alice");

    res = StringTableStr.previous((char *)keys[0], STRLEN(keys[0]), tmp, 64);
    WASM_ASSERT(res == -1, "no prev");

    res = StringTableStr.next((char *)keys[0], STRLEN(keys[0]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "next bob");

    res = StringTableStr.next((char *)keys[1], STRLEN(keys[1]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "next carol");

    res = StringTableStr.next((char *)keys[2], STRLEN(keys[2]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "next dave");

    res = StringTableStr.next((char *)keys[3], STRLEN(keys[3]), tmp, 64);
    WASM_ASSERT(res == -1, "no next");

    res = StringTableStr.next((char *)keys[0], STRLEN(keys[0]), tmp, 0);
    WASM_ASSERT(res == 0, "next 0");

    res = StringTableStr.front(tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "front alice");

    res = StringTableStr.back(tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "back dave");

    res = StringTableStr.lower_bound((char *)keys[0], STRLEN(keys[0]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "lowerbound alice");

    res = StringTableStr.upper_bound((char *)keys[0], STRLEN(keys[0]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "upperbound bob");

    res = StringTableStr.lower_bound((char *)keys[3], STRLEN(keys[3]), tmp, 64);
    WASM_ASSERT(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "upperbound dave");

    res = StringTableStr.upper_bound((char *)keys[3], STRLEN(keys[3]), tmp, 64);
    WASM_ASSERT(res == -1, "no upper_bound");

    return WASM_TEST_PASS;
}

unsigned int test_db::key_str_general() {

  const char* keys[] = { "alice", "bob", "carol", "dave" };
  const char* vals[] = { "data1", "data2", "data3", "data4" };

  const char* atr[]  = { "atr", "atr", "atr", "atr" };
  const char* ztr[]  = { "ztr", "ztr", "ztr", "ztr" };

  uint32_t res=0;

  //fill some data in contiguous tables
  for(int i=0; i < 4; ++i) {
    res = store_str(currentCode(),  N(atr), (char *)keys[i], STRLEN(keys[i]), (char *)atr[i], STRLEN(atr[i]) );
    WASM_ASSERT(res != 0, "atr" );

    res = store_str(currentCode(),  N(ztr), (char *)keys[i], STRLEN(keys[i]), (char *)ztr[i], STRLEN(ztr[i]) );
    WASM_ASSERT(res != 0, "ztr" );
  }

  char tmp[64];

  res = store_str(currentCode(),  N(str), (char *)keys[0], STRLEN(keys[0]), (char *)vals[0], STRLEN(vals[0]) );
  WASM_ASSERT(res != 0, "store alice" );

  res = store_str(currentCode(),  N(str), (char *)keys[1], STRLEN(keys[1]), (char *)vals[1], STRLEN(vals[1]) );
  WASM_ASSERT(res != 0, "store bob" );

  res = store_str(currentCode(),  N(str), (char *)keys[2], STRLEN(keys[2]), (char *)vals[2], STRLEN(vals[2]) );
  WASM_ASSERT(res != 0, "store carol" );

  res = store_str(currentCode(),  N(str), (char *)keys[3], STRLEN(keys[3]), (char *)vals[3], STRLEN(vals[3]) );
  WASM_ASSERT(res != 0, "store dave" );

  res = load_str(currentCode(), currentCode(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "load alice");

  res = load_str(currentCode(), currentCode(), N(str), (char *)keys[1], STRLEN(keys[1]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "load bob");

  res = load_str(currentCode(), currentCode(), N(str), (char *)keys[2], STRLEN(keys[2]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "load carol");

  res = load_str(currentCode(), currentCode(), N(str), (char *)keys[3], STRLEN(keys[3]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "load dave");

  res = previous_str(currentCode(), currentCode(), N(str), (char *)keys[3], STRLEN(keys[3]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "back carol");

  res = previous_str(currentCode(), currentCode(), N(str), (char *)keys[2], STRLEN(keys[2]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "back dave");

  res = previous_str(currentCode(), currentCode(), N(str), (char *)keys[1], STRLEN(keys[1]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "back alice");

  res = previous_str(currentCode(), currentCode(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 64);
  WASM_ASSERT(res == -1, "no prev");

  res = next_str(currentCode(), currentCode(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "next bob");

  res = next_str(currentCode(), currentCode(), N(str), (char *)keys[1], STRLEN(keys[1]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "next carol");

  res = next_str(currentCode(), currentCode(), N(str), (char *)keys[2], STRLEN(keys[2]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "next dave");

  res = next_str(currentCode(), currentCode(), N(str), (char *)keys[3], STRLEN(keys[3]), tmp, 64);
  WASM_ASSERT(res == -1, "no next");

  res = next_str(currentCode(), currentCode(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 0);
  WASM_ASSERT(res == 0, "next 0");

  res = front_str(currentCode(), currentCode(), N(str), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "front alice");

  res = back_str(currentCode(), currentCode(), N(str), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "back dave");

  res = lower_bound_str(currentCode(), currentCode(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "lowerbound alice");

  res = upper_bound_str(currentCode(), currentCode(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "upperbound bob");

  res = lower_bound_str(currentCode(), currentCode(), N(str), (char *)keys[3], STRLEN(keys[3]), tmp, 64);
  WASM_ASSERT(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "upperbound dave");

  res = upper_bound_str(currentCode(), currentCode(), N(str), (char *)keys[3], STRLEN(keys[3]), tmp, 64);
  WASM_ASSERT(res == -1, "no upper_bound");

  return WASM_TEST_PASS;
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

unsigned int store_set_in_table(TestModel3xi64* records, int len, uint64_t table_name) {
  uint32_t res = 0;
  for( int i = 0; i < len; ++i ) {
    TestModel3xi64 *tmp = records+i;
    tmp->table = table_name;
    res = store_i64i64i64(currentCode(),  table_name, tmp,  sizeof(TestModel3xi64));
    WASM_ASSERT(res == 1, "store_set_in_table" );
  }
  return res;
}

unsigned int test_db::key_i64i64i64_general() {
  
  uint32_t res = 0;

  TestModel3xi64 records[] = {
    {1, 1,  0, N()}, // 0
    {1, 1,  1, N()}, // 1
    {1, 2,  2, N()}, // 2
    {2, 1,  3, N()}, // 3
    {2, 2,  4, N()}, // 4
    {2, 2,  5, N()}, // 5
    {3, 1,  6, N()}, // 6
    {4, 0,  7, N()}, // 7
    {4, 5,  8, N()}, // 8
    {5, 1,  9, N()}, // 9
    {5, 2, 10, N()}, //10
  };

  store_set_in_table(records, sizeof(records)/sizeof(records[0]), N(table0));
  store_set_in_table(records, sizeof(records)/sizeof(records[0]), N(table1));
  store_set_in_table(records, sizeof(records)/sizeof(records[0]), N(table2));

  #define CALL(F, O, I, T, V) F##_##I##_##O(currentCode(), currentCode(), T, &V, sizeof(V))

  #define LOAD(I, O, T, V) CALL(load, O, I, T, V)
  #define FRONT(I, O, T, V) CALL(front, O, I, T, V)
  #define BACK(I, O, T, V) CALL(back, O, I, T, V)
  #define NEXT(I, O, T, V) CALL(next, O, I, T, V)
  #define PREV(I, O, T, V) CALL(previous, O, I, T, V)
  #define UPPER(I, O, T, V) CALL(upper_bound, O, I, T, V)
  #define LOWER(I, O, T, V) CALL(lower_bound, O, I, T, V)

  #define LOGME 0
  #define BS(X) ((X) ? "true" : "false")
  #define TABLE1_ASSERT(I, V, msg) \
    if(LOGME) {\
      eos::print(msg, " : ", res, " a:", V.a, " b:", V.b, " c:", V.c, " t:", V.table, " ("); \
      eos::print(BS(res == sizeof(V)), " ", BS(records[I].a == V.a), " ", BS(records[I].b == V.b), " ", BS(records[I].c == V.c), " => ", N(table1), ")\n"); \
    } \
    WASM_ASSERT( res == sizeof(V) && records[I].a == V.a && records[I].b == V.b && \
     records[I].c == V.c /*&& records[I].table == uint64_t(N(table1))*/, msg);

  #define LOAD_OK(I, O, T, INX, MSG) \
    {eos::remove_reference<decltype(V)>::type tmp; my_memset(&tmp, 0, sizeof(tmp));tmp = V; \
    res = LOAD(I, O, T, tmp); \
    TABLE1_ASSERT(INX, tmp, MSG)}

  #define LOAD_ER(I, O, T, MSG) \
    {eos::remove_reference<decltype(V)>::type tmp; my_memset(&tmp, 0, sizeof(tmp));tmp = V; \
    res = LOAD(I, O, T, tmp); \
    WASM_ASSERT(res == -1, MSG)}

  #define FRONT_OK(I, O, T, INX, MSG) \
    {eos::remove_reference<decltype(V)>::type tmp; my_memset(&tmp, 0, sizeof(tmp));tmp = V; \
    res = FRONT(I, O, T, tmp); \
    TABLE1_ASSERT(INX, tmp, MSG)}

  #define BACK_OK(I, O, T, INX, MSG) \
    {eos::remove_reference<decltype(V)>::type tmp; my_memset(&tmp, 0, sizeof(tmp));tmp = V; \
    res = BACK(I, O, T, tmp); \
    TABLE1_ASSERT(INX, tmp, MSG)}

  TestModel3xi64 V;

  V={0}; LOAD_ER(primary, i64i64i64, N(table1), "i64x3 LOAD primary fail 0");
  V={1}; LOAD_OK(primary, i64i64i64, N(table1), 0, "i64x3 LOAD primary 1");
  V={2}; LOAD_OK(primary, i64i64i64, N(table1), 3, "i64x3 LOAD primary 2");
  V={3}; LOAD_OK(primary, i64i64i64, N(table1), 6, "i64x3 LOAD primary 3");
  V={4}; LOAD_OK(primary, i64i64i64, N(table1), 7, "i64x3 LOAD primary 4");
  V={5}; LOAD_OK(primary, i64i64i64, N(table1), 9, "i64x3 LOAD primary 5");
  V={6}; LOAD_ER(primary, i64i64i64, N(table1), "i64x3 LOAD primary fail 6");
  
  V={11,0}; LOAD_OK(secondary, i64i64i64, N(table1), 7, "i64x3 LOAD secondary 0");
  V={11,1}; LOAD_OK(secondary, i64i64i64, N(table1), 0, "i64x3 LOAD secondary 1");
  V={11,2}; LOAD_OK(secondary, i64i64i64, N(table1), 2, "i64x3 LOAD secondary 2");
  V={11,3}; LOAD_ER(secondary, i64i64i64, N(table1), "i64x3 LOAD secondary fail 3");
  V={11,4}; LOAD_ER(secondary, i64i64i64, N(table1), "i64x3 LOAD secondary fail 4");
  V={11,5}; LOAD_OK(secondary, i64i64i64, N(table1), 8, "i64x3 LOAD secondary 5");
  V={11,6}; LOAD_ER(secondary, i64i64i64, N(table1), "i64x3 LOAD secondary fail 6");

  V={11,12,0}; LOAD_OK(tertiary, i64i64i64, N(table1),  0, "i64x3 LOAD tertiary 0");
  V={11,12,1}; LOAD_OK(tertiary, i64i64i64, N(table1),  1, "i64x3 LOAD tertiary 1");
  V={11,12,2}; LOAD_OK(tertiary, i64i64i64, N(table1),  2, "i64x3 LOAD tertiary 2");
  V={11,12,3}; LOAD_OK(tertiary, i64i64i64, N(table1),  3, "i64x3 LOAD tertiary 3");
  V={11,12,4}; LOAD_OK(tertiary, i64i64i64, N(table1),  4, "i64x3 LOAD tertiary 4");
  V={11,12,5}; LOAD_OK(tertiary, i64i64i64, N(table1),  5, "i64x3 LOAD tertiary 5");
  V={11,12,6}; LOAD_OK(tertiary, i64i64i64, N(table1),  6, "i64x3 LOAD tertiary 6");
  V={11,12,7}; LOAD_OK(tertiary, i64i64i64, N(table1),  7, "i64x3 LOAD tertiary 7");
  V={11,12,8}; LOAD_OK(tertiary, i64i64i64, N(table1),  8, "i64x3 LOAD tertiary 8");
  V={11,12,9}; LOAD_OK(tertiary, i64i64i64, N(table1),  9, "i64x3 LOAD tertiary 9");
  V={11,12,10}; LOAD_OK(tertiary, i64i64i64, N(table1), 10, "i64x3 LOAD tertiary 10");
  V={11,12,11}; LOAD_ER(tertiary, i64i64i64, N(table1), "i64x3 LOAD tertiary fail 11");

  #define NEXT_ALL(I, O, T) \
  { \
    auto n = sizeof(I)/sizeof(I[0]); \
    auto j = 0; \
    do { \
      eos::remove_reference<decltype(records[0])>::type tmp = records[I[j]]; \
      res = NEXT(I, i64i64i64, N(table1), tmp);\
      if(j+1<n){ TABLE1_ASSERT(I[j+1], tmp, "i64x3 NEXT " #I " ok "); } \
      else { WASM_ASSERT(res == -1, "i64x3 NEXT " #I " fail "); }\
    } while(++j<n); \
  }

  #define PREV_ALL(I, O, T) \
  { \
    auto n = sizeof(I)/sizeof(I[0]); \
    auto j = n-1; \
    do { \
      eos::remove_reference<decltype(records[0])>::type tmp = records[I[j]]; \
      res = PREV(I, i64i64i64, N(table1), tmp);\
      if(j>0){ TABLE1_ASSERT(I[j-1], tmp, "i64x3 PREV " #I " ok "); } \
      else { WASM_ASSERT(res == -1, "i64x3 PREV " #I " fail "); }\
    } while(--j>0); \
  }

  PREV_ALL(primary,   i64i64i64, N(table1));
  PREV_ALL(secondary, i64i64i64, N(table1));
  PREV_ALL(tertiary,  i64i64i64, N(table1));

  FRONT_OK(primary, i64i64i64, N(table1), primary[0],   "i64x3 FRONT primary");
  FRONT_OK(secondary, i64i64i64, N(table1), secondary[0], "i64x3 FRONT secondary");
  FRONT_OK(tertiary, i64i64i64, N(table1), tertiary[0], "i64x3 FRONT tertiary");

  BACK_OK(primary, i64i64i64, N(table1), primary[10],   "i64x3 BACK primary");
  BACK_OK(secondary, i64i64i64, N(table1), secondary[10], "i64x3 BACK secondary");
  BACK_OK(tertiary, i64i64i64, N(table1), tertiary[10], "i64x3 BACK tertiary");

  #define LOWER_ALL(I, O, T) \
  { \
    auto n = sizeof(I##_lb)/sizeof(I##_lb[0]); \
    auto j = 0; \
    do { \
      eos::remove_reference<decltype(records[0])>::type tmp = records[j]; \
      res = LOWER(I, i64i64i64, N(table1), tmp);\
      TABLE1_ASSERT(I##_lb[j], tmp, "i64x3 LOWER " #I " ok ");\
    } while(++j<n); \
  }

  LOWER_ALL(primary,   i64i64i64, N(table1));
  LOWER_ALL(secondary, i64i64i64, N(table1));
  LOWER_ALL(tertiary,  i64i64i64, N(table1));

  #define UPPER_ALL(I, O, T) \
  { \
    auto n = sizeof(I##_ub)/sizeof(I##_ub[0]); \
    auto j = 0; \
    do { \
      eos::remove_reference<decltype(records[0])>::type tmp = records[j]; \
      res = UPPER(I, i64i64i64, N(table1), tmp);\
      if(res == -1) { WASM_ASSERT(I##_ub[j]==-1,"i64x3 UPPER " #I " fail ") } \
      else { TABLE1_ASSERT(I##_ub[j], tmp, "i64x3 UPPER " #I " ok "); } \
    } while(++j<n); \
  }

  UPPER_ALL(primary,   i64i64i64, N(table1));
  UPPER_ALL(secondary, i64i64i64, N(table1));
  UPPER_ALL(tertiary,  i64i64i64, N(table1));

  TestModel3xi64_V2 v2;
  v2.a = records[6].a;

  res = LOAD(primary, i64i64i64, N(table1), v2);
  WASM_ASSERT(res == sizeof(TestModel3xi64), "load v2");

  v2.new_field = 555;

  res = update_i64i64i64(currentCode(),  N(table1), &v2, sizeof(TestModel3xi64_V2));
  WASM_ASSERT(res == 1, "store v2");  

  res = LOAD(primary, i64i64i64, N(table1), v2);
  WASM_ASSERT(res == sizeof(TestModel3xi64_V2), "load v2 updated");

  res = remove_i64i64i64(currentCode(),  N(table1), &v2);
  WASM_ASSERT(res == 1, "load v2 updated");

  res = LOAD(primary, i64i64i64, N(table1), v2);
  WASM_ASSERT(res == -1, "load not found");

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
