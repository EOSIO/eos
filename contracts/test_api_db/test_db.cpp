#include <eosiolib/types.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/db.h>
#include <eosiolib/memory.hpp>
#include <eosiolib/fixed_key.hpp>
#include "../test_api/test_api.hpp"

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
    while(size--) { *(ptr++)=(char)val; }
  }
  uint32_t my_strlen(const char *str) {
     uint32_t len = 0;
     while(str[len]) ++len;
     return len;
  }
  bool my_memcmp(void *s1, void *s2, uint32_t n) {
     unsigned char *c1 = (unsigned char*)s1;
     unsigned char *c2 = (unsigned char*)s2;
     for (uint32_t i = 0; i < n; i++) {
        if (c1[i] != c2[i]) {
           return false;
        }
     }
     return true;
  }
}

void test_db::primary_i64_general(uint64_t receiver, uint64_t code, uint64_t action)
{
   (void)code; (void)action;
   auto table1 = N(table1);

   int alice_itr = db_store_i64(receiver, table1, receiver, N(alice), "alice's info", strlen("alice's info"));
   db_store_i64(receiver, table1, receiver, N(bob), "bob's info", strlen("bob's info"));
   db_store_i64(receiver, table1, receiver, N(charlie), "charlie's info", strlen("charlies's info"));
   db_store_i64(receiver, table1, receiver, N(allyson), "allyson's info", strlen("allyson's info"));


   // find
   {
      uint64_t prim = 0;
      int itr_next = db_next_i64(alice_itr, &prim);
      int itr_next_expected = db_find_i64(receiver, receiver, table1, N(allyson));
      eosio_assert(itr_next == itr_next_expected && prim == N(allyson), "primary_i64_general - db_find_i64" );
      itr_next = db_next_i64(itr_next, &prim);
      itr_next_expected = db_find_i64(receiver, receiver, table1, N(bob));
      eosio_assert(itr_next == itr_next_expected && prim == N(bob), "primary_i64_general - db_next_i64");
   }

   // next
   {
      int charlie_itr = db_find_i64(receiver, receiver, table1, N(charlie));
      // nothing after charlie
      uint64_t prim = 0;
      int end_itr = db_next_i64(charlie_itr, &prim);
      eosio_assert(end_itr < 0, "primary_i64_general - db_next_i64");
      // prim didn't change
      eosio_assert(prim == 0, "primary_i64_general - db_next_i64");
   }

   // previous
   {
      int charlie_itr = db_find_i64(receiver, receiver, table1, N(charlie));
      uint64_t prim = 0;
      int itr_prev = db_previous_i64(charlie_itr, &prim);
      int itr_prev_expected = db_find_i64(receiver, receiver, table1, N(bob));
      eosio_assert(itr_prev == itr_prev_expected && prim == N(bob), "primary_i64_general - db_previous_i64");

      itr_prev = db_previous_i64(itr_prev, &prim);
      itr_prev_expected = db_find_i64(receiver, receiver, table1, N(allyson));
      eosio_assert(itr_prev == itr_prev_expected && prim == N(allyson), "primary_i64_general - db_previous_i64");

      itr_prev = db_previous_i64(itr_prev, &prim);
      itr_prev_expected = db_find_i64(receiver, receiver, table1, N(alice));
      eosio_assert(itr_prev == itr_prev_expected && prim == N(alice), "primary_i64_general - db_previous_i64");

      itr_prev = db_previous_i64(itr_prev, &prim);
      eosio_assert(itr_prev < 0 && prim == N(alice), "primary_i64_general - db_previous_i64");
   }

   // remove
   {
      int itr = db_find_i64(receiver, receiver, table1, N(alice));
      eosio_assert(itr >= 0, "primary_i64_general - db_find_i64");
      db_remove_i64(itr);
      itr = db_find_i64(receiver, receiver, table1, N(alice));
      eosio_assert(itr < 0, "primary_i64_general - db_find_i64");
   }

   // get
   {
      int itr = db_find_i64(receiver, receiver, table1, N(bob));
      eosio_assert(itr >= 0, "");
      uint32_t buffer_len = 5;
      char value[50];
      auto len = db_get_i64(itr, value, buffer_len);
      value[buffer_len] = '\0';
      std::string s(value);
      eosio_assert(uint32_t(len) == buffer_len, "primary_i64_general - db_get_i64");
      eosio_assert(s == "bob's", "primary_i64_general - db_get_i64  - 5");

      buffer_len = 20;
      len = db_get_i64(itr, value, 0);
      len = db_get_i64(itr, value, len);
      value[len] = '\0';
      std::string sfull(value);
      eosio_assert(sfull == "bob's info", "primary_i64_general - db_get_i64 - full");
   }

   // update
   {
      int itr = db_find_i64(receiver, receiver, table1, N(bob));
      eosio_assert(itr >= 0, "");
      const char* new_value = "bob's new info";
      uint32_t new_value_len = strlen(new_value);
      db_update_i64(itr, receiver, new_value, new_value_len);
      char ret_value[50];
      db_get_i64(itr, ret_value, new_value_len);
      ret_value[new_value_len] = '\0';
      std::string sret(ret_value);
      eosio_assert(sret == "bob's new info", "primary_i64_general - db_update_i64");
   }
}

