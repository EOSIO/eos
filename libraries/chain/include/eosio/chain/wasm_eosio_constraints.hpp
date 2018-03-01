#pragma once

namespace IR {
    struct Module;
};

namespace eosio { namespace chain {

namespace wasm_constraints {
   constexpr unsigned maximum_linear_memory      = 2*1024*1024;//bytes
   constexpr unsigned maximum_mutable_globals    = 1024;        //bytes
   constexpr unsigned maximum_table_elements     = 1024;        //elements
   constexpr unsigned maximum_linear_memory_init = 64*1024;     //bytes

   static constexpr unsigned wasm_page_size      = 64*1024;

   static_assert(maximum_linear_memory%wasm_page_size      == 0, "maximum_linear_memory must be mulitple of wasm page size");
   static_assert(maximum_mutable_globals%4                 == 0, "maximum_mutable_globals must be mulitple of 4");
   static_assert(maximum_table_elements*8%4096             == 0, "maximum_table_elements*8 must be mulitple of 4096");
   static_assert(maximum_linear_memory_init%wasm_page_size == 0, "maximum_linear_memory_init must be mulitple of wasm page size");
}

//Throws if something in the module violates
void validate_eosio_wasm_constraints(const IR::Module& m);

//Throws if an opcode is neither whitelisted nor blacklisted
void check_wasm_opcode_dispositions();

}}
