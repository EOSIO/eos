#include <eosiolib/types.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/db.h>
#include <eosiolib/db.hpp>
#include <eosiolib/memory.hpp>
#include "test_api.hpp"

int primary[11]      = {0,1,2,3,4,5,6,7,8,9,10};
int secondary[11]    = {7,0,1,3,6,9,10,2,4,5,8};
int tertiary[11]     = {0,10,1,2,4,3,5,6,7,8,9};

int primary_lb[11]   = {0,0,0,3,3,3,6,7,7,9,9};
int secondary_lb[11] = {0,0,10,0,10,10,0,7,8,0,10};
int tertiary_lb[11]  = {0,1,2,3,2,5,6,7,8,9,0};

int primary_ub[11]   = {3,3,3,6,6,6,7,9,9,-1,-1};
int secondary_ub[11] = {10,10,8,10,8,8,10,0,-1,10,8};
int tertiary_ub[11]  = {1,2,3,5,3,6,7,8,9,-1,1};

#pragma pack(push, 1)
struct test_model {
   account_name  name;
   unsigned char age;
   uint64_t      phone;
};

struct test_model_v2 : test_model {
  test_model_v2() : new_field(0) {}
  uint64_t new_field;
};

struct test_model_v3 : test_model_v2 {
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

#if 0

unsigned int test_db::key_str_table() {

    const char* keys[] = { "alice", "bob", "carol", "dave" };
    const char* vals[] = { "data1", "data2", "data3", "data4" };

    const char* atr[]  = { "atr", "atr", "atr", "atr" };
    const char* ztr[]  = { "ztr", "ztr", "ztr", "ztr" };
    
    eosio::var_table<N(tester), N(tester), N(atr), N(tester), char*> StringTableAtr;
    eosio::var_table<N(tester), N(tester), N(ztr), N(tester), char*> StringTableZtr;
    eosio::var_table<N(tester), N(tester), N(str), N(tester), char*> StringTableStr;

    uint32_t res = 0;

    // fill some data in contiguous tables
    for( int ii = 0; ii < 4; ++ii ) {
        res = StringTableAtr.store( (char*)keys[ii], STRLEN(keys[ii]), (char*)atr[ii], STRLEN(atr[ii]) );
        eos_assert( res != 0, "atr" );
    
        res = StringTableZtr.store( (char*)keys[ii], STRLEN(keys[ii]), (char*)ztr[ii], STRLEN(ztr[ii]) );
        eos_assert(res != 0, "ztr" );
    }

    char tmp[64];
    
    res = StringTableStr.store ((char *)keys[0], STRLEN(keys[0]), (char *)vals[0], STRLEN(vals[0]));
    eos_assert(res != 0, "store alice" );

    res = StringTableStr.store((char *)keys[1], STRLEN(keys[1]), (char *)vals[1], STRLEN(vals[1]) );
    eos_assert(res != 0, "store bob" );

    res = StringTableStr.store((char *)keys[2], STRLEN(keys[2]), (char *)vals[2], STRLEN(vals[2]) );
    eos_assert(res != 0, "store carol" );

    res = StringTableStr.store((char *)keys[3], STRLEN(keys[3]), (char *)vals[3], STRLEN(vals[3]) );
    eos_assert(res != 0, "store dave" );

    res = StringTableStr.load((char *)keys[0], STRLEN(keys[0]), tmp, 64);
    eos_assert(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "load alice");

    res = StringTableStr.load((char *)keys[1], STRLEN(keys[1]), tmp, 64);
    eos_assert(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "load bob");

    res = StringTableStr.load((char *)keys[2], STRLEN(keys[2]), tmp, 64);
    eos_assert(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "load carol");

    res = StringTableStr.load((char *)keys[3], STRLEN(keys[3]), tmp, 64);
    eos_assert(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "load dave");

    res = StringTableStr.previous((char *)keys[3], STRLEN(keys[3]), tmp, 64);
    eos_assert(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "back carol");

    res = StringTableStr.previous((char *)keys[2], STRLEN(keys[2]), tmp, 64);
    eos_assert(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "back dave");

    res = StringTableStr.previous((char *)keys[1], STRLEN(keys[1]), tmp, 64);
    eos_assert(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "back alice");

    res = StringTableStr.previous((char *)keys[0], STRLEN(keys[0]), tmp, 64);
    eos_assert(res == -1, "no prev");

    res = StringTableStr.next((char *)keys[0], STRLEN(keys[0]), tmp, 64);
    eos_assert(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "next bob");

    res = StringTableStr.next((char *)keys[1], STRLEN(keys[1]), tmp, 64);
    eos_assert(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "next carol");

    res = StringTableStr.next((char *)keys[2], STRLEN(keys[2]), tmp, 64);
    eos_assert(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "next dave");

    res = StringTableStr.next((char *)keys[3], STRLEN(keys[3]), tmp, 64);
    eos_assert(res == -1, "no next");

    res = StringTableStr.next((char *)keys[0], STRLEN(keys[0]), tmp, 0);
    eos_assert(res == 0, "next 0");

    res = StringTableStr.front(tmp, 64);
    eos_assert(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "front alice");

    res = StringTableStr.back(tmp, 64);
    eos_assert(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "back dave");

    res = StringTableStr.lower_bound((char *)keys[0], STRLEN(keys[0]), tmp, 64);
    eos_assert(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "lowerbound alice");

    res = StringTableStr.upper_bound((char *)keys[0], STRLEN(keys[0]), tmp, 64);
    eos_assert(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "upperbound bob");

    res = StringTableStr.lower_bound((char *)keys[3], STRLEN(keys[3]), tmp, 64);
    eos_assert(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "upperbound dave");

    res = StringTableStr.upper_bound((char *)keys[3], STRLEN(keys[3]), tmp, 64);
    eos_assert(res == -1, "no upper_bound");

    return 1; // WASM_TEST_PASS;
}

#endif

#if 0

unsigned int test_db::key_str_general() {

  const char* keys[] = { "alice", "bob", "carol", "dave" };
  const char* vals[] = { "data1", "data2", "data3", "data4" };

  const char* atr[]  = { "atr", "atr", "atr", "atr" };
  const char* ztr[]  = { "ztr", "ztr", "ztr", "ztr" };

  uint32_t res=0;

  //fill some data in contiguous tables
  for(int i=0; i < 4; ++i) {
    res = store_str(current_receiver(),  N(atr), (char *)keys[i], STRLEN(keys[i]), (char *)atr[i], STRLEN(atr[i]) );
    eos_assert(res != 0, "atr" );

    res = store_str(current_receiver(),  N(ztr), (char *)keys[i], STRLEN(keys[i]), (char *)ztr[i], STRLEN(ztr[i]) );
    eos_assert(res != 0, "ztr" );
  }

  char tmp[64];

  res = store_str(current_receiver(),  N(str), (char *)keys[0], STRLEN(keys[0]), (char *)vals[0], STRLEN(vals[0]) );
  eos_assert(res != 0, "store alice" );

  res = store_str(current_receiver(),  N(str), (char *)keys[1], STRLEN(keys[1]), (char *)vals[1], STRLEN(vals[1]) );
  eos_assert(res != 0, "store bob" );

  res = store_str(current_receiver(),  N(str), (char *)keys[2], STRLEN(keys[2]), (char *)vals[2], STRLEN(vals[2]) );
  eos_assert(res != 0, "store carol" );

  res = store_str(current_receiver(),  N(str), (char *)keys[3], STRLEN(keys[3]), (char *)vals[3], STRLEN(vals[3]) );
  eos_assert(res != 0, "store dave" );

  res = load_str(current_receiver(), current_receiver(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 64);
  eos_assert(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "load alice");

  res = load_str(current_receiver(), current_receiver(), N(str), (char *)keys[1], STRLEN(keys[1]), tmp, 64);
  eos_assert(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "load bob");

  res = load_str(current_receiver(), current_receiver(), N(str), (char *)keys[2], STRLEN(keys[2]), tmp, 64);
  eos_assert(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "load carol");

  res = load_str(current_receiver(), current_receiver(), N(str), (char *)keys[3], STRLEN(keys[3]), tmp, 64);
  eos_assert(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "load dave");

  res = previous_str(current_receiver(), current_receiver(), N(str), (char *)keys[3], STRLEN(keys[3]), tmp, 64);
  eos_assert(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "back carol");

  res = previous_str(current_receiver(), current_receiver(), N(str), (char *)keys[2], STRLEN(keys[2]), tmp, 64);
  eos_assert(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "back dave");

  res = previous_str(current_receiver(), current_receiver(), N(str), (char *)keys[1], STRLEN(keys[1]), tmp, 64);
  eos_assert(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "back alice");

  res = previous_str(current_receiver(), current_receiver(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 64);
  eos_assert(res == -1, "no prev");

  res = next_str(current_receiver(), current_receiver(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 64);
  eos_assert(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "next bob");

  res = next_str(current_receiver(), current_receiver(), N(str), (char *)keys[1], STRLEN(keys[1]), tmp, 64);
  eos_assert(res == STRLEN(vals[2]) && my_memcmp((void *)vals[2], (void *)tmp, res), "next carol");

  res = next_str(current_receiver(), current_receiver(), N(str), (char *)keys[2], STRLEN(keys[2]), tmp, 64);
  eos_assert(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "next dave");

  res = next_str(current_receiver(), current_receiver(), N(str), (char *)keys[3], STRLEN(keys[3]), tmp, 64);
  eos_assert(res == -1, "no next");

  res = next_str(current_receiver(), current_receiver(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 0);
  eos_assert(res == 0, "next 0");

  res = front_str(current_receiver(), current_receiver(), N(str), tmp, 64);
  eos_assert(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "front alice");

  res = back_str(current_receiver(), current_receiver(), N(str), tmp, 64);
  eos_assert(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "back dave");

  res = lower_bound_str(current_receiver(), current_receiver(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 64);
  eos_assert(res == STRLEN(vals[0]) && my_memcmp((void *)vals[0], (void *)tmp, res), "lowerbound alice");

  res = upper_bound_str(current_receiver(), current_receiver(), N(str), (char *)keys[0], STRLEN(keys[0]), tmp, 64);
  eos_assert(res == STRLEN(vals[1]) && my_memcmp((void *)vals[1], (void *)tmp, res), "upperbound bob");

  res = lower_bound_str(current_receiver(), current_receiver(), N(str), (char *)keys[3], STRLEN(keys[3]), tmp, 64);
  eos_assert(res == STRLEN(vals[3]) && my_memcmp((void *)vals[3], (void *)tmp, res), "upperbound dave");

  res = upper_bound_str(current_receiver(), current_receiver(), N(str), (char *)keys[3], STRLEN(keys[3]), tmp, 64);
  eos_assert(res == -1, "no upper_bound");

  return 1;//WASM_TEST_PASS;
}

#endif

#if 0

unsigned int test_db::key_i64_general() {

  uint32_t res = 0;

  test_model alice{ N(alice), 20, 4234622};
  test_model bob  { N(bob),   15, 11932435};
  test_model carol{ N(carol), 30, 545342453};
  test_model dave { N(dave),  46, 6535354};

  res = store_i64(current_receiver(),  N(test_table), &dave,  sizeof(test_model));
  eos_assert(res != 0, "store dave" );

  res = store_i64(current_receiver(), N(test_table), &carol, sizeof(test_model));
  eos_assert(res != 0, "store carol" );

  res = store_i64(current_receiver(),   N(test_table), &bob,   sizeof(test_model));
  eos_assert(res != 0, "store bob" );

  res = store_i64(current_receiver(), N(test_table), &alice, sizeof(test_model));
  eos_assert(res != 0, "store alice" );

  //fill with different ages in adjacent tables
  dave.age=123;  store_i64(current_receiver(), N(test_tabld), &dave,  sizeof(test_model));
  dave.age=124;  store_i64(current_receiver(), N(test_tablf), &dave,  sizeof(test_model));
  carol.age=125; store_i64(current_receiver(), N(test_tabld), &carol, sizeof(test_model));
  carol.age=126; store_i64(current_receiver(), N(test_tablf), &carol, sizeof(test_model));
  bob.age=127;   store_i64(current_receiver(), N(test_tabld), &bob,   sizeof(test_model));
  bob.age=128;   store_i64(current_receiver(), N(test_tablf), &bob,   sizeof(test_model));
  alice.age=129; store_i64(current_receiver(), N(test_tabld), &alice, sizeof(test_model));
  alice.age=130; store_i64(current_receiver(), N(test_tablf), &alice, sizeof(test_model));

  test_model tmp;

  res = front_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "front_i64 1");
  my_memset(&tmp, 0, sizeof(test_model));

  res = back_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "front_i64 2");

  res = previous_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(carol) && tmp.age == 30 && tmp.phone == 545342453, "carol previous");
  
  res = previous_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(bob) && tmp.age == 15 && tmp.phone == 11932435, "bob previous");

  res = previous_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "alice previous");

  res = previous_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == -1, "previous null");

