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
//  double_div( i64_to_double(1), i64_to_double(0));
    double_add( 
      i64_to_double(-5), i64_to_double(5) 
    );
}
