#include <eosio/chain/webassembly/wavm.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
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
   } catch( const wasm_exit& e ) {
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
      return -1;

   const uint32_t         num_pages      = Runtime::getMemoryNumPages(default_mem);
   const uint32_t         prev_num_bytes = sbrk_bytes; //_num_bytes;

   // round the absolute value of num_bytes to an alignment boundary
   num_bytes = (num_bytes + 7) & ~7;

   if ((num_bytes > 0) && (prev_num_bytes > (wasm_constraints::maximum_linear_memory - num_bytes)))  // test if allocating too much memory (overflowed)
      return -1;

   const uint32_t num_desired_pages = (sbrk_bytes + num_bytes + IR::numBytesPerPage - 1) >> NBPPL2;

   if(Runtime::growMemory(default_mem, num_desired_pages - num_pages) == -1)
      return -1;
   
   sbrk_bytes += num_bytes;

   return prev_num_bytes;

}

void entry::reset(const info& base_info) {
   MemoryInstance* default_mem = getDefaultMemory(instance);
   if(default_mem) {
      shrinkMemory(default_mem, getMemoryNumPages(default_mem) - 1);
   }
}

void entry::prepare( const info& base_info ) {
   resetGlobalInstances(instance);
   MemoryInstance* memory_instance = getDefaultMemory(instance);
   if(memory_instance) {
      resetMemory(memory_instance, module->memories.defs[0].type);

      char* memstart = &memoryRef<char>(getDefaultMemory(instance), 0);
      memcpy(memstart, base_info.mem_image.data(), base_info.mem_image.size());
      sbrk_bytes = Runtime::getMemoryNumPages(memory_instance) << IR::numBytesPerPageLog2;
   }
}

entry entry::build(const char* wasm_binary, size_t wasm_binary_size) {
   Module* module = new Module();
   Serialization::MemoryInputStream stream((const U8 *) wasm_binary, wasm_binary_size);
   WASM::serialize(stream, *module);

   root_resolver resolver;
   LinkResult link_result = linkModule(*module, resolver);
   ModuleInstance *instance = instantiateModule(*module, std::move(link_result.resolvedImports));
   FC_ASSERT(instance != nullptr);

   return entry(instance, module, 0);
};

info::info( const entry &wavm )
{
   default_sbrk_bytes = wavm.sbrk_bytes;
   const auto* module = wavm.module;

   //populate the module's data segments in to a vector so the initial state can be
   // restored on each invocation
   //Be Warned, this may need to be revisited when module imports make sense. The
   // code won't handle data segments that initalize an imported memory which I think
   // is valid.
   for(const DataSegment& data_segment : module->dataSegments) {
      FC_ASSERT(data_segment.baseOffset.type == InitializerExpression::Type::i32_const);
      FC_ASSERT(module->memories.defs.size());
      const U32 base_offset = data_segment.baseOffset.i32;
      const Uptr memory_size = (module->memories.defs[0].type.size.min << IR::numBytesPerPageLog2);
      if (base_offset >= memory_size || base_offset + data_segment.data.size() > memory_size)
         FC_THROW_EXCEPTION(wasm_execution_error, "WASM data segment outside of valid memory range");
      if (base_offset + data_segment.data.size() > mem_image.size())
         mem_image.resize(base_offset + data_segment.data.size(), 0x00);
      memcpy(mem_image.data() + base_offset, data_segment.data.data(), data_segment.data.size());
   }
}


}}}}
