#pragma once

#if defined(EOSIO_EOS_VM_RUNTIME_ENABLED) || defined(EOSIO_EOS_VM_JIT_RUNTIME_ENABLED)

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/apply_context.hpp>
#include <softfloat_types.h>

//eos-vm includes
#include <eosio/vm/backend.hpp>

// eosio specific specializations
namespace eosio { namespace vm {

   namespace detail {
      // template helpers to determine if argument pair is an array_ptr and size pair
      template <typename...>
      struct array_ptr_type_checker;

      template <typename T>
      struct array_ptr_type_checker<eosio::chain::array_ptr<T>, uint32_t> {
         constexpr array_ptr_type_checker(eosio::chain::array_ptr<T>, uint32_t) {}
         static constexpr bool value = true;
      };

      template <typename T, typename U>
      struct array_ptr_type_checker<T, U> {
         constexpr array_ptr_type_checker(T&&, U&&) {}
         static constexpr bool value = false;
      };

      template <typename T>
      struct array_ptr_type_checker<T> {
         constexpr array_ptr_type_checker(T&&) {}
         static constexpr bool value = false;
      };

      template <typename T, typename U, typename... Args>
      struct array_ptr_type_checker<T, U, Args...> {
         constexpr array_ptr_type_checker(T&& t, U&& u, Args&&... args) {}
         static constexpr bool value = array_ptr_type_checker<T, U>::value &&
                                       decltype(detail::array_ptr_type_checker<Args...>(std::declval<Args>()...))::value;
      };

      struct array_ptr_filter {
         constexpr array_ptr_filter( void* ap, uint32_t len ) : value(static_cast<char*>(ap)), len(len) {}
         static array_ptr_filter create(void* ap, uint32_t l) { return {static_cast<char*>(ap), l}; }
         char*    value;
         uint32_t len;
      };

      constexpr bool is_aliasing(const array_ptr_filter& a, const array_ptr_filter& b) {
         if (a.value < b.value)
            return a.value + a.len >= b.value;
         return b.value + b.len >= a.value;
         //return is_aliasing(std::forward<T>(p1), sz1, std::forward<U>(p2), sz2) && is_aliasing(std::forward<Args>(args)...);
      }
   }

   template<>
   struct wasm_type_converter<eosio::chain::name> {
      static auto from_wasm(uint64_t val) {
         return eosio::chain::name{val};
      }
      static auto to_wasm(eosio::chain::name val) {
         return val.to_uint64_t();
      }
   };

   template<typename T>
   struct wasm_type_converter<eosio::chain::legacy_ptr<T>> : linear_memory_access {
      auto from_wasm(void* val) {
         validate_ptr<T>(val, 1);
         return eosio::vm::aligned_ptr_wrapper<T>{val};
      }
   };

   template<typename T>
   struct wasm_type_converter<eosio::chain::legacy_ref<T>> : linear_memory_access {
      auto from_wasm(void* val) {
         validate_ptr<T>(val, 1);
         return eosio::vm::aligned_ref_wrapper<T>{val};
      }
   };

   template<typename T>
   struct wasm_type_converter<T*> : linear_memory_access {
      auto from_wasm(void* val) {
         validate_ptr<T>(val, 1);
         return eosio::vm::copied_ptr_wrapper<T>{val};
      }
   };

   template<>
   struct wasm_type_converter<char*> : linear_memory_access {
      void* to_wasm(char* val) {
         validate_ptr<char>(val, 1);
         return val;
      }
   };

   template<typename T>
   struct wasm_type_converter<T&> : linear_memory_access {
      auto from_wasm(uint32_t val) {
         EOS_VM_ASSERT( val != 0, wasm_memory_exception, "references cannot be created for null pointers" );
         void* ptr = get_ptr(val);
         validate_ptr<T>(ptr, 1);
         return eosio::vm::copied_ref_wrapper<T>{ptr};
      }
   };

   template<typename T>
   struct wasm_type_converter<eosio::chain::legacy_array_ptr<T>> : linear_memory_access {
      auto from_wasm(void* ptr, uint32_t size) {
         validate_ptr<T>(ptr, size);
         return aligned_array_wrapper<T>(ptr, size);
      }
   };

   template<>
   struct wasm_type_converter<eosio::chain::legacy_array_ptr<char>> : linear_memory_access {
      auto from_wasm(void* ptr, uint32_t size) {
         validate_ptr<char>(ptr, size);
         return eosio::chain::array_ptr<char>((char*)ptr);
      }
      // memcpy/memmove
      auto from_wasm(void* ptr, eosio::chain::legacy_array_ptr<const char> /*src*/, uint32_t size) {
         validate_ptr<char>(ptr, size);
         return eosio::chain::array_ptr<char>((char*)ptr);
      }
      // memset
      auto from_wasm(void* ptr, int /*val*/, uint32_t size) {
         validate_ptr<char>(ptr, size);
         return eosio::chain::array_ptr<char>((char*)ptr);
      }
   };

