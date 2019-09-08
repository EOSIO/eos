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

namespace rodeos {

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
   bool is_running;
};

struct no_offset{};
struct code_offset{ unsigned offset; };    //relative to code_offset
struct import_ordinal{ unsigned import; };

using rodeos_optional_offset_or_import_t = fc::static_variant<no_offset, code_offset, import_ordinal>;

struct code_descriptor {
   digest_type code_hash;
   uint8_t vm_version;
   uint8_t codegen_version;
   size_t code_begin;
   rodeos_optional_offset_or_import_t start_offset;
   unsigned apply_offset;
   int starting_memory_pages;
   std::vector<uint8_t> initdata;
   unsigned initdata_pre_memory_size;
   std::vector<rodeos_optional_offset_or_import_t> table_mappings;

   Runtime::ModuleInstance* mi;
};

}}}

FC_REFLECT(eosio::chain::rodeos::no_offset, );
FC_REFLECT(eosio::chain::rodeos::code_offset, (offset));
FC_REFLECT(eosio::chain::rodeos::import_ordinal, (import));
FC_REFLECT(eosio::chain::rodeos::code_descriptor, (code_hash)(vm_version)(codegen_version)(code_begin)(start_offset)(apply_offset)(starting_memory_pages)(initdata)(initdata_pre_memory_size)(table_mappings));