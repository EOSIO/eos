#pragma once
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

struct vm_interface {
    vm_interface(){};
    void (*init)(int vm_type);
    void (*deinit)();
    void (*validate)(const bytes& code);
    bool (*apply)(const eosio::chain::digest_type& code_id, const fc::raw::shared_string& code);
    bool (*call)(uint64_t contract, uint64_t func_name, uint64_t arg1, uint64_t arg2, uint64_t arg3);
    bool (*is_busy)();
};

}}
