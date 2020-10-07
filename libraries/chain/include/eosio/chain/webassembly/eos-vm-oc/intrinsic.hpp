#pragma once

#include <eosio/chain/webassembly/eos-vm-oc/intrinsic_mapping.hpp>

#include <map>
#include <string>

namespace IR {
   struct FunctionType;
}

namespace eosio { namespace chain { namespace eosvmoc {

struct intrinsic {
   intrinsic(const char* name, void* function_ptr, size_t ordinal);
};

struct intrinsic_entry {
   const void* const function_ptr;
   const size_t ordinal;
};

using intrinsic_map_t = std::map<std::string, intrinsic_entry>;

const intrinsic_map_t& get_internal_intrinsic_map();

std::size_t get_intrinsic_ordinal(std::string_view name);

}}}
