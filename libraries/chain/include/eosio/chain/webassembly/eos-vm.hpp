#pragma once

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/apply_context.hpp>
#include <softfloat_types.h>

//eos-vm includes
/*
#include <eosio/vm/host_function.hpp>
namespace eosio { namespace vm {
   template <>
   struct registered_host_functions<eosio::chain::apply_context>;
} }
*/
#include <eosio/vm/backend.hpp>

// eosio specific specializations
namespace eosio { namespace vm {
   template <>
   struct wasm_type_converter<chain::name> {
      static chain::name from_wasm(uint64_t val) { return chain::name{(uint64_t)val}; }
      static uint64_t    to_wasm(chain::name val) { return val.to_uint64_t(); }
   };

   template <typename T>
   struct wasm_type_converter<eosio::chain::array_ptr<T>> {
      static eosio::chain::array_ptr<T> from_wasm(T* ptr) {
         return eosio::chain::array_ptr<T>(ptr); 
      }
      // overload for validating pointer is in valid memory
      static eosio::chain::array_ptr<T> from_wasm(T* ptr, uint32_t len) {
         volatile T _ignore = *(ptr + (len ? len-1 : 0)); //segfault if the pointer is not in a valid page of memory
         return eosio::chain::array_ptr<T>(ptr);
      }
      //static eosio::chain::array_ptr<T> from_wasm(uint32_t ptr1, uint32_t ptr2, uint32_t len) {}
      //static uint32_t to_wasm(eosio::chain::array_ptr<T> val) { return wasm_type_converter<T*>::to_wasm(val.value); }
   };

   template <typename Ctx>
   struct construct_derived<eosio::chain::transaction_context, Ctx> {
      static auto &value(Ctx& ctx) { return ctx.trx_context; }
   };
   
   template <>
   struct construct_derived<eosio::chain::apply_context, eosio::chain::apply_context> {
      static auto &value(eosio::chain::apply_context& ctx) { return ctx; }
   };

}} // ns eosio::vm

namespace eosio { namespace chain { namespace webassembly { namespace eos_vm_runtime {

using namespace fc;
using namespace eosio::vm;
using namespace eosio::chain::webassembly::common;

class eos_vm_runtime : public eosio::chain::wasm_runtime_interface {
   public:
      eos_vm_runtime();
      std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t>) override;

      void immediately_exit_currently_running_module() override {
         if (_bkend)
            _bkend->exit({});
      }

   private:
      // todo: managing this will get more complicated with sync calls;
      //       immediately_exit_currently_running_module() should probably
      //       move from wasm_runtime_interface to wasm_instantiated_module_interface.
      backend<apply_context>* _bkend = nullptr;  // non owning pointer to allow for immediate exit

   friend class eos_vm_instantiated_module;
};

} } } }// eosio::chain::webassembly::wabt_runtime

#define __EOS_VM_INTRINSIC_NAME(LBL, SUF) LBL##SUF
#define _EOS_VM_INTRINSIC_NAME(LBL, SUF) __INTRINSIC_NAME(LBL, SUF)

#define _REGISTER_EOS_VM_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG) \
   eosio::vm::registered_function<eosio::chain::apply_context, CLS, &CLS::METHOD> _EOS_VM_INTRINSIC_NAME(__eos_vm_intrinsic_fn, __COUNTER__)(std::string(MOD), std::string(NAME));