  res = next_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(bob) && tmp.age == 15 && tmp.phone == 11932435, "bob next");

  res = next_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(carol) && tmp.age == 30 && tmp.phone == 545342453, "carol next");

  res = next_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "dave next");

  res = next_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == -1, "next null");

  my_memset(&alice, 0, sizeof(test_model));

  eos_assert(alice.name == 0 && alice.age == 0 && alice.phone == 0, "my_memset");

  alice.name = N(alice);

  res = load_i64(current_receiver(), current_receiver(), N(test_table), &alice, sizeof(test_model));
  eos_assert(res == sizeof(test_model) && alice.age == 20 && alice.phone == 4234622, "alice error 1");

  alice.age = 21;
  alice.phone = 1234;
  
  res = store_i64(current_receiver(), N(test_table), &alice, sizeof(test_model));
  eos_assert(res == 0, "store alice 2" );

  my_memset(&alice, 0, sizeof(test_model));
  alice.name = N(alice);
  
  res = load_i64(current_receiver(), current_receiver(), N(test_table), &alice, sizeof(test_model));
  eos_assert(res == sizeof(test_model) && alice.age == 21 && alice.phone == 1234, "alice error 2");

  my_memset(&bob, 0, sizeof(test_model));
  bob.name = N(bob);

  my_memset(&carol, 0, sizeof(test_model));
  carol.name = N(carol);

  my_memset(&dave, 0, sizeof(test_model));
  dave.name = N(dave);

  res = load_i64(current_receiver(), current_receiver(), N(test_table), &bob, sizeof(test_model));
  eos_assert(res == sizeof(test_model) && bob.age == 15 && bob.phone == 11932435, "bob error 1");

  res = load_i64(current_receiver(), current_receiver(), N(test_table), &carol, sizeof(test_model));
  eos_assert(res == sizeof(test_model) && carol.age == 30 && carol.phone == 545342453, "carol error 1");

  res = load_i64(current_receiver(), current_receiver(), N(test_table), &dave, sizeof(test_model));
  eos_assert(res == sizeof(test_model) && dave.age == 46 && dave.phone == 6535354, "dave error 1");

  res = load_i64(current_receiver(), N(other_code), N(test_table), &alice, sizeof(test_model));
  eos_assert(res == -1, "other_code");

  res = load_i64(current_receiver(), current_receiver(), N(other_table), &alice, sizeof(test_model));
  eos_assert(res == -1, "other_table");


  test_model_v2 alicev2;
  alicev2.name = N(alice);

  res = load_i64(current_receiver(), current_receiver(), N(test_table), &alicev2, sizeof(test_model_v2));
  eos_assert(res == sizeof(test_model) && alicev2.age == 21 && alicev2.phone == 1234, "alicev2 load");

  alicev2.new_field = 66655444;
  res = store_i64(current_receiver(), N(test_table), &alicev2, sizeof(test_model_v2));
  eos_assert(res == 0, "store alice 3" );

  my_memset(&alicev2, 0, sizeof(test_model_v2));
  alicev2.name = N(alice);

  res = load_i64(current_receiver(), current_receiver(), N(test_table), &alicev2, sizeof(test_model_v2));
  eos_assert(res == sizeof(test_model_v2) && alicev2.age == 21 && alicev2.phone == 1234 && alicev2.new_field == 66655444, "alice model v2");

  my_memset(&tmp, 0, sizeof(test_model));
  tmp.name = N(bob);
  res = lower_bound_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(bob), "lower_bound_i64 bob" );

  my_memset(&tmp, 0, sizeof(test_model));
  tmp.name = N(boc);
  res = lower_bound_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(carol), "lower_bound_i64 carol" );

  my_memset(&tmp, 0, sizeof(test_model));
  tmp.name = N(dave);
  res = lower_bound_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(uint64_t) );
  eos_assert(res == sizeof(uint64_t) && tmp.name == N(dave), "lower_bound_i64 dave" );

  my_memset(&tmp, 0, sizeof(test_model));
  tmp.name = N(davf);
  res = lower_bound_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(uint64_t) );
  eos_assert(res == -1, "lower_bound_i64 fail" );

  my_memset(&tmp, 0, sizeof(test_model));
  tmp.name = N(alice);
  res = upper_bound_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.age == 15 && tmp.name == N(bob), "upper_bound_i64 bob" );

  my_memset(&tmp, 0, sizeof(test_model));
  tmp.name = N(dave);
  res = upper_bound_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == -1, "upper_bound_i64 dave" );

  test_model_v3 tmp2;
  tmp2.name = N(alice);

  res = load_i64(current_receiver(), current_receiver(), N(test_table), &tmp2, sizeof(test_model_v3));
  eos_assert(res == sizeof(test_model_v2) &&
              tmp2.age == 21 && 
              tmp2.phone == 1234 &&
              tmp2.new_field == 66655444,
              "load4update");

  tmp2.another_field = 221122;
  res = update_i64(current_receiver(), N(test_table), &tmp2, sizeof(test_model_v3));
  eos_assert(res == 1, "update_i64");

  res = load_i64(current_receiver(), current_receiver(), N(test_table), &tmp2, sizeof(test_model_v3));
  eos_assert(res == sizeof(test_model_v3) &&
              tmp2.age == 21 && 
              tmp2.phone == 1234 &&
              tmp2.new_field == 66655444 &&
              tmp2.another_field == 221122,
              "load4update");

  tmp2.age = 11;
  res = update_i64(current_receiver(), N(test_table), &tmp2,  sizeof(uint64_t)+1 );
  eos_assert(res == 1, "update_i64 small");

  res = load_i64(current_receiver(), current_receiver(), N(test_table), &tmp2, sizeof(test_model_v3));
  eos_assert(res == sizeof(test_model_v3) &&
              tmp2.age == 11 && 
              tmp2.phone == 1234 &&
              tmp2.new_field == 66655444 &&
              tmp2.another_field == 221122,
              "load_i64 update_i64");


  //Remove dummy records
  uint64_t tables[] { N(test_tabld), N(test_tablf) };
  for(auto& t : tables) {
    while( front_i64( current_receiver(), current_receiver(), t, &tmp, sizeof(test_model) ) != -1 ) {
      remove_i64(current_receiver(), t, &tmp);
    }
  }

  return WASM_TEST_PASS;
}

#endif

#if 0

