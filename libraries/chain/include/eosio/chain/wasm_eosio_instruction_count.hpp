#pragma once

namespace IR {
    struct Module;
};

namespace eosio { namespace chain {

namespace wasm_esio_instruction_count {
   void inject_instruction_count(const IR::Module& module);
}

}}
