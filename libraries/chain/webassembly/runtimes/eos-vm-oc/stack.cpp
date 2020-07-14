#include <eosio/chain/webassembly/eos-vm-oc/stack.hpp>
#include <fc/exception/exception.hpp>
#include <sys/mman.h>

using namespace eosio::chain::eosvmoc;

void execution_stack::reset(std::size_t max_call_depth) {
   if(max_call_depth > call_depth_limit) {
      reset();
      std::size_t new_stack_size = max_call_depth * max_bytes_per_frame + 4*1024*1024;
      void * ptr = mmap(nullptr, new_stack_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, 0, 0);
      FC_ASSERT(ptr != MAP_FAILED, "Failed to allocate wasm stack");
      mprotect(ptr, max_bytes_per_frame, PROT_NONE);
      stack_top = (char*)ptr + new_stack_size;
      stack_size = new_stack_size;
      call_depth_limit = max_call_depth;
   }
}

void execution_stack::reset() {
   if(stack_top) {
      munmap((char*)stack_top - stack_size, stack_size);
      stack_top = nullptr;
      stack_size = 0;
      call_depth_limit = 4*1024*1024 / max_bytes_per_frame;
   }
}
