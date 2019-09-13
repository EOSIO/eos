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
      wavm_instantiated_module(std::vector<uint8_t>&& initial_mem, std::vector<uint8_t>&& wasm, 
                               const digest_type& code_hash, const uint8_t& vm_version, wavm_runtime& wr) :
         _initial_memory(std::move(initial_mem)),
         _wasm(std::move(wasm)),
         _code_hash(code_hash),
         _vm_version(vm_version),
         _wavm_runtime(wr)
      {

      }

      ~wavm_instantiated_module() {}

      void apply(apply_context& context) override {
         the_running_instance_context.apply_ctx = &context;

         const code_descriptor* const cd = _wavm_runtime.cc.get_descriptor_for_code(_code_hash, _vm_version, _wasm, _initial_memory);

         unsigned long long start = __builtin_readcyclecounter();
         auto count_it = fc::make_scoped_exit([&start](){
            the_counter.count += __builtin_readcyclecounter() - start;
         });

         _wavm_runtime.exec.execute(*cd, _wavm_runtime.mem, context);
      }

      const std::vector<uint8_t>     _initial_memory;
      const std::vector<uint8_t>     _wasm;
      const digest_type              _code_hash;
      const uint8_t                  _vm_version;
      wavm_runtime&            _wavm_runtime;
};

wavm_runtime::wavm_runtime(const boost::filesystem::path data_dir, const rodeos::config& rodeos_config) : cc(data_dir, rodeos_config), exec(cc) {
   static detail::wavm_runtime_initializer the_wavm_runtime_initializer;
}

wavm_runtime::~wavm_runtime() {
}

std::unique_ptr<wasm_instantiated_module_interface> wavm_runtime::instantiate_module(std::vector<uint8_t>&& wasm, std::vector<uint8_t>&& initial_memory,
                                                                                     const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version) {

   return std::make_unique<wavm_instantiated_module>(std::move(initial_memory), std::move(wasm), code_hash, vm_type, *this);
}

void wavm_runtime::immediately_exit_currently_running_module() {
   RODEOS_MEMORY_PTR_cb_ptr;
   siglongjmp(*cb_ptr->jmp, 1); ///XXX 1 means clean exit
   __builtin_unreachable();
}

}}}}
