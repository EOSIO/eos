/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
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
         if (size == 0)
            return nullptr;

         eos::print("malloc ", size, "\n");
         adjust_to_mem_block(size);

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
                  current.init(_initial_heap, INITIAL_HEAP_SIZE);
               }
               else
               {
                  // REMOVE logic, just using to test out multiple heap logic till memory can be allocated
                  char* const new_heap = &_initial_heap[INITIAL_HEAP_SIZE + NEW_HEAP_SIZE * (current_heap - 1)];
                  current.init(new_heap, NEW_HEAP_SIZE);
               }
            }
            buffer = current.malloc(size);
            if (buffer != nullptr)
               break;
         }

         // only update the _current_heap if memory was allocated
         if (buffer != nullptr)
         {
            // make sure skipped heap memory is handled
            for (;_current_heap < current_heap; ++_current_heap)
            {
               _available_heaps[_current_heap].cleanup_remaining();
            }
         }

         eos::print("malloc done\n");
         return buffer;
      }

      void* realloc(void* ptr, uint32_t size)
      {
         if (size == 0)
         {
            free(ptr);
            return nullptr;
         }

         eos::print("realloc ", size, "\n");
         const uint32_t REMOVE = size;
         adjust_to_mem_block(size);
         eos::print("allocating ", size, "(", REMOVE, ")\n");

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
                  {
                     eos::print("realloc (in_place) done\n");
                     return realloc_ptr;
                  }
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

         eos::print("realloc (copy) done\n");
         return new_alloc;
      }

      void free(void* ptr)
      {
         if (ptr == nullptr)
            return;

         char* const char_ptr = static_cast<char*>(ptr);
         for (memory* free_heap = _available_heaps; free_heap < _available_heaps + HEAPS_SIZE && free_heap->is_init(); ++free_heap)
         {
            if (free_heap->is_in_heap(char_ptr))
            {
               free_heap->free(char_ptr);
               break;
            }
         }
      }

      void adjust_to_mem_block(uint32_t& size)
      {
         const uint32_t remainder = (size + SIZE_MARKER) & REM_MEM_BLOCK_MASK;
         eos::print("adjust_to_mem_block ", size, " + ", SIZE_MARKER, " remainder=", remainder, "\n");
         if (remainder > 0)
         {
            size += MEM_BLOCK - remainder;
            eos::print("adjust_to_mem_block ", size, "\n");
         }
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
            uint32_t used_up_size = _offset + size + SIZE_MARKER;
            if (used_up_size > _heap_size)
            {
               return nullptr;
            }

            buffer_ptr new_buff(&_heap[_offset + SIZE_MARKER], size, _heap + _heap_size);
            _offset += size + SIZE_MARKER;
            new_buff.mark_alloc();
            return new_buff.ptr();
         }

         char* realloc_in_place(char* const ptr, uint32_t size, uint32_t* orig_ptr_size)
         {
            const char* const END_OF_BUFFER = _heap + _heap_size;

            buffer_ptr orig_buffer(ptr, END_OF_BUFFER);
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
               // use a buffer_ptr to allocate the memory to free
               char* const new_ptr = ptr + size + SIZE_MARKER;
               buffer_ptr excess_to_free(new_ptr, -diff, _heap + _heap_size);
               excess_to_free.mark_free();

               return ptr;
            }
            // if ptr was the last allocated buffer, we can expand
            else if (orig_buffer_end == &_heap[_offset])
            {
               orig_buffer.size(size);
               _offset += diff;

               return ptr;
            }
            if (-diff == 0)
               return ptr;

            if (!orig_buffer.merge_contiguous_if_available(size))
               // could not resize in place
               return nullptr;

            return ptr;
         }

         void free(char* ptr)
         {
            buffer_ptr to_free(ptr, _heap + _heap_size);
            to_free.mark_free();
         }

         void cleanup_remaining()
         {
            if (_offset == _heap_size)
               return;

            // take the remaining memory and act like it was allocated
            const uint32_t size = _heap_size - _offset - SIZE_MARKER;
            buffer_ptr new_buff(&_heap[_offset + SIZE_MARKER], size, _heap + _heap_size);
            _offset = _heap_size;
            new_buff.mark_free();
         }

      private:
         class buffer_ptr
         {
         public:
            buffer_ptr(void* ptr, const char* const heap_end)
            : _ptr(static_cast<char*>(ptr))
            , _size(*reinterpret_cast<uint32_t*>(static_cast<char*>(ptr) - SIZE_MARKER) & ~ALLOC_MEMORY_MASK)
            , _heap_end(heap_end)
            {
            }

            buffer_ptr(void* ptr, uint32_t buff_size, const char* const heap_end)
            : _ptr(static_cast<char*>(ptr))
            , _heap_end(heap_end)
            {
               size(buff_size);
            }

            uint32_t size()
            {
               return _size;
            }

            void size(uint32_t val)
            {
               // keep the same state (allocated or free) as was set before
               const uint32_t memory_state = *reinterpret_cast<uint32_t*>(_ptr - SIZE_MARKER) & ALLOC_MEMORY_MASK;
               *reinterpret_cast<uint32_t*>(_ptr - SIZE_MARKER) = val | memory_state;
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

            void mark_alloc()
            {
               *reinterpret_cast<uint32_t*>(_ptr - SIZE_MARKER) |= ALLOC_MEMORY_MASK;
            }

            void mark_free()
            {
               eos::print("mark_free ptr=", (uint64_t)_ptr, "\n");
               *reinterpret_cast<uint32_t*>(_ptr - SIZE_MARKER) &= ~ALLOC_MEMORY_MASK;
            }

            bool is_alloc() const
            {
               return *reinterpret_cast<const uint32_t*>(_ptr - SIZE_MARKER) | ALLOC_MEMORY_MASK;
            }

            char* merge_contiguous_if_available(uint32_t needed_size)
            {
               return merge_contiguous(needed_size, true);
            }

            char* merge_contiguous(uint32_t needed_size)
            {
               return merge_contiguous(needed_size, false);
            }
         private:
            char* merge_contiguous(uint32_t needed_size, bool all_or_nothing)
            {
               // do not bother if there isn't contiguious space to allocate
               if (all_or_nothing && _heap_end - _ptr < needed_size)
                  return nullptr;

               uint32_t possible_size = _size;
               while (possible_size < needed_size  && (_ptr + possible_size < _heap_end))
               {
                  const uint32_t next_mem_flag_size = *reinterpret_cast<const uint32_t*>(_ptr + possible_size);
                  // if ALLOCed then done with contiguous free memory
                  if (next_mem_flag_size | ALLOC_MEMORY_MASK)
                     break;

                  possible_size += (next_mem_flag_size & ~ALLOC_MEMORY_MASK) + SIZE_MARKER;
               }

               if (all_or_nothing && possible_size < needed_size)
                  return nullptr;

               // combine
               const uint32_t new_size = possible_size < needed_size ? possible_size : needed_size;
               size(new_size);

               if (possible_size > needed_size)
               {
                  const uint32_t freed_size = possible_size - needed_size - SIZE_MARKER;
                  buffer_ptr freed_remainder(_ptr + needed_size + SIZE_MARKER, freed_size, _heap_end);
                  freed_remainder.mark_free();
               }

               return new_size == needed_size ? _ptr : nullptr;
            }

            char* _ptr;
            uint32_t _size;
            const char* const _heap_end;
         };

         uint32_t _heap_size;
         char* _heap;
         uint32_t _offset;
      };

      static const uint32_t SIZE_MARKER = sizeof(uint32_t);
      // allocate memory in 8 char blocks
      static const uint32_t MEM_BLOCK = 8;
      static const uint32_t REM_MEM_BLOCK_MASK = MEM_BLOCK - 1;
      static const uint32_t INITIAL_HEAP_SIZE = 8192;//32768;
      static const uint32_t NEW_HEAP_SIZE = 1024; // should be 65536
      static const uint32_t HEAPS_SIZE = 4; // _initial_heap plus 3 pages (64K each)
      // should be just INITIAL_HEAP_SIZE, adding extra space for testing multiple buffers
      char _initial_heap[INITIAL_HEAP_SIZE + NEW_HEAP_SIZE * (HEAPS_SIZE - 1)];
      memory _available_heaps[HEAPS_SIZE];
      uint32_t _current_heap;
      static const uint32_t ALLOC_MEMORY_MASK = 1 << 31;
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
