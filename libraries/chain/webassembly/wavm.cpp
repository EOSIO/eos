#include <eosio/chain/webassembly/wavm.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>

#include "IR/Module.h"
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "IR/Operators.h"
#include "IR/Validate.h"
#include "Runtime/Linker.h"
#include "Runtime/Intrinsics.h"

#include <vector>
#include <iterator>

using namespace IR;
using namespace Runtime;

struct counter {
   unsigned long long count;
   ~counter() {
      printf("total wasm exectuion time %llu\n", count);
   }
};
static counter the_counter;

namespace eosio { namespace chain { namespace webassembly { namespace wavm {

running_instance_context the_running_instance_context;

namespace detail {
struct wavm_runtime_initializer {
   wavm_runtime_initializer() {
      Runtime::init();
   }
};

using live_module_ref = std::list<ObjectInstance*>::iterator;

struct wavm_live_modules {
   live_module_ref add_live_module(ModuleInstance* module_instance) {
      return live_modules.insert(live_modules.begin(), asObject(module_instance));
   }

   void remove_live_module(live_module_ref it) {
      live_modules.erase(it);
      run_wavm_garbage_collection();
   }

   void run_wavm_garbage_collection() {
      //need to pass in a mutable list of root objects we want the garbage collector to retain
      std::vector<ObjectInstance*> root;
      std::copy(live_modules.begin(), live_modules.end(), std::back_inserter(root));
      Runtime::freeUnreferencedObjects(std::move(root));
   }

   std::list<ObjectInstance*> live_modules;
};

static wavm_live_modules the_wavm_live_modules;

}

class wavm_instantiated_module : public wasm_instantiated_module_interface {
   public:
      wavm_instantiated_module(ModuleInstance* instance, std::unique_ptr<Module> module, std::vector<uint8_t> initial_mem, wavm_runtime& wr) :
         _initial_memory(initial_mem),
         _instance(instance),
         _module_ref(detail::the_wavm_live_modules.add_live_module(instance)),
         _wavm_runtime(wr)
      {
         //The memory instance is reused across all wavm_instantiated_modules, but for wasm instances
         // that didn't declare "memory", getDefaultMemory() won't see it. It would also be possible
         // to say something like if(module->memories.size()) here I believe
         cd.starting_memory_pages = -1;
         if(module->memories.size())
            cd.starting_memory_pages = module->memories.defs.at(0).type.size.min;
         cd.initdata = _initial_memory;
         cd.initdata_pre_memory_size = 0;
         cd.mi = _instance; //XXX
      }

      ~wavm_instantiated_module() {
         detail::the_wavm_live_modules.remove_live_module(_module_ref);
      }

      void apply(apply_context& context) override {
         the_running_instance_context.apply_ctx = &context;

         unsigned long long start = __builtin_readcyclecounter();
         auto count_it = fc::make_scoped_exit([&start](){
            the_counter.count += __builtin_readcyclecounter() - start;
         });

         _wavm_runtime.exec.execute(cd, _wavm_runtime.mem, context);
      }

      std::vector<uint8_t>     _initial_memory;
      //naked pointer because ModuleInstance is opaque
      //_instance is deleted via WAVM's object garbage collection when wavm_rutime is deleted
      ModuleInstance*          _instance;
      detail::live_module_ref  _module_ref;
      MemoryType               _initial_memory_config;
      wavm_runtime&            _wavm_runtime;

      rodeos::code_descriptor cd;
};

wavm_runtime::wavm_runtime() : exec(cc) {
   static detail::wavm_runtime_initializer the_wavm_runtime_initializer;
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

   try {
      eosio::chain::webassembly::common::root_resolver resolver;
      LinkResult link_result = linkModule(*module, resolver);
      ModuleInstance *instance = instantiateModule(*module, std::move(link_result.resolvedImports));
      EOS_ASSERT(instance != nullptr, wasm_exception, "Fail to Instantiate WAVM Module");

      return std::make_unique<wavm_instantiated_module>(instance, std::move(module), initial_memory, *this);
   }
   catch(const Runtime::Exception& e) {
      EOS_THROW(wasm_execution_error, "Failed to stand up WAVM instance: ${m}", ("m", describeExceptionCause(e.cause)));
   }
}

void wavm_runtime::immediately_exit_currently_running_module() {
   RODEOS_MEMORY_PTR_cb_ptr;
   siglongjmp(*cb_ptr->jmp, 1); ///XXX 1 means clean exit
   __builtin_unreachable();
}

}}}}
