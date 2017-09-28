#include <eoslib/message.h>
//#include <eoslib/types.hpp>
#include <eoslib/memory.hpp>

extern "C" {
    void init()
    {
    }

    void verify(const void* const ptr, const uint32_t val, const uint32_t size)
    {
       const char* char_ptr = (const char*)ptr;
       for (uint32_t i = 0; i < size; ++i)
       {
          assert(static_cast<uint32_t>(static_cast<unsigned char>(char_ptr[i])) == val, "buf slot doesn't match");
       }
    }

    /*******************************************************************************************************
     *
     * Current test is written assuming there is one intial "heap"(block of contiguous memory) and that up
     * to 4 "heap"s will be created.  The first heap is assumed to be 8192 chars and that the second through
     * fourth "heap"s are 1024 chars (till adding pages are actually written).
     *
     *******************************************************************************************************/
    void test_extended_memory()
    {
       // initial buffer will be exhausted at 8192

       // 8178 left (10 + ptr header)
       char* ptr1 = (char*)eos::malloc(10);
       assert(ptr1 != nullptr, "should have allocated 10 char buf");
       // leave a little space at end of 1st buffer
       char* ptr2 = (char*)eos::malloc(8159);
       assert(ptr2 != nullptr, "should have allocated 8159 char buf");

       // allocated in 2nd memory heap
       char* ptr3 = (char*)eos::malloc(20);
       assert(ptr3 != nullptr, "should have allocated a 20 char buf");
       verify(ptr3, 0, 20);
       // re-sized in 1st memory heap
       char* ptr2_realloc = (char*)eos::realloc(ptr2, 8174);
       assert(ptr2_realloc != nullptr, "should have returned a 8174 char buf");
       assert(ptr2_realloc == ptr2, "should have enlarged the 8159 char buf");

       // re-sized in 1st memory heap
       char* ptr1_realloc = (char*)eos::realloc(ptr1, 5);
       assert(ptr1_realloc != nullptr, "should have returned a 5 char buf");
       assert(ptr1_realloc == ptr1, "should have shrunk the 10 char buf");

       // allocated in 2nd memory heap
       char* ptr4 = (char*)eos::malloc(20);
       assert(ptr4 != nullptr, "should have allocated another 20 char buf");
       assert(ptr3 + 20 < ptr4, "20 char buf should have been created after ptr3"); // test specific to implementation (can remove for refactor)

       // re-size back to original
       ptr1_realloc = (char*)eos::realloc(ptr1, 10);
       assert(ptr1_realloc != nullptr, "should have returned a 10 char buf");
       assert(ptr1_realloc == ptr1, "should have enlarged the 5 char buf");

       // re-size into 2nd memory heap
       ptr1_realloc = (char*)eos::realloc(ptr1, 11);
       assert(ptr1_realloc != nullptr, "should have returned a 11 char buf");
       assert(ptr1_realloc != ptr1, "should have reallocated the 10 char buf");
       assert(ptr4 + 20 < ptr1_realloc, "11 char buf should have been created after ptr4"); // test specific to implementation (can remove for refactor)

       // allocate rest of 2nd memory heap (1024 chars total)
       ptr1 = ptr1_realloc;
       ptr1_realloc = (char*)eos::realloc(ptr1, 972);
       assert(ptr1_realloc != nullptr, "should have returned a 972 char buf");
       assert(ptr1_realloc == ptr1, "should have resized the 11 char buf");

       // allocated in 3rd memory heap (all of 1024 chars)
       char* ptr5 = (char*)eos::malloc(1020);
       assert(ptr5 != nullptr, "should have allocated a 1020 char buf");
       assert(ptr1_realloc + 972 < ptr5, "972 char buf should have been created after ptr1_realloc"); // test specific to implementation (can remove for refactor)

       // allocated in 4th memory heap
       char* ptr6 = (char*)eos::malloc(996);
       assert(ptr6 != nullptr, "should have allocated a 996 char buf");
       assert(ptr5 + 1020 < ptr6, "1020 char buf should have been created after ptr5"); // test specific to implementation (can remove for refactor)

       // 21 char buffer exceeds inital buffer of 1024 (WILL CHANGE WHEN ACTUALLY ALLOCATING PAGE MEMORY)
       char* ptr7 = (char*)eos::malloc(21);
       assert(ptr7 == nullptr, "should not have allocated a 21 char buf");

       // allocated to 4 less than end (1020 chars)
       char* ptr8 = (char*)eos::malloc(16);
       assert(ptr8 != nullptr, "should have allocated a 16 char buf");
       assert(ptr7 + 20 < ptr8, "16 char buf should have been created after ptr7"); // test specific to implementation (can remove for refactor)

       // at 1020, not enough space to allocated any memory besides the ptr header
       char* ptr9 = (char*)eos::malloc(1);
       assert(ptr9 == nullptr, "should not have allocated a 1 char buf");

       // at 1020, reallocate the last 4 chars
       char* ptr8_realloc = (char*)eos::realloc(ptr8, 20);
       assert(ptr8_realloc != nullptr, "should have returned a 20 char buf");
       assert(ptr8_realloc == ptr8, "should have enlarged the 16 char buf");

       // at 1024, no memory remains
       char* ptr10 = (char*)eos::malloc(10);
       assert(ptr10 == nullptr, "should not have allocated a 10 char buf");
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action )
    {
       if( code == N(testextmem) )
       {
          if( action == N(transfer) )
          {
             test_extended_memory();
          }
       }
    }
}
