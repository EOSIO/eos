/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/message.h>
//#include <eoslib/types.hpp>
#include <eoslib/memory.hpp>

extern "C" {

    const uint32_t _64K = 65536;

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

       // 8176 left (12 + ptr header)
       char* ptr1 = (char*)eosio::malloc(12);
       assert(ptr1 != nullptr, "should have allocated 12 char buf");
       // leave a little space at end of 1st buffer
       char* ptr2 = (char*)eosio::malloc(8159);
       assert(ptr2 != nullptr, "should have allocated 8159 char buf");

       // allocated in 2nd memory heap
       char* ptr3 = (char*)eosio::malloc(20);
       assert(ptr3 != nullptr, "should have allocated a 20 char buf");
       verify(ptr3, 0, 20);
       // re-sized in 1st memory heap
       char* ptr2_realloc = (char*)eosio::realloc(ptr2, 8172);
       assert(ptr2_realloc != nullptr, "should have returned a 8172 char buf");
       assert(ptr2_realloc == ptr2, "should have enlarged the 8159 char buf");

       // re-sized in 1st memory heap
       char* ptr1_realloc = (char*)eosio::realloc(ptr1, 5);
       assert(ptr1_realloc != nullptr, "should have returned a 5 char buf");
       assert(ptr1_realloc == ptr1, "should have shrunk the 10 char buf");

       // allocated in 2nd memory heap
       char* ptr4 = (char*)eosio::malloc(20);
       assert(ptr4 != nullptr, "should have allocated another 20 char buf");
       assert(ptr3 + 20 < ptr4, "20 char buf should have been created after ptr3"); // test specific to implementation (can remove for refactor)

       // re-size back to original
       ptr1_realloc = (char*)eosio::realloc(ptr1, 10);
       assert(ptr1_realloc != nullptr, "should have returned a 10 char buf");
       assert(ptr1_realloc == ptr1, "should have enlarged the 5 char buf");

       // re-size into 2nd memory heap
       ptr1_realloc = (char*)eosio::realloc(ptr1, 13);
       assert(ptr1_realloc != nullptr, "should have returned a 13 char buf");
       assert(ptr1_realloc != ptr1, "should have reallocated the 12 char buf");
       assert(ptr4 + 20 < ptr1_realloc, "11 char buf should have been created after ptr4"); // test specific to implementation (can remove for refactor)

       // allocate rest of 2nd memory heap (1024 chars total)
       ptr1 = ptr1_realloc;
       ptr1_realloc = (char*)eosio::realloc(ptr1, 972);
       assert(ptr1_realloc != nullptr, "should have returned a 972 char buf");
       assert(ptr1_realloc == ptr1, "should have resized the 11 char buf");

       // allocated in 3rd memory heap (all of 1024 chars)
       char* ptr5 = (char*)eosio::malloc(1020);
       assert(ptr5 != nullptr, "should have allocated a 1020 char buf");
       assert(ptr1_realloc + 972 < ptr5, "972 char buf should have been created after ptr1_realloc"); // test specific to implementation (can remove for refactor)

       // allocated in 4th memory heap
       char* ptr6 = (char*)eosio::malloc(996);
       assert(ptr6 != nullptr, "should have allocated a 996 char buf");
       assert(ptr5 + 1020 < ptr6, "1020 char buf should have been created after ptr5"); // test specific to implementation (can remove for refactor)
    }

    void test_page_memory()
    {
       auto prev = sbrk(0);

       assert(reinterpret_cast<uint32_t>(prev) == _64K, "Should initially have 1 64K page allocated");

       prev = sbrk(1);

       assert(reinterpret_cast<uint32_t>(prev) == _64K, "Should still point to the end of 1st 64K page");

       prev = sbrk(2);

       assert(reinterpret_cast<uint32_t>(prev) == _64K + 8, "Should point to 8 past the end of 1st 64K page");

       prev = sbrk(_64K - 17);

       assert(reinterpret_cast<uint32_t>(prev) == _64K + 16, "Should point to 16 past the end of 1st 64K page");

       prev = sbrk(1);

       assert(reinterpret_cast<uint32_t>(prev) == 2*_64K, "Should point to the end of 2nd 64K page");

       prev = sbrk(_64K);

       assert(reinterpret_cast<uint32_t>(prev) == 2*_64K + 8, "Should point to 8 past the end of the 2nd 64K page");

       prev = sbrk(_64K - 15);

       assert(reinterpret_cast<uint32_t>(prev) == 3*_64K + 8, "Should point to 8 past the end of the 3rd 64K page");

       prev = sbrk(2*_64K - 1);

       assert(reinterpret_cast<uint32_t>(prev) == 4*_64K, "Should point to the end of 4th 64K page");

       prev = sbrk(2*_64K);

       assert(reinterpret_cast<uint32_t>(prev) == 6*_64K, "Should point to the end of 6th 64K page");

       prev = sbrk(2*_64K + 1);

       assert(reinterpret_cast<uint32_t>(prev) == 8*_64K, "Should point to the end of 8th 64K page");

       prev = sbrk(6*_64K - 15);

       assert(reinterpret_cast<uint32_t>(prev) == 10*_64K + 8, "Should point to 8 past the end of 13th 64K page");

       prev = sbrk(0);

       assert(reinterpret_cast<uint32_t>(prev) == 16*_64K, "Should point to the end of 16th 64K page");
    }

    void test_page_memory_exceeded()
    {
       auto prev = sbrk(15*_64K);
       assert(reinterpret_cast<uint32_t>(prev) == _64K, "Should have allocated up to the 1M of memory limit");
       sbrk(1);
       assert(0, "Should have thrown exception for trying to allocate more than 1M of memory");
    }

    void test_page_memory_negative_bytes()
    {
       sbrk(-1);
       assert(0, "Should have thrown exception for trying to remove memory");
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
       else if( code == N(testpagemem) )
       {
          if( action == N(transfer) )
          {
             test_page_memory();
          }
       }
       else if( code == N(testmemexc) )
       {
          if( action == N(transfer) )
          {
             test_page_memory_exceeded();
          }
       }
       else if( code == N(testnegbytes) )
       {
          if( action == N(transfer) )
          {
             test_page_memory_negative_bytes();
          }
       }
    }
}
