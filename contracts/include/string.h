/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <stdint.h>

extern "C" {
 /**
  * Copy a block of memory from source to destination.
  * Source and destination blocks may not overlap.
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
  * Copy a block of memory from source to destination.
  * Source and destination blocks may overlap.
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
  void* memmove( void* destination, const void* source, uint32_t num );

  /**
   * Compare block of memory from source to destination.
   * @brief Copy a block of memory from source to destination.
   * @param ptr1       Pointer to first data to compare
   * @param ptr2       Pointer to second data to compare
   * @param num        Number of bytes to compare.
   *
   * @return the destination pointer
   *
   */
   int32_t memcmp( void* ptr1, const void* ptr2, uint32_t num );


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

   //Not implemented yet:

   char* strchr(const char *s, int c);

   char* strrchr(const char *s, int c);

   char* strpbrk(const char *s, const char *charset);

   void* memchr(const void *s, int c, size_t n);

   char* strstr(const char *haystack, const char *needle);

   //char* strcasestr(const char *haystack, const char *needle);

   //char* strnstr(const char *haystack, const char *needle, size_t len);
}

