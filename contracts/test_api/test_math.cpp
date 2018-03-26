#include <eosiolib/types.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/math.hpp>

#include "test_api.hpp"

void test_math::test_multeq() {
  u128_action act;
  auto n = read_action_data(&act, sizeof(u128_action));
  eosio_assert( n == sizeof(u128_action), "test_multeq n == sizeof(u128_action)" );

  uint128_t self  = *(act.values);
  uint128_t other = *(act.values+1);
  eosio::multeq(self, other);
  eosio_assert( self == act.values[2], "test_multeq act.values[0] == act.values[2]" );
}

void test_math::test_diveq() {
  u128_action act;
  auto n = read_action_data(&act, sizeof(u128_action));
  eosio_assert( n == sizeof(u128_action), "test_diveq n == sizeof(u128_action)" );

  uint128_t self  = *(act.values);
  uint128_t other = *(act.values+1);

  eosio::diveq(self, other);
  eosio_assert( self == act.values[2], "test_diveq act.values[0] == act.values[2]" );
}

void test_math::test_diveq_by_0() {
  unsigned __int128 a = 100;
  unsigned __int128 b = 0;
  eosio::diveq(a, b);
}
  

void test_math::test_i64_to_double()
{
   uint64_t i[4];
   read_action_data(&i, sizeof(i));

   uint64_t d = i64_to_double(2);
   eosio_assert(i[0] == d, "test_i64_to_double i[0] == d");

   d = i64_to_double(uint64_t(-2));
   eosio_assert(i[1] == d, "test_i64_to_double i[1] == d");

   d = i64_to_double(100000);
   eosio_assert(i[2] == d, "test_i64_to_double i[2] == d");

   d = i64_to_double(uint64_t(-100000));
   eosio_assert(i[3] == d, "test_i64_to_double i[3] == d");

   d = i64_to_double(0);
   eosio_assert(0 == d, "test_i64_to_double 0 == d");
}

void test_math::test_double_to_i64()
{
   uint64_t d[4];
   read_action_data(&d, sizeof(d));
   
   int64_t i = (int64_t)double_to_i64(d[0]);
   eosio_assert(2 == i, "test_double_to_i64 2 == i");

   i = (int64_t)double_to_i64(d[1]);
   eosio_assert(-2 == i, "test_double_to_i64 -2 == i");

   i = (int64_t)double_to_i64(d[2]);
   eosio_assert(100000 == i, "test_double_to_i64 100000 == i");

   i = (int64_t)double_to_i64(d[3]);
   eosio_assert(-100000 == i, "test_double_to_i64 -100000 == i");

   i = (int64_t)double_to_i64(0);
   eosio_assert(0 == i, "test_double_to_i64 0 == i");
}

void test_math::test_double_api() {
  
  uint64_t res = double_mult( 
    double_div( i64_to_double(2), i64_to_double(7) ),
    double_add( i64_to_double(3), i64_to_double(2) )
  );
  
  eosio_assert( double_to_i64(res) == 1, "double funcs");

  res = double_eq(
    double_div( i64_to_double(5), i64_to_double(9) ),
    double_div( i64_to_double(5), i64_to_double(9) )
  );

  eosio_assert(res == 1, "double_eq");

  res = double_gt(
    double_div( i64_to_double(9999999), i64_to_double(7777777) ),
    double_div( i64_to_double(9999998), i64_to_double(7777777) )
  );

  eosio_assert(res == 1, "double_gt");

  res = double_lt(
    double_div( i64_to_double(9999998), i64_to_double(7777777) ),
    double_div( i64_to_double(9999999), i64_to_double(7777777) )
  );

  eosio_assert(res == 1, "double_lt");
}

void test_math::test_double_api_div_0() {
    double_div( i64_to_double(1),
          double_add( 
          i64_to_double((uint64_t)-5), i64_to_double(5) 
    ));

   eosio_assert(false, "should've thrown an error");
}