void test_db::primary_i64_lowerbound(uint64_t receiver, uint64_t code, uint64_t action)
{
   (void)code;(void)action;
   auto table = N(mytable);
   db_store_i64(receiver, table, receiver, N(alice), "alice's info", strlen("alice's info"));
   db_store_i64(receiver, table, receiver, N(bob), "bob's info", strlen("bob's info"));
   db_store_i64(receiver, table, receiver, N(charlie), "charlie's info", strlen("charlies's info"));
   db_store_i64(receiver, table, receiver, N(emily), "emily's info", strlen("emily's info"));
   db_store_i64(receiver, table, receiver, N(allyson), "allyson's info", strlen("allyson's info"));
   db_store_i64(receiver, table, receiver, N(joe), "nothing here", strlen("nothing here"));

   const std::string err = "primary_i64_lowerbound";

   {
      int lb = db_lowerbound_i64(receiver, receiver, table, N(alice));
      eosio_assert(lb == db_find_i64(receiver, receiver, table, N(alice)), err.c_str());
   }
   {
      int lb = db_lowerbound_i64(receiver, receiver, table, N(billy));
      eosio_assert(lb == db_find_i64(receiver, receiver, table, N(bob)), err.c_str());
   }
   {
      int lb = db_lowerbound_i64(receiver, receiver, table, N(frank));
      eosio_assert(lb == db_find_i64(receiver, receiver, table, N(joe)), err.c_str());
   }
   {
      int lb = db_lowerbound_i64(receiver, receiver, table, N(joe));
      eosio_assert(lb == db_find_i64(receiver, receiver, table, N(joe)), err.c_str());
   }
   {
      int lb = db_lowerbound_i64(receiver, receiver, table, N(kevin));
      eosio_assert(lb < 0, err.c_str());
   }
}

void test_db::primary_i64_upperbound(uint64_t receiver, uint64_t code, uint64_t action)
{
   (void)code;(void)action;
   auto table = N(mytable);
   const std::string err = "primary_i64_upperbound";
   {
      int ub = db_upperbound_i64(receiver, receiver, table, N(alice));
      eosio_assert(ub == db_find_i64(receiver, receiver, table, N(allyson)), err.c_str());
   }
   {
      int ub = db_upperbound_i64(receiver, receiver, table, N(billy));
      eosio_assert(ub == db_find_i64(receiver, receiver, table, N(bob)), err.c_str());
   }
   {
      int ub = db_upperbound_i64(receiver, receiver, table, N(frank));
      eosio_assert(ub == db_find_i64(receiver, receiver, table, N(joe)), err.c_str());
   }
   {
      int ub = db_upperbound_i64(receiver, receiver, table, N(joe));
      eosio_assert(ub < 0, err.c_str());
   }
   {
      int ub = db_upperbound_i64(receiver, receiver, table, N(kevin));
      eosio_assert(ub < 0, err.c_str());
   }
}

