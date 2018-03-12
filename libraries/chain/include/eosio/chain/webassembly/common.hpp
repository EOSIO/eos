#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>

using namespace fc;

namespace eosio { namespace chain { namespace webassembly { namespace common {

   struct wasm_context {
      wasm_context(wasm_cache::entry &code, apply_context& ctx, wasm_interface::vm_type vm)
      : code(code)
      , context(ctx)
      , vm(vm)
      {
      }
      eosio::chain::wasm_cache::entry& code;
      apply_context& context;
      wasm_interface::vm_type vm;
   };

   class intrinsics_accessor {
   public:
      static wasm_context &get_context(wasm_interface &wasm);
   };

   template<typename T>
   struct class_from_wasm {
      /**
       * by default this is just constructing an object
       * @param wasm - the wasm_interface to use
       * @return
       */
      static auto value(wasm_interface &wasm) {
         return T(wasm);
      }
   };

   template<>
   struct class_from_wasm<apply_context> {
      /**
       * Don't construct a new apply_context, just return a reference to the existing ont
       * @param wasm
       * @return
       */
      static auto &value(wasm_interface &wasm) {
         return intrinsics_accessor::get_context(wasm).context;
      }
   };

   /**
    * class to represent an in-wasm-memory array
    * it is a hint to the transcriber that the next parameter will
    * be a size (data bytes length) and that the pair are validated together
    * This triggers the template specialization of intrinsic_invoker_impl
    * @tparam T
    */
   template<typename T>
   struct array_ptr {
      explicit array_ptr (T * value) : value(value) {}

      typename std::add_lvalue_reference<T>::type operator*() const {
         return *value;
      }

      T *operator->() const noexcept {
         return value;
      }

      template<typename U>
      operator U *() const {
         return static_cast<U *>(value);
      }

      T *value;
   };

   /**
    * class to represent an in-wasm-memory char array that must be null terminated
    */
   struct null_terminated_ptr {
      explicit null_terminated_ptr(char* value) : value(value) {}

      typename std::add_lvalue_reference<char>::type operator*() const {
         return *value;
      }

      char *operator->() const noexcept {
         return value;
      }

      template<typename U>
      operator U *() const {
         return static_cast<U *>(value);
      }

      char *value;
   };

 } } } } // eosio::chain
