#include <eosio/chain/webassembly/eos-vm.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <fstream>
//eos-vm includes
#include <eosio/vm/backend.hpp>

namespace eosio { namespace chain { namespace webassembly { namespace eos_vm_runtime {

using namespace eosio::vm;

namespace wasm_constraints = eosio::chain::wasm_constraints;

using backend_t = backend<apply_context>;

class eos_vm_instantiated_module : public wasm_instantiated_module_interface {
   public:
      
      eos_vm_instantiated_module(eos_vm_runtime* runtime, std::unique_ptr<backend_t> mod) :
         _runtime(runtime),
         _instantiated_module(std::move(mod)) {}

      void apply(apply_context& context) override {
         auto* alloc = wasm_interface::get_wasm_allocator();
         _instantiated_module->set_wasm_allocator(alloc);
         _runtime->_bkend = _instantiated_module.get();
         _runtime->_bkend->initialize(&context);
         // clamp WASM memory to maximum_linear_memory/wasm_page_size
         auto& module = _runtime->_bkend->get_module();
         if (module.memories.size() && 
               ((module.memories.at(0).limits.maximum > wasm_constraints::maximum_linear_memory / wasm_constraints::wasm_page_size) 
               || !module.memories.at(0).limits.flags)) {
            module.memories.at(0).limits.flags = true;
            module.memories.at(0).limits.maximum = wasm_constraints::maximum_linear_memory / wasm_constraints::wasm_page_size;
         }
         const auto& res = _runtime->_bkend->call(
             &context, "env", "apply", context.get_receiver().to_uint64_t(),
             context.get_action().account.to_uint64_t(),
             context.get_action().name.to_uint64_t());
         _runtime->_bkend = nullptr;
      }

   private:
      eos_vm_runtime*            _runtime;
      std::unique_ptr<backend_t> _instantiated_module;
};

eos_vm_runtime::eos_vm_runtime() {}

std::unique_ptr<wasm_instantiated_module_interface> eos_vm_runtime::instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t>) {
   std::ofstream mf("temp.wasm");
   mf.write((char*)code_bytes, code_size);
   mf.close();
   wasm_code_ptr code((uint8_t*)code_bytes, 0);
   std::unique_ptr<backend_t> bkend = std::make_unique<backend_t>(code, code_size);
   registered_host_functions<apply_context>::resolve(bkend->get_module());
   return std::make_unique<eos_vm_instantiated_module>(this, std::move(bkend));
}

}}}}