void test_db::idx64_general(uint64_t receiver, uint64_t code, uint64_t action)
{
   (void)code;(void)action;
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

   for (uint32_t i = 0; i < sizeof(records)/sizeof(records[0]); ++i) {
      db_idx64_store(receiver, table, receiver, records[i].ssn, &records[i].name);
   }

   // find_primary
   {
      secondary_type sec = 0;
      int itr = db_idx64_find_primary(receiver, receiver, table, &sec, 999);
      eosio_assert(itr < 0 && sec == 0, "idx64_general - db_idx64_find_primary");
      itr = db_idx64_find_primary(receiver, receiver, table, &sec, 110);
      eosio_assert(itr >= 0 && sec == N(joe), "idx64_general - db_idx64_find_primary");
      uint64_t prim_next = 0;
      int itr_next = db_idx64_next(itr, &prim_next);
      eosio_assert(itr_next < 0 && prim_next == 0, "idx64_general - db_idx64_find_primary");
   }

   // iterate forward starting with charlie
   {
      secondary_type sec = 0;
      int itr = db_idx64_find_primary(receiver, receiver, table, &sec, 234);
      eosio_assert(itr >= 0 && sec == N(charlie), "idx64_general - db_idx64_find_primary");

      uint64_t prim_next = 0;
      int itr_next = db_idx64_next(itr, &prim_next);
      eosio_assert(itr_next >= 0 && prim_next == 976, "idx64_general - db_idx64_next");
      secondary_type sec_next = 0;
      int itr_next_expected = db_idx64_find_primary(receiver, receiver, table, &sec_next, prim_next);
      eosio_assert(itr_next == itr_next_expected && sec_next == N(emily), "idx64_general - db_idx64_next");

      itr_next = db_idx64_next(itr_next, &prim_next);
      eosio_assert(itr_next >= 0 && prim_next == 110, "idx64_general - db_idx64_next");
      itr_next_expected = db_idx64_find_primary(receiver, receiver, table, &sec_next, prim_next);
      eosio_assert(itr_next == itr_next_expected && sec_next == N(joe), "idx64_general - db_idx64_next");

      itr_next = db_idx64_next(itr_next, &prim_next);
      eosio_assert(itr_next < 0 && prim_next == 110, "idx64_general - db_idx64_next");
   }

   // iterate backward staring with second bob
   {
      secondary_type sec = 0;
      int itr = db_idx64_find_primary(receiver, receiver, table, &sec, 781);
      eosio_assert(itr >= 0 && sec == N(bob), "idx64_general - db_idx64_find_primary");

      uint64_t prim_prev = 0;
      int itr_prev = db_idx64_previous(itr, &prim_prev);
      eosio_assert(itr_prev >= 0 && prim_prev == 540, "idx64_general - db_idx64_previous");

      secondary_type sec_prev = 0;
      int itr_prev_expected = db_idx64_find_primary(receiver, receiver, table, &sec_prev, prim_prev);
      eosio_assert(itr_prev == itr_prev_expected && sec_prev == N(bob), "idx64_general - db_idx64_previous");

      itr_prev = db_idx64_previous(itr_prev, &prim_prev);
      eosio_assert(itr_prev >= 0 && prim_prev == 650, "idx64_general - db_idx64_previous");
      itr_prev_expected = db_idx64_find_primary(receiver, receiver, table, &sec_prev, prim_prev);
      eosio_assert(itr_prev == itr_prev_expected && sec_prev == N(allyson), "idx64_general - db_idx64_previous");

      itr_prev = db_idx64_previous(itr_prev, &prim_prev);
      eosio_assert(itr_prev >= 0 && prim_prev == 265, "idx64_general - db_idx64_previous");
      itr_prev_expected = db_idx64_find_primary(receiver, receiver, table, &sec_prev, prim_prev);
      eosio_assert(itr_prev == itr_prev_expected && sec_prev == N(alice), "idx64_general - db_idx64_previous");

      itr_prev = db_idx64_previous(itr_prev, &prim_prev);
      eosio_assert(itr_prev < 0 && prim_prev == 265, "idx64_general - db_idx64_previous");
   }

   // find_secondary
   {
      uint64_t prim = 0;
      auto sec = N(bob);
      int itr = db_idx64_find_secondary(receiver, receiver, table, &sec, &prim);
      eosio_assert(itr >= 0 && prim == 540, "idx64_general - db_idx64_find_secondary");

      sec = N(emily);
      itr = db_idx64_find_secondary(receiver, receiver, table, &sec, &prim);
      eosio_assert(itr >= 0 && prim == 976, "idx64_general - db_idx64_find_secondary");

      sec = N(frank);
      itr = db_idx64_find_secondary(receiver, receiver, table, &sec, &prim);
      eosio_assert(itr < 0 && prim == 976, "idx64_general - db_idx64_find_secondary");
   }

   // update and remove
   {
      auto one_more_bob = N(bob);
      const uint64_t ssn = 421;
      int itr = db_idx64_store(receiver, table, receiver, ssn, &one_more_bob);
      auto new_name = N(billy);
      db_idx64_update(itr, receiver, &new_name);
      secondary_type sec = 0;
      int sec_itr = db_idx64_find_primary(receiver, receiver, table, &sec, ssn);
      eosio_assert(sec_itr == itr && sec == new_name, "idx64_general - db_idx64_update");
      db_idx64_remove(itr);
      int itrf = db_idx64_find_primary(receiver, receiver, table, &sec, ssn);
      eosio_assert(itrf < 0, "idx64_general - db_idx64_remove");
   }
}

