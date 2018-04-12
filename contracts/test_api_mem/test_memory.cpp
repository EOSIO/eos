/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/memory.hpp>


void verify_mem(const void* const ptr, const uint32_t val, const uint32_t size)
{
   const char* char_ptr = (const char*)ptr;
   for (uint32_t i = 0; i < size; ++i)
   {
      eosio_assert(static_cast<uint32_t>(static_cast<unsigned char>(char_ptr[i])) == val, "buf slot doesn't match");
   }
}

/*
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
*/

/*
* malloc and realloc always allocate on 8 byte boundaries based off of total allocation, so
* if the requested size + the 2 byte header is not divisible by 8, then the allocated space
* will be larger than the requested size
*/
void test_memory::test_memory_allocs()
{
    char* ptr1 = (char*)malloc(0);
    eosio_assert(ptr1 == nullptr, "should not have allocated a 0 char buf");

    // 20 chars - 20 + 4(header) which is divisible by 8
    ptr1 = (char*)malloc(20);
    eosio_assert(ptr1 != nullptr, "should have allocated a 20 char buf");
    verify_mem(ptr1, 0, 20);
    // existing memory layout -> |24|

    // 36 chars allocated - 30 + 4 plus an extra 6 to be divisible by 8
    char* ptr1_realloc = (char*)realloc(ptr1, 30);
    eosio_assert(ptr1_realloc != nullptr, "should have returned a 30 char buf");
    eosio_assert(ptr1_realloc == ptr1, "should have enlarged the 20 char buf");
    // existing memory layout -> |40|

    // 20 chars allocated
    char* ptr2 = (char*)malloc(20);
    eosio_assert(ptr2 != nullptr, "should have allocated another 20 char buf");
    eosio_assert(ptr1 + 36 < ptr2, "20 char buf should have been created after ptr1"); // test specific to implementation (can remove for refactor)
    verify_mem(ptr1, 0, 36);
    eosio_assert(ptr1[36] != 0, "should not have empty bytes following since block allocated"); // test specific to implementation (can remove for refactor)
    // existing memory layout -> |40|24|

    //shrink the buffer
    ptr1[14] = 0x7e;
    // 20 chars allocated (still)
    ptr1_realloc = (char*)realloc(ptr1, 15);
    eosio_assert(ptr1_realloc != nullptr, "should have returned a 15 char buf");
    eosio_assert(ptr1_realloc == ptr1, "should have shrunk the reallocated 30 char buf");
    verify_mem(ptr1, 0, 14); // test specific to implementation (can remove for refactor)
    eosio_assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched");
    // existing memory layout -> |24(shrunk)|16(freed)|24|

    //same size the buffer (verify corner case)
    // 20 chars allocated (still)
    ptr1_realloc = (char*)realloc(ptr1, 15);
    eosio_assert(ptr1_realloc != nullptr, "should have returned a reallocated 15 char buf");
    eosio_assert(ptr1_realloc == ptr1, "should have reallocated 15 char buf as the same buf");
    eosio_assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched for unchanged buf");

    //same size as max allocated buffer -- test specific to implementation (can remove for refactor)
    ptr1_realloc = (char*)realloc(ptr1, 30);
    eosio_assert(ptr1_realloc != nullptr, "should have returned a 30 char buf");
    eosio_assert(ptr1_realloc == ptr1, "should have increased the buf back to orig max"); //test specific to implementation (can remove for refactor)
    eosio_assert(ptr1[14] == 0x7e, "remaining 15 chars of buf should be untouched for expanded buf");

    //increase buffer beyond (indicated) allocated space
    // 36 chars allocated (still)
    ptr1_realloc = (char*)realloc(ptr1, 36);
    eosio_assert(ptr1_realloc != nullptr, "should have returned a 36 char buf");
    eosio_assert(ptr1_realloc == ptr1, "should have increased char buf to actual size"); // test specific to implementation (can remove for refactor)

    //increase buffer beyond allocated space
    ptr1[35] = 0x7f;
    // 44 chars allocated - 37 + 4 plus an extra 7 to be divisible by 8
    ptr1_realloc = (char*)realloc(ptr1, 37);
    eosio_assert(ptr1_realloc != nullptr, "should have returned a 37 char buf");
    eosio_assert(ptr1_realloc != ptr1, "should have had to create new 37 char buf from 36 char buf");
    eosio_assert(ptr2 < ptr1_realloc, "should have been created after ptr2"); // test specific to implementation (can remove for refactor)
    eosio_assert(ptr1_realloc[14] == 0x7e, "orig 36 char buf's content should be copied");
    eosio_assert(ptr1_realloc[35] == 0x7f, "orig 36 char buf's content should be copied");

    //realloc with nullptr
    char* nullptr_realloc = (char*)realloc(nullptr, 50);
    eosio_assert(nullptr_realloc != nullptr, "should have returned a 50 char buf and ignored nullptr");
    eosio_assert(ptr1_realloc < nullptr_realloc, "should have created after ptr1_realloc"); // test specific to implementation (can remove for refactor)

    //realloc with invalid ptr
    char* invalid_ptr_realloc = (char*)realloc(nullptr_realloc + 4, 10);
    eosio_assert(invalid_ptr_realloc != nullptr, "should have returned a 10 char buf and ignored invalid ptr");
    eosio_assert(nullptr_realloc < invalid_ptr_realloc, "should have created invalid_ptr_realloc after nullptr_realloc"); // test specific to implementation (can remove for refactor)
}

