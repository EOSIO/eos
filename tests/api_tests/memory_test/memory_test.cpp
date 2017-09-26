/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
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

    void print(const void* const ptr, const uint32_t size)
    {
       const char* char_ptr = (const char*)ptr;
       eos::print("\n{ ");
       for (uint32_t i = 0; i < size; ++i)
       {
          const char* delim = (i % 8 == 7) ? ", " : " ";
          eos::print("", static_cast<uint32_t>(static_cast<unsigned char>(char_ptr[i])), delim);
       }
       eos::print("}\n");
    }

    /*
     * malloc and realloc always allocate on 8 byte boundaries based off of total allocation, so
     * if the requested size + the 2 byte header is not divisible by 8, then the allocated space
     * will be larger than the requested size
     */
    void test_memory()
    {
       char* ptr1 = (char*)eos::malloc(0);
       assert(ptr1 == nullptr, "should not have allocated a 0 char buf");

       // 20 chars - 20 + 4(header) which is divisible by 8
       ptr1 = (char*)eos::malloc(20);
       assert(ptr1 != nullptr, "should have allocated a 20 char buf");
       verify(ptr1, 0, 20);
       // existing memory layout -> |24|

       // 36 chars allocated - 30 + 4 plus an extra 6 to be divisible by 8
       char* ptr1_realloc = (char*)eos::realloc(ptr1, 30);
       assert(ptr1_realloc != nullptr, "should have returned a 30 char buf");
       assert(ptr1_realloc == ptr1, "should have enlarged the 20 char buf");
       // existing memory layout -> |40|

       // 20 chars allocated
       char* ptr2 = (char*)eos::malloc(20);
       assert(ptr2 != nullptr, "should have allocated another 20 char buf");
       assert(ptr1 + 36 < ptr2, "20 char buf should have been created after ptr1"); // test specific to implementation (can remove for refactor)
       verify(ptr1, 0, 36);
       assert(ptr1[36] != 0, "should not have empty bytes following since block allocated"); // test specific to implementation (can remove for refactor)
       // existing memory layout -> |40|24|

       //shrink the buffer
       ptr1[14] = 0x7e;
       // 20 chars allocated (still)
       ptr1_realloc = (char*)eos::realloc(ptr1, 15);
       assert(ptr1_realloc != nullptr, "should have returned a 15 char buf");
       assert(ptr1_realloc == ptr1, "should have shrunk the reallocated 30 char buf");
       verify(ptr1, 0, 14); // test specific to implementation (can remove for refactor)
       assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched");
       // existing memory layout -> |24(shrunk)|16(freed)|24|

       //same size the buffer (verify corner case)
       // 20 chars allocated (still)
       ptr1_realloc = (char*)eos::realloc(ptr1, 15);
       assert(ptr1_realloc != nullptr, "should have returned a reallocated 15 char buf");
       assert(ptr1_realloc == ptr1, "should have reallocated 15 char buf as the same buf");
       assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched for unchanged buf");

       //same size as max allocated buffer -- test specific to implementation (can remove for refactor)
       ptr1_realloc = (char*)eos::realloc(ptr1, 30);
       assert(ptr1_realloc != nullptr, "should have returned a 30 char buf");
       assert(ptr1_realloc == ptr1, "should have increased the buf back to orig max"); //test specific to implementation (can remove for refactor)
       assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched for expanded buf");

       //increase buffer beyond (indicated) allocated space
       // 36 chars allocated (still)
       ptr1_realloc = (char*)eos::realloc(ptr1, 36);
       assert(ptr1_realloc != nullptr, "should have returned a 36 char buf");
       assert(ptr1_realloc == ptr1, "should have increased char buf to actual size"); // test specific to implementation (can remove for refactor)

       //increase buffer beyond allocated space
       ptr1[35] = 0x7f;
       // 44 chars allocated - 37 + 4 plus an extra 7 to be divisible by 8
       ptr1_realloc = (char*)eos::realloc(ptr1, 37);
       assert(ptr1_realloc != nullptr, "should have returned a 37 char buf");
       assert(ptr1_realloc != ptr1, "should have had to create new 37 char buf from 36 char buf");
       assert(ptr2 < ptr1_realloc, "should have been created after ptr2"); // test specific to implementation (can remove for refactor)
       assert(ptr1_realloc[14] == 0x7e, "orig 36 char buf's content should be copied");
       assert(ptr1_realloc[35] == 0x7f, "orig 36 char buf's content should be copied");

       //realloc with nullptr
       char* nullptr_realloc = (char*)eos::realloc(nullptr, 50);
       assert(nullptr_realloc != nullptr, "should have returned a 50 char buf and ignored nullptr");
       assert(ptr1_realloc < nullptr_realloc, "should have created after ptr1_realloc"); // test specific to implementation (can remove for refactor)

       //realloc with invalid ptr
       char* invalid_ptr_realloc = (char*)eos::realloc(nullptr_realloc + 4, 10);
       assert(invalid_ptr_realloc != nullptr, "should have returned a 10 char buf and ignored invalid ptr");
       assert(nullptr_realloc < invalid_ptr_realloc, "should have created invalid_ptr_realloc after nullptr_realloc"); // test specific to implementation (can remove for refactor)
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

       // realloc to buffer (tests verifying realloc boundary logic and malloc logic
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

    void test_memcmp()
    {
       char buf1[] = "abcde";
       char buf2[] = "abcde";
       int32_t res1 = memcmp(buf1, buf2, 6);
       assert(res1 == 0, "first data should be equal to second data");

       char buf3[] = "abcde";
       char buf4[] = "fghij";
       int32_t res2 = memcmp(buf3, buf4, 6);
       assert(res2 < 0, "first data should be smaller than second data");

       char buf5[] = "fghij";
       char buf6[] = "abcde";
       int32_t res3 = memcmp(buf5, buf6, 6);
       assert(res3 > 0, "first data should be larger than second data");
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
       else if( code == N(testmemcmp) )
       {
          if( action == N(transfer) )
          {
             test_memcmp();
          }
       }
    }
}
