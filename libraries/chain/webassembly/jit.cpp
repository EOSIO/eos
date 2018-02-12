#include <eosio/chain/webassembly/jit.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>

#include "IR/Module.h"
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "IR/Operators.h"
#include "IR/Validate.h"
#include "Runtime/Linker.h"
#include "Runtime/Intrinsics.h"


using namespace IR;
using namespace Runtime;

namespace eosio { namespace chain { namespace webassembly { namespace jit {

/**
 * Integration with the WASM Linker to resolve our intrinsics
 */
struct root_resolver : Runtime::Resolver
{
   bool resolve(const string& mod_name,
                const string& export_name,
                ObjectType type,
                ObjectInstance*& out) override
   { try {
      // Try to resolve an intrinsic first.
      if(IntrinsicResolver::singleton.resolve(mod_name,export_name,type, out)) {
         return true;
      }

      FC_ASSERT( !"unresolvable", "${module}.${export}", ("module",mod_name)("export",export_name) );
      return false;
   } FC_CAPTURE_AND_RETHROW( (mod_name)(export_name) ) }
};

void entry::reset(const info& base_info) {
   if(getDefaultMemory(instance)) {
      char* memstart = &memoryRef<char>( getDefaultMemory(instance), 0 );
      memset( memstart + base_info.mem_end, 0, ((1<<16) - base_info.mem_end) );
      memcpy( memstart, base_info.mem_image.data(), base_info.mem_end);
   }
   resetGlobalInstances(instance);
}


entry entry::build(const char* wasm_binary, size_t wasm_binary_size) {
   Module* module = new Module();
   Serialization::MemoryInputStream stream((const U8 *) wasm_binary, wasm_binary_size);
   WASM::serializeWithInjection(stream, *module);
   validate_eosio_wasm_constraints(*module);

   root_resolver resolver;
   LinkResult link_result = linkModule(*module, resolver);
   ModuleInstance *instance = instantiateModule(*module, std::move(link_result.resolvedImports));
   FC_ASSERT(instance != nullptr);
   return entry(instance, module);
};

}}}}