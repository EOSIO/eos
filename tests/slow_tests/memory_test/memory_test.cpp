#include "memory_test.hpp" /// defines transfer struct (abi)
#include <eoslib/message.h>
//#include <eoslib/types.hpp>
#include <eoslib/memory.hpp>

extern "C" {
    void init()
    {
    }

    void verify_empty(const void* const ptr, const uint32_t size)
    {
       const char* char_ptr = (const char*)ptr;
       for (uint32_t i = 0; i < size; ++i)
       {
          assert((uint32_t)char_ptr[i] == 0, "buffer slot not empty");
       }
    }

    void test()
    {
       char* ptr1 = (char*)eos::malloc(20);
       assert(ptr1 != nullptr, "should have allocated a buffer");
       verify_empty(ptr1, 20);
       char* ptr1_realloc = (char*)eos::realloc(ptr1, 30);
       assert(ptr1_realloc != nullptr, "should have returned a buffer");
       assert(ptr1_realloc == ptr1, "should have just enlarged the buffer");
       char* ptr2 = (char*)eos::malloc(20);
       assert(ptr2 != nullptr, "should have allocated a buffer");
       assert(ptr1 + 30 < ptr2, "should have been created after ptr1"); // test specific to implementation (can remove for refactor)
       verify_empty(ptr1, 30);
       assert(ptr1[30] != 0, "should not have empty bytes following since block allocated"); // test specific to implementation (can remove for refactor)

       //shrink the buffer
       ptr1[14] = 0x7e;
       ptr1[29] = 0x7f;
       ptr1_realloc = (char*)eos::realloc(ptr1, 15);
       assert(ptr1_realloc != nullptr, "should have returned a buffer");
       assert(ptr1_realloc == ptr1, "should have just shrunk the buffer");
       assert(ptr1[14] == 0x7e, "remaining portion of buffer should be untouched");
       verify_empty(ptr1 + 15, 15); // test specific to implementation (can remove for refactor)

       //same size the buffer (verify corner case
       ptr1_realloc = (char*)eos::realloc(ptr1, 15);
       assert(ptr1_realloc != nullptr, "should have returned a buffer");
       assert(ptr1_realloc == ptr1, "should have same buffer");
       assert(ptr1[14] == 0x7e, "remaining portion of buffer should be untouched");

       //same size as max allocated buffer -- test specific to implementation (can remove for refactor)
       ptr1_realloc = (char*)eos::realloc(ptr1, 30);
       assert(ptr1_realloc != nullptr, "should have returned a buffer");
       assert(ptr1_realloc == ptr1, "should have just increased the buffer back to original max"); //test specific to implementation (can remove for refactor)
       assert(ptr1[14] == 0x7e, "remaining portion of buffer should be untouched");

       //increase buffer beyond allocated space
       ptr1[29] = 0x7f;
       ptr1_realloc = (char*)eos::realloc(ptr1, 31);
       assert(ptr1_realloc != nullptr, "should have returned a buffer");
       assert(ptr1_realloc != ptr1, "should have had to reallocate the buffer");
       assert(ptr2 < ptr1_realloc, "should have been created after ptr2"); // test specific to implementation (can remove for refactor)
       assert(ptr1_realloc[14] == 0x7e, "original buffers content should be copied");
       assert(ptr1_realloc[29] == 0x7f, "original buffers content should be copied");

       //realloc with nullptr
       char* nullptr_realloc = (char*)eos::realloc(nullptr, 50);
       assert(nullptr_realloc != nullptr, "should have returned a buffer");
       assert(ptr1_realloc < nullptr_realloc, "should have been created after ptr1_realloc"); // test specific to implementation (can remove for refactor)

       //realloc with invalid ptr
       char* invalid_ptr_realloc = (char*)eos::realloc(nullptr_realloc + 4, 10);
       assert(invalid_ptr_realloc != nullptr, "should have returned a buffer");
       assert(nullptr_realloc < invalid_ptr_realloc, "should have been created after ptr2"); // test specific to implementation (can remove for refactor)
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action )
    {
       if( code == N(currency) )
       {
          if( action == N(transfer) )
          {
             test();
          }
       }
    }
}
