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

       // allocate rest of 2nd memory heap
       ptr1 = ptr1_realloc;
       ptr1_realloc = (char*)eos::realloc(ptr1, 972);
       assert(ptr1_realloc != nullptr, "should have returned a 972 char buf");
       assert(ptr1_realloc == ptr1, "should have resized the 11 char buf");

       // allocated in 3rd memory heap
       char* ptr5 = (char*)eos::malloc(1020);
       assert(ptr5 != nullptr, "should have allocated a 1020 char buf");
       assert(ptr1_realloc + 972 < ptr5, "972 char buf should have been created after ptr1_realloc"); // test specific to implementation (can remove for refactor)

       // allocated in 4th memory heap
       char* ptr6 = (char*)eos::malloc(996);
       assert(ptr6 != nullptr, "should have allocated a 1024 char buf");
       assert(ptr5 + 1020 < ptr6, "1020 char buf should have been created after ptr5"); // test specific to implementation (can remove for refactor)

       // not enough space
       char* ptr7 = (char*)eos::malloc(21);
       assert(ptr7 == nullptr, "should not have allocated a 21 char buf");

       // allocated to end
       char* ptr8 = (char*)eos::malloc(20);
       assert(ptr8 != nullptr, "should have allocated a 20 char buf");
       assert(ptr7 + 20 < ptr8, "20 char buf should have been created after ptr7"); // test specific to implementation (can remove for refactor)

       // nothing allocated
       char* ptr9 = (char*)eos::malloc(10);
       assert(ptr9 == nullptr, "should not have allocated a 10 char buf");
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
