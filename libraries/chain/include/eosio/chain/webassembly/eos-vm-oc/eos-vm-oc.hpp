#pragma once

#include <eosio/chain/types.hpp>

#include <exception>

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>

#include <vector>
#include <list>

namespace Runtime {
struct ModuleInstance;
}

namespace eosio { namespace chain {

class apply_context;

namespace eosvmoc {

struct control_block {
   uint64_t magic;
   uintptr_t execution_thread_code_start;
   size_t execution_thread_code_length;
   uintptr_t execution_thread_memory_start;
   size_t execution_thread_memory_length;
   apply_context* ctx;
   std::exception_ptr* eptr;
   unsigned current_call_depth_remaining;
   unsigned bouce_buffer_ptr;  //XX remove me
   int64_t current_linear_memory_pages; //-1 if no memory
   uintptr_t full_linear_memory_start;
   sigjmp_buf* jmp;
   std::list<std::vector<uint8_t>>* bounce_buffers;
   uintptr_t running_code_base;
   bool is_running;
};

struct no_offset{};
struct code_offset{ size_t offset; };    //relative to code_begin
struct intrinsic_ordinal{ size_t ordinal; };

using eosvmoc_optional_offset_or_import_t = fc::static_variant<no_offset, code_offset, intrinsic_ordinal>;

struct code_descriptor {
   digest_type code_hash;
   uint8_t vm_version;
   uint8_t codegen_version;
   size_t code_begin;
   eosvmoc_optional_offset_or_import_t start;
   unsigned apply_offset;
   int starting_memory_pages;
   std::vector<uint8_t> initdata;
   unsigned initdata_pre_memory_size;
};

}}}

FC_REFLECT(eosio::chain::eosvmoc::no_offset, );
FC_REFLECT(eosio::chain::eosvmoc::code_offset, (offset));
FC_REFLECT(eosio::chain::eosvmoc::intrinsic_ordinal, (ordinal));
FC_REFLECT(eosio::chain::eosvmoc::code_descriptor, (code_hash)(vm_version)(codegen_version)(code_begin)(start)(apply_offset)(starting_memory_pages)(initdata)(initdata_pre_memory_size));