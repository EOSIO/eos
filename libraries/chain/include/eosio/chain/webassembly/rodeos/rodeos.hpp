#pragma once

#include <eosio/chain/types.hpp>

#include <exception>

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>

namespace Runtime {
struct ModuleInstance;
}

namespace eosio { namespace chain {

class apply_context;

namespace rodeos {

struct control_block {
   uint64_t magic;
   uintptr_t execution_thread_code_start;
   size_t execution_thread_code_length;
   uintptr_t execution_thread_memory_start;
   size_t execution_thread_memory_length;
   apply_context* ctx;
   std::exception_ptr* eptr;
   unsigned current_call_depth_remaining;
   unsigned bouce_buffer_ptr;
   int64_t current_linear_memory_pages; //-1 if no memory
   uintptr_t full_linear_memory_start;
   sigjmp_buf* jmp;
   bool is_running;
};

struct code_descriptor {
   digest_type code_hash;
   uint8_t vm_version;
   uint8_t codegen_version;
   size_t code_offset;
   unsigned start_offset;  //these are relative to code_offset
   unsigned apply_offset;
   int starting_memory_pages;
   std::vector<uint8_t> initdata;
   unsigned initdata_pre_memory_size;
   std::vector<int> table_mappings; // <0 is negative value of intrinsic offset, otherwise wasm func with offset from code_offset

   Runtime::ModuleInstance* mi;
};

}}}