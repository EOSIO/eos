#pragma once

#include <eoslib/memory.h>
#include <eoslib/print.hpp>

namespace eos {

   using ::memset;
   using ::memcpy;

  /**
   *  @defgroup memorycppapi Memory C++ API
   *  @brief Defines common memory functions
   *  @ingroup memoryapi
   *
   *  @{
   */

   class memory
   {
      friend void* malloc(uint32_t size);
      friend void* realloc(void* ptr, uint32_t size);
      friend void free(void* ptr);

   public:
      memory()
      : _offset(0)
      {
         memset(_initial_heap, 0, sizeof(_initial_heap));
      }

   private:
      void* malloc(uint32_t size)
      {
         if (_offset + size + SIZE_MARKER > INITIAL_HEAP_SIZE || size == 0)
            return nullptr;

         buffer_ptr new_buff(&_initial_heap[_offset + SIZE_MARKER], size);
         _offset += size + SIZE_MARKER;
         return new_buff.ptr();
      }

      void* realloc(void* ptr, uint32_t size)
      {
         uint32_t orig_ptr_size = 0;
         const char* const END_OF_BUFFER = _initial_heap + INITIAL_HEAP_SIZE;
         char* const char_ptr = static_cast<char*>(ptr);
         if (ptr != nullptr)
         {
            buffer_ptr orig_buffer(ptr);
            if (orig_buffer.size_ptr() >= _initial_heap && ptr < END_OF_BUFFER)
            {
               orig_ptr_size = orig_buffer.size();
               // is the passed in pointer valid
               char* const orig_buffer_end = orig_buffer.end();
               if (orig_buffer_end < END_OF_BUFFER)
               {
                  // is there enough memory to allocate new buffer
                  if (ptr >= END_OF_BUFFER - size)
                  {
                     // not handling in current implementation
                     return nullptr;
                  }

                  const int32_t diff = size - orig_ptr_size;
                  if (diff < 0)
                  {
                     memset(orig_buffer_end + diff, 0, -diff);
                     // if ptr was the last allocated buffer, we can contract
                     if (orig_buffer_end == &_initial_heap[_offset])
                     {
                        _offset += diff;
                     }
                     // else current implementation doesn't worry about freeing excess memory

                     return ptr;
                  }
                  // if ptr was the last allocated buffer, we can expand
                  else if (orig_buffer_end == &_initial_heap[_offset])
                  {
                     orig_buffer.size(size);
                     _offset += diff;

                     return ptr;
                  }
                  else if (diff == 0)
                     return ptr;
               }
               else
               {
                  orig_ptr_size = 0;
               }
            }
         }

         char* new_alloc = static_cast<char*>(malloc(size));

         const uint32_t copy_size = (size < orig_ptr_size) ? size : orig_ptr_size;
         if (copy_size > 0)
         {
            memcpy(new_alloc, ptr, copy_size);
            free (ptr);
         }

         return new_alloc;
      }

      void free(void* )
      {
         // currently no-op
      }

      class buffer_ptr
      {
      public:
         buffer_ptr(void* ptr)
         : _ptr(static_cast<char*>(ptr))
         , _size(*(uint32_t*)(static_cast<char*>(ptr) - SIZE_MARKER))
         {
         }

         buffer_ptr(void* ptr, uint32_t buff_size)
         : _ptr(static_cast<char*>(ptr))
         {
            size(buff_size);
         }

         const void* size_ptr()
         {
            return _ptr - SIZE_MARKER;
         }

         uint32_t size()
         {
            return _size;
         }

         void size(uint32_t val)
         {
            *reinterpret_cast<uint32_t*>(_ptr - SIZE_MARKER) = val;
            _size = val;
         }

         char* end()
         {
            return _ptr + _size;
         }

         char* ptr()
         {
            return _ptr;
         }
      private:

         char* const _ptr;
         uint32_t _size;
      };

      static const uint32_t SIZE_MARKER = sizeof(uint32_t);
      static const uint32_t INITIAL_HEAP_SIZE = 8192;//32768;
      char _initial_heap[INITIAL_HEAP_SIZE];
      uint32_t _offset;
   } memory_heap;

  /**
   * Allocate a block of memory.
   * @brief Allocate a block of memory.
   * @param size  Size of memory block
   *
   * Example:
   * @code
   * uint64_t* int_buffer = malloc(500 * sizeof(uint64_t));
   * @endcode
   */
   inline void* malloc(uint32_t size)
   {
      return memory_heap.malloc(size);
   }

   /**
    * Allocate a block of memory.
    * @brief Allocate a block of memory.
    * @param size  Size of memory block
    *
    * Example:
    * @code
    * uint64_t* int_buffer = malloc(500 * sizeof(uint64_t));
    * ...
    * uint64_t* bigger_int_buffer = realloc(int_buffer, 600 * sizeof(uint64_t));
    * @endcode
    */

   inline void* realloc(void* ptr, uint32_t size)
   {
      return memory_heap.realloc(ptr, size);
   }

   /**
    * Free a block of memory.
    * @brief Free a block of memory.
    * @param ptr  Pointer to memory block to free.
    *
    * Example:
    * @code
    * uint64_t* int_buffer = malloc(500 * sizeof(uint64_t));
    * ...
    * free(int_buffer);
    * @endcode
    */
    inline void free(void* ptr)
    {
       return memory_heap.free(ptr);
    }
   /// @} /// mathcppapi
}
