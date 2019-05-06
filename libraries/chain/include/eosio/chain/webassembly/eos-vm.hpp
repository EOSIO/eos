#pragma once

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/apply_context.hpp>
#include <softfloat_types.h>

//eos-vm includes
#include <eosio/wasm_backend/backend.hpp>

// eosio specific specializations
namespace eosio { namespace wasm_backend {
   template <>
   struct reduce_type<chain::name> {
      typedef uint64_t type;
   };

   template <typename S, typename Args, size_t I, typename T, typename WAlloc, typename Cleanups>
   constexpr auto get_value(operand_stack& op, Cleanups&, WAlloc*, T&& val) 
   	-> std::enable_if_t<std::is_same_v<i64_const_t, T> && std::is_same_v<chain::name, std::decay_t<S>>, S> {
      return {(uint64_t)val.data.ui};
   } 

   // we can clean these up if we go with custom vms
   template <typename T>
   struct reduce_type<eosio::chain::array_ptr<T>> {
      typedef uint32_t type;
   };
   
   template <typename S, typename Args, size_t I, typename T, typename WAlloc, typename Cleanups>
   constexpr auto get_value(operand_stack& op, Cleanups& cleanups, WAlloc* walloc, T&& val) 
   	-> std::enable_if_t<std::is_same_v<i32_const_t, T> && 
	   std::is_same_v< eosio::chain::array_ptr<typename S::type>, S> &&
      !std::is_lvalue_reference_v<S> && !std::is_pointer_v<S>, S> {
      //size_t i = std::tuple_size<Args>::value-1;
      using ptr_ty = typename S::type;
      using under_ty = std::remove_pointer_t<ptr_ty>;
      std::cout << (int*)walloc->template get_base_ptr<char>() << "val.data " << val.data.ui << "\n"; 
      auto* ptr = (ptr_ty*)((walloc->template get_base_ptr<char>())+val.data.ui);
      /*
      if constexpr (std::tuple_size<Args>::value > I) {
         const auto& next_arg = std::get<to_wasm_t<typename std::tuple_element<I, Args>::type>>(op.get_back(i-I)).data.ui;
         size_t sz = 0;
         if constexpr ( (std::tuple_size<Args>::value > I+1) 
			   && std::is_same_v<decltype(next_arg), eosio::chain::array_ptr<under_ty>>) {
            sz = std::get<to_wasm_t<typename std::tuple_element<I+1, Args>::type>>(op.get_back(i-(I+1))).data.ui;
            std::cout << "sz0 " << sz << "\n";
         } else {
            sz = next_arg;
            std::cout << "sz1 " << sz << " sizeof " << sizeof(under_ty) << " alignof " << alignof(under_ty) << "\n";
         }
         if ((uintptr_t)ptr % 16 != 0) {
            std::cout << "aligning\n";
            align_ptr_triple apt;
            apt.s = sizeof(under_ty)*sz;
            std::vector<std::remove_const_t<under_ty>> cpy(sz > 0 ? sz : 1);
            apt.o = (void*)ptr;
            ptr = (decltype(ptr))cpy.data();
            apt.n = (void*)ptr;
            //memcpy((std::remove_const_t<under_ty>*)apt.n, (const under_ty*)apt.o, apt.s);
            memmove( static_cast<char*>(apt.n), static_cast<const char*>(apt.o), apt.s);
            if constexpr (!std::is_const_v<under_ty>)
               cleanups.emplace_back(std::move(apt));
         }
      }
      */
      return eosio::chain::array_ptr<under_ty>(ptr);
   }

   template <>
   struct reduce_type<eosio::chain::null_terminated_ptr> {
      typedef uint32_t type;
   };
   
   template <typename S, typename Args, size_t I, typename T, typename WAlloc, typename Cleanups>
   constexpr auto get_value(operand_stack& op, Cleanups&, WAlloc* walloc, T&& val) 
   	-> std::enable_if_t<std::is_same_v<i32_const_t, T> && 
           std::is_same_v< eosio::chain::null_terminated_ptr, S> &&
           !std::is_lvalue_reference_v<S> && !std::is_pointer_v<S>, S> {
      return eosio::chain::null_terminated_ptr((char*)(walloc->template get_base_ptr<uint8_t>()+val.data.ui));
   }

}} // ns eosio::wasm_backend

namespace eosio { namespace chain { namespace webassembly { namespace eos_vm_runtime {

using namespace fc;
using namespace eosio::wasm_backend;
using namespace eosio::chain::webassembly::common;

class eos_vm_runtime : public eosio::chain::wasm_runtime_interface {
   public:
      eos_vm_runtime();
      std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t>) override;

      void immediately_exit_currently_running_module() override { _bkend->immediate_exit(); }

   private:
      backend<apply_context>* _bkend;  // non owning pointer to allow for immediate exit
};

} } } }// eosio::chain::webassembly::wabt_runtime

#define __EOS_VM_INTRINSIC_NAME(LBL, SUF) LBL##SUF
#define _EOS_VM_INTRINSIC_NAME(LBL, SUF) __INTRINSIC_NAME(LBL, SUF)

#define _REGISTER_EOS_VM_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG) \
   eosio::wasm_backend::registered_function<eosio::chain::apply_context, CLS, &CLS::METHOD> _EOS_VM_INTRINSIC_NAME(__eos_vm_intrinsic_fn, __COUNTER__)(std::string(MOD), std::string(NAME));