void test_db::idx64_lowerbound(uint64_t receiver, uint64_t code, uint64_t action)
{
   (void)code;(void)action;
   const auto table = N(myindextable);
   typedef uint64_t secondary_type;
   const std::string err = "idx64_lowerbound";
   {
      secondary_type lb_sec = N(alice);
      uint64_t lb_prim = 0;
      const uint64_t ssn = 265;
      int lb = db_idx64_lowerbound(receiver, receiver, table, &lb_sec, &lb_prim);
      eosio_assert(lb_prim == ssn && lb_sec == N(alice), err.c_str());
      eosio_assert(lb == db_idx64_find_primary(receiver, receiver, table, &lb_sec, ssn), err.c_str());
   }
   {
      secondary_type lb_sec = N(billy);
      uint64_t lb_prim = 0;
      const uint64_t ssn = 540;
      int lb = db_idx64_lowerbound(receiver, receiver, table, &lb_sec, &lb_prim);
      eosio_assert(lb_prim == ssn && lb_sec == N(bob), err.c_str());
      eosio_assert(lb == db_idx64_find_primary(receiver, receiver, table, &lb_sec, ssn), err.c_str());
   }
   {
      secondary_type lb_sec = N(joe);
      uint64_t lb_prim = 0;
      const uint64_t ssn = 110;
      int lb = db_idx64_lowerbound(receiver, receiver, table, &lb_sec, &lb_prim);
      eosio_assert(lb_prim == ssn && lb_sec == N(joe), err.c_str());
      eosio_assert(lb == db_idx64_find_primary(receiver, receiver, table, &lb_sec, ssn), err.c_str());
   }
   {
      secondary_type lb_sec = N(kevin);
      uint64_t lb_prim = 0;
      int lb = db_idx64_lowerbound(receiver, receiver, table, &lb_sec, &lb_prim);
      eosio_assert(lb_prim == 0 && lb_sec == N(kevin), err.c_str());
      eosio_assert(lb < 0, "");
   }
}

void test_db::idx64_upperbound(uint64_t receiver, uint64_t code, uint64_t action)
{
   (void)code;(void)action;
   const auto table = N(myindextable);
   typedef uint64_t secondary_type;
   const std::string err = "idx64_upperbound";
   {
      secondary_type ub_sec = N(alice);
      uint64_t ub_prim = 0;
      const uint64_t allyson_ssn = 650;
      int ub = db_idx64_upperbound(receiver, receiver, table, &ub_sec, &ub_prim);
      eosio_assert(ub_prim == allyson_ssn && ub_sec == N(allyson), "");
      eosio_assert(ub == db_idx64_find_primary(receiver, receiver, table, &ub_sec, allyson_ssn), err.c_str());
   }
   {
      secondary_type ub_sec = N(billy);
      uint64_t ub_prim = 0;
      const uint64_t bob_ssn = 540;
      int ub = db_idx64_upperbound(receiver, receiver, table, &ub_sec, &ub_prim);
      eosio_assert(ub_prim == bob_ssn && ub_sec == N(bob), "");
      eosio_assert(ub == db_idx64_find_primary(receiver, receiver, table, &ub_sec, bob_ssn), err.c_str());
   }
   {
      secondary_type ub_sec = N(joe);
      uint64_t ub_prim = 0;
      int ub = db_idx64_upperbound(receiver, receiver, table, &ub_sec, &ub_prim);
      eosio_assert(ub_prim == 0 && ub_sec == N(joe), err.c_str());
      eosio_assert(ub < 0, err.c_str());
   }
   {
      secondary_type ub_sec = N(kevin);
      uint64_t ub_prim = 0;
      int ub = db_idx64_upperbound(receiver, receiver, table, &ub_sec, &ub_prim);
      eosio_assert(ub_prim == 0 && ub_sec == N(kevin), err.c_str());
      eosio_assert(ub < 0, err.c_str());
   }
}

