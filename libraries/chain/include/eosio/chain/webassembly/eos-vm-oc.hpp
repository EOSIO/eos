#pragma once

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/apply_context.hpp>
#include <softfloat.hpp>
#include "IR/Types.h"

#include <eosio/chain/webassembly/eos-vm-oc/eos-vm-oc.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/memory.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/executor.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/code_cache.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/config.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/intrinsic.hpp>

#include <boost/hana/equal.hpp>

namespace eosio { namespace chain { namespace webassembly { namespace eosvmoc {

using namespace IR;
using namespace Runtime;
using namespace fc;

using namespace eosio::chain::eosvmoc;

class eosvmoc_instantiated_module;

struct timer : eosvmoc::timer_base {
   timer(apply_context& context) : context(context) {}
   void set_expiration_callback(void (*fn)(void*), void* data) override;
   void checktime() override;
   apply_context& context;
};

class eosvmoc_runtime : public eosio::chain::wasm_runtime_interface {
   public:
      eosvmoc_runtime(const boost::filesystem::path data_dir, const eosvmoc::config& eosvmoc_config, const chainbase::database& db);
      ~eosvmoc_runtime();
      bool inject_module(IR::Module&) override { return false; }
      std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t> initial_memory,
                                                                             const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version) override;

      void immediately_exit_currently_running_module() override;

      friend eosvmoc_instantiated_module;
      eosvmoc::code_cache_sync cc;
      eosvmoc::executor exec;
      eosvmoc::memory mem;
};

/**
 * validate an in-wasm-memory array
 * @tparam T
 */
template<typename T>
inline void* array_ptr_impl (size_t ptr, size_t length)
{
   constexpr int cb_full_linear_memory_start_segment_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(full_linear_memory_start);
   constexpr int cb_first_invalid_memory_address_segment_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(first_invalid_memory_address);

   size_t end = ptr + length*sizeof(T);

   asm volatile("cmp %%gs:%c[firstInvalidMemory], %[End]\n"
                "jle 1f\n"
                "mov %%gs:%c[maximumEosioWasmMemory], %[Ptr]\n"      //always invalid address
                "1:\n"
                "add %%gs:%c[linearMemoryStart], %[Ptr]\n"
                : [Ptr] "+r" (ptr),
                  [End] "+r" (end)
                : [linearMemoryStart] "i" (cb_full_linear_memory_start_segment_offset),
                  [firstInvalidMemory] "i" (cb_first_invalid_memory_address_segment_offset),
                  [maximumEosioWasmMemory] "i" (wasm_constraints::maximum_linear_memory)
                : "cc"
               );


   return (void*)ptr;
}

/**
 * validate an in-wasm-memory char array that must be null terminated
 */
inline char* null_terminated_ptr_impl(uint64_t ptr)
{
   constexpr int cb_full_linear_memory_start_segment_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(full_linear_memory_start);
   constexpr int cb_first_invalid_memory_address_segment_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(first_invalid_memory_address);

   char dumpster;
   uint64_t scratch;

   asm volatile("mov %%gs:(%[Ptr]), %[Dumpster]\n"                   //probe memory location at ptr to see if valid
                "mov %%gs:%c[firstInvalidMemory], %[Scratch]\n"      //get first invalid memory address
                "cmpb $0, %%gs:-1(%[Scratch])\n"                     //is last byte in valid linear memory 0?
                "je 2f\n"                                            //if so, this will be a null terminated string one way or another
                "mov %[Ptr],%[Scratch]\n"
                "1:\n"                                               //start loop looking for either 0, or until we SEGV
                "inc %[Scratch]\n"
                "cmpb   $0,%%gs:(%[Scratch])\n"
                "jne 1b\n"
                "2:\n"
                "add %%gs:%c[linearMemoryStart], %[Ptr]\n"           //add address of linear memory 0 to ptr
                : [Ptr] "+r" (ptr),
                  [Dumpster] "=r" (dumpster),
                  [Scratch] "=r" (scratch)
                : [linearMemoryStart] "i" (cb_full_linear_memory_start_segment_offset),
                  [firstInvalidMemory] "i" (cb_first_invalid_memory_address_segment_offset)
                : "cc"
               );

   return (char*)ptr;
}

inline auto convert_native_to_wasm(char* ptr) {
   constexpr int cb_full_linear_memory_start_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(full_linear_memory_start);
   char* full_linear_memory_start;
   asm("mov %%gs:%c[fullLinearMemOffset], %[fullLinearMem]\n"
      : [fullLinearMem] "=r" (full_linear_memory_start)
      : [fullLinearMemOffset] "i" (cb_full_linear_memory_start_offset)
      );
   U64 delta = (U64)(ptr - full_linear_memory_start);
   return (U32)delta;
}

struct eos_vm_oc_execution_interface {
   inline const auto& operand_from_back(std::size_t index) const { return *(os - index - 1); }

   template <typename T>
   inline void* validate_pointer(vm::wasm_ptr_t ptr, vm::wasm_size_t len) const {
      return array_ptr_impl<T>(ptr, len);
   }

   inline void* validate_null_terminated_pointer(vm::wasm_ptr_t ptr) const {
      return null_terminated_ptr_impl(ptr);
   }

   eosio::vm::native_value* os;
};

struct eos_vm_oc_type_converter : public basic_type_converter<eos_vm_oc_execution_interface> {
   using base_type = basic_type_converter<eos_vm_oc_execution_interface>;
   using base_type::basic_type_converter;
   using base_type::to_wasm;

