#include <eosio/chain/webassembly/wavm.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/io/json.hpp>

#include "IR/Module.h"
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "IR/Operators.h"
#include "IR/Validate.h"
#include "Runtime/Linker.h"
#include "Runtime/Intrinsics.h"

#include <mutex>

using namespace IR;
using namespace Runtime;

namespace eosio { namespace chain { namespace webassembly { namespace wavm {

running_instance_context the_running_instance_context;

class wavm_instantiated_module : public wasm_instantiated_module_interface {
   public:
      wavm_instantiated_module(ModuleInstance* instance, std::unique_ptr<Module> module, std::vector<uint8_t> initial_mem) :
         _initial_memory(initial_mem),
         _instance(instance),
         _module(std::move(module))
      {}

      void apply(apply_context& context) override {
         vector<Value> args = {Value(context.receiver.to_uint64_t()),
	                            Value(context.act.account.to_uint64_t()),
                               Value(context.act.name.to_uint64_t())};

         call("apply", args, context);
      }

   private:
      void call(const string &entry_point, const vector <Value> &args, apply_context &context) {
         try {
            FunctionInstance* call = asFunctionNullable(getInstanceExport(_instance,entry_point));
            if( !call )
               return;

            EOS_ASSERT( getFunctionType(call)->parameters.size() == args.size(), wasm_exception, "" );

            //The memory instance is reused across all wavm_instantiated_modules, but for wasm instances
            // that didn't declare "memory", getDefaultMemory() won't see it
            MemoryInstance* default_mem = getDefaultMemory(_instance);
            if(default_mem) {
               //reset memory resizes the sandbox'ed memory to the module's init memory size and then
               // (effectively) memzeros it all
               resetMemory(default_mem, _module->memories.defs[0].type);

               char* memstart = &memoryRef<char>(getDefaultMemory(_instance), 0);
               memcpy(memstart, _initial_memory.data(), _initial_memory.size());
            }

            the_running_instance_context.memory = default_mem;
            the_running_instance_context.apply_ctx = &context;

            resetGlobalInstances(_instance);
            runInstanceStartFunc(_instance);
            Runtime::invokeFunction(call,args);
         } catch( const wasm_exit& e ) {
         } catch( const Runtime::Exception& e ) {
             FC_THROW_EXCEPTION(wasm_execution_error,
                         "cause: {cause}\n{callstack}",
                         ("cause", string(describeExceptionCause(e.cause)))
                         ("callstack", fc::json::to_string(e.callStack)));
         } FC_CAPTURE_AND_RETHROW()
      }


      std::vector<uint8_t>     _initial_memory;
      //naked pointer because ModuleInstance is opaque
      //_instance is deleted via WAVM's object garbage collection when wavm_rutime is deleted
      ModuleInstance*          _instance;
      std::unique_ptr<Module>  _module;
};


wavm_runtime::runtime_guard::runtime_guard() {
   // TODO clean this up
   //check_wasm_opcode_dispositions();
   Runtime::init();
}

wavm_runtime::runtime_guard::~runtime_guard() {
   Runtime::freeUnreferencedObjects({});
}

static weak_ptr<wavm_runtime::runtime_guard> __runtime_guard_ptr;
static std::mutex __runtime_guard_lock;

wavm_runtime::wavm_runtime() {
   std::lock_guard<std::mutex> l(__runtime_guard_lock);
   if (__runtime_guard_ptr.use_count() == 0) {
      _runtime_guard = std::make_shared<runtime_guard>();
      __runtime_guard_ptr = _runtime_guard;
   } else {
      _runtime_guard = __runtime_guard_ptr.lock();
   }
}

wavm_runtime::~wavm_runtime() {
}

std::unique_ptr<wasm_instantiated_module_interface> wavm_runtime::instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t> initial_memory) {
   std::unique_ptr<Module> module = std::make_unique<Module>();
   try {
      Serialization::MemoryInputStream stream((const U8*)code_bytes, code_size);
      WASM::serialize(stream, *module);
   } catch(const Serialization::FatalSerializationException& e) {
      EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
   } catch(const IR::ValidationException& e) {
      EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
   }

   eosio::chain::webassembly::common::root_resolver resolver;
   LinkResult link_result = linkModule(*module, resolver);
   ModuleInstance *instance = instantiateModule(*module, std::move(link_result.resolvedImports));
   EOS_ASSERT(instance != nullptr, wasm_exception, "Fail to Instantiate WAVM Module");

   return std::make_unique<wavm_instantiated_module>(instance, std::move(module), initial_memory);
}

void wavm_runtime::immediately_exit_currently_running_module() {
#ifdef _WIN32
   throw wasm_exit();
#else
   Platform::immediately_exit();
#endif
}

}}}}
