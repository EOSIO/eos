#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>

#define EOSIO_INJECTED_MODULE_NAME "eosio_injection"

using namespace fc;

namespace eosio { namespace chain {

   class apply_context;
   class transaction_context;

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

   /**
    * class to represent an in-wasm-memory array
    * it is a hint to the transcriber that the next parameter will
    * be a size (data bytes length) and that the pair are validated together
    * This triggers the template specialization of intrinsic_invoker_impl
    * @tparam T
    */
   template<typename T>
   struct array_ptr {
      using type = T;
      array_ptr() = default;
      explicit array_ptr (T * value) : value(value) {}

      typename std::add_lvalue_reference<T>::type operator*() const {
         return *value;
      }

      T *operator->() const noexcept {
         return value;
      }

      operator T *() const {
         return value;
      }

      template<typename DataStream>
      friend inline DataStream& operator<<( DataStream& ds, const array_ptr<T>& ) {
         uint64_t zero = 0;
         ds.write( (const char*)&zero, sizeof(zero) ); // Serializing pointers is not currently supported
         return ds;
      }

      T *value;
   };

   /**
    * class to represent an in-wasm-memory char array that must be null terminated
    */
   struct null_terminated_ptr {
      null_terminated_ptr() = default;
      explicit null_terminated_ptr(char* value) : value(value) {}

      typename std::add_lvalue_reference<char>::type operator*() const {
         return *value;
      }

      char *operator->() const noexcept {
         return value;
      }

      operator char *() const {
         return value;
      }

      template<typename DataStream>
      friend inline DataStream& operator<<( DataStream& ds, const null_terminated_ptr& ) {
         uint64_t zero = 0;
         ds.write( (const char*)&zero, sizeof(zero) ); // Serializing pointers is not currently supported
         return ds;
      }

      char *value;
   };

   template<typename... Params>
   digest_type calc_arguments_hash( Params&&... params ) {
      digest_type::encoder enc;
      (fc::raw::pack( enc, std::forward<Params>(params) ), ...);
      return enc.result();
   }

 } } // eosio::chain
