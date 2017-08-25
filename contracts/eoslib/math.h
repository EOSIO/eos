#pragma once

#include <eoslib/types.h>

extern "C" {
  /**
   *  @defgroup mathcapi Math C API
   *  @brief defines builtin math functions
   *  @ingroup mathapi
   *
   *  @{
   */

 /**
  * Multiply two 128 bit unsigned integers and assign the value to the first parameter.
  * @brief Multiply two 128 unsigned bit integers
  * @param self  Value to be multiplied. It will be replaced with the result.
  * @param other Value to be multiplied.
  */
  void multeq_i128( uint128_t* self, const uint128_t* other );
  /**
   * Divide two 128 bit unsigned integers and assign the value to the first parameter.
   * It will throw an exception if other is zero.
   * @brief Divide two 128 unsigned bit integers
   * @param self  Numerator. It will be replaced with the result
   * @param other Denominator
   */
  void diveq_i128 ( uint128_t* self, const uint128_t* other );

  /**
   * Get the result of addition between two double interpreted as 64 bit unsigned integer
   * This function will first reinterpret_cast both inputs to double (50 decimal digit precision), add them together, and reinterpret_cast the result back to 64 bit unsigned integer.
   * @brief Addition between two double
   * @param a Value in double interpreted as 64 bit unsigned integer
   * @param b Value in double interpreted as 64 bit unsigned integer
   * @return Result of addition reinterpret_cast to 64 bit unsigned integers
   */
  uint64_t double_add(uint64_t a, uint64_t b);

  /**
   * Get the result of multiplication between two double interpreted as 64 bit unsigned integer
   * This function will first reinterpret_cast both inputs to double (50 decimal digit precision), multiply them together, and reinterpret_cast the result back to 64 bit unsigned integer.
   * @brief Multiplication between two double
   * @param a Value in double interpreted as 64 bit unsigned integer
   * @param b Value in double interpreted as 64 bit unsigned integer
   * @return Result of multiplication reinterpret_cast to 64 bit unsigned integers
   */
  uint64_t double_mult(uint64_t a, uint64_t b);

  /**
   * Get the result of division between two double interpreted as 64 bit unsigned integer
   * This function will first reinterpret_cast both inputs to double (50 decimal digit precision), divide numerator with denominator, and reinterpret_cast the result back to 64 bit unsigned integer.
   * Throws an error if b is zero (after it is reinterpret_cast to double)
   * @brief Division between two double
   * @param a Numerator in double interpreted as 64 bit unsigned integer
   * @param b Denominator in double interpreted as 64 bit unsigned integer
   * @return Result of division reinterpret_cast to 64 bit unsigned integers
   */
  uint64_t double_div(uint64_t a, uint64_t b);

  /**
   * Get the result of less than comparison between two double
   * This function will first reinterpret_cast both inputs to double (50 decimal digit precision) before doing the less than comparison.
   * @brief Less than comparison between two double
   * @param a Value in double interpreted as 64 bit unsigned integer
   * @param b Value in double interpreted as 64 bit unsigned integer
   * @return 1 if first input is smaller than second input, 0 otherwise
   */
  uint32_t double_lt(uint64_t a, uint64_t b);

  /**
   * Get the result of equality check between two double
   * This function will first reinterpret_cast both inputs to double (50 decimal digit precision) before doing equality check.
   * @brief Equality check between two double
   * @param a Value in double interpreted as 64 bit unsigned integer
   * @param b Value in double interpreted as 64 bit unsigned integer
   * @return 1 if first input is equal to second input, 0 otherwise
   */
  uint32_t double_eq(uint64_t a, uint64_t b);

  /**
   * Get the result of greater than comparison between two double
   * This function will first reinterpret_cast both inputs to double (50 decimal digit precision) before doing the greater than comparison.
   * @brief Greater than comparison between two double
   * @param a Value in double interpreted as 64 bit unsigned integer
   * @param b Value in double interpreted as 64 bit unsigned integer
   * @return 1 if first input is greater than second input, 0 otherwise
   */
  uint32_t double_gt(uint64_t a, uint64_t b);

  /**
   * Convert double (interpreted as 64 bit unsigned  integer) to 64 bit unsigned integer.
   * This function will first reinterpret_cast the input to double (50 decimal digit precision) then convert it to double, then reinterpret_cast it to 64 bit unsigned integer.
   * @brief Convert double to 64 bit unsigned integer
   * @param self Value in double interpreted as 64 bit unsigned integer
   * @return Result of conversion in 64 bit unsigned integer
   */
  uint64_t double_to_i64(uint64_t a);

  /**
   * Convert 64 bit unsigned integer to double (interpreted as 64 bit unsigned integer).
   * This function will convert the input to double (50 decimal digit precision) then reinterpret_cast it to 64 bit unsigned integer.
   * @brief Convert 64 bit unsigned integer to double (interpreted as 64 bit unsigned  integer)
   * @param self Value to be converted
   * @return Result of conversion in double (interpreted as 64 bit unsigned integer)
   */
  uint64_t i64_to_double(uint64_t a);

  /// @}
} // extern "C"
