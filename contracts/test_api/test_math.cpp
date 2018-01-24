#include <eoslib/eos.hpp>
#include <eoslib/print.hpp>

#include "test_api.hpp"

using namespace eosio;

void test_math::test_multeq() {
  u128_action act;
  auto n = read_action(&act, sizeof(u128_action));
  assert( n == sizeof(u128_action), "test_multeq n == sizeof(u128_action)" );

  uint128_t self  = *(act.values);
  uint128_t other = *(act.values+1);
  eosio::multeq(self, other);
  assert( self == act.values[2], "test_multeq act.values[0] == act.values[2]" );
}

void test_math::test_diveq() {
  u128_action act;
  auto n = read_action(&act, sizeof(u128_action));
  assert( n == sizeof(u128_action), "test_diveq n == sizeof(u128_action)" );

  uint128_t self  = *(act.values);
  uint128_t other = *(act.values+1);

  eosio::diveq(self, other);
  assert( self == act.values[2], "test_diveq act.values[0] == act.values[2]" );
}

void test_math::test_diveq_by_0() {
  unsigned __int128 a = 100;
  unsigned __int128 b = 0;
  eosio::diveq(a, b);
}
  

void test_math::test_i64_to_double()
{
   uint64_t i[4];
   read_action(&i, sizeof(i));

   uint64_t d = i64_to_double(2);
   assert(i[0] == d, "test_i64_to_double i[0] == d");

   d = i64_to_double(-2);
   assert(i[1] == d, "test_i64_to_double i[1] == d");

   d = i64_to_double(100000);
   assert(i[2] == d, "test_i64_to_double i[2] == d");

   d = i64_to_double(-100000);
   assert(i[3] == d, "test_i64_to_double i[3] == d");

   d = i64_to_double(0);
   assert(0 == d, "test_i64_to_double 0 == d");
}

void test_math::test_double_to_i64()
{
   uint64_t d[4];
   read_action(&d, sizeof(d));
   
   int64_t i = double_to_i64(d[0]);
   assert(2 == i, "test_double_to_i64 2 == i");

   i = double_to_i64(d[1]);
   assert(-2 == i, "test_double_to_i64 -2 == i");

   i = double_to_i64(d[2]);
   assert(100000 == i, "test_double_to_i64 100000 == i");

   i = double_to_i64(d[3]);
   assert(-100000 == i, "test_double_to_i64 -100000 == i");

   i = double_to_i64(0);
   assert(0 == i, "test_double_to_i64 0 == i");
}

void test_math::test_double_api() {
  
  uint64_t res = double_mult( 
    double_div( i64_to_double(2), i64_to_double(7) ),
    double_add( i64_to_double(3), i64_to_double(2) )
  );
  
  assert( double_to_i64(res) == 1, "double funcs");

  res = double_eq(
    double_div( i64_to_double(5), i64_to_double(9) ),
    double_div( i64_to_double(5), i64_to_double(9) )
  );

  assert(res == 1, "double_eq");

  res = double_gt(
    double_div( i64_to_double(9999999), i64_to_double(7777777) ),
    double_div( i64_to_double(9999998), i64_to_double(7777777) )
  );

  assert(res == 1, "double_gt");

  res = double_lt(
    double_div( i64_to_double(9999998), i64_to_double(7777777) ),
    double_div( i64_to_double(9999999), i64_to_double(7777777) )
  );

  assert(res == 1, "double_lt");
}

void test_math::test_double_api_div_0() {
    double_div( i64_to_double(1),
          double_add( 
      i64_to_double(-5), i64_to_double(5) 
    ));

   assert(false, "should've thrown an error");
}