   template<>
   struct wasm_type_converter<eosio::chain::legacy_array_ptr<const char>> : linear_memory_access {
      auto from_wasm(void* ptr, uint32_t size) {
         validate_ptr<char>(ptr, size);
         return eosio::chain::array_ptr<const char>((char*)ptr);
      }
      // memcmp
      auto from_wasm(void* ptr, eosio::chain::legacy_array_ptr<const char> /*src*/, uint32_t size) {
         validate_ptr<char>(ptr, size);
         return eosio::chain::array_ptr<const char>((char*)ptr);
      }
   };

   template<typename T>
   struct wasm_type_converter<eosio::chain::array_ptr<T>> : linear_memory_access {
      auto from_wasm(void* ptr, uint32_t size) {
         validate_ptr<T>(ptr, size);
         EOS_VM_ASSERT( reinterpret_cast<std::uintptr_t>(ptr) % alignof(T) != 0, wasm_memory_exception, "WASM arrays must be aligned to type T" );
         return filtered_wrapper<&detail::is_aliasing, detail::array_ptr_filter>(detail::array_ptr_filter(ptr, size));
      }
   };

   template<>
   struct wasm_type_converter<eosio::chain::array_ptr<char>> : linear_memory_access {
      auto from_wasm(void* ptr, uint32_t size) {
         validate_ptr<char>(ptr, size);
         return eosio::chain::array_ptr<char>((char*)ptr);
      }
      // memcpy/memmove
      auto from_wasm(void* ptr, eosio::chain::array_ptr<const char> /*src*/, uint32_t size) {
         validate_ptr<char>(ptr, size);
         return eosio::chain::array_ptr<char>((char*)ptr);
      }
      // memset
      auto from_wasm(void* ptr, int /*val*/, uint32_t size) {
         validate_ptr<char>(ptr, size);
         return eosio::chain::array_ptr<char>((char*)ptr);
      }
   };

   template<>
   struct wasm_type_converter<eosio::chain::array_ptr<const char>> : linear_memory_access {
      auto from_wasm(void* ptr, uint32_t size) {
         validate_ptr<char>(ptr, size);
         return eosio::chain::array_ptr<const char>((char*)ptr);
      }
      // memcmp
      auto from_wasm(void* ptr, eosio::chain::array_ptr<const char> /*src*/, uint32_t size) {
         validate_ptr<char>(ptr, size);
         return eosio::chain::array_ptr<const char>((char*)ptr);
      }
   };

   template <typename Ctx>
   struct construct_derived<eosio::chain::transaction_context, Ctx> {
      static auto &value(Ctx& ctx) { return ctx.trx_context; }
   };

   template <>
   struct construct_derived<eosio::chain::apply_context, eosio::chain::apply_context> {
      static auto &value(eosio::chain::apply_context& ctx) { return ctx; }
   };

   template<>
   struct wasm_type_converter<eosio::chain::null_terminated_ptr> : linear_memory_access {
      auto from_wasm(void* ptr) {
         validate_c_str(ptr);
         return eosio::chain::null_terminated_ptr{ static_cast<char*>(ptr) };
      }
   };

}} // ns eosio::vm

namespace eosio { namespace chain { namespace webassembly { namespace eos_vm_runtime {

using namespace fc;
using namespace eosio::vm;
using namespace eosio::chain::webassembly::common;

template<typename Backend>
class eos_vm_runtime : public eosio::chain::wasm_runtime_interface {
   public:
      eos_vm_runtime();
      bool inject_module(IR::Module&) override;
      std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t>,
                                                                             const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version) override;

      void immediately_exit_currently_running_module() override;

   private:
      // todo: managing this will get more complicated with sync calls;
      //       immediately_exit_currently_running_module() should probably
      //       move from wasm_runtime_interface to wasm_instantiated_module_interface.
      backend<apply_context, Backend>* _bkend = nullptr;  // non owning pointer to allow for immediate exit

   template<typename Impl>
   friend class eos_vm_instantiated_module;
};

} } } }// eosio::chain::webassembly::wabt_runtime

#define __EOS_VM_INTRINSIC_NAME(LBL, SUF) LBL##SUF
#define _EOS_VM_INTRINSIC_NAME(LBL, SUF) __INTRINSIC_NAME(LBL, SUF)

#define _REGISTER_EOS_VM_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG) \
   eosio::vm::registered_function<eosio::chain::apply_context, CLS, &CLS::METHOD> _EOS_VM_INTRINSIC_NAME(__eos_vm_intrinsic_fn, __COUNTER__)(std::string(MOD), std::string(NAME));

#else

#define _REGISTER_EOS_VM_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG)

#endif
