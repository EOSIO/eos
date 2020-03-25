#pragma once

#if defined(EOSIO_EOS_VM_RUNTIME_ENABLED) || defined(EOSIO_EOS_VM_JIT_RUNTIME_ENABLED)

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/apply_context.hpp>
#include <softfloat_types.h>

//eos-vm includes
#include <eosio/vm/backend.hpp>

// eosio specific specializations
namespace eosio { namespace chain {
   template <typename T>
   using legacy_array_ptr = eosio::vm::reference_proxy<span<T>>;

   template <typename T>
   using legacy_ptr       = eosio::vm::reference_proxy<T>;
}} // ns eosio::chain


namespace eosio { namespace chain { namespace webassembly {
   struct eosio_type_converter : public eosio::vm::type_converter<> {
      using base_type = eosio::vm::type_converter<>;
      using base_type::running_context;

      using elem_type = decltype(std::declval<type_converter>().get_interface().operand_from_back(0));

      EOS_VM_FROM_WASM(T, T*, (elem_type&& ptr) { return reference_proxy<T>{as.value<T*>(std::move(ptr))}; }
      EOS_VM_FROM_WASM(T, T&, (elem_type&& ptr) { return reference_proxy<T>{as.value<T&>(std::move(ptr))}; }
   };
}}} // ns eosio::chain::webassembly

namespace eosio { namespace chain { namespace webassembly { namespace eos_vm_runtime {

using namespace fc;
using namespace eosio::vm;
using namespace eosio::chain::webassembly::common;

template<typename Backend>
class eos_vm_runtime : public eosio::chain::wasm_runtime_interface {
   public:
      eos_vm_runtime();
      bool inject_module(IR::Module&) override;
      std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t>,
                                                                             const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version) override;

      void immediately_exit_currently_running_module() override;

   private:
      // todo: managing this will get more complicated with sync calls;
      //       immediately_exit_currently_running_module() should probably
      //       move from wasm_runtime_interface to wasm_instantiated_module_interface.
      backend<apply_context, Backend>* _bkend = nullptr;  // non owning pointer to allow for immediate exit

   template<typename Impl>
   friend class eos_vm_instantiated_module;
};

} } } }// eosio::chain::webassembly::wabt_runtime
