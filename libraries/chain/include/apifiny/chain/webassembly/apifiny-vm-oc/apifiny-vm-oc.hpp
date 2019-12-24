#pragma once

#include <apifiny/chain/types.hpp>
#include <apifiny/chain/webassembly/apifiny-vm-oc/apifiny-vm-oc.h>

#include <exception>

#include <stdint.h>
#include <stddef.h>
#include <setjmp.h>

#include <vector>
#include <list>

namespace apifiny { namespace chain {

class apply_context;

namespace apifinyvmoc {

using control_block = apifiny_vm_oc_control_block;

struct no_offset{};
struct code_offset{ size_t offset; };    //relative to code_begin
struct intrinsic_ordinal{ size_t ordinal; };

using apifinyvmoc_optional_offset_or_import_t = fc::static_variant<no_offset, code_offset, intrinsic_ordinal>;

struct code_descriptor {
   digest_type code_hash;
   uint8_t vm_version;
   uint8_t codegen_version;
   size_t code_begin;
   apifinyvmoc_optional_offset_or_import_t start;
   unsigned apply_offset;
   int starting_memory_pages;
   size_t initdata_begin;
   unsigned initdata_size;
   unsigned initdata_prologue_size;
};

enum apifinyvmoc_exitcode : int {
   APIFINYVMOC_EXIT_CLEAN_EXIT = 1,
   APIFINYVMOC_EXIT_CHECKTIME_FAIL,
   APIFINYVMOC_EXIT_SEGV,
   APIFINYVMOC_EXIT_EXCEPTION
};

}}}

FC_REFLECT(apifiny::chain::apifinyvmoc::no_offset, );
FC_REFLECT(apifiny::chain::apifinyvmoc::code_offset, (offset));
FC_REFLECT(apifiny::chain::apifinyvmoc::intrinsic_ordinal, (ordinal));
FC_REFLECT(apifiny::chain::apifinyvmoc::code_descriptor, (code_hash)(vm_version)(codegen_version)(code_begin)(start)(apply_offset)(starting_memory_pages)(initdata_begin)(initdata_size)(initdata_prologue_size));

#define APIFINYVMOC_INTRINSIC_INIT_PRIORITY __attribute__((init_priority(198)))