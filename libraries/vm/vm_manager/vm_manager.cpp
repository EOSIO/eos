#include "vm_manager.hpp"
#include <vm_interface.hpp>

#include <eosiolib_native/vm_api.h>
#include <chain_api.hpp>
#include "native.hpp"

using namespace eosio::chain;

extern "C" {
    void wasm_interface_init_1 (int vm_type);
    void wasm_interface_deinit_1();
    void wasm_interface_validate_1 (const bytes& code);
    bool wasm_interface_apply_1 (const eosio::chain::digest_type& code_id, const fc::raw::shared_string& code);
    bool wasm_interface_is_busy_1 ();
    bool wasm_interface_call_1(uint64_t contract, uint64_t func_name, uint64_t arg1, uint64_t arg2, uint64_t arg3);

    void wasm_interface_init_2(int vm_type);
    void wasm_interface_deinit_2();
    void wasm_interface_validate_2(const bytes& code);
    bool wasm_interface_apply_2(const eosio::chain::digest_type& code_id, const fc::raw::shared_string& code);
    bool wasm_interface_is_busy_2 ();
    bool wasm_interface_call_2(uint64_t contract, uint64_t func_name, uint64_t arg1, uint64_t arg2, uint64_t arg3);

    void wasm_interface_init_3(int vm_type);
    void wasm_interface_deinit_3();
    void wasm_interface_validate_3(const bytes& code);
    bool wasm_interface_apply_3(const eosio::chain::digest_type& code_id, const fc::raw::shared_string& code);
    bool wasm_interface_is_busy_3();
    bool wasm_interface_call_3(uint64_t contract, uint64_t func_name, uint64_t arg1, uint64_t arg2, uint64_t arg3);


    void wasm_interface_init_4(int vm_type);
    void wasm_interface_deinit_4();
    void wasm_interface_validate_4(const bytes& code);
    bool wasm_interface_apply_4(const eosio::chain::digest_type& code_id, const fc::raw::shared_string& code);
    bool wasm_interface_is_busy_4();
    bool wasm_interface_call_4(uint64_t contract, uint64_t func_name, uint64_t arg1, uint64_t arg2, uint64_t arg3);

}

void lua_vm_init(struct vm_api* api);
void lua_vm_deinit(void);

int lua_vm_setcode(uint64_t account, const vector<char>& code, vector<char>& output);
int lua_vm_apply(uint64_t receiver, uint64_t account, uint64_t act);
int lua_vm_call(uint64_t account, uint64_t func, uint64_t arg1, uint64_t arg2, uint64_t arg3, void* extra_args, size_t extra_args_size);

int lua_vm_preload(uint64_t account);
int lua_vm_load(uint64_t account);
int lua_vm_unload(uint64_t account);

namespace eosio { namespace chain {

struct vm_manager_impl {
    vm_manager_impl(){};
    vm_interface vm_wabt;
    vm_interface vm_wavm;
    vm_interface vm_wabt_call;
    vm_interface vm_wavm_call;
};

vm_manager* vm_manager::current_instance = nullptr;

vm_manager::vm_manager(): my(new vm_manager_impl()),
                        vm_wabt(my->vm_wabt),
                        vm_wavm(my->vm_wavm),
                        vm_wabt_call(my->vm_wabt_call),
                        vm_wavm_call(my->vm_wavm_call)  {
    wasm_interface_init_1(1); // wabt
    wasm_interface_init_2(0); // wavm
    wasm_interface_init_3(1); // wabt
    wasm_interface_init_4(0); // wavm

    vm_wabt.init = wasm_interface_init_1;
    vm_wabt.deinit = wasm_interface_deinit_1;
    vm_wabt.validate = wasm_interface_validate_1;
    vm_wabt.apply = wasm_interface_apply_1;
    vm_wabt.is_busy = wasm_interface_is_busy_1;

    vm_wavm.init = wasm_interface_init_2;
    vm_wavm.deinit = wasm_interface_deinit_2;
    vm_wavm.validate = wasm_interface_validate_2;
    vm_wavm.apply = wasm_interface_apply_2;
    vm_wavm.is_busy = wasm_interface_is_busy_2;

    vm_wabt_call.init = wasm_interface_init_3;
    vm_wabt_call.deinit = wasm_interface_deinit_3;
    vm_wabt_call.validate = wasm_interface_validate_3;
    vm_wabt_call.apply = wasm_interface_apply_3;
    vm_wabt_call.is_busy = wasm_interface_is_busy_3;

    vm_wavm_call.init = wasm_interface_init_4;
    vm_wavm_call.deinit = wasm_interface_deinit_4;
    vm_wavm_call.validate = wasm_interface_validate_4;
    vm_wavm_call.apply = wasm_interface_apply_4;
    vm_wavm_call.is_busy = wasm_interface_is_busy_4;


    current_instance = this;
}

vm_manager::~vm_manager() {
    wasm_interface_deinit_1();
    wasm_interface_deinit_2();
    current_instance = nullptr;
}

vm_manager& vm_manager::get() {
    get_vm_api()->eosio_assert(current_instance != nullptr, "current_instance should not be null");
    return *current_instance;
}

void vm_manager::setcode(int vm_type, uint64_t account, const bytes& code, bytes& output) {
    string s;
    get_chain_api_cpp()->n2str(account, s);
    vmelog("++++vm_manager::setcode, account %s\n", s.c_str());
    if (vm_type == VM_TYPE_WASM) {
        vm_wabt.validate(code);
        output = code;
    } else {
        get_vm_api()->eosio_assert(0, "bad vm type");
    }
}

void vm_manager::apply(int vm_type, const eosio::chain::digest_type& code_id, const fc::raw::shared_string& code) {
    if (vm_type == VM_TYPE_WASM) {
        if (arg_stack.size()) {
            std::queue<std::tuple<const char*, size_t>>().swap(arg_stack);
        }
        if (return_stack.size()) {
            std::queue<std::tuple<char*, size_t>>().swap(return_stack);
        }
        uint64_t receiver;
        receiver = get_vm_api()->current_receiver();
        if (get_vm_api()->is_privileged(receiver)) {
            if (!vm_wavm.apply(code_id, code)) {
                vm_wabt.apply(code_id, code);
            }
        } else {
            vm_wabt.apply(code_id, code);
        }
    }
}

bool vm_manager::is_busy() {
    return vm_wavm.is_busy();
}

}}

