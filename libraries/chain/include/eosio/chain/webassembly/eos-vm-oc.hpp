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

#include <boost/hana/string.hpp>

namespace eosio { namespace chain { namespace webassembly { namespace eosvmoc {

using namespace IR;
using namespace Runtime;
using namespace fc;

using namespace eosio::chain::eosvmoc;

class eosvmoc_instantiated_module;

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
                "mov %%gs:(%[End]), %[Ptr]\n"      // invalid pointer if out of range
                "1:\n"
                "add %%gs:%c[linearMemoryStart], %[Ptr]\n"
                : [Ptr] "+r" (ptr),
                  [End] "+r" (end)
                : [linearMemoryStart] "i" (cb_full_linear_memory_start_segment_offset),
                  [firstInvalidMemory] "i" (cb_first_invalid_memory_address_segment_offset)
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
struct wasm_to_value_type<U32> {
   static constexpr auto value = ValueType::i32;
};
template<>
struct wasm_to_value_type<U64> {
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
struct wasm_to_rvalue_type<U32> {
   static constexpr auto value = ResultType::i32;
};
template<>
struct wasm_to_rvalue_type<U64> {
   static constexpr auto value = ResultType::i64;
};
template<>
struct wasm_to_rvalue_type<void> {
   static constexpr auto value = ResultType::none;
};


template<typename T>
constexpr auto wasm_to_rvalue_type_v = wasm_to_rvalue_type<T>::value;


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

struct eos_vm_oc_execution_interface {};

struct eos_vm_oc_type_converter : public eosio::vm::type_converter<webassembly::interface, eos_vm_oc_execution_interface> {
   using base_type = eosio::vm::type_converter<webassembly::interface, eos_vm_oc_execution_interface>;
   using base_type::type_converter;
   using base_type::to_wasm;
   using base_type::as_result;
   using base_type::get_host;

   EOS_VM_FROM_WASM(bool, (uint32_t value)) { return value ? 1 : 0; }

   EOS_VM_FROM_WASM(memcpy_params, (vm::wasm_ptr_t dst, vm::wasm_ptr_t src, vm::wasm_size_t size)) {
      auto d = array_ptr_impl<char>(dst, size);
      auto s = array_ptr_impl<char>(src, size);
      array_ptr_impl<char>(dst, 1);
      return { d, s, size };
   }

   EOS_VM_FROM_WASM(memcmp_params, (vm::wasm_ptr_t lhs, vm::wasm_ptr_t rhs, vm::wasm_size_t size)) {
     auto l = array_ptr_impl<char>(lhs, size);
     auto r = array_ptr_impl<char>(rhs, size);
     return { l, r, size };
   }

   EOS_VM_FROM_WASM(memset_params, (vm::wasm_ptr_t dst, int32_t val, vm::wasm_size_t size)) {
     auto d = array_ptr_impl<char>(dst, size);
     array_ptr_impl<char>(dst, 1);
     return { d, val, size };
   }

   template <typename T>
   auto from_wasm(vm::wasm_ptr_t ptr) const
      -> std::enable_if_t< std::is_pointer_v<T>,
                           vm::argument_proxy<T> > {
      void* p = array_ptr_impl<std::remove_pointer_t<T>>(ptr, 1);
      return {p};
   }

   template <typename T>
   auto from_wasm(vm::wasm_ptr_t ptr, vm::wasm_size_t len, vm::tag<T> = {}) const
      -> std::enable_if_t<vm::is_span_type_v<T>, T> {
      void* p = array_ptr_impl<typename T::value_type>(ptr, len);
      return {static_cast<typename T::pointer>(p), len};
   }

   template <typename T>
   auto from_wasm(vm::wasm_ptr_t ptr, vm::wasm_size_t len, vm::tag<T> = {}) const
      -> std::enable_if_t< vm::is_argument_proxy_type_v<T> &&
                           vm::is_span_type_v<typename T::proxy_type>, T> {
      void* p = array_ptr_impl<typename T::pointee_type>(ptr, len);
      return {p, len};
   }

   template <typename T>
   auto from_wasm(vm::wasm_ptr_t ptr, vm::tag<T> = {}) const
      -> std::enable_if_t< vm::is_argument_proxy_type_v<T> &&
                           std::is_pointer_v<typename T::proxy_type>, T> {
      if constexpr(T::is_legacy()) {
         EOS_ASSERT(ptr != 0, wasm_execution_error, "references cannot be created for null pointers");
      }
      void* p = array_ptr_impl<typename T::pointee_type>(ptr, 1);
      return {p};
   }

