#pragma once

namespace IR {
    struct Module;
};

namespace eosio { namespace chain {

namespace wasm_constraints {
   //Be aware that some of these are required to be a multiple of some internal number
   constexpr unsigned maximum_linear_memory      = 1024*1024;  //bytes
   constexpr unsigned maximum_mutable_globals    = 1024;       //bytes
   constexpr unsigned maximum_table_elements     = 1024;       //elements
   constexpr unsigned maximum_linear_memory_init = 64*1024;    //bytes
}

//Throws if something in the module violates
void validate_eosio_wasm_constraints(const IR::Module& m);

}}