   vm::wasm_ptr_t to_wasm(void*&& ptr) {
      return convert_native_to_wasm(static_cast<char*>(ptr));
   }

   template<typename T>
   inline decltype(auto) as_value(const vm::native_value& val) const {
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
         return vm::no_match_t{};
   }
};

template<typename TC, typename Args, std::size_t... Is>
auto get_ct_args(std::index_sequence<Is...>);

inline uint32_t make_native_type(vm::i32_const_t x) { return x.data.ui; }
inline uint64_t make_native_type(vm::i64_const_t x) { return x.data.ui; }
inline float make_native_type(vm::f32_const_t x) { return x.data.f; }
inline double make_native_type(vm::f64_const_t x) { return x.data.f; }

template<typename TC, typename Args, std::size_t... Is>
auto get_ct_args_one(std::index_sequence<Is...>) {
   return std::tuple<decltype(make_native_type(std::declval<TC>().as_result(std::declval<std::tuple_element_t<Is, Args>>())))...>();
}

template<typename TC, typename T>
auto get_ct_args_i() {
   if constexpr (vm::detail::has_from_wasm_v<T, TC>) {
      using args_tuple = vm::detail::from_wasm_type_deducer_t<TC, T>;
      return get_ct_args_one<TC, args_tuple>(std::make_index_sequence<std::tuple_size_v<args_tuple>>());
   } else {
      return std::tuple<decltype(make_native_type(std::declval<TC>().as_result(std::declval<T>())))>();
   }
}

template<typename TC, typename Args, std::size_t... Is>
auto get_ct_args(std::index_sequence<Is...>) {
   return std::tuple_cat(get_ct_args_i<TC, std::tuple_element_t<Is, Args>>()...);
}

template<typename TC>
struct result_resolver {
   template<typename T>
   auto operator,(T&& res) {
      return make_native_type(vm::detail::resolve_result(tc, static_cast<T&&>(res)));
   }
   TC& tc;
};
template<typename TC>
result_resolver(TC&) -> result_resolver<TC>;

template<auto F, typename Interface, typename TC, typename Preconditions, bool is_injected, typename... A>
auto fn(A... a) {
   try {
      if constexpr(!is_injected) {
         constexpr int cb_current_call_depth_remaining_segment_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(current_call_depth_remaining);
         constexpr int depth_assertion_intrinsic_offset = OFFSET_OF_FIRST_INTRINSIC - (int)boost::hana::index_if(intrinsic_table, ::boost::hana::equal.to(BOOST_HANA_STRING("eosvmoc_internal.depth_assert"))).value()*8;
         asm volatile("cmpl   $1,%%gs:%c[callDepthRemainOffset]\n"
                      "jne    1f\n"
                      "callq  *%%gs:%c[depthAssertionIntrinsicOffset]\n"
                      "1:\n"
                      :
                      : [callDepthRemainOffset] "i" (cb_current_call_depth_remaining_segment_offset),
                        [depthAssertionIntrinsicOffset] "i" (depth_assertion_intrinsic_offset)
                      : "cc");
      }
      using native_args = vm::flatten_parameters_t<AUTO_PARAM_WORKAROUND(F)>;
      eosio::vm::native_value stack[] = { a... };
      constexpr int cb_ctx_ptr_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(ctx);
      Interface* host;
      asm("mov %%gs:%c[applyContextOffset], %[cPtr]\n"
          : [cPtr] "=r" (host)
          : [applyContextOffset] "i" (cb_ctx_ptr_offset)
          );
      TC tc{host, eos_vm_oc_execution_interface{stack + sizeof...(A)}};
      return result_resolver{tc}, eosio::vm::invoke_with_host<F, Preconditions, native_args>(tc, host, std::make_index_sequence<sizeof...(A)>());
   }
   catch(...) {
      *reinterpret_cast<std::exception_ptr*>(eos_vm_oc_get_exception_ptr()) = std::current_exception();
   }
   siglongjmp(*eos_vm_oc_get_jmp_buf(), EOSVMOC_EXIT_EXCEPTION);
   __builtin_unreachable();
}

template<auto F, typename Cls, typename TC, typename Preconditions, typename Args, bool is_injected, std::size_t... Is>
constexpr auto create_function(std::index_sequence<Is...>) {
   return &fn<F, Cls, TC, Preconditions, is_injected, std::tuple_element_t<Is, Args>...>;
}

template<auto F, typename Cls, typename TC, typename Preconditions, bool is_injected>
constexpr auto create_function() {
   using native_args = vm::flatten_parameters_t<AUTO_PARAM_WORKAROUND(F)>;
   using wasm_args = decltype(get_ct_args<TC, native_args>(std::make_index_sequence<std::tuple_size_v<native_args>>()));
   return create_function<F, Cls, TC, Preconditions, wasm_args, is_injected>(std::make_index_sequence<std::tuple_size_v<wasm_args>>());
}

inline intrinsic_map_t& get_intrinsic_map() {
   static intrinsic_map_t instance;
   return instance;
}

template<auto F, bool injected, typename Preconditions, typename Name>
void register_eosvm_oc(Name n) {
   constexpr auto fn = create_function<F, webassembly::interface, eos_vm_oc_type_converter, Preconditions, injected>();
   auto& map = get_intrinsic_map();
   map.insert({n.c_str(),
      {
         reinterpret_cast<void*>(fn),
         ::boost::hana::index_if(intrinsic_table, ::boost::hana::equal.to(n)).value()
      }
   });
}

} } } }// eosio::chain::webassembly::eosvmoc
