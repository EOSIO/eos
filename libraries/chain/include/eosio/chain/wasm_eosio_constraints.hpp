#pragma once

#include <functional>
#include <vector>

namespace IR {
   struct Module;
}

namespace eosio { namespace chain { namespace wasm_constraints {
   constexpr unsigned maximum_linear_memory      = 33*1024*1024;//bytes
   constexpr unsigned maximum_mutable_globals    = 1024;        //bytes
   constexpr unsigned maximum_table_elements     = 1024;        //elements
   constexpr unsigned maximum_section_elements   = 1024;        //elements
   constexpr unsigned maximum_linear_memory_init = 64*1024;     //bytes
   constexpr unsigned maximum_func_local_bytes   = 8192;        //bytes
   constexpr unsigned maximum_call_depth         = 250;         //nested calls
   constexpr unsigned maximum_code_size          = 20*1024*1024; 

   static constexpr unsigned wasm_page_size      = 64*1024;

   static_assert(maximum_linear_memory%wasm_page_size      == 0, "maximum_linear_memory must be mulitple of wasm page size");
   static_assert(maximum_mutable_globals%4                 == 0, "maximum_mutable_globals must be mulitple of 4");
   static_assert(maximum_table_elements*8%4096             == 0, "maximum_table_elements*8 must be mulitple of 4096");
   static_assert(maximum_linear_memory_init%wasm_page_size == 0, "maximum_linear_memory_init must be mulitple of wasm page size");
   static_assert(maximum_func_local_bytes%8                == 0, "maximum_func_local_bytes must be mulitple of 8");
   static_assert(maximum_func_local_bytes>32                   , "maximum_func_local_bytes must be greater than 32");
} // namespace  wasm_constraints

}} // namespace eosio, chain
