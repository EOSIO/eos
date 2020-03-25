#pragma once

#include <cstddef>

namespace eosio { namespace chain { namespace eosvmoc {

extern "C" void eosvmoc_switch_stack(void* stack, void(*fn)(void*), void* data);

// Allows wasm code to run with a stack whose size can be adjusted based
// on the configurable max_call_depth.  It is assumed that max_call_depth
// is rarely changed.
struct execution_stack {
   // Must match the limit enforced by codegen.
   static constexpr std::size_t max_bytes_per_frame = 16*1024;

   void * stack_top = nullptr;
   std::size_t stack_size = 0;
   // Assume that the default stack is large enough if we can't consume more than 4 MB
   std::size_t call_depth_limit = 4*1024*1024 / max_bytes_per_frame;

   execution_stack() = default;
   execution_stack(const execution_stack&) = delete;
   execution_stack& operator=(const execution_stack&) = delete;

   void reset(std::size_t max_call_depth);
   void reset();

   template<typename F>
   void run(F&& f) {
      if (stack_top) {
         eosvmoc_switch_stack(stack_top, [](void* data) { (*static_cast<F*>(data))(); }, &f);
      } else {
         f();
      }
   }

   ~execution_stack() {
      reset();
   }
};

}}}
