#include <eosio/chain/webassembly/eos-vm-oc.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>

#include <vector>
#include <iterator>

struct counter {
   unsigned long long count;
   ~counter() {
      printf("total wasm exectuion time %llu\n", count);
   }
};
static counter the_counter;

namespace eosio { namespace chain { namespace webassembly { namespace eosvmoc {

class eosvmoc_instantiated_module : public wasm_instantiated_module_interface {
   public:
      eosvmoc_instantiated_module(std::vector<uint8_t>&& initial_mem, std::vector<uint8_t>&& wasm, 
                               const digest_type& code_hash, const uint8_t& vm_version, eosvmoc_runtime& wr) :
         _code_hash(code_hash),
         _vm_version(vm_version),
         _eosvmoc_runtime(wr)
      {

      }

      ~eosvmoc_instantiated_module() {
         _eosvmoc_runtime.cc.free_code(_code_hash, _vm_version);
      }

      void apply(apply_context& context) override {
         const code_descriptor* const cd = _eosvmoc_runtime.cc.get_descriptor_for_code(_code_hash, _vm_version);

         unsigned long long start = __builtin_readcyclecounter();
         auto count_it = fc::make_scoped_exit([&start](){
            the_counter.count += __builtin_readcyclecounter() - start;
         });

         _eosvmoc_runtime.exec.execute(*cd, _eosvmoc_runtime.mem, context);
      }

      const digest_type              _code_hash;
      const uint8_t                  _vm_version;
      eosvmoc_runtime&            _eosvmoc_runtime;
};

eosvmoc_runtime::eosvmoc_runtime(const boost::filesystem::path data_dir, const eosvmoc::config& eosvmoc_config, const chainbase::database& db)
   : cc(data_dir, eosvmoc_config, db), exec(cc) {
}

eosvmoc_runtime::~eosvmoc_runtime() {
}

std::unique_ptr<wasm_instantiated_module_interface> eosvmoc_runtime::instantiate_module(std::vector<uint8_t>&& wasm, std::vector<uint8_t>&& initial_memory,
                                                                                     const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version) {

   return std::make_unique<eosvmoc_instantiated_module>(std::move(initial_memory), std::move(wasm), code_hash, vm_type, *this);
}

void eosvmoc_runtime::immediately_exit_currently_running_module() {}

}}}}
