#pragma once

#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
#include <vector>
#include <list>
namespace eosio { namespace chain {class apply_context;}}
#endif

struct eos_vm_oc_control_block {
   uint64_t magic;
   uintptr_t execution_thread_code_start;
   size_t execution_thread_code_length;
   uintptr_t execution_thread_memory_start;
   size_t execution_thread_memory_length;
#ifdef __cplusplus
   void* ctx;
   std::exception_ptr* eptr;
#else
   void* ctx;
   void* eptr;
#endif
   unsigned current_call_depth_remaining;
   int64_t current_linear_memory_pages; //-1 if no memory
   char* full_linear_memory_start;
   sigjmp_buf* jmp;
#ifdef __cplusplus
   std::list<std::vector<std::byte>>* bounce_buffers;
#else
   void* bounce_buffers;
#endif
   uintptr_t running_code_base;
   int64_t  first_invalid_memory_address;
   unsigned is_running;
   int64_t max_linear_memory_pages;
   void* globals;
};
