#include <apifiny/chain/webassembly/apifiny-vm-oc.hpp>
#include <apifiny/chain/wasm_apifiny_constraints.hpp>
#include <apifiny/chain/wasm_apifiny_injection.hpp>
#include <apifiny/chain/apply_context.hpp>
#include <apifiny/chain/exceptions.hpp>

#include <vector>
#include <iterator>

namespace apifiny { namespace chain { namespace webassembly { namespace apifinyvmoc {

class apifinyvmoc_instantiated_module : public wasm_instantiated_module_interface {
   public:
      apifinyvmoc_instantiated_module(const digest_type& code_hash, const uint8_t& vm_version, apifinyvmoc_runtime& wr) :
         _code_hash(code_hash),
         _vm_version(vm_version),
         _apifinyvmoc_runtime(wr)
      {

      }

      ~apifinyvmoc_instantiated_module() {
         _apifinyvmoc_runtime.cc.free_code(_code_hash, _vm_version);
      }

      void apply(apply_context& context) override {
         const code_descriptor* const cd = _apifinyvmoc_runtime.cc.get_descriptor_for_code_sync(_code_hash, _vm_version);
         EOS_ASSERT(cd, wasm_execution_error, "EOS VM OC instantiation failed");

         _apifinyvmoc_runtime.exec.execute(*cd, _apifinyvmoc_runtime.mem, context);
      }

      const digest_type              _code_hash;
      const uint8_t                  _vm_version;
      apifinyvmoc_runtime&               _apifinyvmoc_runtime;
};

apifinyvmoc_runtime::apifinyvmoc_runtime(const boost::filesystem::path data_dir, const apifinyvmoc::config& apifinyvmoc_config, const chainbase::database& db)
   : cc(data_dir, apifinyvmoc_config, db), exec(cc) {
}

apifinyvmoc_runtime::~apifinyvmoc_runtime() {
}

std::unique_ptr<wasm_instantiated_module_interface> apifinyvmoc_runtime::instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t> initial_memory,
                                                                                     const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version) {

   return std::make_unique<apifinyvmoc_instantiated_module>(code_hash, vm_type, *this);
}

//never called. EOS VM OC overrides apifiny_exit to its own implementation
void apifinyvmoc_runtime::immediately_exit_currently_running_module() {}

}}}}
