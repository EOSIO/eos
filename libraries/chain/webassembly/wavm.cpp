#include <eosio/chain/webassembly/wavm.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/apply_context.hpp>

#include "IR/Module.h"
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "IR/Operators.h"
#include "IR/Validate.h"
#include "Runtime/Linker.h"
#include "Runtime/Intrinsics.h"


using namespace IR;
using namespace Runtime;

namespace eosio { namespace chain { namespace webassembly { namespace wavm {

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

void entry::call(const string &entry_point, const vector <Value> &args, apply_context &context)
{
   try {
      FunctionInstance* call = asFunctionNullable(getInstanceExport(instance,entry_point) );
      if( !call ) {
         return;
      }

      FC_ASSERT( getFunctionType(call)->parameters.size() == args.size() );

      runInstanceStartFunc(instance);
      Runtime::invokeFunction(call,args);
   } catch( const Runtime::Exception& e ) {
      FC_THROW_EXCEPTION(wasm_execution_error,
                         "cause: ${cause}\n${callstack}",
                         ("cause", string(describeExceptionCause(e.cause)))
                         ("callstack", e.callStack));
   } FC_CAPTURE_AND_RETHROW()
}

void entry::call_apply(apply_context& context)
{
   vector<Value> args = {Value(uint64_t(context.act.account)),
                         Value(uint64_t(context.act.name))};

   call("apply", args, context);
}

void entry::call_error(apply_context& context)
{
   vector<Value> args = {/* */};
   call("error", args, context);
}

int entry::sbrk(int num_bytes) {
   // TODO: omitted checktime function from previous version of sbrk, may need to be put back in at some point
   constexpr uint32_t NBPPL2  = IR::numBytesPerPageLog2;

   MemoryInstance*  default_mem    = Runtime::getDefaultMemory(instance);
   if(!default_mem)
      throw eosio::chain::page_memory_error();

   const uint32_t         num_pages      = Runtime::getMemoryNumPages(default_mem);
   const uint32_t         min_bytes      = (num_pages << NBPPL2) > UINT32_MAX ? UINT32_MAX : num_pages << NBPPL2;
   const uint32_t         prev_num_bytes = sbrk_bytes; //_num_bytes;

   // round the absolute value of num_bytes to an alignment boundary
   num_bytes = (num_bytes + 7) & ~7;

   if ((num_bytes > 0) && (prev_num_bytes > (wasm_constraints::maximum_linear_memory - num_bytes)))  // test if allocating too much memory (overflowed)
      throw eosio::chain::page_memory_error();
   else if ((num_bytes < 0) && (prev_num_bytes < (min_bytes - num_bytes))) // test for underflow
      throw eosio::chain::page_memory_error();

   // update the number of bytes allocated, and compute the number of pages needed
   sbrk_bytes += num_bytes;
   const uint32_t num_desired_pages = (sbrk_bytes + IR::numBytesPerPage - 1) >> NBPPL2;

   // grow or shrink the memory to the desired number of pages
   if (num_desired_pages > num_pages)
      Runtime::growMemory(default_mem, num_desired_pages - num_pages);
   else if (num_desired_pages < num_pages)
      Runtime::shrinkMemory(default_mem, num_pages - num_desired_pages);

   return prev_num_bytes;

}

void entry::reset(const info& base_info) {
   MemoryInstance* default_mem = getDefaultMemory(instance);
   if(default_mem) {
      shrinkMemory(default_mem, getMemoryNumPages(default_mem) - 1);

      char* memstart = &memoryRef<char>( getDefaultMemory(instance), 0 );
      memset( memstart + base_info.mem_end, 0, ((1<<16) - base_info.mem_end) );
      memcpy( memstart, base_info.mem_image.data(), base_info.mem_end);
   }
   resetGlobalInstances(instance);

   sbrk_bytes = base_info.default_sbrk_bytes;
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

   MemoryInstance* current_memory = Runtime::getDefaultMemory(instance);

   uint32_t sbrk_bytes = 0;
   if(current_memory) {
      sbrk_bytes = Runtime::getMemoryNumPages(current_memory) << IR::numBytesPerPageLog2;
   }

   return entry(instance, module, sbrk_bytes);
};

}}}}