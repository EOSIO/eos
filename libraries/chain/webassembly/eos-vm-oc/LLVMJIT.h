#pragma once

#include "Inline/BasicTypes.h"
#include "IR/Module.h"

#pragma push_macro("N")
#undef N
#include "llvm/IR/Module.h"
#pragma pop_macro("N")
#include <vector>
#include <map>

namespace eosio { namespace chain { namespace eosvmoc {

struct instantiated_code {
   std::vector<uint8_t> code;
   std::map<unsigned, uintptr_t> function_offsets;
   unsigned table_offset;
};

namespace LLVMJIT {
   bool getFunctionIndexFromExternalName(const char* externalName,Uptr& outFunctionDefIndex);
   llvm::Module* emitModule(const IR::Module& module, const std::map<const IR::FunctionType*, uint32_t>& wavm_func_type_to_ordinal);
   instantiated_code instantiateModule(const IR::Module& module, const std::map<const IR::FunctionType*, uint32_t>& wavm_func_type_to_ordinal);
}
}}}