/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/message.h>
//#include <eosiolib/types.hpp>
#include <eosiolib/memory.hpp>

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
       eosio::print("\n{ ");
       for (uint32_t i = 0; i < size; ++i)
       {
          const char* delim = (i % 8 == 7) ? ", " : " ";
          eosio::print("", static_cast<uint32_t>(static_cast<unsigned char>(char_ptr[i])), delim);
       }
       eosio::print("}\n");
    }

    /*
     * malloc and realloc always allocate on 8 byte boundaries based off of total allocation, so
     * if the requested size + the 2 byte header is not divisible by 8, then the allocated space
     * will be larger than the requested size
     */
    void test_memory()
    {
       char* ptr1 = (char*)eosio::malloc(0);
       assert(ptr1 == nullptr, "should not have allocated a 0 char buf");

       // 20 chars - 20 + 4(header) which is divisible by 8
       ptr1 = (char*)eosio::malloc(20);
       assert(ptr1 != nullptr, "should have allocated a 20 char buf");
       verify(ptr1, 0, 20);
       // existing memory layout -> |24|

       // 36 chars allocated - 30 + 4 plus an extra 6 to be divisible by 8
       char* ptr1_realloc = (char*)eosio::realloc(ptr1, 30);
       assert(ptr1_realloc != nullptr, "should have returned a 30 char buf");
       assert(ptr1_realloc == ptr1, "should have enlarged the 20 char buf");
       // existing memory layout -> |40|

       // 20 chars allocated
       char* ptr2 = (char*)eosio::malloc(20);
       assert(ptr2 != nullptr, "should have allocated another 20 char buf");
       assert(ptr1 + 36 < ptr2, "20 char buf should have been created after ptr1"); // test specific to implementation (can remove for refactor)
       verify(ptr1, 0, 36);
       assert(ptr1[36] != 0, "should not have empty bytes following since block allocated"); // test specific to implementation (can remove for refactor)
       // existing memory layout -> |40|24|

       //shrink the buffer
       ptr1[14] = 0x7e;
       // 20 chars allocated (still)
       ptr1_realloc = (char*)eosio::realloc(ptr1, 15);
       assert(ptr1_realloc != nullptr, "should have returned a 15 char buf");
       assert(ptr1_realloc == ptr1, "should have shrunk the reallocated 30 char buf");
       verify(ptr1, 0, 14); // test specific to implementation (can remove for refactor)
       assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched");
       // existing memory layout -> |24(shrunk)|16(freed)|24|

       //same size the buffer (verify corner case)
       // 20 chars allocated (still)
       ptr1_realloc = (char*)eosio::realloc(ptr1, 15);
       assert(ptr1_realloc != nullptr, "should have returned a reallocated 15 char buf");
       assert(ptr1_realloc == ptr1, "should have reallocated 15 char buf as the same buf");
       assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched for unchanged buf");

       //same size as max allocated buffer -- test specific to implementation (can remove for refactor)
       ptr1_realloc = (char*)eosio::realloc(ptr1, 30);
       assert(ptr1_realloc != nullptr, "should have returned a 30 char buf");
       assert(ptr1_realloc == ptr1, "should have increased the buf back to orig max"); //test specific to implementation (can remove for refactor)
       assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched for expanded buf");

       //increase buffer beyond (indicated) allocated space
       // 36 chars allocated (still)
       ptr1_realloc = (char*)eosio::realloc(ptr1, 36);
       assert(ptr1_realloc != nullptr, "should have returned a 36 char buf");
       assert(ptr1_realloc == ptr1, "should have increased char buf to actual size"); // test specific to implementation (can remove for refactor)

       //increase buffer beyond allocated space
       ptr1[35] = 0x7f;
       // 44 chars allocated - 37 + 4 plus an extra 7 to be divisible by 8
       ptr1_realloc = (char*)eosio::realloc(ptr1, 37);
       assert(ptr1_realloc != nullptr, "should have returned a 37 char buf");
       assert(ptr1_realloc != ptr1, "should have had to create new 37 char buf from 36 char buf");
       assert(ptr2 < ptr1_realloc, "should have been created after ptr2"); // test specific to implementation (can remove for refactor)
       assert(ptr1_realloc[14] == 0x7e, "orig 36 char buf's content should be copied");
       assert(ptr1_realloc[35] == 0x7f, "orig 36 char buf's content should be copied");

       //realloc with nullptr
       char* nullptr_realloc = (char*)eosio::realloc(nullptr, 50);
       assert(nullptr_realloc != nullptr, "should have returned a 50 char buf and ignored nullptr");
       assert(ptr1_realloc < nullptr_realloc, "should have created after ptr1_realloc"); // test specific to implementation (can remove for refactor)

       //realloc with invalid ptr
       char* invalid_ptr_realloc = (char*)eosio::realloc(nullptr_realloc + 4, 10);
       assert(invalid_ptr_realloc != nullptr, "should have returned a 10 char buf and ignored invalid ptr");
       assert(nullptr_realloc < invalid_ptr_realloc, "should have created invalid_ptr_realloc after nullptr_realloc"); // test specific to implementation (can remove for refactor)
    }

    // this test verifies that malloc can allocate 15 64K pages and treat them as one big heap space (if sbrk is not called in the mean time)
    void test_memory_hunk()
    {
       // try to allocate the largest buffer we can, which is 15 contiguous 64K pages (with the 4 char space for the ptr header)
       char* ptr1 = (char*)eosio::malloc(15 * 64 * 1024 - 4);
       assert(ptr1 != nullptr, "should have allocated a ~983K char buf");
    }

    void test_memory_hunks()
    {
       // leave 784 bytes of initial buffer to allocate later (rounds up to nearest 8 byte boundary,
       // 16 bytes bigger than remainder left below in 15 64K page heap))
       char* ptr1 = (char*)eosio::malloc(7404);
       assert(ptr1 != nullptr, "should have allocated a 7404 char buf");

       char* last_ptr = nullptr;
       // 96 * (10 * 1024 - 15) => 15 ~64K pages with 768 byte buffer left to allocate
       for (int i = 0; i < 96; ++i)
       {
          char* ptr2 =  (char*)eosio::malloc(10 * 1024 - 15);
          assert(ptr2 != nullptr, "should have allocated a ~10K char buf");
          if (last_ptr != nullptr)
          {
             // - 15 rounds to -8
             assert(last_ptr + 10 * 1024 - 8 == ptr2, "should allocate the very next ptr"); // test specific to implementation (can remove for refactor)
          }

          last_ptr = ptr2;
       }

       // try to allocate a buffer slightly larger than the remaining buffer| 765 + 4 rounds to 776
       char* ptr3 =  (char*)eosio::malloc(765);
       assert(ptr3 != nullptr, "should have allocated a 772 char buf");
       assert(ptr1 + 7408 == ptr3, "should allocate the very next ptr after ptr1 in initial heap"); // test specific to implementation (can remove for refactor)

       // use all but 8 chars
       char* ptr4 = (char*)eosio::malloc(764);
       assert(ptr4 != nullptr, "should have allocated a 764 char buf");
       assert(last_ptr + 10 * 1024 - 8 == ptr4, "should allocate the very next ptr after last_ptr at end of contiguous heap"); // test specific to implementation (can remove for refactor)

       // use up remaining 8 chars
       char* ptr5 = (char*)eosio::malloc(4);
       assert(ptr5 != nullptr, "should have allocated a 4 char buf");
       assert(ptr3 + 776 == ptr5, "should allocate the very next ptr after ptr3 in initial heap"); // test specific to implementation (can remove for refactor)

       // nothing left to allocate
       char* ptr6 = (char*)eosio::malloc(4);
       assert(ptr6 == nullptr, "should not have allocated a char buf");
    }

    void test_memory_hunks_disjoint()
    {
       // leave 8 bytes of initial buffer to allocate later
       char* ptr1 = (char*)eosio::malloc(8 * 1024 - 12);
       assert(ptr1 != nullptr, "should have allocated a 8184 char buf");

       // can only make 14 extra (64K) heaps for malloc, since calls to sbrk will eat up part
       char* loop_ptr1[14];
       // 14 * (64 * 1024 - 28) => 14 ~64K pages with each page having 24 bytes left to allocate
       for (int i = 0; i < 14; ++i)
       {
          // allocates a new heap for each request, since sbrk call doesn't allow contiguous heaps to grow
          loop_ptr1[i] = (char*)eosio::malloc(64 * 1024 - 28);
          assert(loop_ptr1[i] != nullptr, "should have allocated a 64K char buf");

          assert(sbrk(4) != nullptr, "should be able to allocate 8 bytes");
       }

       // the 15th extra heap is reduced in size because of the 14 * 8 bytes allocated by sbrk calls
       // will leave 8 bytes to allocate later (verifying that we circle back in the list
       char* ptr2 = (char*)eosio::malloc(65412);
       assert(ptr2 != nullptr, "should have allocated a 65412 char buf");

       char* loop_ptr2[14];
       for (int i = 0; i < 14; ++i)
       {
          // 12 char buffer to leave 8 bytes for another pass
          loop_ptr2[i] = (char*)eosio::malloc(12);
          assert(loop_ptr2[i] != nullptr, "should have allocated a 12 char buf");
          assert(loop_ptr1[i] + 64 * 1024 - 24 == loop_ptr2[i], "loop_ptr2[i] should be very next pointer after loop_ptr1[i]");
       }

       // this shows that searching for free ptrs starts at the last loop to find free memory, not at the begining
       char* ptr3 = (char*)eosio::malloc(4);
       assert(ptr3 != nullptr, "should have allocated a 4 char buf");
       assert(loop_ptr2[13] + 16 == ptr3, "should allocate the very next ptr after loop_ptr2[13]"); // test specific to implementation (can remove for refactor)

       char* ptr4 = (char*)eosio::malloc(4);
       assert(ptr4 != nullptr, "should have allocated a 4 char buf");
       assert(ptr2 + 65416 == ptr4, "should allocate the very next ptr after ptr2 in last heap"); // test specific to implementation (can remove for refactor)

       char* ptr5 = (char*)eosio::malloc(4);
       assert(ptr5 != nullptr, "should have allocated a 4 char buf");
       assert(ptr1 + 8184 == ptr5, "should allocate the very next ptr after ptr1 in last heap"); // test specific to implementation (can remove for refactor)

       // will eat up remaining memory (14th heap already used up)
       char* loop_ptr3[13];
       for (int i = 0; i < 13; ++i)
       {
          // 4 char buffer to use up buffer
          loop_ptr3[i] = (char*)eosio::malloc(4);
          assert(loop_ptr3[i] != nullptr, "should have allocated a 4 char buf");
          assert(loop_ptr2[i] + 16 == loop_ptr3[i], "loop_ptr3[i] should be very next pointer after loop_ptr2[i]");
       }

       char* ptr6 = (char*)eosio::malloc(4);
       assert(ptr6 == nullptr, "should not have allocated a char buf");

       eosio::free(loop_ptr1[3]);
       eosio::free(loop_ptr2[3]);
       eosio::free(loop_ptr3[3]);

       char* slot3_ptr[64];
       for (int i = 0; i < 64; ++i)
       {
          slot3_ptr[i] = (char*)eosio::malloc(1020);
          assert(slot3_ptr[i] != nullptr, "should have allocated a 1020 char buf");
          if (i == 0)
             assert(loop_ptr1[3] == slot3_ptr[0], "loop_ptr1[3] should be very next pointer after slot3_ptr[0]");
          else
             assert(slot3_ptr[i - 1] + 1024 == slot3_ptr[i], "slot3_ptr[i] should be very next pointer after slot3_ptr[i-1]");
       }

       char* ptr7 = (char*)eosio::malloc(4);
       assert(ptr7 == nullptr, "should not have allocated a char buf");
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
       else if( code == N(testmemhunk) )
       {
          if( action == N(transfer) )
          {
             test_memory_hunk();
          }
       }
       else if( code == N(testmemhunks) )
       {
          if( action == N(transfer) )
          {
             test_memory_hunks();
          }
       }
       else if( code == N(testdisjoint) )
       {
          if( action == N(transfer) )
          {
             test_memory_hunks_disjoint();
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
