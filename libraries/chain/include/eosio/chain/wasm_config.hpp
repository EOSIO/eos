#pragma once

#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <cstdint>
#include <fc/reflect/reflect.hpp>

namespace eosio { namespace chain {

struct wasm_config {
   std::uint32_t max_mutable_global_bytes;
   std::uint32_t max_table_elements;
   std::uint32_t max_section_elements;
   std::uint32_t max_linear_memory_init;
   std::uint32_t max_func_local_bytes;
   std::uint32_t max_nested_structures;
   std::uint32_t max_symbol_bytes;
   std::uint32_t max_module_bytes;
   std::uint32_t max_code_bytes;
   std::uint32_t max_pages;
   std::uint32_t max_call_depth;
};

inline constexpr bool operator==(const wasm_config& lhs, const wasm_config& rhs) {
   return lhs.max_mutable_global_bytes == rhs.max_mutable_global_bytes &&
     lhs.max_table_elements == rhs.max_table_elements &&
     lhs.max_section_elements == rhs.max_section_elements &&
     lhs.max_linear_memory_init == rhs.max_linear_memory_init &&
     lhs.max_func_local_bytes == rhs.max_func_local_bytes &&
     lhs.max_nested_structures == rhs.max_nested_structures &&
     lhs.max_symbol_bytes == rhs.max_symbol_bytes &&
     lhs.max_module_bytes == rhs.max_module_bytes &&
     lhs.max_code_bytes == rhs.max_code_bytes &&
     lhs.max_pages == rhs.max_pages &&
     lhs.max_call_depth == rhs.max_call_depth;
}
inline constexpr bool operator!=(const wasm_config& lhs, const wasm_config& rhs) {
   return !(lhs == rhs);
}

}}

FC_REFLECT(eosio::chain::wasm_config,
           (max_mutable_global_bytes)
           (max_table_elements)
           (max_section_elements)
           (max_linear_memory_init)
           (max_func_local_bytes)
           (max_nested_structures)
           (max_symbol_bytes)
           (max_module_bytes)
           (max_code_bytes)
           (max_pages)
           (max_call_depth)
)