// this test verifies that malloc can allocate 15 64K pages and treat them as one big heap space (if sbrk is not called in the mean time)
void test_memory::test_memory_hunk()
{
   // try to allocate the largest buffer we can, which is 15 contiguous 64K pages (with the 4 char space for the ptr header)
   char* ptr1 = (char*)malloc(15 * 64 * 1024 - 4);
   eosio_assert(ptr1 != nullptr, "should have allocated a ~983K char buf");
}

void test_memory::test_memory_hunks()
{
   // leave 784 bytes of initial buffer to allocate later (rounds up to nearest 8 byte boundary,
   // 16 bytes bigger than remainder left below in 15 64K page heap))
   char* ptr1 = (char*)malloc(7404);
   eosio_assert(ptr1 != nullptr, "should have allocated a 7404 char buf");

   char* last_ptr = nullptr;
   // 96 * (10 * 1024 - 15) => 15 ~64K pages with 768 byte buffer left to allocate
   for (int i = 0; i < 96; ++i)
   {
      char* ptr2 =  (char*)malloc(10 * 1024 - 15);
      eosio_assert(ptr2 != nullptr, "should have allocated a ~10K char buf");
      if (last_ptr != nullptr)
      {
         // - 15 rounds to -8
         eosio_assert(last_ptr + 10 * 1024 - 8 == ptr2, "should allocate the very next ptr"); // test specific to implementation (can remove for refactor)
      }

      last_ptr = ptr2;
   }

   // try to allocate a buffer slightly larger than the remaining buffer| 765 + 4 rounds to 776
   char* ptr3 =  (char*)malloc(765);
   eosio_assert(ptr3 != nullptr, "should have allocated a 772 char buf");
   eosio_assert(ptr1 + 7408 == ptr3, "should allocate the very next ptr after ptr1 in initial heap"); // test specific to implementation (can remove for refactor)

   // use all but 8 chars
   char* ptr4 = (char*)malloc(764);
   eosio_assert(ptr4 != nullptr, "should have allocated a 764 char buf");
   eosio_assert(last_ptr + 10 * 1024 - 8 == ptr4, "should allocate the very next ptr after last_ptr at end of contiguous heap"); // test specific to implementation (can remove for refactor)

   // use up remaining 8 chars
   char* ptr5 = (char*)malloc(4);
   eosio_assert(ptr5 != nullptr, "should have allocated a 4 char buf");
   eosio_assert(ptr3 + 776 == ptr5, "should allocate the very next ptr after ptr3 in initial heap"); // test specific to implementation (can remove for refactor)

   // nothing left to allocate
   char* ptr6 = (char*)malloc(4);
   eosio_assert(ptr6 == nullptr, "should not have allocated a char buf");
}

