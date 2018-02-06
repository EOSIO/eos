#pragma once

namespace IR {
    struct Module;
};

namespace eosio { namespace chain {

//Throws if something in the module violates
void validate_eosio_wasm_constraints(const IR::Module& m);

}}
