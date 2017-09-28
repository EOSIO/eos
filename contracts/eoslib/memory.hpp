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

   class memory_manager
   {
   friend void* malloc(uint32_t size);
   friend void* realloc(void* ptr, uint32_t size);
   friend void free(void* ptr);
   public:
      memory_manager()
      : _current_heap(0)
      {
      }

   private:
      void* malloc(uint32_t size)
      {
         // first pass of loop never has to initialize the slot in _available_heap
         uint32_t needs_init = 0;
         char* buffer = nullptr;
         uint32_t current_heap = _current_heap;
         for (;current_heap < HEAPS_SIZE; ++current_heap)
         {
            memory& current = _available_heaps[current_heap];
            if(!current.is_init())
            {
               char* new_heap = nullptr;
               if (current_heap == 0)
               {
                  memset(_initial_heap, 0, sizeof(_initial_heap));
                  current.init(_initial_heap, INITIAL_HEAP_SIZE);
               }
               else
               {
                  // REMOVE logic, just using to test out multiple heap logic till memory can be allocated
                  char* const new_heap = &_initial_heap[INITIAL_HEAP_SIZE + NEW_HEAP_SIZE * (current_heap - 1)];
                  _available_heaps[current_heap].init(new_heap, NEW_HEAP_SIZE);
               }
            }
            buffer = current.malloc(size);
            if (buffer != nullptr)
               break;
         }

         // only update the current_heap if memory was allocated
         if (buffer != nullptr)
         {
            _current_heap = current_heap;
         }

         return buffer;
      }

      void* realloc(void* ptr, uint32_t size)
      {
         char* realloc_ptr = nullptr;
         uint32_t orig_ptr_size = 0;
         if (ptr != nullptr)
         {
            char* const char_ptr = static_cast<char*>(ptr);
            for (memory* realloc_heap = _available_heaps; realloc_heap < _available_heaps + HEAPS_SIZE && realloc_heap->is_init(); ++realloc_heap)
            {
               if (realloc_heap->is_in_heap(char_ptr))
               {
                  realloc_ptr = realloc_heap->realloc_in_place(char_ptr, size, &orig_ptr_size);

                  if (realloc_ptr != nullptr)
                     return realloc_ptr;
                  else
                     break;
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

      class memory
      {
      public:
         memory()
         : _heap_size(0)
         , _heap(nullptr)
         , _offset(0)
         {
         }

         void init(char* const mem_heap, uint32_t size)
         {
            _heap_size = size;
            _heap = mem_heap;
            _offset = 0;
            memset(_heap, 0, _heap_size);
         }

         uint32_t is_init() const
         {
            return _heap != nullptr;
         }

         uint32_t is_in_heap(const char* const ptr) const
         {
            const char* const END_OF_BUFFER = _heap + _heap_size;
            const char* const FIRST_PTR_OF_BUFFER = _heap + SIZE_MARKER;
            return ptr >= FIRST_PTR_OF_BUFFER && ptr < END_OF_BUFFER;
         }

         uint32_t is_capacity_remaining() const
         {
            return _offset + SIZE_MARKER < _heap_size;
         }

         char* malloc(uint32_t size)
         {
            if (_offset + size + SIZE_MARKER > _heap_size || size == 0)
            {
               return nullptr;
            }

            buffer_ptr new_buff(&_heap[_offset + SIZE_MARKER], size);
            _offset += size + SIZE_MARKER;
            return new_buff.ptr();
         }

         char* realloc_in_place(char* const ptr, uint32_t size, uint32_t* orig_ptr_size)
         {
            const char* const END_OF_BUFFER = _heap + _heap_size;

            buffer_ptr orig_buffer(ptr);
            *orig_ptr_size = orig_buffer.size();
            // is the passed in pointer valid
            char* const orig_buffer_end = orig_buffer.end();
            if (orig_buffer_end > END_OF_BUFFER)
            {
               *orig_ptr_size = 0;
               return nullptr;
            }

            if (ptr > END_OF_BUFFER - size)
            {
               // cannot resize in place
               return nullptr;
            }

            const int32_t diff = size - *orig_ptr_size;
            if (diff < 0)
            {
               memset(orig_buffer_end + diff, 0, -diff);
               // if ptr was the last allocated buffer, we can contract
               if (orig_buffer_end == &_heap[_offset])
               {
                  _offset += diff;
               }
               // else current implementation doesn't worry about freeing excess memory

               return ptr;
            }
            // if ptr was the last allocated buffer, we can expand
            else if (orig_buffer_end == &_heap[_offset])
            {
               orig_buffer.size(size);
               _offset += diff;

               return ptr;
            }
            else if (diff == 0)
            {
               return ptr;
            }

            // could not resize in place
            return nullptr;
         }

         void free(char* )
         {
            // currently no-op
         }

      private:
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

         uint32_t _heap_size;
         char* _heap;
         uint32_t _offset;
      };

      static const uint32_t SIZE_MARKER = sizeof(uint32_t);
      static const uint32_t INITIAL_HEAP_SIZE = 8192;//32768;
      static const uint32_t NEW_HEAP_SIZE = 1024; // should be 65536
      static const uint32_t HEAPS_SIZE = 4; // _initial_heap plus 3 pages (64K each)
      // should be just INITIAL_HEAP_SIZE, adding extra space for testing multiple buffers
      char _initial_heap[INITIAL_HEAP_SIZE + NEW_HEAP_SIZE * (HEAPS_SIZE - 1)];
      memory _available_heaps[HEAPS_SIZE];
      uint32_t _current_heap;
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
