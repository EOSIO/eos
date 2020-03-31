#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/vm/reference_proxy.hpp>
#include <eosio/vm/span.hpp>
#include <eosio/vm/types.hpp>

using namespace fc;

namespace eosio { namespace chain {

   class apply_context;
   class transaction_context;

   namespace detail {
      template <typename T>
      struct is_whitelisted_legacy_type {
         static constexpr bool value = std::is_same_v<fc::sha256, std::decay_t<T>> || std::is_same_v<float128_t, std::decay_t<T>> || std::is_arithmetic_v<std::decay_t<T>>;
      };
      template <typename T>
      struct is_whitelisted_type {
         static constexpr bool value = std::is_arithmetic_v<std::decay_t<T>>;
      };
   }

   template <typename T>
   inline static constexpr bool is_whitelisted_type_v = detail::is_whitelisted_type<T>::value;

   template <typename T>
   inline static constexpr bool is_whitelisted_legacy_type_v = detail::is_whitelisted_legacy_type<T>::value;

   template <typename... Ts>
   inline static constexpr bool are_whitelisted_types_v = (... && detail::is_whitelisted_type<Ts>::value);

   template <typename... Ts>
   inline static constexpr bool are_whitelisted_legacy_types_v = (... && detail::is_whitelisted_legacy_type<Ts>::value);

   template <typename T>
   using legacy_ptr = eosio::vm::reference_proxy<T, true>;

   template <typename T>
   using legacy_array_ptr = eosio::vm::reference_proxy<eosio::vm::span<T>>;

   struct null_terminated_ptr : eosio::vm::span<const char> {
      using base_t = eosio::vm::span<const char>;
      null_terminated_ptr(const char* ptr) : base_t(ptr, strlen(ptr)) {}
   };

   using wasm_size_t = eosio::vm::wasm_size_t;
}} // eosio::chain
