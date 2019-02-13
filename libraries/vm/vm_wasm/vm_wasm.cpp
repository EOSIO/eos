#include <stdio.h>
#include <stdlib.h>

#include <eosio/chain/webassembly/wabt.hpp>
#include <eosio/chain/wasm_interface.hpp>
#include <vm_interface.hpp>


using namespace eosio::chain;
#if _INDEX == 1
    #define WASM_INTERFACE_INIT wasm_interface_init_1
    #define WASM_INTERFACE_DEINIT wasm_interface_deinit_1
    #define WASM_INTERFACE_VALIDATE wasm_interface_validate_1
    #define WASM_INTERFACE_APPLY wasm_interface_apply_1
    #define WASM_INTERFACE_IS_BUSY wasm_interface_is_busy_1
    #define WASM_INTERFACE_CALL wasm_interface_call_1
#elif _INDEX == 2
    #define WASM_INTERFACE_INIT wasm_interface_init_2
    #define WASM_INTERFACE_DEINIT wasm_interface_deinit_2
    #define WASM_INTERFACE_VALIDATE wasm_interface_validate_2
    #define WASM_INTERFACE_APPLY wasm_interface_apply_2
    #define WASM_INTERFACE_IS_BUSY wasm_interface_is_busy_2
    #define WASM_INTERFACE_CALL wasm_interface_call_2
#elif _INDEX == 3
    #define WASM_INTERFACE_INIT wasm_interface_init_3
    #define WASM_INTERFACE_DEINIT wasm_interface_deinit_3
    #define WASM_INTERFACE_VALIDATE wasm_interface_validate_3
    #define WASM_INTERFACE_APPLY wasm_interface_apply_3
    #define WASM_INTERFACE_IS_BUSY wasm_interface_is_busy_3
    #define WASM_INTERFACE_CALL wasm_interface_call_3
#elif _INDEX == 4
    #define WASM_INTERFACE_INIT wasm_interface_init_4
    #define WASM_INTERFACE_DEINIT wasm_interface_deinit_4
    #define WASM_INTERFACE_VALIDATE wasm_interface_validate_4
    #define WASM_INTERFACE_APPLY wasm_interface_apply_4
    #define WASM_INTERFACE_IS_BUSY wasm_interface_is_busy_4
    #define WASM_INTERFACE_CALL wasm_interface_call_4
#endif

static wasm_interface* wif = nullptr;

extern "C" void WASM_INTERFACE_INIT(int vm_type) {
    if (wif) {
        return;
    }
    wif = new wasm_interface((eosio::chain::wasm_interface::vm_type)vm_type);
}

extern "C" void WASM_INTERFACE_DEINIT() {
    if (!wif) {
        return;
    }
    delete wif;
    wif = nullptr;
}

extern "C" void WASM_INTERFACE_VALIDATE(const bytes& code) {
    wif->validate(code);
}

extern "C" bool WASM_INTERFACE_APPLY(const eosio::chain::digest_type& code_id, const fc::raw::shared_string& code) {
    return wif->apply(code_id, code);
}

extern "C" bool WASM_INTERFACE_IS_BUSY( ) {
    return wif->is_busy();
}
