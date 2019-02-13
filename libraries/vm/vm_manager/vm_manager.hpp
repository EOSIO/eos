#pragma once

#include <eosio/chain/types.hpp>
#include <queue>
#include <tuple>

namespace eosio { namespace chain {

struct vm_interface;
struct vm_manager_impl;

class vm_manager {
public:
    static vm_manager* current_instance;
    static vm_manager& get();
    vm_manager();
    ~vm_manager();
    void setcode(int vm_type, uint64_t account, const bytes& code, bytes& output);
    void apply(int vm_type, const digest_type& code_id, const fc::raw::shared_string& code);
    void call(uint64_t contract, uint64_t func_name, uint64_t arg1, uint64_t arg2, uint64_t arg3, const char* extra_args, size_t in_size, char* out, size_t out_size);
    int get_arg(char* arg, size_t size);
    int set_result(const char* result, size_t size);

    bool is_busy();

    std::unique_ptr<vm_manager_impl> my;

    vm_interface& vm_wabt;
    vm_interface& vm_wavm;

    vm_interface& vm_wabt_call;
    vm_interface& vm_wavm_call;

    std::queue<std::tuple<const char*, size_t>> arg_stack;
    std::queue<std::tuple<char*, size_t>> return_stack;

};

}}


