#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/vm/host_function.hpp>
#include <eosio/vm/argument_proxy.hpp>
#include <eosio/vm/span.hpp>
#include <eosio/vm/types.hpp>

using namespace fc;

namespace eosio { namespace chain { namespace webassembly {
   // forward declaration
   class interface;
}}} // ns eosio::chain::webassembly

// TODO need to fix up wasm_tests to not use this macro
#define EOSIO_INJECTED_MODULE_NAME "eosio_injection"

namespace eosio { namespace chain {

   class apply_context;
   class transaction_context;

   inline static constexpr auto eosio_injected_module_name = EOSIO_INJECTED_MODULE_NAME;

   template <typename T, std::size_t Extent = eosio::vm::dynamic_extent>
   using span = eosio::vm::span<T, Extent>;

   template <typename T, std::size_t Align = alignof(T)>
   using legacy_ptr = eosio::vm::argument_proxy<T*, Align>;

   template <typename T, std::size_t Align = alignof(T)>
   using legacy_span = eosio::vm::argument_proxy<eosio::vm::span<T>, Align>;

   struct null_terminated_ptr : eosio::vm::span<const char> {
      using base_type = eosio::vm::span<const char>;
      null_terminated_ptr(const char* ptr) : base_type(ptr, strlen(ptr)) {}
   };

   // wrapper pointer type to keep the validations from being done
   template <typename T>
   struct unvalidated_ptr {
      static_assert(sizeof(T) == 1); // currently only going to support these for char and const char
      operator T*() { return ptr; }
      T* operator->() { return ptr; }
      T& operator*() { return *ptr; }
      T* ptr;
   };

   // define the type converter for eosio
   struct type_converter : public eosio::vm::type_converter<webassembly::interface, eosio::vm::execution_interface> {
      using base_type = eosio::vm::type_converter<webassembly::interface, eosio::vm::execution_interface>;
      using base_type::type_converter;
      using base_type::from_wasm;
      using base_type::to_wasm;
      using base_type::as_value;
      using base_type::as_result;
      using base_type::elem_type;
      using base_type::get_host;

      template <typename T>
      auto from_wasm(const void* ptr) const
         -> std::enable_if_t<std::is_same_v<T, unvalidated_ptr<const char>>, T> {
         return {static_cast<const char*>(ptr)};
      }

      template <typename T>
      auto from_wasm(void* ptr) const
         -> std::enable_if_t<std::is_same_v<T, unvalidated_ptr<char>>, T> {
         return {static_cast<char*>(ptr)};
      }

      template <typename T>
      auto from_wasm(void* ptr) const
         -> std::enable_if_t< std::is_pointer_v<T>,
                              vm::argument_proxy<T> > {
         validate_pointer<std::remove_pointer_t<T>>(ptr, 1);
         return {ptr};
      }

      EOS_VM_FROM_WASM(null_terminated_ptr, (const void* ptr)) {
         validate_null_terminated_pointer(ptr);
         return {static_cast<const char*>(ptr)};
      }
      EOS_VM_FROM_WASM(name, (uint64_t e)) { return name{e}; }
      uint64_t to_wasm(name&& n) { return n.to_uint64_t(); }
      EOS_VM_FROM_WASM(float32_t, (float f)) { return ::to_softfloat32(f); }
      EOS_VM_FROM_WASM(float64_t, (double f)) { return ::to_softfloat64(f); }
   };

   using eos_vm_host_functions_t = eosio::vm::registered_host_functions<webassembly::interface,
                                                                        eosio::vm::execution_interface,
                                                                        eosio::chain::type_converter>;
   using wasm_size_t = eosio::vm::wasm_size_t;

}} // eosio::chain
