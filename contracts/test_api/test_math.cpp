#include <eoslib/types.hpp>
#include <eoslib/message.hpp>
#include <eoslib/math.hpp>

#include "test_api.hpp"

unsigned int test_math::test_multeq_i128() {
  u128_msg msg;
  auto n = readMessage(&msg, sizeof(u128_msg));
  WASM_ASSERT( n == sizeof(u128_msg), "test_multeq_i128 n == sizeof(u128_msg)" );
  multeq_i128(msg.values, msg.values+1);
  WASM_ASSERT( msg.values[0] == msg.values[2], "test_multeq_i128 msg.values[0] == msg.values[2]" );
  return WASM_TEST_PASS;
}

unsigned int test_math::test_diveq_i128() {
  u128_msg msg;
  auto n = readMessage(&msg, sizeof(u128_msg));
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
