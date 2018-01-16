#include <eoslib/eos.hpp>

#include "test_api.hpp"

using namespace eosio;

unsigned int test_math::test_multeq() {
  u128_msg msg;
  auto n = read_action(&msg, sizeof(u128_msg));
  assert( n == sizeof(u128_msg), "test_multeq n == sizeof(u128_msg)" );

  uint128_t self  = *(msg.values);
  uint128_t other = *(msg.values+1);

  eosio::multeq(self, other);
  assert( self == msg.values[2], "test_multeq msg.values[0] == msg.values[2]" );
  return 0;
}

unsigned int test_math::test_diveq() {
  u128_msg msg;
  auto n = read_action(&msg, sizeof(u128_msg));
  assert( n == sizeof(u128_msg), "test_diveq n == sizeof(u128_msg)" );

  uint128_t self  = *(msg.values);
  uint128_t other = *(msg.values+1);

  eosio::diveq(self, other);
  assert( msg.values[0] == msg.values[2], "test_diveq msg.values[0] == msg.values[2]" );
  return 0;
}

unsigned int test_math::test_diveq_by_0() {
  unsigned __int128 a = 100;
  unsigned __int128 b = 0;
  eosio::diveq(a, b);
  return 0;
}
unsigned int test_math::test_double_api() {
  
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
  
  return 0;
}

unsigned int test_math::test_double_api_div_0() {
  
  double_div( i64_to_double(1), 
    double_add( 
      i64_to_double(-5), i64_to_double(5) 
    )
  );

  return 0;
}
