#include <eosiolib/eosio.hpp>
#include <eosiolib/memory.hpp>
#include "../test_api/test_api.hpp"

//using namespace eosio;

void verify( const void* const ptr, const uint32_t val, const uint32_t size) {
	const char* char_ptr = (const char*)ptr;
	for (uint32_t i = 0; i < size; ++i)
		eosio_assert(static_cast<uint32_t>(static_cast<unsigned char>(char_ptr[i])) == val, "buffer slot doesn't match");
}

#define PRINT_PTR(x) prints("PTR : "); print((uint32_t)x, 4); prints("\n");

void test_extended_memory::test_page_memory() {
	constexpr uint32_t _64K = 64*1024;
   /*
    * Test test_extended_memory::test_page_memory `ensure initial page size`
    * Given I have not tried to increase the "program break" yet,
    * when I call sbrk(0), then I should get the end of the first page, which should be 64K.
    */
	auto prev = sbrk(0);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == _64K, "Should initially have 1 64K page allocated");

   /*
    * Test test_extended_memory::test_page_memory `ensure sbrk returns previous end of program break`
    * Given I have not tried to increase memory,
    * when I call sbrk(1), then I should get the end of the first page, which should be 64K.
    */
	prev = sbrk(1);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == _64K, "Should still be pointing to the end of the 1st 64K page");

   /*
    * Test test_extended_memory::test_page_memory `ensure sbrk aligns allocations`
    * Given that I allocated 1 byte via sbrk, 
    * when I call sbrk(2), then I should get 8 bytes past the previous end because of maintaining 8 byte alignment.
    */
	prev = sbrk(2);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == _64K+8, "Should point to 8 past the end of 1st 64K page");

   /*
    * Test test_extended_memory::test_page_memory `ensure sbrk aligns allocations 2`
    * Given that I allocated 2 bytes via sbrk, 
    * when I call sbrk(_64K-17), then I should get 8 bytes past the previous end because of maintaining 8 byte alignment.
    */
	prev = sbrk(_64K - 17);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == _64K+16, "Should point to 16 past the end of the 1st 64K page");

	prev = sbrk(1);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == 2*_64K, "Should point to the end of the 2nd 64K page");

	prev = sbrk(_64K);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == 2*_64K+8, "Should point to 8 past the end of the 2nd 64K page");

	prev = sbrk(_64K - 15);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == 3*_64K+8, "Should point to 8 past the end of the 3rd 64K page");

	prev = sbrk(2*_64K-1);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == 4*_64K, "Should point to the end of the 4th 64K page");

	prev = sbrk(2*_64K);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == 6*_64K, "Should point to the end of the 6th 64K page");

	prev = sbrk(2*_64K+1);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == 8*_64K, "Should point to the end of the 8th 64K page");

	prev = sbrk(6*_64K-15);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == 10*_64K+8, "Should point to 8 past the end of the 10th 64K page");

	prev = sbrk(0);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == 16*_64K, "Should point to 8 past the end of the 16th 64K page");
}

void test_extended_memory::test_page_memory_exceeded() {
   /*
    * Test test_extended_memory::test_page_memory_exceeded `ensure sbrk won't allocation more than 1M of memory`
    * Given that I have not tried to increase allocated memory,
    * when I increase allocated memory with sbrk(15*64K), then I should get the end of the first page.
    */
	auto prev = sbrk(15*64*1024);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == 64*1024, "Should have allocated 1M of memory");

   /*
    * Test test_extended_memory::test_page_memory_exceeded `ensure sbrk won't allocation more than 1M of memory 2`
    */
   prev = sbrk(0);
	eosio_assert(reinterpret_cast<uint32_t>(prev) == (1024*1024), "Should have allocated 1M of memory");

	eosio_assert(reinterpret_cast<int32_t>(sbrk(1)) == -1, "sbrk should have failed for trying to allocate too much memory");
}

void test_extended_memory::test_page_memory_negative_bytes() {
   sbrk((uint32_t)-1);
   eosio_assert(0, "Should have thrown exception for trying to remove memory");
}

void test_extended_memory::test_initial_buffer() {
	// initial buffer should be exhausted at 8192 bytes
	// 8176 left ( 12 + ptr header )
	char* ptr1 = (char*)malloc(12);
	eosio_assert(ptr1 != nullptr, "should have allocated 12 char buffer");

	char* ptr2 = (char*)malloc(8159);
	eosio_assert(ptr2 != nullptr, "should have allocate 8159 char buffer");

	// should overrun initial heap, allocated in 2nd heap
	char* ptr3 = (char*)malloc(20);
	eosio_assert(ptr3 != nullptr, "should have allocated a 20 char buffer");
   verify(ptr3, 0, 20);

}
