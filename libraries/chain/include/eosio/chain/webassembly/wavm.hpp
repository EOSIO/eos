#pragma once

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <softfloat.hpp>
#include "Runtime/Runtime.h"
#include "IR/Types.h"


namespace eosio { namespace chain { namespace webassembly { namespace wavm {

using namespace IR;
using namespace Runtime;
using namespace fc;
using namespace eosio::chain::webassembly::common;

class wavm_runtime : public eosio::chain::wasm_runtime_interface {
   public:
      wavm_runtime();
      ~wavm_runtime();
      bool inject_module(IR::Module&) override;
      std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t> initial_memory,
                                                                             const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version) override;

      void immediately_exit_currently_running_module() override;
};

//This is a temporary hack for the single threaded implementation
struct running_instance_context {
   MemoryInstance* memory;
   apply_context*  apply_ctx;
};
extern running_instance_context the_running_instance_context;

template<typename T>
struct wasm_to_value_type;

template<>
struct wasm_to_value_type<F32> {
   static constexpr auto value = ValueType::f32;
};

template<>
struct wasm_to_value_type<F64> {
   static constexpr auto value = ValueType::f64;
};
template<>
struct wasm_to_value_type<I32> {
   static constexpr auto value = ValueType::i32;
};
template<>
struct wasm_to_value_type<I64> {
   static constexpr auto value = ValueType::i64;
};

template<typename T>
constexpr auto wasm_to_value_type_v = wasm_to_value_type<T>::value;

template<typename T>
struct wasm_to_rvalue_type;
template<>
struct wasm_to_rvalue_type<F32> {
   static constexpr auto value = ResultType::f32;
};
template<>
struct wasm_to_rvalue_type<F64> {
   static constexpr auto value = ResultType::f64;
};
template<>
struct wasm_to_rvalue_type<I32> {
   static constexpr auto value = ResultType::i32;
};
template<>
struct wasm_to_rvalue_type<I64> {
   static constexpr auto value = ResultType::i64;
};
template<>
struct wasm_to_rvalue_type<void> {
   static constexpr auto value = ResultType::none;
};
template<>
struct wasm_to_rvalue_type<const name&> {
   static constexpr auto value = ResultType::i64;
};
template<>
struct wasm_to_rvalue_type<name> {
   static constexpr auto value = ResultType::i64;
};

template<>
struct wasm_to_rvalue_type<char*> {
   static constexpr auto value = ResultType::i32;
};

template<>
struct wasm_to_rvalue_type<fc::time_point_sec> {
   static constexpr auto value = ResultType::i32;
};


template<typename T>
constexpr auto wasm_to_rvalue_type_v = wasm_to_rvalue_type<T>::value;

template<typename T>
struct is_reference_from_value {
   static constexpr bool value = false;
};

template<>
struct is_reference_from_value<name> {
   static constexpr bool value = true;
};

template<>
struct is_reference_from_value<fc::time_point_sec> {
   static constexpr bool value = true;
};

template<typename T>
constexpr bool is_reference_from_value_v = is_reference_from_value<T>::value;



struct void_type {
};

/**
 * Forward declaration of provider for FunctionType given a desired C ABI signature
 */
template<typename>
struct wasm_function_type_provider;

/**
 * specialization to destructure return and arguments
 */
template<typename Ret, typename ...Args>
struct wasm_function_type_provider<Ret(Args...)> {
   static const FunctionType *type() {
      return FunctionType::get(wasm_to_rvalue_type_v<Ret>, {wasm_to_value_type_v<Args> ...});
   }
};

#define __INTRINSIC_NAME(LABEL, SUFFIX) LABEL##SUFFIX
#define _INTRINSIC_NAME(LABEL, SUFFIX) __INTRINSIC_NAME(LABEL,SUFFIX)

#define _REGISTER_WAVM_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG)\
   static Intrinsics::Function _INTRINSIC_NAME(__intrinsic_fn, __COUNTER__) (\
      MOD "." NAME,\
      eosio::chain::webassembly::wavm::wasm_function_type_provider<WASM_SIG>::type(),\
      nullptr\
   );

} } } }// eosio::chain::webassembly::wavm
