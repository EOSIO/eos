#pragma once

#include <fc/exception/exception.hpp>

#include <eosio/vm/backend.hpp>
#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
#   include <boost/hana/equal.hpp>
#   include <eosio/chain/webassembly/eos-vm-oc/intrinsic_interface.hpp>
#endif
#include <boost/hana/string.hpp>

namespace b1::rodeos {

using wasm_size_t = eosio::vm::wasm_size_t;

template <typename T, std::size_t Align = alignof(T)>
using legacy_ptr = eosio::vm::argument_proxy<T*, Align>;

template <typename T, std::size_t Align = alignof(T)>
using legacy_span = eosio::vm::argument_proxy<eosio::vm::span<T>, Align>;

struct null_terminated_ptr : eosio::vm::span<const char> {
   using base_type = eosio::vm::span<const char>;
   null_terminated_ptr(const char* ptr) : base_type(ptr, strlen(ptr)) {}
};

inline size_t legacy_copy_to_wasm(char* dest, size_t dest_size, const char* src, size_t src_size) {
   if (dest_size == 0)
      return src_size;
   auto copy_size = std::min(dest_size, src_size);
   memcpy(dest, src, copy_size);
   return copy_size;
}

struct memcpy_params {
   void*                  dst;
   const void*            src;
   eosio::vm::wasm_size_t size;
};

struct memcmp_params {
   const void*            lhs;
   const void*            rhs;
   eosio::vm::wasm_size_t size;
};

struct memset_params {
   const void*            dst;
   const int32_t          val;
   eosio::vm::wasm_size_t size;
};

template <typename Host, typename Execution_Interface = eosio::vm::execution_interface>
struct type_converter : eosio::vm::type_converter<Host, Execution_Interface> {
   using base_type = eosio::vm::type_converter<Host, Execution_Interface>;
   using base_type::base_type;
   using base_type::from_wasm;

   EOS_VM_FROM_WASM(bool, (uint32_t value)) { return value ? 1 : 0; }

   EOS_VM_FROM_WASM(memcpy_params,
                    (eosio::vm::wasm_ptr_t dst, eosio::vm::wasm_ptr_t src, eosio::vm::wasm_size_t size)) {
      auto d = this->template validate_pointer<char>(dst, size);
      auto s = this->template validate_pointer<char>(src, size);
      this->template validate_pointer<char>(dst, 1);
      return { d, s, size };
   }

   EOS_VM_FROM_WASM(memcmp_params,
                    (eosio::vm::wasm_ptr_t lhs, eosio::vm::wasm_ptr_t rhs, eosio::vm::wasm_size_t size)) {
      auto l = this->template validate_pointer<char>(lhs, size);
      auto r = this->template validate_pointer<char>(rhs, size);
      return { l, r, size };
   }

   EOS_VM_FROM_WASM(memset_params, (eosio::vm::wasm_ptr_t dst, int32_t val, eosio::vm::wasm_size_t size)) {
      auto d = this->template validate_pointer<char>(dst, size);
      this->template validate_pointer<char>(dst, 1);
      return { d, val, size };
   }

   template <typename T>
   auto from_wasm(uint32_t ptr) const -> std::enable_if_t<std::is_pointer_v<T>, eosio::vm::argument_proxy<T>> {
      auto p = this->template validate_pointer<std::remove_pointer_t<T>>(ptr, 1);
      return { p };
   }

   template <typename T>
   auto from_wasm(eosio::vm::wasm_ptr_t ptr, eosio::vm::tag<T> = {}) const
         -> std::enable_if_t<eosio::vm::is_argument_proxy_type_v<T> && std::is_pointer_v<typename T::proxy_type>, T> {
      auto p = this->template validate_pointer<typename T::pointee_type>(ptr, 1);
      return { p };
   }

   EOS_VM_FROM_WASM(null_terminated_ptr, (uint32_t ptr)) {
      void* p = this->validate_null_terminated_pointer(ptr);
      return { static_cast<const char*>(p) };
   }
}; // type_converter

template <typename Cls>
using registered_host_functions =
      eosio::vm::registered_host_functions<Cls, eosio::vm::execution_interface,
                                           type_converter<Cls, eosio::vm::execution_interface>>;

#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED

template <typename Cls>
struct eos_vm_oc_type_converter : public type_converter<Cls, eosio::chain::eosvmoc::eos_vm_oc_execution_interface> {
   using base_type = type_converter<Cls, eosio::chain::eosvmoc::eos_vm_oc_execution_interface>;
   using base_type::base_type;
   using base_type::from_wasm;
   using base_type::to_wasm;

   eosio::vm::wasm_ptr_t to_wasm(void*&& ptr) {
      return eosio::chain::eosvmoc::convert_native_to_wasm(static_cast<char*>(ptr));
   }

   template <typename T>
   inline decltype(auto) as_value(const eosio::vm::native_value& val) const {
      if constexpr (std::is_integral_v<T> && sizeof(T) == 4)
         return static_cast<T>(val.i32);
      else if constexpr (std::is_integral_v<T> && sizeof(T) == 8)
         return static_cast<T>(val.i64);
      else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4)
         return static_cast<T>(val.f32);
      else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8)
         return static_cast<T>(val.f64);
      // No direct pointer support
      else
         return eosio::vm::no_match_t{};
   }
};

inline eosio::chain::eosvmoc::intrinsic_map_t& get_intrinsic_map() {
   static eosio::chain::eosvmoc::intrinsic_map_t the_map;
   return the_map;
};

template <auto F, typename Cls, typename Preconditions, typename Name>
void register_eosvm_oc(Name n) {
   constexpr auto fn = eosio::chain::eosvmoc::create_function<F, Cls, eos_vm_oc_type_converter<Cls>, Preconditions>();
   get_intrinsic_map().insert(
         { n.c_str(),
           { reinterpret_cast<void*>(fn),
             ::boost::hana::index_if(eosio::chain::eosvmoc::intrinsic_table, ::boost::hana::equal.to(n)).value() } });
}
#endif

template <typename Rft, auto F, typename Name>
void register_host_function(Name fn_name) {
#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
   register_eosvm_oc<F, typename Rft::host_type_t, std::tuple<>>(BOOST_HANA_STRING("env.") + fn_name);
#endif
   Rft::template add<F>("env", fn_name.c_str());
}

#define RODEOS_REGISTER_CALLBACK(RFT, DERIVED, NAME)                                                                   \
   ::b1::rodeos::register_host_function<RFT, &DERIVED::NAME>(BOOST_HANA_STRING(#NAME));

} // namespace b1::rodeos
