#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>

#define EOSIO_INJECTED_MODULE_NAME "eosio_injection"

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
   static constexpr bool is_whitelisted_type_v = detail::is_whitelisted_type<T>::value;

   template <typename T>
   static constexpr bool is_whitelisted_legacy_type_v = detail::is_whitelisted_legacy_type<T>::value;

 } } // eosio::chain
