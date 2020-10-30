#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/webassembly/eos-vm.hpp>
#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/producer_schedule.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/core/ignore_unused.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wasm_interface_private.hpp>
#include <eosio/chain/wasm_eosio_validation.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/protocol_state_object.hpp>
#include <eosio/chain/account_object.hpp>
#include <fc/exception/exception.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/io/raw.hpp>

#include <softfloat.hpp>
#include <compiler_builtins.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <fstream>
#include <string.h>

#if defined(EOSIO_EOS_VM_RUNTIME_ENABLED) || defined(EOSIO_EOS_VM_JIT_RUNTIME_ENABLED)
#include <eosio/vm/allocator.hpp>
#endif

namespace eosio { namespace chain {

   wasm_interface::wasm_interface(vm_type vm, bool eosvmoc_tierup, const chainbase::database& d, const boost::filesystem::path data_dir, const eosvmoc::config& eosvmoc_config, bool profile)
     : my( new wasm_interface_impl(vm, eosvmoc_tierup, d, data_dir, eosvmoc_config, profile) ) {}

   wasm_interface::~wasm_interface() {}

   void wasm_interface::validate(const controller& control, const bytes& code) {
      const auto& pso = control.db().get<protocol_state_object>();

      if (control.is_builtin_activated(builtin_protocol_feature_t::configurable_wasm_limits)) {
         const auto& gpo = control.get_global_properties();
         webassembly::eos_vm_runtime::validate( code, gpo.wasm_configuration, pso.whitelisted_intrinsics );
         return;
      }
      Module module;
      try {
         Serialization::MemoryInputStream stream((U8*)code.data(), code.size());
         WASM::serialize(stream, module);
      } catch(const Serialization::FatalSerializationException& e) {
         EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
      } catch(const IR::ValidationException& e) {
         EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
      }

      wasm_validations::wasm_binary_validation validator(control, module);
      validator.validate();

      webassembly::eos_vm_runtime::validate( code, pso.whitelisted_intrinsics );

      //there are a couple opportunties for improvement here--
      //Easy: Cache the Module created here so it can be reused for instantiaion
      //Hard: Kick off instantiation in a separate thread at this location
	 }

   void wasm_interface::indicate_shutting_down() {
      my->is_shutting_down = true;
   }

   void wasm_interface::code_block_num_last_used(const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version, const uint32_t& block_num) {
      my->code_block_num_last_used(code_hash, vm_type, vm_version, block_num);
   }

   void wasm_interface::current_lib(const uint32_t lib) {
      my->current_lib(lib);
   }

   void wasm_interface::apply( const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version, apply_context& context ) {
#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
      if(my->eosvmoc) {
         const chain::eosvmoc::code_descriptor* cd = nullptr;
         try {
            cd = my->eosvmoc->cc.get_descriptor_for_code(code_hash, vm_version);
         }
         catch(...) {
            //swallow errors here, if EOS VM OC has gone in to the weeds we shouldn't bail: continue to try and run baseline
            //In the future, consider moving bits of EOS VM that can fire exceptions and such out of this call path
            static bool once_is_enough;
            if(!once_is_enough)
               elog("EOS VM OC has encountered an unexpected failure");
            once_is_enough = true;
         }
         if(cd) {
            uint64_t max_call_depth = eosio::chain::wasm_constraints::maximum_call_depth+1;
            uint64_t max_pages = eosio::chain::wasm_constraints::maximum_linear_memory/eosio::chain::wasm_constraints::wasm_page_size;
            if(context.control.is_builtin_activated(builtin_protocol_feature_t::configurable_wasm_limits)) {
               const wasm_config& config = context.control.get_global_properties().wasm_configuration;
               max_call_depth = config.max_call_depth;
               max_pages = config.max_pages;
            }
            webassembly::interface iface(context);
            webassembly::eosvmoc::timer timer{context};
            my->eosvmoc->exec.execute(*cd, my->eosvmoc->mem, &iface, max_call_depth, max_pages,
                                      &timer,
                                      context.get_receiver().to_uint64_t(), context.get_action().account.to_uint64_t(), context.get_action().name.to_uint64_t());
            return;
         }
      }
#endif
      my->get_instantiated_module(code_hash, vm_type, vm_version, context.trx_context)->apply(context);
   }

   void wasm_interface::exit() {
      my->runtime_interface->immediately_exit_currently_running_module();
   }

   wasm_instantiated_module_interface::~wasm_instantiated_module_interface() {}
   wasm_runtime_interface::~wasm_runtime_interface() {}

std::istream& operator>>(std::istream& in, wasm_interface::vm_type& runtime) {
   std::string s;
   in >> s;
   if (s == "eos-vm")
      runtime = eosio::chain::wasm_interface::vm_type::eos_vm;
   else if (s == "eos-vm-jit")
      runtime = eosio::chain::wasm_interface::vm_type::eos_vm_jit;
   else if (s == "eos-vm-oc")
      runtime = eosio::chain::wasm_interface::vm_type::eos_vm_oc;
   else
      in.setstate(std::ios_base::failbit);
   return in;
}

} } /// eosio::chain