void test_memory::test_memory_hunks_disjoint()
{
   // leave 8 bytes of initial buffer to allocate later
   char* ptr1 = (char*)malloc(8 * 1024 - 12);
   eosio_assert(ptr1 != nullptr, "should have allocated a 8184 char buf");

   // can only make 14 extra (64K) heaps for malloc, since calls to sbrk will eat up part
   char* loop_ptr1[14];
   // 14 * (64 * 1024 - 28) => 14 ~64K pages with each page having 24 bytes left to allocate
   for (int i = 0; i < 14; ++i)
   {
      // allocates a new heap for each request, since sbrk call doesn't allow contiguous heaps to grow
      loop_ptr1[i] = (char*)malloc(64 * 1024 - 28);
      eosio_assert(loop_ptr1[i] != nullptr, "should have allocated a 64K char buf");

      eosio_assert(reinterpret_cast<int32_t>(sbrk(4)) != -1, "should be able to allocate 8 bytes");
   }

   // the 15th extra heap is reduced in size because of the 14 * 8 bytes allocated by sbrk calls
   // will leave 8 bytes to allocate later (verifying that we circle back in the list
   char* ptr2 = (char*)malloc(65412);
   eosio_assert(ptr2 != nullptr, "should have allocated a 65412 char buf");

   char* loop_ptr2[14];
   for (int i = 0; i < 14; ++i)
   {
      // 12 char buffer to leave 8 bytes for another pass
      loop_ptr2[i] = (char*)malloc(12);
      eosio_assert(loop_ptr2[i] != nullptr, "should have allocated a 12 char buf");
      eosio_assert(loop_ptr1[i] + 64 * 1024 - 24 == loop_ptr2[i], "loop_ptr2[i] should be very next pointer after loop_ptr1[i]");
   }

   // this shows that searching for free ptrs starts at the last loop to find free memory, not at the begining
   char* ptr3 = (char*)malloc(4);
   eosio_assert(ptr3 != nullptr, "should have allocated a 4 char buf");
   eosio_assert(loop_ptr2[13] + 16 == ptr3, "should allocate the very next ptr after loop_ptr2[13]"); // test specific to implementation (can remove for refactor)

   char* ptr4 = (char*)malloc(4);
   eosio_assert(ptr4 != nullptr, "should have allocated a 4 char buf");
   eosio_assert(ptr2 + 65416 == ptr4, "should allocate the very next ptr after ptr2 in last heap"); // test specific to implementation (can remove for refactor)

   char* ptr5 = (char*)malloc(4);
   eosio_assert(ptr5 != nullptr, "should have allocated a 4 char buf");
   eosio_assert(ptr1 + 8184 == ptr5, "should allocate the very next ptr after ptr1 in last heap"); // test specific to implementation (can remove for refactor)

   // will eat up remaining memory (14th heap already used up)
   char* loop_ptr3[13];
   for (int i = 0; i < 13; ++i)
   {
      // 4 char buffer to use up buffer
      loop_ptr3[i] = (char*)malloc(4);
      eosio_assert(loop_ptr3[i] != nullptr, "should have allocated a 4 char buf");
      eosio_assert(loop_ptr2[i] + 16 == loop_ptr3[i], "loop_ptr3[i] should be very next pointer after loop_ptr2[i]");
   }

   char* ptr6 = (char*)malloc(4);
   eosio_assert(ptr6 == nullptr, "should not have allocated a char buf");

   free(loop_ptr1[3]);
   free(loop_ptr2[3]);
   free(loop_ptr3[3]);

   char* slot3_ptr[64];
   for (int i = 0; i < 64; ++i)
   {
      slot3_ptr[i] = (char*)malloc(1020);
      eosio_assert(slot3_ptr[i] != nullptr, "should have allocated a 1020 char buf");
      if (i == 0)
         eosio_assert(loop_ptr1[3] == slot3_ptr[0], "loop_ptr1[3] should be very next pointer after slot3_ptr[0]");
      else
         eosio_assert(slot3_ptr[i - 1] + 1024 == slot3_ptr[i], "slot3_ptr[i] should be very next pointer after slot3_ptr[i-1]");
   }

   char* ptr7 = (char*)malloc(4);
   eosio_assert(ptr7 == nullptr, "should not have allocated a char buf");
}   

