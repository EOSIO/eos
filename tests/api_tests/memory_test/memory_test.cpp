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

    void test_memory()
    {
       char* ptr1 = (char*)eos::malloc(0);
       assert(ptr1 == nullptr, "should not have allocated a 0 char buf");

       ptr1 = (char*)eos::malloc(20);
       assert(ptr1 != nullptr, "should have allocated a 20 char buf");
       verify(ptr1, 0, 20);
       char* ptr1_realloc = (char*)eos::realloc(ptr1, 30);
       assert(ptr1_realloc != nullptr, "should have returned a 30 char buf");
       assert(ptr1_realloc == ptr1, "should have enlarged the 20 char buf");
       char* ptr2 = (char*)eos::malloc(20);
       assert(ptr2 != nullptr, "should have allocated another 20 char buf");
       assert(ptr1 + 30 < ptr2, "20 char buf should have been created after ptr1"); // test specific to implementation (can remove for refactor)
       verify(ptr1, 0, 30);
       assert(ptr1[30] != 0, "should not have empty bytes following since block allocated"); // test specific to implementation (can remove for refactor)

       //shrink the buffer
       ptr1[14] = 0x7e;
       ptr1[29] = 0x7f;
       ptr1_realloc = (char*)eos::realloc(ptr1, 15);
       assert(ptr1_realloc != nullptr, "should have returned a 15 char buf");
       assert(ptr1_realloc == ptr1, "should have shrunk the reallocated 30 char buf");
       assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched");
       verify(ptr1 + 15, 0, 15); // test specific to implementation (can remove for refactor)

       //same size the buffer (verify corner case)
       ptr1_realloc = (char*)eos::realloc(ptr1, 15);
       assert(ptr1_realloc != nullptr, "should have returned a reallocated 15 char buf");
       assert(ptr1_realloc == ptr1, "should have reallocated 15 char buf as the same buf");
       assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched for unchanged buf");

       //same size as max allocated buffer -- test specific to implementation (can remove for refactor)
       ptr1_realloc = (char*)eos::realloc(ptr1, 30);
       assert(ptr1_realloc != nullptr, "should have returned a 30 char buf");
       assert(ptr1_realloc == ptr1, "should have increased the buf back to orig max"); //test specific to implementation (can remove for refactor)
       assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched for expanded buf");

       //increase buffer beyond allocated space
       ptr1[29] = 0x7f;
       ptr1_realloc = (char*)eos::realloc(ptr1, 31);
       assert(ptr1_realloc != nullptr, "should have returned a 31 char buf");
       assert(ptr1_realloc != ptr1, "should have had to create new 31 char buf from 30 char buf");
       assert(ptr2 < ptr1_realloc, "should have been created after ptr2"); // test specific to implementation (can remove for refactor)
       assert(ptr1_realloc[14] == 0x7e, "orig 30 char buf's content should be copied");
       assert(ptr1_realloc[29] == 0x7f, "orig 30 char buf's content should be copied");

       //realloc with nullptr
       char* nullptr_realloc = (char*)eos::realloc(nullptr, 50);
       assert(nullptr_realloc != nullptr, "should have returned a 50 char buf and ignored nullptr");
       assert(ptr1_realloc < nullptr_realloc, "should have created after ptr1_realloc"); // test specific to implementation (can remove for refactor)

       //realloc with invalid ptr
       char* invalid_ptr_realloc = (char*)eos::realloc(nullptr_realloc + 4, 10);
       assert(invalid_ptr_realloc != nullptr, "should have returned a 10 char buf and ignored invalid ptr");
       assert(nullptr_realloc < invalid_ptr_realloc, "should have created invalid_ptr_realloc after nullptr_realloc"); // test specific to implementation (can remove for refactor)

       // try to re-allocate past max
       ptr1 = ptr1_realloc;
       ptr1_realloc = (char*)eos::realloc(ptr1, 8028);
       assert(ptr1_realloc == nullptr, "realloc request past end should return a nullptr");

       // re-allocate to max
       ptr1_realloc = (char*)eos::realloc(invalid_ptr_realloc, 8027);
       assert(ptr1_realloc != nullptr, "should have returned a buf");
       assert(ptr1_realloc == invalid_ptr_realloc, "should have extended the orig buf");
    }

    void test_memory_bounds()
    {
       // full buffer is 8188 (8192 - ptr size header)

       // try to malloc past buffer
       char* ptr1 = (char*)eos::malloc(8189);
       assert(ptr1 == nullptr, "request of more than available should fail");

       // takes up 5 (1 + ptr size header)
       ptr1 = (char*)eos::malloc(1);
       assert(ptr1 != nullptr, "should have allocated 1 char buf");

       // takes up 5 (1 + ptr size header)
       char* ptr2 = (char*)eos::malloc(1);
       assert(ptr2 != nullptr, "should have allocated 2nd 1 char buf");

       // realloc to buffer (tests verifying relloc boundary logic and malloc logic
       char* ptr1_realloc = (char*)eos::realloc(ptr1, 8178);
       assert(ptr1_realloc != nullptr, "realloc request to end, should have allocated a buf");
       assert(ptr1_realloc != ptr1, "should have reallocate the large buf");
       verify(ptr1_realloc, 0, 8178);
    }

    void test_memset_memcpy()
    {
       char buf1[40] = {};
       char buf2[40] = {};

       verify(buf1, 0, 40);
       verify(buf2, 0, 40);

       memset(buf1, 0x22, 20);
       verify(buf1, 0x22, 20);
       verify(&buf1[20], 0, 20);

       memset(&buf2[20], 0xff, 20);
       verify(buf2, 0, 20);
       verify(&buf2[20], 0xff, 20);

       memcpy(&buf1[10], &buf2[10], 20);
       verify(buf1, 0x22, 10);
       verify(&buf1[10], 0, 10);
       verify(&buf1[20], 0xff, 10);
       verify(&buf1[30], 0, 10);

       memset(&buf1[1], 1, 1);
       verify(buf1, 0x22, 1);
       verify(&buf1[1], 1, 1);
       verify(&buf1[2], 0x22, 8);

       // verify adjacent non-overlapping buffers
       char buf3[50] = {};
       memset(&buf3[25], 0xee, 25);
       verify(buf3, 0, 25);
       memcpy(buf3, &buf3[25], 25);
       verify(buf3, 0xee, 50);

       memset(buf3, 0, 25);
       verify(&buf3[25], 0xee, 25);
       memcpy(&buf3[25], buf3, 25);
       verify(buf3, 0, 50);
    }

    void test_memcpy_overlap_start()
    {
       char buf3[99] = {};
       memset(buf3, 0xee, 50);
       memset(&buf3[50], 0xff, 49);
       memcpy(&buf3[49], buf3, 50);
    }


    void test_memcpy_overlap_end()
    {
       char buf3[99] = {};
       memset(buf3, 0xee, 50);
       memset(&buf3[50], 0xff, 49);
       memcpy(buf3, &buf3[49], 50);
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action )
    {
       if( code == N(testmemory) )
       {
          if( action == N(transfer) )
          {
             test_memory();
          }
       }
       else if( code == N(testbounds) )
       {
          if( action == N(transfer) )
          {
             test_memory_bounds();
          }
       }
       else if( code == N(testmemset) )
       {
          if( action == N(transfer) )
          {
             test_memset_memcpy();
          }
       }
       else if( code == N(testolstart) )
       {
          if( action == N(transfer) )
          {
             test_memcpy_overlap_start();
          }
       }
       else if( code == N(testolend) )
       {
          if( action == N(transfer) )
          {
             test_memcpy_overlap_end();
          }
       }
    }
}
