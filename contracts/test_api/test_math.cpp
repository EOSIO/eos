#include <eoslib/types.hpp>
#include <eoslib/message.hpp>
#include <eoslib/math.hpp>

#include "test_api.hpp"

unsigned int test_math::test_multeq_i128() {
  u128_msg msg;
  auto n = read_message(&msg, sizeof(u128_msg));
  WASM_ASSERT( n == sizeof(u128_msg), "test_multeq_i128 n == sizeof(u128_msg)" );
  multeq_i128(msg.values, msg.values+1);
  WASM_ASSERT( msg.values[0] == msg.values[2], "test_multeq_i128 msg.values[0] == msg.values[2]" );
  return WASM_TEST_PASS;
}

unsigned int test_math::test_diveq_i128() {
  u128_msg msg;
  auto n = read_message(&msg, sizeof(u128_msg));
  WASM_ASSERT( n == sizeof(u128_msg), "test_diveq_i128 n == sizeof(u128_msg)" );
  diveq_i128(msg.values, msg.values+1);
  WASM_ASSERT( msg.values[0] == msg.values[2], "test_diveq_i128 msg.values[0] == msg.values[2]" );
  return WASM_TEST_PASS;
}

unsigned int test_math::test_diveq_i128_by_0() {
  unsigned __int128 a = 100;
  unsigned __int128 b = 0;
  diveq_i128(&a, &b);
  return WASM_TEST_PASS;
}

unsigned int test_math::test_double_api() {
  
  uint64_t res = double_mult( 
    double_div( i64_to_double(2), i64_to_double(7) ),
    double_add( i64_to_double(3), i64_to_double(2) )
  );
  
  WASM_ASSERT( double_to_i64(res) == 1, "double funcs");

  res = double_eq(
    double_div( i64_to_double(5), i64_to_double(9) ),
    double_div( i64_to_double(5), i64_to_double(9) )
  );

  WASM_ASSERT(res == 1, "double_eq");

  res = double_gt(
    double_div( i64_to_double(9999999), i64_to_double(7777777) ),
    double_div( i64_to_double(9999998), i64_to_double(7777777) )
  );

  WASM_ASSERT(res == 1, "double_gt");

  res = double_lt(
    double_div( i64_to_double(9999998), i64_to_double(7777777) ),
    double_div( i64_to_double(9999999), i64_to_double(7777777) )
  );

  WASM_ASSERT(res == 1, "double_lt");
  
  return WASM_TEST_PASS;
}

unsigned int test_math::test_double_api_div_0() {
  
  double_div( i64_to_double(1), 
    double_add( 
      i64_to_double(-5), i64_to_double(5) 
    )
  );

  return WASM_TEST_PASS;
}