void test_db::test_invalid_access(uint64_t receiver, uint64_t code, uint64_t action)
{
   (void)code;(void)action;
   auto act = eosio::get_action(1, 0);
   auto ia = eosio::unpack<invalid_access_action>(act.data);
   uint64_t scope = N(access);
   uint64_t table = scope;
   uint64_t pk    = scope;

   int32_t itr = -1;
   uint64_t value;
   switch( ia.index ) {
      case 1:
         itr = db_idx64_find_primary(ia.code, scope, table, &value, pk);
      break;
      case 0:
      default:
         itr = db_find_i64(ia.code, scope, table, pk);
      break;
   }
   if( ia.store ) {
      uint64_t value_to_store = ia.val;
      if( itr < 0 ) {
         switch( ia.index ) {
            case 1:
               db_idx64_store( scope, table, receiver, pk, &value_to_store );
            break;
            case 0:
            default:
               db_store_i64( scope, table, receiver, pk, &value_to_store, sizeof(value_to_store) );
            break;
         }
      } else {
         switch( ia.index ) {
            case 1:
               db_idx64_update( itr, receiver, &value_to_store);
            break;
            case 0:
            default:
               db_update_i64( itr, receiver, &value_to_store, sizeof(value_to_store) );
            break;
         }
      }
      //eosio::print("test_invalid_access: stored ", value_to_store, "\n");
   } else {
      eosio_assert( itr >= 0, "test_invalid_access: could not find row" );
      switch( ia.index ) {
         case 1:
         break;
         case 0:
         default:
            eosio_assert( db_get_i64( itr, &value, sizeof(value) ) == sizeof(value),
                          "test_invalid_access: value in primary table was incorrect size" );
         break;
      }
      //eosio::print("test_invalid_access: expected ", ia.val, " and retrieved ", value, "\n");
      eosio_assert( value == ia.val, "test_invalid_access: value did not match" );
   }
}

void test_db::idx_double_nan_create_fail(uint64_t receiver, uint64_t, uint64_t) {
   double x = 0.0;
   x = x / x; // create a NaN
   db_idx_double_store( N(nan), N(nan), receiver, 0, &x); // should fail
}

void test_db::idx_double_nan_modify_fail(uint64_t receiver, uint64_t, uint64_t) {
   double x = 0.0;
   db_idx_double_store( N(nan), N(nan), receiver, 0, &x);
   auto itr = db_idx_double_find_primary(receiver, N(nan), N(nan), &x, 0);
   x = 0.0;
   x = x / x; // create a NaN
   db_idx_double_update(itr, 0, &x); // should fail
}

void test_db::idx_double_nan_lookup_fail(uint64_t receiver, uint64_t, uint64_t) {
   auto act = eosio::get_action(1, 0);
   auto lookup_type = eosio::unpack<uint32_t>(act.data);

   uint64_t pk;
   double x = 0.0;
   db_idx_double_store( N(nan), N(nan), receiver, 0, &x);
   x = x / x; // create a NaN
   switch( lookup_type ) {
      case 0: // find
         db_idx_double_find_secondary(receiver, N(nan), N(nan), &x, &pk);
      break;
      case 1: // lower bound
         db_idx_double_lowerbound(receiver, N(nan), N(nan), &x, &pk);
      break;
      case 2: // upper bound
         db_idx_double_upperbound(receiver, N(nan), N(nan), &x, &pk);
      break;
      default:
         eosio_assert( false, "idx_double_nan_lookup_fail: unexpected lookup_type" );
   }
}

void test_db::misaligned_secondary_key256_tests(uint64_t receiver, uint64_t, uint64_t) {
   auto key = eosio::key256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL);
   char* ptr = (char*)(&key);
   ptr += 1;
   // test that store doesn't crash on unaligned data
   db_idx256_store( N(testapi), N(testtable), N(testapi), 1, (eosio::key256*)(ptr), 2 );
   // test that find_primary doesn't crash on unaligned data
   db_idx256_find_primary( N(testapi), N(testtable), N(testapi), (eosio::key256*)(ptr), 2, 0);
}
