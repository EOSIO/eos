#include "memory.hpp"

eosio::memory_manager memory_heap;

extern "C" {

void* malloc(size_t size)
{
   return eosio::memory_heap.malloc(size);
}

void* realloc(void* ptr, size_t size)
{
   return eosio::memory_heap.realloc(ptr, size);
}

void free(void* ptr)
{
   return eosio::memory_heap.free(ptr);
}

}
