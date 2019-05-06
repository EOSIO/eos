#include <eosio/chain/webassembly/eos-vm.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <fstream>
//eos-vm includes
#include <eosio/wasm_backend/backend.hpp>

namespace eosio { namespace chain { namespace webassembly { namespace eos_vm_runtime {

using namespace eosio::wasm_backend;

namespace wasm_constraints = eosio::chain::wasm_constraints;

using backend_t = backend<apply_context>;

class eos_vm_instantiated_module : public wasm_instantiated_module_interface {
   public:
      
      eos_vm_instantiated_module(std::unique_ptr<backend_t> mod) :
         _instantiated_module(std::move(mod)) {}

      void apply(apply_context& context) override {
         _instantiated_module->set_wasm_allocator( wasm_interface::get_wasm_allocator() );
         //if (!(const auto& res = _instantiated_module->run_start()))
         //   EOS_ASSERT(false, wasm_execution_error, "eos-vm start function failure (${s})", ("s", res.to_string()));
        
         std::cout << "CTX " << &context << "\n"; 
         const auto& res = _instantiated_module->call(&context, "env", "apply",
                                          (uint64_t)context.get_receiver(),
                                          (uint64_t)context.get_action().account,
                                          (uint64_t)context.get_action().name);
         //EOS_ASSERT(res, wasm_execution_error, "eos-vm execution failure (${s})", ("s", res.to_string()));
      }

   private:
      std::unique_ptr<backend_t> _instantiated_module;
};

eos_vm_runtime::eos_vm_runtime() {}

std::unique_ptr<wasm_instantiated_module_interface> eos_vm_runtime::instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t>) {
   std::vector<uint8_t> cb((uint8_t*)code_bytes, (uint8_t*)code_bytes+code_size);
   //std::vector<uint8_t> cb;
   //cb.resize(code_size);
   //memcpy(cb.data(), code_bytes, code_size);
   std::ofstream mf("temp.wasm");
   mf.write((char*)cb.data(), cb.size());
   mf.close();
   std::unique_ptr<backend_t> bkend = std::make_unique<backend_t>( cb );
   registered_host_functions<apply_context>::resolve(bkend->get_module());
   _bkend = bkend.get();
   return std::make_unique<eos_vm_instantiated_module>(std::move(bkend));
}

}}}}
