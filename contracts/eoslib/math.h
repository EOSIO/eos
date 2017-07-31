#pragma once

extern "C" {
  /**
   *  @defgroup mathcapi Math C API
   *  @brief defines builtin math functions
   *  @ingroup mathapi
   */
  void multeq_i128( uint128_t* self, const uint128_t* other );
  void diveq_i128 ( uint128_t* self, const uint128_t* other );
} // extern "C"