unsigned int test_db::key_i64_remove_all() {
  
  uint32_t res = 0;
  uint64_t key;

  key = N(alice);
  res = remove_i64(current_receiver(), N(test_table), &key);
  eos_assert(res == 1, "remove alice");
  
  key = N(bob);
  res = remove_i64(current_receiver(),   N(test_table), &key);
  eos_assert(res == 1, "remove bob");
  
  key = N(carol);
  res = remove_i64(current_receiver(), N(test_table), &key);
  eos_assert(res == 1, "remove carol");
  
  key = N(dave);
  res = remove_i64(current_receiver(),  N(test_table), &key);
  eos_assert(res == 1, "remove dave");

  test_model tmp;
  res = front_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == -1, "front_i64 remove");

  res = back_i64( current_receiver(), current_receiver(), N(test_table), &tmp, sizeof(test_model) );
  eos_assert(res == -1, "back_i64_i64 remove");
  
  key = N(alice);
  res = remove_i64(current_receiver(), N(test_table), &key);
  eos_assert(res == 0, "remove alice 1");
  
  key = N(bob);
  res = remove_i64(current_receiver(),   N(test_table), &key);
  eos_assert(res == 0, "remove bob 1");
  
  key = N(carol);
  res = remove_i64(current_receiver(), N(test_table), &key);
  eos_assert(res == 0, "remove carol 1");
  
  key = N(dave);
  res = remove_i64(current_receiver(),  N(test_table), &key);
  eos_assert(res == 0, "remove dave 1");

 
  return WASM_TEST_PASS;
}

#endif

#if 0

