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
};

namespace LLVMJIT {
   bool getFunctionIndexFromExternalName(const char* externalName,Uptr& outFunctionDefIndex);
   llvm::Module* emitModule(const IR::Module& module);
   instantiated_code instantiateModule(const IR::Module& module);
}
}}}