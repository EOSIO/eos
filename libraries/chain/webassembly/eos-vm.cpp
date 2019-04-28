#include <eosio/chain/webassembly/wabt.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>

//eos-vm includes
#include <eosio/wasm_backend/backend.hpp>

namespace eosio { namespace chain { namespace webassembly { namespace eos_vm_runtime {

using namespace eosio::wasm_backend;

namespace wasm_constraints = eosio::chain::wasm_constraints;

using backend_t = backend<registered_host_functions<apply_context>>;

class eos_vm_instantiated_module : public wasm_instantiated_module_interface {
   public:
      
      eos_vm_instantiated_module(std::unique_ptr<backend_t> mod) :
         _instantiated_module(std::move(mod)) {}

      void apply(apply_context& context) override {
         _instantiated_module->set_wasm_allocator( context.get_wasm_allocator() );
         if (!(const auto& res = _instantiated_module->run_start()))
            EOS_ASSERT(false, wasm_execution_error, "eos-vm start function failure (${s})", ("s", res.to_string()));

         if (!(const auto& res = _instantiated_module(context.get_registered_host_functions(), "env", "apply",
                                             (uint64_t)context.get_receiver(),
                                             (uint64_t)context.get_action().account(),
                                             (uint64_t)context.get_action().name))
             EOS_ASSERT(false, wasm_execution_error, "eos-vm execution failure (${s})", ("s", res.to_string()));
      }

   private:
      std::unique_ptr<backend_t> _instantiated_module;
};

eos_vm_runtime::eos_vm_runtime() {}

std::unique_ptr<wasm_instantiated_module_interface> eos_vm_runtime::instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t>) {
   std::vector<uint8_t> cb((uint8_t*)code_bytes, (uint8_t*)code_bytes+code_size);
   std::unique_ptr<backend_t> bkend = std::make_unique<backend>( cb );
   _bkend = bkend.get();
   return std::make_unique<wabt_instantiated_module>(std::move(bkend));
}

void eos_vm_runtime::immediately_exit_currently_running_module() {
   _bkend->immediate_exit();
}

}}}}
