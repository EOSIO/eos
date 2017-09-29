#pragma once

#include <eoslib/types.h>

/**
 *  @defgroup memoryapi Memory API
 *  @brief Defines common memory functions
 *  @ingroup contractdev
 */

extern "C" {
  /**
   *  @defgroup memorycapi Memory C API
   *  @brief Defines common memory functions
   *  @ingroup memoryapi
   *
   *  @{
   */

 /**
   * Allocate page(s) of memory to accommodate the
   * requested number of bytes.
   * @brief Allocate page memory
   * @param num_bytes  Number of bytes to add.
   *
   * @return void pointer to the previous end of allocated bytes
   *
   * Example:
   * @code
   * // allocate a whole new page, the returned offset is the pointer to the
   * // newly allocated page
   * char* new_page = static_cast<char*>(sbrk(65536));
   * memset(new_page, 0, 65536);
   * @endcode
   */
  void* sbrk( uint32_t num_bytes );

 /**
  * Copy a block of memory from source to destination.
  * @brief Copy a block of memory from source to destination.
  * @param destination  Pointer to the destination to copy to.
  * @param source       Pointer to the source for copy from.
  * @param num          Number of bytes to copy.
  *
  * @return the destination pointer
  *
  * Example:
  * @code
  * char dest[6] = { 0 };
  * char source[6] = { 'H', 'e', 'l', 'l', 'o', '\0' };
  * memcpy(dest, source, 6 * sizeof(char));
  * prints(dest); // Output: Hello
  * @endcode
  */
  void* memcpy( void* destination, const void* source, uint32_t num );

  /**
   * Fill block of memory.
   * @brief Fill a block of memory with the provided value.
   * @param ptr    Pointer to memory to fill.
   * @param value  Value to set (it is passed as an int but converted to unsigned char).
   * @param num    Number of bytes to be set to the value.
   *
   * @return the destination pointer
   *
   * Example:
   * @code
   * char ptr[6] = { 'H', 'e', 'l', 'l', 'o', '\0' };
   * memset(ptr, 'y', 6 * sizeof(char));
   * prints(ptr); // Output: yyyyyy
   * @endcode
   */
   void* memset( void* ptr, uint32_t value, uint32_t num );
  /// @}
} // extern "C"
