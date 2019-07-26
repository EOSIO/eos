#pragma once

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/apply_context.hpp>
#include <softfloat_types.h>

#include <eosio/vm/backend.hpp>

#include <thread>
#include <mutex>

// eosio specific specializations
namespace eosio { namespace vm {
   template <>
   struct reduce_type<chain::name> {
      typedef uint64_t type;
   };

   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc*, T&& val) 
         -> std::enable_if_t<std::is_same_v<i64_const_t, T> && std::is_same_v<chain::name, std::decay_t<S>>, S> {
      return std::move(chain::name{(uint64_t)val.data.ui});
   } 

   // we can clean these up if we go with custom vms
   template <typename T>
   struct reduce_type<eosio::chain::array_ptr<T>> {
      typedef uint32_t type;
   };
   
   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc* walloc, T&& val) 
         -> std::enable_if_t<std::is_same_v<i32_const_t, T> && 
         std::is_same_v< eosio::chain::array_ptr<typename S::type>, S> &&
         !std::is_lvalue_reference_v<S> && !std::is_pointer_v<S>, S> {
      using ptr_ty = typename S::type;
      return eosio::chain::array_ptr<ptr_ty>((ptr_ty*)((walloc->template get_base_ptr<char>())+val.data.ui));
   }

   template <typename Ctx>
   struct construct_derived<eosio::chain::transaction_context, Ctx> {
      static auto &value(Ctx& ctx) { return ctx.trx_context; }
   };
   
   template <>
   struct construct_derived<eosio::chain::apply_context, eosio::chain::apply_context> {
      static auto &value(eosio::chain::apply_context& ctx) { return ctx; }
   };

   template <>
   struct reduce_type<eosio::chain::null_terminated_ptr> {
      typedef uint32_t type;
   };
   
   template <typename S, typename Args, typename T, typename WAlloc>
   constexpr auto get_value(WAlloc* walloc, T&& val) 
         -> std::enable_if_t<std::is_same_v<i32_const_t, T> && 
         std::is_same_v< eosio::chain::null_terminated_ptr, S> &&
         !std::is_lvalue_reference_v<S> && !std::is_pointer_v<S>, S> {
      return eosio::chain::null_terminated_ptr((char*)(walloc->template get_base_ptr<uint8_t>()+val.data.ui));
   }
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