   EOS_VM_FROM_WASM(null_terminated_ptr, (vm::wasm_ptr_t ptr)) {
      auto p = null_terminated_ptr_impl(ptr);
      return {static_cast<const char*>(p)};
   }
   EOS_VM_FROM_WASM(name, (uint64_t e)) { return name{e}; }
   uint64_t to_wasm(name&& n) { return n.to_uint64_t(); }
   vm::wasm_ptr_t to_wasm(void*&& ptr) {
      return convert_native_to_wasm(static_cast<char*>(ptr));
   }
   EOS_VM_FROM_WASM(float32_t, (float f)) { return ::to_softfloat32(f); }
   EOS_VM_FROM_WASM(float64_t, (double f)) { return ::to_softfloat64(f); }

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

template<typename Args, std::size_t... Is>
auto get_ct_args(std::index_sequence<Is...>);

inline uint32_t make_native_type(vm::i32_const_t x) { return x.data.ui; }
inline uint64_t make_native_type(vm::i64_const_t x) { return x.data.ui; }
inline float make_native_type(vm::f32_const_t x) { return x.data.f; }
inline double make_native_type(vm::f64_const_t x) { return x.data.f; }
inline void make_native_type(vm::maybe_void_t) {}

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

template<typename Args, std::size_t... Is>
auto get_ct_args(std::index_sequence<Is...>) {
   return std::tuple_cat(get_ct_args_i<eos_vm_oc_type_converter, std::tuple_element_t<Is, Args>>()...);
}

template <std::size_t Offset, typename Tuple, std::size_t... Is>
auto subtuple_impl(Tuple&& t, std::index_sequence<Is...>) {
   return std::make_tuple(std::get<Is + Offset>(t)...);
}

///
/// @returns the tuple with the first \p N elements of \p t
///
template <std::size_t N, typename Tuple>
auto tuple_fronts(Tuple&& t) {
   return subtuple_impl<0>(std::forward<Tuple>(t), std::make_index_sequence<N>());
}

///
/// @returns returns the part of the tuple that follows the first \p N elements
///
template <std::size_t N, typename Tuple>
auto remove_fronts(Tuple&& t) {
   constexpr std::size_t sz = std::tuple_size_v<std::decay_t<Tuple>>;
   return subtuple_impl<N>(std::forward<Tuple>(t), std::make_index_sequence<sz - N>());
}

///
/// @returns  the new tuple in which the element \p t1 is followed by the
/// elements of \p tuple.
///
template <typename T1, typename... Args>
auto push_front(T1&& t1, std::tuple<Args...>&& tuple) {
   return std::apply([&](Args&&... args) { return std::make_tuple(t1, std::forward<Args>(args)...); },
                     std::forward<std::tuple<Args...>>(tuple));
}

///
/// @returns the first element of an index sequence
///
template <std::size_t N, std::size_t... Is>
constexpr std::size_t head(std::index_sequence<N, Is...>) {
   return N;
}

///
/// @returns returns the part of the index sequence that follows the first item
///
template <std::size_t N, std::size_t... Is>
constexpr auto tail(std::index_sequence<N, Is...>) {
   return std::index_sequence<Is...>();
}

///
/// Partition the specified tuple into a tuple of sub-tuples in accordance to 
/// the sizes specified in \p sizes.
///
/// For example, given a tuple <tt>(1,2,3,4,5)</tt> and the sizes <tt>(1,2,2)</tt>, 
/// the returned tuple would be <tt>(1,(2,3),(4,5))</tt>.
///
template <typename Tuple, typename SizeSeq>
auto partition_tuple(Tuple&& tuple, SizeSeq sizes) {
   if constexpr (std::tuple_size_v<std::decay_t<Tuple>> == 0)
      return tuple;
   else {
      constexpr std::size_t first_size = head(sizes);
      return push_front(tuple_fronts<first_size>(tuple), partition_tuple(remove_fronts<first_size>(tuple), tail(sizes)));
   }
}

template <typename S, typename... Args>
constexpr auto create_value(eos_vm_oc_type_converter& tc, const std::tuple<Args...>& args) {
   if constexpr (vm::detail::has_from_wasm_v<S, eos_vm_oc_type_converter>) {
      return std::apply([&tc](Args... arg) { return tc.template from_wasm<S>(arg...); }, args);
   } else {
      return S(std::get<0>(args));
   }
}

template <typename T>
constexpr std::size_t value_operand_size() {
   if constexpr (!std::is_same_v<vm::no_match_t, decltype(std::declval<eos_vm_oc_type_converter>().template as_value<T>(
                                                     vm::native_value()))>)
      return 1;
   else
      return std::tuple_size_v<vm::detail::from_wasm_type_deducer_t<eos_vm_oc_type_converter, T>>;
}

template <typename... NativeType>
constexpr auto operand_sizes(eos_vm_oc_type_converter&, std::tuple<NativeType...>*) {
   return std::index_sequence<value_operand_size<NativeType>()...>();
}

template <typename NativeTypes, typename PartitionedArgs, std::size_t... Is>
auto to_native_values(eos_vm_oc_type_converter& tc, PartitionedArgs partitioned, std::index_sequence<Is...>) {
   return std::make_tuple(create_value<std::tuple_element_t<Is, NativeTypes>>(tc, std::get<Is>(partitioned))...);
}

template<auto F, typename Interface, typename Preconditions, bool is_injected, typename... A>
auto fn(A... a) {
   try {
      if constexpr(!is_injected) {
         constexpr int cb_current_call_depth_remaining_segment_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(current_call_depth_remaining);
         constexpr int depth_assertion_intrinsic_offset = OFFSET_OF_FIRST_INTRINSIC - (int) find_intrinsic_index("eosvmoc_internal.depth_assert") * 8;

         asm volatile("cmpl   $1,%%gs:%c[callDepthRemainOffset]\n"
                      "jne    1f\n"
                      "callq  *%%gs:%c[depthAssertionIntrinsicOffset]\n"
                      "1:\n"
                      :
                      : [callDepthRemainOffset] "i" (cb_current_call_depth_remaining_segment_offset),
                        [depthAssertionIntrinsicOffset] "i" (depth_assertion_intrinsic_offset)
                      : "cc");
      }
      
      constexpr int cb_ctx_ptr_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(ctx);
      apply_context* ctx;
      asm("mov %%gs:%c[applyContextOffset], %[cPtr]\n"
          : [cPtr] "=r" (ctx)
          : [applyContextOffset] "i" (cb_ctx_ptr_offset)
          );
      Interface host(*ctx);
      eos_vm_oc_type_converter tc{&host, eos_vm_oc_execution_interface{}};
      using native_args = vm::flatten_parameters_t<AUTO_PARAM_WORKAROUND(F)>;
      constexpr std::size_t num_native_args = std::tuple_size_v<native_args>;
      constexpr auto sizes = operand_sizes(tc, static_cast<native_args *>(nullptr));
      auto partitioned = partition_tuple(std::make_tuple(a...), sizes);
      auto native_values = to_native_values<native_args>(tc, partitioned, std::make_index_sequence<num_native_args>());
      return make_native_type(vm::invoke_with_host_impl<F, Preconditions>(tc, &host, std::move(native_values)));
   }
   catch(...) {
      *reinterpret_cast<std::exception_ptr*>(eos_vm_oc_get_exception_ptr()) = std::current_exception();
   }
   siglongjmp(*eos_vm_oc_get_jmp_buf(), EOSVMOC_EXIT_EXCEPTION);
   __builtin_unreachable();
}

template<auto F, typename Preconditions, typename Args, bool is_injected, std::size_t... Is>
constexpr auto create_function(std::index_sequence<Is...>) {
   return &fn<F, webassembly::interface, Preconditions, is_injected, std::tuple_element_t<Is, Args>...>;
}

template<auto F, typename Preconditions, bool is_injected>
constexpr auto create_function() {
   using native_args = vm::flatten_parameters_t<AUTO_PARAM_WORKAROUND(F)>;
   using wasm_args = decltype(get_ct_args<native_args>(std::make_index_sequence<std::tuple_size_v<native_args>>()));
   return create_function<F, Preconditions, wasm_args, is_injected>(std::make_index_sequence<std::tuple_size_v<wasm_args>>());
}

template<auto F, bool injected, typename Preconditions, typename Name>
void register_eosvm_oc(Name n) {
   // Has special handling
   if(n == BOOST_HANA_STRING("env.eosio_exit")) return;
   constexpr auto fn = create_function<F, Preconditions, injected>();
   constexpr auto index = find_intrinsic_index(n.c_str());
   intrinsic the_intrinsic(
      n.c_str(),
      wasm_function_type_provider<std::remove_pointer_t<decltype(fn)>>::type(),
      reinterpret_cast<void*>(fn),
      index
   );
}

} } } }// eosio::chain::webassembly::eosvmoc
