#pragma once

#include <eoslib/types.h>

extern "C" {
  /**
   *  @defgroup mathcapi Math C API
   *  @brief defines builtin math functions
   *  @ingroup mathapi
   */
  void multeq_i128( uint128_t* self, const uint128_t* other );
  void diveq_i128 ( uint128_t* self, const uint128_t* other );

  uint64_t double_add(uint64_t a, uint64_t b);
  uint64_t double_mult(uint64_t a, uint64_t b);
  uint64_t double_div(uint64_t a, uint64_t b);
  
  uint32_t double_lt(uint64_t a, uint64_t b);
  uint32_t double_eq(uint64_t a, uint64_t b);
  uint32_t double_gt(uint64_t a, uint64_t b);

  uint64_t double_to_i64(uint64_t a);
  uint64_t i64_to_double(uint64_t a);

} // extern "C"
