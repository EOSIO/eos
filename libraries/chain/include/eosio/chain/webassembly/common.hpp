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

   template<typename T>
   struct class_from_wasm {
      /**
       * by default this is just constructing an object
       * @param wasm - the wasm_interface to use
       * @return
       */
      static auto value(apply_context& ctx) {
         return T(ctx);
      }
   };

   template<>
   struct class_from_wasm<transaction_context> {
      /**
       * by default this is just constructing an object
       * @param wasm - the wasm_interface to use
       * @return
       */
      template <typename ApplyCtx>
      static auto &value(ApplyCtx& ctx) {
         return ctx.trx_context;
      }
   };

   template<>
   struct class_from_wasm<apply_context> {
      /**
       * Don't construct a new apply_context, just return a reference to the existing ont
       * @param wasm
       * @return
       */
      static auto &value(apply_context& ctx) {
         return ctx;
      }
   };


   template <typename T>
   struct legacy_ptr {
      static_assert(is_whitelisted_legacy_type_v<T>);
      explicit legacy_ptr (T * value) : value(value) {}

      T& operator*() const {
         return *value;
      }

      T* operator->() const noexcept {
         return value;
      }

      operator T *() const {
         return value;
      }

      T* value;
   };

   template <typename T>
   struct legacy_ref {
      static_assert(is_whitelisted_legacy_type_v<T>);
      explicit legacy_ref (T* value) : value(&value) {}

      T& operator*() const {
         return *value;
      }

      T& operator->() const noexcept {
         return &value;
      }

      operator T *() const {
         return &value;
      }

      T* value;
   };
   /**
    * class to represent an in-wasm-memory array
    * it is a hint to the transcriber that the next parameter will
    * be a size (data bytes length) and that the pair are validated together
    * This triggers the template specialization of intrinsic_invoker_impl
    * @tparam T
    */
   template<typename T>
   struct legacy_array_ptr {
      explicit legacy_array_ptr (T * value) : value(value) {}

      typename std::add_lvalue_reference<T>::type operator*() const {
         return *value;
      }

      T *operator->() const noexcept {
         return value;
      }

      operator T *() const {
         return value;
      }

      T *value;
   };

   /**
    * class to represent an in-wasm-memory array
    * it is a hint to the transcriber that the next parameter will
    * be a size (data bytes length) and that the pair are validated together
    * This triggers the template specialization of intrinsic_invoker_impl
    * This is different than the legacy_array_ptr, in that it should assert if alignment isn't maintained for T
    * @tparam T
    */
   template <typename T>
   struct array_ptr : legacy_array_ptr<T> {
      explicit array_ptr (T* value) : legacy_array_ptr<T>(value) {}
   };

   /**
    * class to represent an in-wasm-memory char array that must be null terminated
    */
   struct null_terminated_ptr : legacy_array_ptr<char> {
      explicit null_terminated_ptr(char* value) : legacy_array_ptr<char>(value) {}
   };

 } } // eosio::chain