void test_memory::test_memset_memcpy()
{   
   char buf1[40] = {};
   char buf2[40] = {};

   verify_mem(buf1, 0, 40);
   verify_mem(buf2, 0, 40);

   memset(buf1, 0x22, 20);
   verify_mem(buf1, 0x22, 20);
   verify_mem(&buf1[20], 0, 20);

   memset(&buf2[20], 0xff, 20);
   verify_mem(buf2, 0, 20);
   verify_mem(&buf2[20], 0xff, 20);

   memcpy(&buf1[10], &buf2[10], 20);
   verify_mem(buf1, 0x22, 10);
   verify_mem(&buf1[10], 0, 10);
   verify_mem(&buf1[20], 0xff, 10);
   verify_mem(&buf1[30], 0, 10);

   memset(&buf1[1], 1, 1);
   verify_mem(buf1, 0x22, 1);
   verify_mem(&buf1[1], 1, 1);
   verify_mem(&buf1[2], 0x22, 8);

   // verify adjacent non-overlapping buffers
   char buf3[50] = {};
   memset(&buf3[25], 0xee, 25);
   verify_mem(buf3, 0, 25);
   memcpy(buf3, &buf3[25], 25);
   verify_mem(buf3, 0xee, 50);

   memset(buf3, 0, 25);
   verify_mem(&buf3[25], 0xee, 25);
   memcpy(&buf3[25], buf3, 25);
   verify_mem(buf3, 0, 50);
}

void test_memory::test_memcpy_overlap_start()
{
   char buf3[99] = {};
   memset(buf3, 0xee, 50);
   memset(&buf3[50], 0xff, 49);
   memcpy(&buf3[49], buf3, 50);
}


void test_memory::test_memcpy_overlap_end()
{
   char buf3[99] = {};
   memset(buf3, 0xee, 50);
   memset(&buf3[50], 0xff, 49);
   memcpy(buf3, &buf3[49], 50);
}

void test_memory::test_memcmp()
{
   char buf1[] = "abcde";
   char buf2[] = "abcde";
   int32_t res1 = memcmp(buf1, buf2, 6);
   eosio_assert(res1 == 0, "first data should be equal to second data");

   char buf3[] = "abcde";
   char buf4[] = "fghij";
   int32_t res2 = memcmp(buf3, buf4, 6);
   eosio_assert(res2 < 0, "first data should be smaller than second data");

   char buf5[] = "fghij";
   char buf6[] = "abcde";
   int32_t res3 = memcmp(buf5, buf6, 6);
   eosio_assert(res3 > 0, "first data should be larger than second data");
}

void test_memory::test_outofbound_0()
{
    memset((char *)0, 0xff, 1024 * 1024 * 1024); // big memory
}

void test_memory::test_outofbound_1()
{
    memset((char *)16, 0xff, 0xffffffff); // memory wrap around
}

void test_memory::test_outofbound_2()
{
    char buf[1024] = {0};
    char *ptr = (char *)malloc(1048576);
    memcpy(buf, ptr, 1048576); // stack memory out of bound
}

void test_memory::test_outofbound_3()
{
    char *ptr = (char *)malloc(128);
    memset(ptr, 0xcc, 1048576); // heap memory out of bound
}

template <typename T>
void test_memory_store() {
    T *ptr = (T *)(8192 * 1024 - 1);
    ptr[0] = (T)1;
}

template <typename T>
void test_memory_load() {
    T *ptr = (T *)(8192 * 1024 - 1);
    volatile T tmp = ptr[0];
}

void test_memory::test_outofbound_4()
{
    test_memory_store<char>();
}
void test_memory::test_outofbound_5()
{
    test_memory_store<short>();
}
void test_memory::test_outofbound_6()
{
    test_memory_store<int>();
}
void test_memory::test_outofbound_7()
{
    test_memory_store<double>();
}
void test_memory::test_outofbound_8()
{
    test_memory_load<char>();
}
void test_memory::test_outofbound_9()
{
    test_memory_load<short>();
}
void test_memory::test_outofbound_10()
{
    test_memory_load<int>();
}
void test_memory::test_outofbound_11()
{
    test_memory_load<double>();
}

void test_memory::test_outofbound_12()
{
    volatile unsigned int a = 0xffffffff;
    double *ptr = (double *)a; // store with memory wrap
    ptr[0] = 1;
}

void test_memory::test_outofbound_13()
{
    volatile unsigned int a = 0xffffffff;
    double *ptr = (double *)a; // load with memory wrap
    volatile double tmp = ptr[0];
}
