#include <eoslib/eos.hpp>
#include "../test_api.hpp"
#include <eoslib/memory.hpp>

using namespace eosio;

void verify( const void* const ptr, const uint32_t val, const uint32_t size) {
	const char* char_ptr = (const char*)ptr;
	for (uint32_t i = 0; i < size; ++i)
		assert(static_cast<uint32_t>(static_cast<unsigned char>(char_ptr[i])) == val, "buffer slot doesn't match");
}

#define PRINT_PTR(x) prints("PTR : "); print((uint32_t)x, 4); prints("\n");

void test_extended_memory::test_page_memory() {
	constexpr uint32_t _64K = 64*1024;
	auto prev = sbrk(0);
	assert(reinterpret_cast<uint32_t>(prev) == _64K, "Should initially have 1 64K page allocated");

	prev = sbrk(1);
	assert(reinterpret_cast<uint32_t>(prev) == _64K, "Should still be pointing to the end of the 1st 64K page");

	prev = sbrk(2);
	assert(reinterpret_cast<uint32_t>(prev) == _64K+8, "Should point to 8 past the end of 1st 64K page");

	prev = sbrk(_64K - 17);
	assert(reinterpret_cast<uint32_t>(prev) == _64K+16, "Should point to 16 past the end of the 1st 64K page");

	prev = sbrk(1);
	assert(reinterpret_cast<uint32_t>(prev) == 2*_64K, "Should point to the end of the 2nd 64K page");

	prev = sbrk(_64K);
	assert(reinterpret_cast<uint32_t>(prev) == 2*_64K+8, "Should point to 8 past the end of the 2nd 64K page");

	prev = sbrk(_64K - 15);
	assert(reinterpret_cast<uint32_t>(prev) == 3*_64K+8, "Should point to 8 past the end of the 3rd 64K page");

	prev = sbrk(2*_64K-1);
	assert(reinterpret_cast<uint32_t>(prev) == 4*_64K, "Should point to the end of the 4th 64K page");

	prev = sbrk(2*_64K);
	assert(reinterpret_cast<uint32_t>(prev) == 6*_64K, "Should point to the end of the 6th 64K page");

	prev = sbrk(2*_64K+1);
	assert(reinterpret_cast<uint32_t>(prev) == 8*_64K, "Should point to the end of the 8th 64K page");

	prev = sbrk(6*_64K-15);
	assert(reinterpret_cast<uint32_t>(prev) == 10*_64K+8, "Should point to 8 past the end of the 10th 64K page");

	prev = sbrk(0);
	assert(reinterpret_cast<uint32_t>(prev) == 16*_64K, "Should point to 8 past the end of the 16th 64K page");
}

void test_extended_memory::test_page_memory_exceeded() {
	auto prev = sbrk(15*64*1024);
	assert(reinterpret_cast<uint32_t>(prev) == 64*1024, "Should have allocated 1M of memory");
	sbrk(1);
	assert(0, "Should have thrown exception for trying to allocate too much memory");
}

void test_extended_memory::test_page_memory_negative_bytes() {
	sbrk(-1);
	assert(0, "Should have thrown exception for trying to remove memory");
}

void test_extended_memory::test_initial_buffer() {
	// initial buffer should be exhausted at 8192 bytes
	// 8176 left ( 12 + ptr header )
	char* ptr1 = (char*)eosio::malloc(12);
	assert(ptr1 != nullptr, "should have allocated 12 char buffer");

	char* ptr2 = (char*)eosio::malloc(8159);
	assert(ptr2 != nullptr, "should have allocate 8159 char buffer");

	// should overrun initial heap, allocated in 2nd heap
	char* ptr3 = (char*)eosio::malloc(20);
	assert(ptr3 != nullptr, "should have allocated a 20 char buffer");
   verify(ptr3, 0, 20);

}