unsigned int test_db::key_i64_small_load() {
  uint64_t dummy = 0;
  load_i64(current_receiver(), current_receiver(), N(just_uint64), &dummy, sizeof(uint64_t)-1);
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_small_store() {
  uint64_t dummy = 0;
  store_i64(current_receiver(), N(just_uint64), &dummy, sizeof(uint64_t)-1);
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_store_scope() {
  uint64_t dummy = 0;
  store_i64(current_receiver(), N(just_uint64), &dummy, sizeof(uint64_t));
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_remove_scope() {
  uint64_t dummy = 0;
  store_i64(current_receiver(), N(just_uint64), &dummy, sizeof(uint64_t));
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_not_found() {
  uint64_t dummy = 1000;

  auto res = load_i64(current_receiver(), current_receiver(), N(just_uint64), &dummy, sizeof(uint64_t));
  eos_assert(res == -1, "i64_not_found load");

  res = remove_i64(current_receiver(),  N(just_uint64), &dummy);
  eos_assert(res == 0, "i64_not_found remove");
  return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_front_back() {
  
  uint32_t res = 0;

  test_model dave { N(dave),  46, 6535354};
  test_model carol{ N(carol), 30, 545342453};
  store_i64(current_receiver(), N(b), &dave,  sizeof(test_model));
  store_i64(current_receiver(), N(b), &carol, sizeof(test_model));

  test_model bob  { N(bob),   15, 11932435};
  test_model alice{ N(alice), 20, 4234622};
  store_i64(current_receiver(), N(a), &bob, sizeof(test_model));
  store_i64(current_receiver(), N(a), &alice,  sizeof(test_model));

  test_model tmp;

  my_memset(&tmp, 0, sizeof(test_model));
  res = front_i64( current_receiver(), current_receiver(), N(a), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "key_i64_front 1");

  my_memset(&tmp, 0, sizeof(test_model));
  res = back_i64( current_receiver(), current_receiver(), N(a), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(bob) && tmp.age == 15 && tmp.phone == 11932435, "key_i64_front 2");

  my_memset(&tmp, 0, sizeof(test_model));
  res = front_i64( current_receiver(), current_receiver(), N(b), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(carol) && tmp.age == 30 && tmp.phone == 545342453, "key_i64_front 3");

  my_memset(&tmp, 0, sizeof(test_model));
  res = back_i64( current_receiver(), current_receiver(), N(b), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "key_i64_front 4");

  uint64_t key = N(carol);
  remove_i64(current_receiver(), N(b), &key);

  my_memset(&tmp, 0, sizeof(test_model));
  res = front_i64( current_receiver(), current_receiver(), N(b), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "key_i64_front 5");

  my_memset(&tmp, 0, sizeof(test_model));
  res = back_i64( current_receiver(), current_receiver(), N(b), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(dave) && tmp.age == 46 && tmp.phone == 6535354, "key_i64_front 6");

  my_memset(&tmp, 0, sizeof(test_model));
  res = front_i64( current_receiver(), current_receiver(), N(a), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "key_i64_front 7");

  my_memset(&tmp, 0, sizeof(test_model));
  res = back_i64( current_receiver(), current_receiver(), N(a), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(bob) && tmp.age == 15 && tmp.phone == 11932435, "key_i64_front 8");

  key = N(dave);
  remove_i64(current_receiver(), N(b), &key);
  
  res = front_i64( current_receiver(), current_receiver(), N(b), &tmp, sizeof(test_model) );
  eos_assert(res == -1, "key_i64_front 9");
  res = back_i64( current_receiver(), current_receiver(), N(b), &tmp, sizeof(test_model) );
  eos_assert(res == -1, "key_i64_front 10");

  key = N(bob);
  remove_i64(current_receiver(), N(a), &key);

  my_memset(&tmp, 0, sizeof(test_model));
  res = front_i64( current_receiver(), current_receiver(), N(a), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "key_i64_front 11");

  my_memset(&tmp, 0, sizeof(test_model));
  res = back_i64( current_receiver(), current_receiver(), N(a), &tmp, sizeof(test_model) );
  eos_assert(res == sizeof(test_model) && tmp.name == N(alice) && tmp.age == 20 && tmp.phone == 4234622, "key_i64_front 12");

  key = N(alice);
  remove_i64(current_receiver(), N(a), &key);

  res = front_i64( current_receiver(), current_receiver(), N(a), &tmp, sizeof(test_model) );
  eos_assert(res == -1, "key_i64_front 13");
  res = back_i64( current_receiver(), current_receiver(), N(a), &tmp, sizeof(test_model) );
  eos_assert(res == -1, "key_i64_front 14");

  return WASM_TEST_PASS;
}

unsigned int store_set_in_table(uint64_t table_name)
{

  uint32_t res = 0;
  
  TestModel128x2 alice0{0, 500, N(alice0), table_name};
  TestModel128x2 alice1{1, 400, N(alice1), table_name};
  TestModel128x2 alice2{2, 300, N(alice2), table_name};
  TestModel128x2 alice22{2, 200, N(alice22), table_name};

  res = store_i128i128(current_receiver(),  table_name, &alice0,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store alice0" );

  res = store_i128i128(current_receiver(),  table_name, &alice1,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store alice1" );

  res = store_i128i128(current_receiver(),  table_name, &alice2,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store alice2" );

  res = store_i128i128(current_receiver(),  table_name, &alice22,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store alice22" );

  TestModel128x2 bob0{10, 1, N(bob0), table_name};
  TestModel128x2 bob1{11, 2, N(bob1), table_name};
  TestModel128x2 bob2{12, 3, N(bob2), table_name};
  TestModel128x2 bob3{13, 4, N(bob3), table_name};

  res = store_i128i128(current_receiver(),  table_name, &bob0,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store bob0" );

  res = store_i128i128(current_receiver(),  table_name, &bob1,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store bob1" );

  res = store_i128i128(current_receiver(),  table_name, &bob2,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store bob2" );

  res = store_i128i128(current_receiver(),  table_name, &bob3,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store bob3" );

  TestModel128x2 carol0{20, 900, N(carol0), table_name};
  TestModel128x2 carol1{21, 800, N(carol1), table_name};
  TestModel128x2 carol2{22, 700, N(carol2), table_name};
  TestModel128x2 carol3{23, 600, N(carol3), table_name};

  res = store_i128i128(current_receiver(),  table_name, &carol0,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store carol0" );

  res = store_i128i128(current_receiver(),  table_name, &carol1,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store carol1" );

  res = store_i128i128(current_receiver(),  table_name, &carol2,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store carol2" );

  res = store_i128i128(current_receiver(),  table_name, &carol3,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store carol3" );

  TestModel128x2 dave0{30, 8, N(dave0), table_name};
  TestModel128x2 dave1{31, 7, N(dave1), table_name};
  TestModel128x2 dave2{32, 5, N(dave2), table_name};
  TestModel128x2 dave3{33, 4, N(dave3), table_name};

  res = store_i128i128(current_receiver(),  table_name, &dave0,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store dave0" );

  res = store_i128i128(current_receiver(),  table_name, &dave1,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store dave1" );

  res = store_i128i128(current_receiver(),  table_name, &dave2,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store dave2" );

  res = store_i128i128(current_receiver(),  table_name, &dave3,  sizeof(TestModel128x2));
  eos_assert(res == 1, "store dave3" );

  return WASM_TEST_PASS;
}

unsigned int store_set_in_table(TestModel3xi64* records, int len, uint64_t table_name) {
  uint32_t res = 0;
  for( int i = 0; i < len; ++i ) {
    TestModel3xi64 *tmp = records+i;
    tmp->table = table_name;
    res = store_i64i64i64(current_receiver(),  table_name, tmp,  sizeof(TestModel3xi64));
    eos_assert(res == 1, "store_set_in_table" );
  }
  return res;
}

#endif

#if 0

unsigned int test_db::key_i64i64i64_general() {
  
  uint32_t res = 0;

  TestModel3xi64 records[] = {
    {1, 1,  0, N()}, // 0    <---------------------------
    {1, 1,  1, N()}, // 1                               |
    {1, 2,  2, N()}, // 2    <---------------           |
    {2, 1,  3, N()}, // 3                   |           |
    {2, 2,  2, N()}, // 4    same {secondary,tertiary}  |
    {2, 2,  5, N()}, // 5                               |
    {3, 1,  6, N()}, // 6                               |
    {4, 0,  7, N()}, // 7                               |
    {4, 5,  8, N()}, // 8                               |
    {5, 1,  9, N()}, // 9                               |
    {5, 2,  0, N()}, //10    same {tertiary}-------------
  };

  store_set_in_table(records, sizeof(records)/sizeof(records[0]), N(table1));
  store_set_in_table(records, sizeof(records)/sizeof(records[0]), N(table2));
  store_set_in_table(records, sizeof(records)/sizeof(records[0]), N(table3));

  #define CALL(F, O, I, T, V) F##_##I##_##O(current_receiver(), current_receiver(), T, &V, sizeof(V))

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
      eosio::print(msg, " : ", res, " a:", V.a, " b:", V.b, " c:", V.c, " t:", V.table, "inx:", uint64_t(I), " ("); \
      eosio::print(BS(res == sizeof(V)), " ", BS(records[I].a == V.a), " ", BS(records[I].b == V.b), " ", BS(records[I].c == V.c), " => ", N(table2), ")\n"); \
    } \
    eos_assert( res == sizeof(V) && records[I].a == V.a && records[I].b == V.b && \
     records[I].c == V.c /*&& records[I].table == uint64_t(N(table2))*/, msg);

  #define LOAD_OK(I, O, T, INX, MSG) \
    {eosio::remove_reference<decltype(V)>::type tmp; my_memset(&tmp, 0, sizeof(tmp));tmp = V; \
    res = LOAD(I, O, T, tmp); \
    TABLE1_ASSERT(INX, tmp, MSG)}

  #define LOAD_ER(I, O, T, MSG) \
    {eosio::remove_reference<decltype(V)>::type tmp; my_memset(&tmp, 0, sizeof(tmp));tmp = V; \
    res = LOAD(I, O, T, tmp); \
    eos_assert(res == -1, MSG)}

  #define FRONT_OK(I, O, T, INX, MSG) \
    {eosio::remove_reference<decltype(V)>::type tmp; my_memset(&tmp, 0, sizeof(tmp));tmp = V; \
    res = FRONT(I, O, T, tmp); \
    TABLE1_ASSERT(INX, tmp, MSG)}

  #define BACK_OK(I, O, T, INX, MSG) \
    {eosio::remove_reference<decltype(V)>::type tmp; my_memset(&tmp, 0, sizeof(tmp));tmp = V; \
    res = BACK(I, O, T, tmp); \
    TABLE1_ASSERT(INX, tmp, MSG)}

  TestModel3xi64 V;

  V={0}; LOAD_ER(primary, i64i64i64, N(table2), "i64x3 LOAD primary fail 0");
  V={1}; LOAD_OK(primary, i64i64i64, N(table2), 0, "i64x3 LOAD primary 1");
  V={2}; LOAD_OK(primary, i64i64i64, N(table2), 3, "i64x3 LOAD primary 2");
  V={3}; LOAD_OK(primary, i64i64i64, N(table2), 6, "i64x3 LOAD primary 3");
  V={4}; LOAD_OK(primary, i64i64i64, N(table2), 7, "i64x3 LOAD primary 4");
  V={5}; LOAD_OK(primary, i64i64i64, N(table2), 9, "i64x3 LOAD primary 5");
  V={6}; LOAD_ER(primary, i64i64i64, N(table2), "i64x3 LOAD primary fail 6");
  
  V={11,0}; LOAD_OK(secondary, i64i64i64, N(table2), 7, "i64x3 LOAD secondary 0");
  V={11,1}; LOAD_OK(secondary, i64i64i64, N(table2), 0, "i64x3 LOAD secondary 1");
  V={11,2}; LOAD_OK(secondary, i64i64i64, N(table2),10, "i64x3 LOAD secondary 2");
  V={11,3}; LOAD_ER(secondary, i64i64i64, N(table2), "i64x3 LOAD secondary fail 3");
  V={11,4}; LOAD_ER(secondary, i64i64i64, N(table2), "i64x3 LOAD secondary fail 4");
  V={11,5}; LOAD_OK(secondary, i64i64i64, N(table2), 8, "i64x3 LOAD secondary 5");
  V={11,6}; LOAD_ER(secondary, i64i64i64, N(table2), "i64x3 LOAD secondary fail 6");

  V={11,12,0}; LOAD_OK(tertiary, i64i64i64, N(table2),  0, "i64x3 LOAD tertiary 0");
  V={11,12,1}; LOAD_OK(tertiary, i64i64i64, N(table2),  1, "i64x3 LOAD tertiary 1");
  V={11,12,2}; LOAD_OK(tertiary, i64i64i64, N(table2),  2, "i64x3 LOAD tertiary 2");
  V={11,12,3}; LOAD_OK(tertiary, i64i64i64, N(table2),  3, "i64x3 LOAD tertiary 3");
  V={11,12,4}; LOAD_ER(tertiary, i64i64i64, N(table2), "i64x3 LOAD tertiary 4");
  V={11,12,5}; LOAD_OK(tertiary, i64i64i64, N(table2),  5, "i64x3 LOAD tertiary 5");
  V={11,12,6}; LOAD_OK(tertiary, i64i64i64, N(table2),  6, "i64x3 LOAD tertiary 6");
  V={11,12,7}; LOAD_OK(tertiary, i64i64i64, N(table2),  7, "i64x3 LOAD tertiary 7");
  V={11,12,8}; LOAD_OK(tertiary, i64i64i64, N(table2),  8, "i64x3 LOAD tertiary 8");
  V={11,12,9}; LOAD_OK(tertiary, i64i64i64, N(table2),  9, "i64x3 LOAD tertiary 9");
  V={11,12,10}; LOAD_ER(tertiary, i64i64i64, N(table2), "i64x3 LOAD tertiary 10");
  V={11,12,11}; LOAD_ER(tertiary, i64i64i64, N(table2), "i64x3 LOAD tertiary fail 11");

  #define NEXT_ALL(I, O, T) \
  { \
    auto n = sizeof(I)/sizeof(I[0]); \
    auto j = 0; \
    do { \
      eosio::remove_reference<decltype(records[0])>::type tmp = records[I[j]]; \
      res = NEXT(I, i64i64i64, N(table2), tmp);\
      if(j+1<n){ TABLE1_ASSERT(I[j+1], tmp, "i64x3 NEXT " #I " ok "); } \
      else { eos_assert(res == -1, "i64x3 NEXT " #I " fail "); }\
    } while(++j<n); \
  }

  #define PREV_ALL(I, O, T) \
  { \
    auto n = sizeof(I)/sizeof(I[0]); \
    auto j = n-1; \
    do { \
      eosio::remove_reference<decltype(records[0])>::type tmp = records[I[j]]; \
      res = PREV(I, i64i64i64, N(table2), tmp);\
      if(j>0){ TABLE1_ASSERT(I[j-1], tmp, "i64x3 PREV " #I " ok "); } \
      else { eos_assert(res == -1, "i64x3 PREV " #I " fail "); }\
    } while(--j>0); \
  }

  NEXT_ALL(primary,   i64i64i64, N(table2));
  NEXT_ALL(secondary, i64i64i64, N(table2));
  NEXT_ALL(tertiary,  i64i64i64, N(table2));

  PREV_ALL(primary,   i64i64i64, N(table2));
  PREV_ALL(secondary, i64i64i64, N(table2));
  PREV_ALL(tertiary,  i64i64i64, N(table2));

  FRONT_OK(primary, i64i64i64, N(table2), primary[0],   "i64x3 FRONT primary");
  FRONT_OK(secondary, i64i64i64, N(table2), secondary[0], "i64x3 FRONT secondary");
  FRONT_OK(tertiary, i64i64i64, N(table2), tertiary[0], "i64x3 FRONT tertiary");

  BACK_OK(primary, i64i64i64, N(table2), primary[10],   "i64x3 BACK primary");
  BACK_OK(secondary, i64i64i64, N(table2), secondary[10], "i64x3 BACK secondary");
  BACK_OK(tertiary, i64i64i64, N(table2), tertiary[10], "i64x3 BACK tertiary");

  #define LOWER_ALL(I, O, T) \
  { \
    auto n = sizeof(I##_lb)/sizeof(I##_lb[0]); \
    auto j = 0; \
    do { \
      eosio::remove_reference<decltype(records[0])>::type tmp = records[j]; \
      res = LOWER(I, i64i64i64, N(table2), tmp);\
      TABLE1_ASSERT(I##_lb[j], tmp, "i64x3 LOWER " #I " ok ");\
    } while(++j<n); \
  }

  LOWER_ALL(primary,   i64i64i64, N(table2));
  LOWER_ALL(secondary, i64i64i64, N(table2));
  LOWER_ALL(tertiary,  i64i64i64, N(table2));

  #define UPPER_ALL(I, O, T) \
  { \
    auto n = sizeof(I##_ub)/sizeof(I##_ub[0]); \
    auto j = 0; \
    do { \
      eosio::remove_reference<decltype(records[0])>::type tmp = records[j]; \
      res = UPPER(I, i64i64i64, N(table2), tmp);\
      if(res == -1) { eos_assert(I##_ub[j]==-1,"i64x3 UPPER " #I " fail ") } \
      else { TABLE1_ASSERT(I##_ub[j], tmp, "i64x3 UPPER " #I " ok "); } \
    } while(++j<n); \
  }

  UPPER_ALL(primary,   i64i64i64, N(table2));
  UPPER_ALL(secondary, i64i64i64, N(table2));
  UPPER_ALL(tertiary,  i64i64i64, N(table2));

  TestModel3xi64_V2 v2;
  v2.a = records[6].a;

  res = LOAD(primary, i64i64i64, N(table2), v2);
  eos_assert(res == sizeof(TestModel3xi64), "load v2");

  v2.new_field = 555;

  res = update_i64i64i64(current_receiver(),  N(table2), &v2, sizeof(TestModel3xi64_V2));
  eos_assert(res == 1, "store v2");  

  res = LOAD(primary, i64i64i64, N(table2), v2);
  eos_assert(res == sizeof(TestModel3xi64_V2), "load v2 updated");

  res = remove_i64i64i64(current_receiver(),  N(table2), &v2);
  eos_assert(res == 1, "load v2 updated");

  res = LOAD(primary, i64i64i64, N(table2), v2);
  eos_assert(res == -1, "load not found");

  return WASM_TEST_PASS;
}

unsigned int test_db::key_i128i128_general() {

  uint32_t res = 0;

  if(store_set_in_table(N(table4)) != WASM_TEST_PASS)
    return WASM_TEST_FAIL;

  if(store_set_in_table(N(table5)) != WASM_TEST_PASS)
    return WASM_TEST_FAIL;

  if(store_set_in_table(N(table6)) != WASM_TEST_PASS)
    return WASM_TEST_FAIL;

  TestModel128x2 tmp;
  my_memset(&tmp, 0, sizeof(TestModel128x2));
  tmp.number = 21;

  res = load_primary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );

  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.price == 800 &&
               tmp.extra == N(carol1) &&
               tmp.table_name == N(table5),
              "carol1 primary load");

  my_memset(&tmp, 0, sizeof(TestModel128x2));
  tmp.price = 4;
  
  res = load_secondary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 13 &&
               tmp.price == 4 &&
               tmp.extra == N(bob3) &&
               tmp.table_name == N(table5),
              "bob3 secondary load");

  res = front_primary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 0 &&
               tmp.price == 500 &&
               tmp.extra == N(alice0) &&
               tmp.table_name == N(table5),
              "front primary load");
  
  res = previous_primary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert(res == -1, "previous primary fail");

  res = next_primary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 1 &&
               tmp.price == 400 &&
               tmp.extra == N(alice1) &&
               tmp.table_name == N(table5),
              "next primary ok");

  res = front_secondary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 10 &&
               tmp.price == 1 &&
               tmp.extra == N(bob0) &&
               tmp.table_name == N(table5),
              "front secondary ok");
  
  res = previous_secondary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert(res == -1, "previous secondary fail");

  res = next_secondary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 11 &&
               tmp.price == 2 &&
               tmp.extra == N(bob1) &&
               tmp.table_name == N(table5),
              "next secondary ok");

  res = back_primary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 33 &&
               tmp.price == 4 &&
               tmp.extra == N(dave3) &&
               tmp.table_name == N(table5),
              "back primary ok");
  
  res = next_primary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert(res == -1, "next primary fail");

  res = previous_primary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 32 &&
               tmp.price == 5 &&
               tmp.extra == N(dave2) &&
               tmp.table_name == N(table5),
              "previous primary ok");

  res = back_secondary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 20 &&
               tmp.price == 900 &&
               tmp.extra == N(carol0) &&
               tmp.table_name == N(table5),
              "back secondary ok");
  
  res = next_secondary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert(res == -1, "next secondary fail");

  res = previous_secondary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 21 &&
               tmp.price == 800 &&
               tmp.extra == N(carol1) &&
               tmp.table_name == N(table5),
              "previous secondary ok");

  tmp.number = 1;

  res = lower_bound_primary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 1 &&
               tmp.price == 400 &&
               tmp.extra == N(alice1) &&
               tmp.table_name == N(table5),
              "lb primary ok");

  res = upper_bound_primary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 2 &&
               tmp.price == 200 &&
               tmp.extra == N(alice22) &&
               tmp.table_name == N(table5),
              "ub primary ok");

  tmp.price = 800;

  res = lower_bound_secondary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp, sizeof(TestModel128x2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp.number == 21 &&
               tmp.price == 800 &&
               tmp.extra == N(carol1) &&
               tmp.table_name == N(table5),
              "lb secondary ok");

  TestModel128x2_V2 tmp2;
  tmp2.price = 800;

  res = upper_bound_secondary_i128i128( current_receiver(), current_receiver(), N(table5), &tmp2, sizeof(TestModel128x2_V2) );
  eos_assert( res == sizeof(TestModel128x2) &&
               tmp2.number == 20 &&
               tmp2.price == 900 &&
               tmp2.extra == N(carol0) &&
               tmp2.table_name == N(table5),
              "ub secondary ok");
  
  tmp2.new_field = 123456;
  res = update_i128i128(current_receiver(), N(table5), &tmp2, sizeof(TestModel128x2_V2));
  eos_assert( res == 1, "update_i128i128 ok");

  my_memset(&tmp2, 0, sizeof(TestModel128x2_V2));
  tmp2.number = 20;

  res = load_primary_i128i128(current_receiver(), current_receiver(), N(table5), &tmp2, sizeof(TestModel128x2_V2));
  eos_assert( res == sizeof(TestModel128x2_V2) &&
               tmp2.number == 20 &&
               tmp2.price == 900 &&
               tmp2.extra == N(carol0) &&
               tmp2.table_name == N(table5) &&
               tmp2.new_field == 123456,
              "lp update_i128i128 ok");

  tmp2.extra = N(xxxxx);
  res = update_i128i128(current_receiver(), N(table5), &tmp2, sizeof(uint128_t)*2+sizeof(uint64_t));
  eos_assert( res == 1, "update_i128i128 small ok");

  res = load_primary_i128i128(current_receiver(), current_receiver(), N(table5), &tmp2, sizeof(TestModel128x2_V2));
  eos_assert( res == sizeof(TestModel128x2_V2) &&
               tmp2.number == 20 &&
               tmp2.price == 900 &&
               tmp2.extra == N(xxxxx) &&
               tmp2.table_name == N(table5) &&
               tmp2.new_field == 123456,
              "lp small update_i128i128 ok");

  return WASM_TEST_PASS;
}

#endif

#if 0

void set_key_str(int i, char* key_4_digit)
{
   const char nums[] = "0123456789";
   key_4_digit[0] = nums[(i % 10000) / 1000];
   key_4_digit[1] = nums[(i % 1000) / 100];
   key_4_digit[2] = nums[(i % 100) / 10];
   key_4_digit[3] = nums[(i % 10)];
}

unsigned int test_db::key_str_setup_limit()
{
   // assuming max memory: 5 MBytes
   // assuming row overhead: 16 Bytes
   // key length: 30 bytes
   // value length: 2498 bytes
   // -> key + value bytes: 2528 Bytes
   // 1024 * 2 * (2528 + 32)
   char key[] = "0000abcdefghijklmnopqrstuvwxy";
   const uint32_t value_size = 2498;
   char* value = static_cast<char*>(eosio::malloc(value_size));
   value[4] = '\0';
   for(int i = 0; i < 1024 * 2; ++i)
   {
      set_key_str(i, key);
      // set the value with the same prefix to be able to identify
      set_key_str(i, value);
      store_str(N(dblimits), N(dblstr), key, sizeof(key), value, value_size);
   }
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_str_min_exceed_limit()
{
   char key = '1';
   char value = '1';
   // assuming max memory: 5 MBytes
   // assuming row overhead: 16 Bytes
   // key length: 1 bytes
   // value length: 1 bytes
   // -> key + value bytes: 8 Bytes
   // 8 + 32 = 40 Bytes (not enough space)
   store_str(N(dblimits), N(dblstr), &key, 1, &value, 1);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_str_under_limit()
{
   // assuming max memory: 5 MBytes
   // assuming row overhead: 16 Bytes
   // key length: 30 bytes
   // value length: 2489 bytes
   // -> key + value bytes: 2520 Bytes
   // 1024 * 2 * (2520 + 32) = 5,226,496 => 16K bytes remaining
   char key[] = "0000abcdefghijklmnopqrstuvwxy";
   const uint32_t value_size = 2489;
   char* value = static_cast<char*>(eosio::malloc(value_size));
   value[4] = '\0';
   for(int i = 0; i < 1024 * 2; ++i)
   {
      set_key_str(i, key);
      // set the value with the same prefix to be able to identify
      set_key_str(i, value);
      store_str(N(dblimits), N(dblstr), key, sizeof(key), value, value_size);
   }
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_str_available_space_exceed_limit()
{
   // key length: 30 bytes
   // value length: 16323 bytes
   // -> key + value bytes: 16360 Bytes (rounded to byte boundary)
   // 16,392 Bytes => exceeds 16K bytes remaining
   char key[] = "0000abcdefghijklmnopqrstuvwxy";
   set_key_str(9999, key);
   const uint32_t value_size = 16323;
   char* value = static_cast<char*>(eosio::malloc(value_size));
   store_str(N(dblimits), N(dblstr), key, sizeof(key), value, value_size);
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_str_another_under_limit()
{
   // 16K bytes remaining
   // key length: 30 bytes
   // value length: 18873 bytes
   // -> key + value bytes: 18904 Bytes (rounded to byte boundary)
   // 16,384 Bytes => just under 16K bytes remaining
   char key[] = "0000abcdefghijklmnopqrstuvwxy";
   set_key_str(0, key);
   uint32_t value_size = 18873;
   char* value = static_cast<char*>(eosio::malloc(value_size));
   update_str(N(dblimits), N(dblstr), key, sizeof(key), value, value_size);
   // 0 bytes remaining

   // key length: 30 bytes
   // value length: 2489 bytes
   // -> key + value bytes: 2520 Bytes
   set_key_str(1, key);
   remove_str(N(dblimits), N(dblstr), key, sizeof(key));
   // 2,552 Bytes remaining

   // leave too little room for 32 Byte overhead + (key + value = 8 Byte min)
   // key length: 30 bytes
   // value length: 4909 bytes
   // -> key + value bytes: 5040 Bytes
   value_size = 2489 + 2514;
   set_key_str(2, key);
   value = static_cast<char*>(eosio::realloc(value, value_size));
   update_str(N(dblimits), N(dblstr), key, sizeof(key), value, value_size);
   eosio::free(value);

   return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_setup_limit()
{
   // assuming max memory: 5M Bytes
   // assuming row overhead: 16 Bytes
   // key length: 8 bytes
   // value length: 315 * 8 bytes (rounded to byte boundary)
   // -> key + value bytes: 2528 Bytes
   // 1024 * 2 * (2528 + 32) = 5,242,880
   const uint64_t value_size = 315 * sizeof(uint64_t) + 1;
   auto value = (uint64_t*)eosio::malloc(value_size);
   for(int i = 0; i < 1024 * 2; ++i)
   {
      value[0] = i;
      store_i64(N(dblimits), N(dbli64), (char*)value, value_size);
   }
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_min_exceed_limit()
{
   // will allocate 8 + 32 Bytes
   // at 5M Byte limit, so cannot store anything
   uint64_t value = (uint64_t)-1;
   store_i64(N(dblimits), N(dbli64), (char*)&value, sizeof(uint64_t));
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_under_limit()
{
   // updating keys' values
   // key length: 8 bytes
   // value length: 299 * 8 bytes
   // -> key + value bytes: 2400 Bytes
   // 1024 * 2 * (2400 + 32) = 4,980,736
   const uint64_t value_size = 300 * sizeof(uint64_t);
   auto value = (uint64_t*)eosio::malloc(value_size);
   for(int i = 0; i < 1024 * 2; ++i)
   {
      value[0] = i;
      store_i64(N(dblimits), N(dbli64), (char*)value, value_size);
   }
   // 262,144 Bytes remaining
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_available_space_exceed_limit()
{
   // 262,144 Bytes remaining
   // key length: 8 bytes
   // value length: 32764 * 8 bytes
   // -> key + value bytes: 262,120 Bytes
   // storing 262,152 Bytes exceeds remaining
   const uint64_t value_size = 32765 * sizeof(uint64_t);
   auto value = (uint64_t*)eosio::malloc(value_size);
   value[0] = 1024 * 2;
   store_i64(N(dblimits), N(dbli64), (char*)value, value_size);
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i64_another_under_limit()
{
   // 262,144 Bytes remaining
   // key length: 8 bytes
   // value length: 33067 * 8 bytes (rounded to byte boundary)
   // -> key + value bytes: 264,544 Bytes
   // replacing storage bytes so 264,544 - 2400 = 262,144 Bytes (0 Bytes remaining)
   uint64_t value_size = 33067 * sizeof(uint64_t) + 7;
   auto value = (uint64_t*)eosio::malloc(value_size);
   value[0] = 15;
   update_i64(N(dblimits), N(dbli64), (char*)value, value_size);

   // 0 Bytes remaining
   // key length: 8 bytes
   // previous value length: 299 * 8 bytes
   // -> key + value bytes: 2400 Bytes
   // free up 2,432 Bytes
   value[0] = 14;
   remove_i64(N(dblimits), N(dbli64), (char*)value);

   // 2,432 Bytes remaining
   // key length: 8 bytes
   // previous value length: 294 * 8 bytes (rounded to byte boundary)
   // -> key + value bytes: 2,368 Bytes
   // 2,400 Bytes allocated
   value_size = 295 * sizeof(uint64_t) + 3;
   value = (uint64_t*)eosio::realloc(value, value_size);
   value[0] = 1024 * 2;
   store_i64(N(dblimits), N(dbli64), (char*)value, value_size);
   // 32 Bytes remaining (smallest row entry is 40 Bytes)

   eosio::free(value);

   return WASM_TEST_PASS;
}

unsigned int test_db::key_i128i128_setup_limit()
{
   // assuming max memory: 5M Bytes
   // assuming row overhead: 16 Bytes
   // keys length: 32 bytes
   // value length: 312 * 8 bytes (rounded to byte boundary)
   // -> key + value bytes: 2528 Bytes
   // 1024 * 2 * (2528 + 32) = 5,242,880
   const uint64_t value_size = 315 * sizeof(uint64_t) + 1;
   auto value = (uint128_t*)eosio::malloc(value_size);
   for(int i = 0; i < 1024 * 2; ++i)
   {
      value[0] = i;
      value[1] = value[0] + 1;
      store_i128i128(N(dblimits), N(dbli128i128), (char*)value, value_size);
   }
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i128i128_min_exceed_limit()
{
   // will allocate 32 + 32 Bytes
   // at 5M Byte limit, so cannot store anything
   const uint64_t value_size = 2 * sizeof(uint128_t);
   auto value = (uint128_t*)eosio::malloc(value_size);
   value[0] = (uint128_t)-1;
   value[1] = value[0] + 1;
   store_i128i128(N(dblimits), N(dbli128i128), (char*)&value, value_size);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i128i128_under_limit()
{
   // updating keys' values
   // keys length: 32 bytes
   // value length: 296 * 8 bytes
   // -> key + value bytes: 2400 Bytes
   // 1024 * 2 * (2400 + 32) = 4,980,736
   const uint64_t value_size = 300 * sizeof(uint64_t);
   auto value = (uint128_t*)eosio::malloc(value_size);
   for(int i = 0; i < 1024 * 2; ++i)
   {
      value[0] = i;
      value[1] = value[0] + 1;
      store_i128i128(N(dblimits), N(dbli128i128), (char*)value, value_size);
   }
   // 262,144 Bytes remaining
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i128i128_available_space_exceed_limit()
{
   // 262,144 Bytes remaining
   // keys length: 32 bytes
   // value length: 32761 * 8 bytes
   // -> key + value bytes: 262,120 Bytes
   // storing 262,152 Bytes exceeds remaining
   const uint64_t value_size = 32765 * sizeof(uint64_t);
   auto value = (uint128_t*)eosio::malloc(value_size);
   value[0] = 1024 * 2;
   value[1] = value[0] + 1;
   store_i128i128(N(dblimits), N(dbli128i128), (char*)value, value_size);
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i128i128_another_under_limit()
{
   // 262,144 Bytes remaining
   // keys length: 32 bytes
   // value length: 33064 * 8 bytes (rounded to byte boundary)
   // -> key + value bytes: 264,544 Bytes
   // replacing storage bytes so 264,544 - 2400 = 262,144 Bytes (0 Bytes remaining)
   uint64_t value_size = 33067 * sizeof(uint64_t) + 7;
   auto value = (uint128_t*)eosio::malloc(value_size);
   value[0] = 15;
   value[1] = value[0] + 1;
   update_i128i128(N(dblimits), N(dbli128i128), (char*)value, value_size);

   // 0 Bytes remaining
   // keys length: 32 bytes
   // previous value length: 296 * 8 bytes
   // -> key + value bytes: 2400 Bytes
   // free up 2,432 Bytes
   value[0] = 14;
   value[1] = value[0] + 1;
   remove_i128i128(N(dblimits), N(dbli128i128), (char*)value);

   // 2,432 Bytes remaining
   // keys length: 32 bytes
   // previous value length: 288 * 8 bytes (rounded to byte boundary)
   // -> key + value bytes: 2,344 Bytes
   // 2,376 Bytes allocated
   value_size = 292 * sizeof(uint64_t) + 3;
   value = (uint128_t*)eosio::realloc(value, value_size);
   value[0] = 1024 * 2;
   value[1] = value[0] + 1;
   store_i128i128(N(dblimits), N(dbli128i128), (char*)value, value_size);
   // 56 Bytes remaining (smallest row entry is 64 Bytes)

   eosio::free(value);

   return WASM_TEST_PASS;
}

unsigned int test_db::key_i64i64i64_setup_limit()
{
   // assuming max memory: 5M Bytes
   // assuming row overhead: 16 Bytes
   // keys length: 24 bytes
   // value length: 313 * 8 bytes (rounded to byte boundary)
   // -> key + value bytes: 2528 Bytes
   // 1024 * 2 * (2528 + 32) = 5,242,880
   const uint64_t value_size = 315 * sizeof(uint64_t) + 1;
   auto value = (uint64_t*)eosio::malloc(value_size);
   for(int i = 0; i < 1024 * 2; ++i)
   {
      value[0] = i;
      value[1] = value[0] + 1;
      value[2] = value[0] + 2;
      store_i64i64i64(N(dblimits), N(dbli64i64i64), (char*)value, value_size);
   }
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i64i64i64_min_exceed_limit()
{
   // will allocate 24 + 32 Bytes
   // at 5M Byte limit, so cannot store anything
   const uint64_t value_size = 3 * sizeof(uint64_t);
   auto value = (uint64_t*)eosio::malloc(value_size);
   value[0] = (uint64_t)-1;
   value[1] = value[0] + 1;
   value[2] = value[0] + 2;
   store_i64i64i64(N(dblimits), N(dbli64i64i64), (char*)&value, value_size);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i64i64i64_under_limit()
{
   // updating keys' values
   // keys length: 24 bytes
   // value length: 297 * 8 bytes
   // -> key + value bytes: 2400 Bytes
   // 1024 * 2 * (2400 + 32) = 4,980,736
   const uint64_t value_size = 300 * sizeof(uint64_t);
   auto value = (uint64_t*)eosio::malloc(value_size);
   for(int i = 0; i < 1024 * 2; ++i)
   {
      value[0] = i;
      value[1] = value[0] + 1;
      value[2] = value[0] + 2;
      store_i64i64i64(N(dblimits), N(dbli64i64i64), (char*)value, value_size);
   }
   // 262,144 Bytes remaining
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i64i64i64_available_space_exceed_limit()
{
   // 262,144 Bytes remaining
   // keys length: 24 bytes
   // value length: 32762 * 8 bytes
   // -> key + value bytes: 262,120 Bytes
   // storing 262,152 Bytes exceeds remaining
   const uint64_t value_size = 32765 * sizeof(uint64_t);
   auto value = (uint64_t*)eosio::malloc(value_size);
   value[0] = 1024 * 2;
   value[1] = value[0] + 1;
   value[2] = value[0] + 2;
   store_i64i64i64(N(dblimits), N(dbli64i64i64), (char*)value, value_size);
   eosio::free(value);
   return WASM_TEST_PASS;
}

unsigned int test_db::key_i64i64i64_another_under_limit()
{
   // 262,144 Bytes remaining
   // keys length: 24 bytes
   // value length: 33065 * 8 bytes (rounded to byte boundary)
   // -> key + value bytes: 264,544 Bytes
   // replacing storage bytes so 264,544 - 2400 = 262,144 Bytes (0 Bytes remaining)
   uint64_t value_size = 33067 * sizeof(uint64_t) + 7;
   auto value = (uint64_t*)eosio::malloc(value_size);
   value[0] = 15;
   value[1] = value[0] + 1;
   value[2] = value[0] + 2;
   update_i64i64i64(N(dblimits), N(dbli64i64i64), (char*)value, value_size);

   // 0 Bytes remaining
   // keys length: 24 bytes
   // previous value length: 297 * 8 bytes
   // -> key + value bytes: 2400 Bytes
   // free up 2,432 Bytes
   value[0] = 14;
   value[1] = value[0] + 1;
   value[2] = value[0] + 2;
   remove_i64i64i64(N(dblimits), N(dbli64i64i64), (char*)value);

   // 2,432 Bytes remaining
   // keys length: 24 bytes
   // previous value length: 290 * 8 bytes (rounded to byte boundary)
   // -> key + value bytes: 2,352 Bytes
   // 2,384 Bytes allocated
   value_size = 295 * sizeof(uint64_t) + 3;
   value = (uint64_t*)eosio::realloc(value, value_size);
   value[0] = 1024 * 2;
   value[1] = value[0] + 1;
   value[2] = value[0] + 2;
   store_i64i64i64(N(dblimits), N(dbli64i64i64), (char*)value, value_size);
   // 48 Bytes remaining (smallest row entry is 56 Bytes)

   eosio::free(value);

   return WASM_TEST_PASS;
}

#endif

void test_db::primary_i64_general()
{
   auto table1 = N(table1);

   int alice_itr = db_store_i64(current_receiver(), table1, current_receiver(), N(alice), "alice's info", strlen("alice's info"));
   db_store_i64(current_receiver(), table1, current_receiver(), N(bob), "bob's info", strlen("bob's info"));
   db_store_i64(current_receiver(), table1, current_receiver(), N(charlie), "charlie's info", strlen("charlies's info"));
   db_store_i64(current_receiver(), table1, current_receiver(), N(allyson), "allyson's info", strlen("allyson's info"));


   // find
   {
      uint64_t prim = 0;
      int itr_next = db_next_i64(alice_itr, &prim);
      int itr_next_expected = db_find_i64(current_receiver(), current_receiver(), table1, N(allyson));
      eosio_assert(itr_next == itr_next_expected && prim == N(allyson), "primary_i64_general - db_find_i64" );
      itr_next = db_next_i64(itr_next, &prim);
      itr_next_expected = db_find_i64(current_receiver(), current_receiver(), table1, N(bob));
      eosio_assert(itr_next == itr_next_expected && prim == N(bob), "primary_i64_general - db_next_i64");
   }

   // next
   {
      int charlie_itr = db_find_i64(current_receiver(), current_receiver(), table1, N(charlie));
      // nothing after charlie
      uint64_t prim = 0;
      int end_itr = db_next_i64(charlie_itr, &prim);
      eosio_assert(end_itr == -1, "primary_i64_general - db_next_i64");
      // prim didn't change
      eosio_assert(prim == 0, "primary_i64_general - db_next_i64");
   }

   // previous
   {
      int charlie_itr = db_find_i64(current_receiver(), current_receiver(), table1, N(charlie));
      uint64_t prim = 0;
      int itr_prev = db_previous_i64(charlie_itr, &prim);
      int itr_prev_expected = db_find_i64(current_receiver(), current_receiver(), table1, N(bob));
      eosio_assert(itr_prev == itr_prev_expected && prim == N(bob), "primary_i64_general - db_previous_i64");

      itr_prev = db_previous_i64(itr_prev, &prim);
      itr_prev_expected = db_find_i64(current_receiver(), current_receiver(), table1, N(allyson));
      eosio_assert(itr_prev == itr_prev_expected && prim == N(allyson), "primary_i64_general - db_previous_i64");

      itr_prev = db_previous_i64(itr_prev, &prim);
      itr_prev_expected = db_find_i64(current_receiver(), current_receiver(), table1, N(alice));
      eosio_assert(itr_prev == itr_prev_expected && prim == N(alice), "primary_i64_general - db_previous_i64");

      itr_prev = db_previous_i64(itr_prev, &prim);
      itr_prev_expected = -1;
      eosio_assert(itr_prev == itr_prev_expected && prim == N(alice), "primary_i64_general - db_previous_i64");
   }

   // remove
   {
      int itr = db_find_i64(current_receiver(), current_receiver(), table1, N(alice));
      eosio_assert(itr != -1, "primary_i64_general - db_find_i64");
      db_remove_i64(itr);
      itr = db_find_i64(current_receiver(), current_receiver(), table1, N(alice));
      eosio_assert(itr == -1, "primary_i64_general - db_find_i64");
   }

   // get
   {
      int itr = db_find_i64(current_receiver(), current_receiver(), table1, N(bob));
      eosio_assert(itr != -1, "");
      int buffer_len = 5;
      char value[50];
      auto len = db_get_i64(itr, value, buffer_len);
      value[buffer_len] = '\0';
      std::string s(value);
      eosio_assert(len == strlen("bob's info"), "primary_i64_general - db_get_i64");
      eosio_assert(s == "bob's", "primary_i64_general - db_get_i64");
      
      buffer_len = 20;
      db_get_i64(itr, value, buffer_len);
      value[buffer_len] = '\0';
      std::string sfull(value);
      eosio_assert(sfull == "bob's info", "primary_i64_general - db_get_i64");
   }

   // update
   {
      int itr = db_find_i64(current_receiver(), current_receiver(), table1, N(bob));
      eosio_assert(itr != -1, "");
      const char* new_value = "bob's new info";
      int new_value_len = strlen(new_value); 
      db_update_i64(itr, current_receiver(), new_value, new_value_len);
      char ret_value[50];
      auto len = db_get_i64(itr, ret_value, new_value_len);
      ret_value[new_value_len] = '\0';
      std::string sret(ret_value);
      eosio_assert(sret == "bob's new info", "primary_i64_general - db_update_i64");
   }
}

void test_db::primary_i64_lowerbound()
{
   auto table = N(mytable);
   db_store_i64(current_receiver(), table, current_receiver(), N(alice), "alice's info", strlen("alice's info"));
   db_store_i64(current_receiver(), table, current_receiver(), N(bob), "bob's info", strlen("bob's info"));
   db_store_i64(current_receiver(), table, current_receiver(), N(charlie), "charlie's info", strlen("charlies's info"));
   db_store_i64(current_receiver(), table, current_receiver(), N(emily), "emily's info", strlen("emily's info"));
   db_store_i64(current_receiver(), table, current_receiver(), N(allyson), "allyson's info", strlen("allyson's info"));
   db_store_i64(current_receiver(), table, current_receiver(), N(joe), "nothing here", strlen("nothing here"));

   const std::string err = "primary_i64_lowerbound";

   {
      int lb = db_lowerbound_i64(current_receiver(), current_receiver(), table, N(alice));
      eosio_assert(lb == db_find_i64(current_receiver(), current_receiver(), table, N(alice)), err.c_str());
   }
   {
      int lb = db_lowerbound_i64(current_receiver(), current_receiver(), table, N(billy));
      eosio_assert(lb == db_find_i64(current_receiver(), current_receiver(), table, N(bob)), err.c_str());
   }
   {
      int lb = db_lowerbound_i64(current_receiver(), current_receiver(), table, N(frank));
      eosio_assert(lb == db_find_i64(current_receiver(), current_receiver(), table, N(joe)), err.c_str());
   }
   {
      int lb = db_lowerbound_i64(current_receiver(), current_receiver(), table, N(joe));
      eosio_assert(lb == db_find_i64(current_receiver(), current_receiver(), table, N(joe)), err.c_str());
   }
   {
      int lb = db_lowerbound_i64(current_receiver(), current_receiver(), table, N(kevin));
      eosio_assert(lb == -1, err.c_str());
   }
}

void test_db::primary_i64_upperbound()
{
   auto table = N(mytable);
   const std::string err = "primary_i64_upperbound";
   {
      int ub = db_upperbound_i64(current_receiver(), current_receiver(), table, N(alice));
      eosio_assert(ub == db_find_i64(current_receiver(), current_receiver(), table, N(allyson)), err.c_str());
   }
   {
      int ub = db_upperbound_i64(current_receiver(), current_receiver(), table, N(billy));
      eosio_assert(ub == db_find_i64(current_receiver(), current_receiver(), table, N(bob)), err.c_str());
   }
   {
      int ub = db_upperbound_i64(current_receiver(), current_receiver(), table, N(frank));
      eosio_assert(ub == db_find_i64(current_receiver(), current_receiver(), table, N(joe)), err.c_str());
   }
   {
      int ub = db_upperbound_i64(current_receiver(), current_receiver(), table, N(joe));
      eosio_assert(ub == -1, err.c_str());
   }
   {
      int ub = db_upperbound_i64(current_receiver(), current_receiver(), table, N(kevin));
      eosio_assert(ub == -1, err.c_str());
   }
}

void test_db::idx64_general()
{
   const auto table = N(myindextable);

   typedef uint64_t secondary_type;

   struct record {
      uint64_t ssn;
      secondary_type name;
   };

   record records[] = {{265, N(alice)},
                       {781, N(bob)},
                       {234, N(charlie)},
                       {650, N(allyson)},
                       {540, N(bob)},
                       {976, N(emily)},
                       {110, N(joe)}
   };

   for (int i = 0; i < sizeof(records)/sizeof(records[0]); ++i) {
      db_idx64_store(current_receiver(), table, current_receiver(), records[i].ssn, &records[i].name);
   }

   // find_primary
   {
      secondary_type sec = 0;
      int itr = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec, 999);
      eosio_assert(itr == -1 && sec == 0, "idx64_general - db_idx64_find_primary");
      itr = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec, 110);
      eosio_assert(itr != -1 && sec == N(joe), "idx64_general - db_idx64_find_primary");
      uint64_t prim_next = 0;
      int itr_next = db_idx64_next(itr, &prim_next);
      eosio_assert(itr_next == -1 && prim_next == 0, "idx64_general - db_idx64_find_primary");
   }

   // iterate forward starting with charlie
   {
      secondary_type sec = 0;
      int itr = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec, 234);
      eosio_assert(itr != -1 && sec == N(charlie), "idx64_general - db_idx64_find_primary");

      uint64_t prim_next = 0;
      int itr_next = db_idx64_next(itr, &prim_next);
      eosio_assert(itr_next != -1 && prim_next == 976, "idx64_general - db_idx64_next");
      secondary_type sec_next = 0;
      int itr_next_expected = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec_next, prim_next);
      eosio_assert(itr_next == itr_next_expected && sec_next == N(emily), "idx64_general - db_idx64_next");

      itr_next = db_idx64_next(itr_next, &prim_next);
      eosio_assert(itr_next != -1 && prim_next == 110, "idx64_general - db_idx64_next");
      itr_next_expected = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec_next, prim_next);
      eosio_assert(itr_next == itr_next_expected && sec_next == N(joe), "idx64_general - db_idx64_next");

      itr_next = db_idx64_next(itr_next, &prim_next);
      eosio_assert(itr_next == -1 && prim_next == 110, "idx64_general - db_idx64_next");
   }

   // iterate backward staring with second bob
   {
      secondary_type sec = 0;
      int itr = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec, 781);
      eosio_assert(itr != -1 && sec == N(bob), "idx64_general - db_idx64_find_primary");

      uint64_t prim_prev = 0;
      int itr_prev = db_idx64_previous(itr, &prim_prev);
      eosio_assert(itr_prev != -1 && prim_prev == 540, "idx64_general - db_idx64_previous");

      secondary_type sec_prev = 0;
      int itr_prev_expected = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec_prev, prim_prev);
      eosio_assert(itr_prev == itr_prev_expected && sec_prev == N(bob), "idx64_general - db_idx64_previous");

      itr_prev = db_idx64_previous(itr_prev, &prim_prev);
      eosio_assert(itr_prev != -1 && prim_prev == 650, "idx64_general - db_idx64_previous");
      itr_prev_expected = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec_prev, prim_prev);
      eosio_assert(itr_prev == itr_prev_expected && sec_prev == N(allyson), "idx64_general - db_idx64_previous");

      itr_prev = db_idx64_previous(itr_prev, &prim_prev);
      eosio_assert(itr_prev != -1 && prim_prev == 265, "idx64_general - db_idx64_previous");
      itr_prev_expected = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec_prev, prim_prev);
      eosio_assert(itr_prev == itr_prev_expected && sec_prev == N(alice), "idx64_general - db_idx64_previous");

      itr_prev = db_idx64_previous(itr_prev, &prim_prev);
      eosio_assert(itr_prev == -1 && prim_prev == 265, "idx64_general - db_idx64_previous");
   }

   // find_secondary
   {
      uint64_t prim = 0;
      auto sec = N(bob);
      int itr = db_idx64_find_secondary(current_receiver(), current_receiver(), table, &sec, &prim);
      eosio_assert(itr != -1 && prim == 540, "idx64_general - db_idx64_find_secondary");

      sec = N(emily);
      itr = db_idx64_find_secondary(current_receiver(), current_receiver(), table, &sec, &prim);
      eosio_assert(itr != -1 && prim == 976, "idx64_general - db_idx64_find_secondary");

      sec = N(frank);
      itr = db_idx64_find_secondary(current_receiver(), current_receiver(), table, &sec, &prim);
      eosio_assert(itr == -1 && prim == 976, "idx64_general - db_idx64_find_secondary");
   }

   // update and remove
   {
      auto one_more_bob = N(bob);
      const uint64_t ssn = 421;
      int itr = db_idx64_store(current_receiver(), table, current_receiver(), ssn, &one_more_bob);
      auto new_name = N(billy);
      db_idx64_update(itr, current_receiver(), &new_name);
      secondary_type sec = 0;
      int sec_itr = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec, ssn);
      eosio_assert(sec_itr == itr && sec == new_name, "idx64_general - db_idx64_update");
      db_idx64_remove(itr);
      int itrf = db_idx64_find_primary(current_receiver(), current_receiver(), table, &sec, ssn);
      eosio_assert(itrf == -1, "idx64_general - db_idx64_remove");
   }
}

void test_db::idx64_lowerbound()
{
   const auto table = N(myindextable);
   typedef uint64_t secondary_type;
   const std::string err = "idx64_lowerbound";
   {
      secondary_type lb_sec = N(alice);
      uint64_t lb_prim = 0;
      const uint64_t ssn = 265;
      int lb = db_idx64_lowerbound(current_receiver(), current_receiver(), table, &lb_sec, &lb_prim);
      eosio_assert(lb_prim == ssn && lb_sec == N(alice), err.c_str());
      eosio_assert(lb == db_idx64_find_primary(current_receiver(), current_receiver(), table, &lb_sec, ssn), err.c_str());
   }
   {
      secondary_type lb_sec = N(billy);
      uint64_t lb_prim = 0;
      const uint64_t ssn = 540;
      int lb = db_idx64_lowerbound(current_receiver(), current_receiver(), table, &lb_sec, &lb_prim);
      eosio_assert(lb_prim == ssn && lb_sec == N(bob), err.c_str());
      eosio_assert(lb == db_idx64_find_primary(current_receiver(), current_receiver(), table, &lb_sec, ssn), err.c_str());
   }
   {
      secondary_type lb_sec = N(joe);
      uint64_t lb_prim = 0;
      const uint64_t ssn = 110;
      int lb = db_idx64_lowerbound(current_receiver(), current_receiver(), table, &lb_sec, &lb_prim);
      eosio_assert(lb_prim == ssn && lb_sec == N(joe), err.c_str());
      eosio_assert(lb == db_idx64_find_primary(current_receiver(), current_receiver(), table, &lb_sec, ssn), err.c_str());
   }
   {
      secondary_type lb_sec = N(kevin);
      uint64_t lb_prim = 0;
      int lb = db_idx64_lowerbound(current_receiver(), current_receiver(), table, &lb_sec, &lb_prim);
      eosio_assert(lb_prim == 0 && lb_sec == N(kevin), err.c_str());
      eosio_assert(lb == -1, "");
   }
}

void test_db::idx64_upperbound()
{
   const auto table = N(myindextable);
   typedef uint64_t secondary_type;
   const std::string err = "idx64_upperbound";
   {
      secondary_type ub_sec = N(alice);
      uint64_t ub_prim = 0;
      const uint64_t alice_ssn = 265, allyson_ssn = 650;
      int ub = db_idx64_upperbound(current_receiver(), current_receiver(), table, &ub_sec, &ub_prim);
      eosio_assert(ub_prim == allyson_ssn && ub_sec == N(allyson), "");
      eosio_assert(ub == db_idx64_find_primary(current_receiver(), current_receiver(), table, &ub_sec, allyson_ssn), err.c_str());
   }
   {
      secondary_type ub_sec = N(billy);
      uint64_t ub_prim = 0;
      const uint64_t bob_ssn = 540;
      int ub = db_idx64_upperbound(current_receiver(), current_receiver(), table, &ub_sec, &ub_prim);
      eosio_assert(ub_prim == bob_ssn && ub_sec == N(bob), "");
      eosio_assert(ub == db_idx64_find_primary(current_receiver(), current_receiver(), table, &ub_sec, bob_ssn), err.c_str());
   }
   {
      secondary_type ub_sec = N(joe);
      uint64_t ub_prim = 0;
      const uint64_t ssn = 110;
      int ub = db_idx64_upperbound(current_receiver(), current_receiver(), table, &ub_sec, &ub_prim);
      eosio_assert(ub_prim == 0 && ub_sec == N(joe), err.c_str());
      eosio_assert(ub == -1, err.c_str());
   }
   {
      secondary_type ub_sec = N(kevin);
      uint64_t ub_prim = 0;
      int ub = db_idx64_upperbound(current_receiver(), current_receiver(), table, &ub_sec, &ub_prim);
      eosio_assert(ub_prim == 0 && ub_sec == N(kevin), err.c_str());
      eosio_assert(ub == -1, err.c_str());
   }
}
