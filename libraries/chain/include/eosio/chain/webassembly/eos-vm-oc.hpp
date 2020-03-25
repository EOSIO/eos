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
using namespace eosio::chain::webassembly::common;

using namespace eosio::chain::eosvmoc;

class eosvmoc_instantiated_module;

class eosvmoc_runtime : public eosio::chain::wasm_runtime_interface {
   public:
      eosvmoc_runtime(const boost::filesystem::path data_dir, const eosvmoc::config& eosvmoc_config, const chainbase::database& db);
      ~eosvmoc_runtime();
      bool inject_module(IR::Module&) override { return false; }
      std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t> initial_memory,
                                                                             const digest_type& code_hash, const uint8_t& vm_type, const uint8_t& vm_version,
                                                                             const wasm_config& config) override;

      void immediately_exit_currently_running_module() override;

      friend eosvmoc_instantiated_module;
      eosvmoc::code_cache_sync cc;
      eosvmoc::executor exec;
      eosvmoc::memory mem;
};

/**
 * class to represent an in-wasm-memory array
 * it is a hint to the transcriber that the next parameter will
 * be a size (in Ts) and that the pair are validated together
 * This triggers the template specialization of intrinsic_invoker_impl
 * @tparam T
 */
template<typename T>
inline array_ptr<T> array_ptr_impl (size_t ptr, size_t length)
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


   return array_ptr<T>((T*)ptr);
}

/**
 * class to represent an in-wasm-memory char array that must be null terminated
 */
inline null_terminated_ptr null_terminated_ptr_impl(uint64_t ptr)
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

   return null_terminated_ptr((char*)ptr);
}

template<typename T>
struct void_ret_wrapper {
   using type = T;
};

template<>
struct void_ret_wrapper<void> {
   using type = char;
};

template<typename T>
using void_ret_wrapper_t = typename void_ret_wrapper<T>::type;

/**
 * template that maps native types to WASM VM types
 * @tparam T the native type
 */
template<typename T>
struct native_to_wasm {
   using type = void;
};

/**
 * specialization for mapping pointers to int32's
 */
template<typename T>
struct native_to_wasm<T *> {
   using type = I32;
};

/**
 * Mappings for native types
 */
template<>
struct native_to_wasm<float> {
   using type = F32;
};
template<>
struct native_to_wasm<double> {
   using type = F64;
};
template<>
struct native_to_wasm<int32_t> {
   using type = I32;
};
template<>
struct native_to_wasm<uint32_t> {
   using type = I32;
};
template<>
struct native_to_wasm<int64_t> {
   using type = I64;
};
template<>
struct native_to_wasm<uint64_t> {
   using type = I64;
};
template<>
struct native_to_wasm<bool> {
   using type = I32;
};
template<>
struct native_to_wasm<const name &> {
   using type = I64;
};
template<>
struct native_to_wasm<name> {
   using type = I64;
};
template<>
struct native_to_wasm<const fc::time_point_sec &> {
   using type = I32;
};

template<>
struct native_to_wasm<fc::time_point_sec> {
   using type = I32;
};

// convenience alias
template<typename T>
using native_to_wasm_t = typename native_to_wasm<T>::type;

template<typename T>
inline auto convert_native_to_wasm(T val) {
   return native_to_wasm_t<T>(val);
}

inline auto convert_native_to_wasm(const name &val) {
   return native_to_wasm_t<const name &>(val.to_uint64_t());
}

inline auto convert_native_to_wasm(const fc::time_point_sec& val) {
   return native_to_wasm_t<const fc::time_point_sec &>(val.sec_since_epoch());
}

inline auto convert_native_to_wasm(char* ptr) {
   constexpr int cb_full_linear_memory_start_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(full_linear_memory_start);
   char* full_linear_memory_start;
   asm("mov %%gs:%c[fullLinearMemOffset], %[fullLinearMem]\n"
      : [fullLinearMem] "=r" (full_linear_memory_start)
      : [fullLinearMemOffset] "i" (cb_full_linear_memory_start_offset)
      );
   U64 delta = (U64)(ptr - full_linear_memory_start);
   array_ptr_impl<char>(delta, 1);
   return (U32)delta;
}

template<typename T>
inline auto convert_wasm_to_native(native_to_wasm_t<T> val) {
   return T(val);
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

/**
 * Forward declaration of the invoker type which transcribes arguments to/from a native method
 * and injects the appropriate checks
 *
 * @tparam Ret - the return type of the native function
 * @tparam NativeParameters - a std::tuple of the remaining native parameters to transcribe
 * @tparam WasmParameters - a std::tuple of the transribed parameters
 */
template<bool is_injected, typename Ret, typename NativeParameters, typename WasmParameters>
struct intrinsic_invoker_impl;

/**
 * Specialization for the fully transcribed signature
 * @tparam Ret - the return type of the native function
 * @tparam Translated - the arguments to the wasm function
 */
template<bool is_injected, typename Ret, typename ...Translated>
struct intrinsic_invoker_impl<is_injected, Ret, std::tuple<>, std::tuple<Translated...>> {
   using next_method_type        = Ret (*)(Translated...);

   template<next_method_type Method>
   static native_to_wasm_t<Ret> invoke(Translated... translated) {
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
         return convert_native_to_wasm(Method(translated...));
      }
      catch(...) {
         *reinterpret_cast<std::exception_ptr*>(eos_vm_oc_get_exception_ptr()) = std::current_exception();
      }
      siglongjmp(*eos_vm_oc_get_jmp_buf(), EOSVMOC_EXIT_EXCEPTION);
      __builtin_unreachable();
   }

   template<next_method_type Method>
   static const auto fn() {
      return invoke<Method>;
   }
};

/**
 * specialization of the fully transcribed signature for void return values
 * @tparam Translated - the arguments to the wasm function
 */
template<bool is_injected, typename ...Translated>
struct intrinsic_invoker_impl<is_injected, void_type, std::tuple<>, std::tuple<Translated...>> {
   using next_method_type        = void_type (*)(Translated...);

   template<next_method_type Method>
   static void invoke(Translated... translated) {
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
         Method(translated...);
         return;
      }
      catch(...) {
         *reinterpret_cast<std::exception_ptr*>(eos_vm_oc_get_exception_ptr()) = std::current_exception();
      }
      siglongjmp(*eos_vm_oc_get_jmp_buf(), EOSVMOC_EXIT_EXCEPTION);
      __builtin_unreachable();
   }

   template<next_method_type Method>
   static const auto fn() {
      return invoke<Method>;
   }
};

/**
 * Sepcialization for transcribing  a simple type in the native method signature
 * @tparam Ret - the return type of the native method
 * @tparam Input - the type of the native parameter to transcribe
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<bool is_injected, typename Ret, typename Input, typename... Inputs, typename... Translated>
struct intrinsic_invoker_impl<is_injected, Ret, std::tuple<Input, Inputs...>, std::tuple<Translated...>> {
   using translated_type = native_to_wasm_t<Input>;
   using next_step = intrinsic_invoker_impl<is_injected, Ret, std::tuple<Inputs...>, std::tuple<Translated..., translated_type>>;
   using then_type = Ret (*)(Input, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(Inputs... rest, Translated... translated, translated_type last) {
      auto native = convert_wasm_to_native<Input>(last);
      return Then(native, rest..., translated...);
   };

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * Specialization for transcribing  a array_ptr type in the native method signature
 * This type transcribes into 2 wasm parameters: a pointer and byte length and checks the validity of that memory
 * range before dispatching to the native method
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<bool is_injected, typename T, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<is_injected, Ret, std::tuple<array_ptr<T>, uint32_t, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<is_injected, Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32, I32>>;
   using then_type = Ret(*)(array_ptr<T>, uint32_t, Inputs..., Translated...);

   template<then_type Then, typename U=T>
   static auto translate_one(Inputs... rest, Translated... translated, I32 ptr, I32 size) -> std::enable_if_t<std::is_const<U>::value, Ret> {
      static_assert(!std::is_pointer<U>::value, "Currently don't support array of pointers");
      const auto length = size_t((U32)size);
      T* base = array_ptr_impl<T>((U32)ptr, length);
      if ( reinterpret_cast<uintptr_t>(base) % alignof(T) != 0 ) {
         std::vector<std::byte>& copy = reinterpret_cast<std::list<std::vector<std::byte>>*>(eos_vm_oc_get_bounce_buffer_list())->emplace_back(length > 0 ? length*sizeof(T) : 1);
         T* copy_ptr = (T*)&copy[0];
         memcpy( (void*)copy.data(), (void*)base, length * sizeof(T) );
         return Then(static_cast<array_ptr<T>>(copy_ptr), length, rest..., translated...);
      }
      return Then(static_cast<array_ptr<T>>(base), length, rest..., translated...);
   };

   template<then_type Then, typename U=T>
   static auto translate_one(Inputs... rest, Translated... translated, I32 ptr, I32 size) -> std::enable_if_t<!std::is_const<U>::value, Ret> {
      static_assert(!std::is_pointer<U>::value, "Currently don't support array of pointers");
      const auto length = size_t((U32)size);
      T* base = array_ptr_impl<T>((U32)ptr, length);
      if ( reinterpret_cast<uintptr_t>(base) % alignof(T) != 0 ) {
         std::vector<std::byte>& copy = reinterpret_cast<std::list<std::vector<std::byte>>*>(eos_vm_oc_get_bounce_buffer_list())->emplace_back(length > 0 ? length*sizeof(T) : 1);
         T* copy_ptr = (T*)&copy[0];
         memcpy( (void*)copy.data(), (void*)base, length * sizeof(T) );
         Ret ret = Then(static_cast<array_ptr<T>>(copy_ptr), length, rest..., translated...);
         memcpy( (void*)base, (void*)copy.data(), length * sizeof(T) );
         return ret;
      }
      return Then(static_cast<array_ptr<T>>(base), length, rest..., translated...);
   };

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * Specialization for transcribing  a null_terminated_ptr type in the native method signature
 * This type transcribes 1 wasm parameters: a char pointer which is validated to contain
 * a null value before the end of the allocated memory.
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<bool is_injected, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<is_injected, Ret, std::tuple<null_terminated_ptr, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<is_injected, Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32>>;
   using then_type = Ret(*)(null_terminated_ptr, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(Inputs... rest, Translated... translated, I32 ptr) {
      return Then(null_terminated_ptr_impl((U32)ptr), rest..., translated...);
   };

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * Specialization for transcribing  a pair of array_ptr types in the native method signature that share size
 * This type transcribes into 3 wasm parameters: 2 pointers and byte length and checks the validity of those memory
 * ranges before dispatching to the native method
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<bool is_injected, typename T, typename U, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<is_injected, Ret, std::tuple<array_ptr<T>, array_ptr<U>, uint32_t, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<is_injected, Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32, I32, I32>>;
   using then_type = Ret(*)(array_ptr<T>, array_ptr<U>, uint32_t, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(Inputs... rest, Translated... translated, I32 ptr_t, I32 ptr_u, I32 size) {
      static_assert(std::is_same<std::remove_const_t<T>, char>::value && std::is_same<std::remove_const_t<U>, char>::value, "Currently only support array of (const)chars");
      const auto length = size_t((U32)size);
      return Then(array_ptr_impl<T>((U32)ptr_t, length), array_ptr_impl<U>((U32)ptr_u, length), length, rest..., translated...);
   };

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * Specialization for transcribing memset parameters
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<bool is_injected, typename Ret>
struct intrinsic_invoker_impl<is_injected, Ret, std::tuple<array_ptr<char>, int, uint32_t>, std::tuple<>> {
   using next_step = intrinsic_invoker_impl<is_injected, Ret, std::tuple<>, std::tuple<I32, I32, I32>>;
   using then_type = Ret(*)(array_ptr<char>, int, uint32_t);

   template<then_type Then>
   static Ret translate_one(I32 ptr, I32 value, I32 size) {
      const auto length = size_t((U32)size);
      return Then(array_ptr_impl<char>((U32)ptr, length), value, length);
   };

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * Specialization for transcribing  a pointer type in the native method signature
 * This type transcribes into an int32  pointer checks the validity of that memory
 * range before dispatching to the native method
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<bool is_injected, typename T, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<is_injected, Ret, std::tuple<T *, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<is_injected, Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32>>;
   using then_type = Ret (*)(T *, Inputs..., Translated...);

   template<then_type Then, typename U=T>
   static auto translate_one(Inputs... rest, Translated... translated, I32 ptr) -> std::enable_if_t<std::is_const<U>::value, Ret> {
      T* base = array_ptr_impl<T>((U32)ptr, 1);
      if ( reinterpret_cast<uintptr_t>(base) % alignof(T) != 0 ) {
         std::remove_const_t<T> copy;
         T* copy_ptr = &copy;
         memcpy( (void*)copy_ptr, (void*)base, sizeof(T) );
         return Then(copy_ptr, rest..., translated...);
      }
      return Then(base, rest..., translated...);
   };

   template<then_type Then, typename U=T>
   static auto translate_one(Inputs... rest, Translated... translated, I32 ptr) -> std::enable_if_t<!std::is_const<U>::value, Ret> {
      T* base = array_ptr_impl<T>((U32)ptr, 1);
      if ( reinterpret_cast<uintptr_t>(base) % alignof(T) != 0 ) {
         std::remove_const_t<T> copy;
         T* copy_ptr = &copy;
         memcpy( (void*)copy_ptr, (void*)base, sizeof(T) );
         Ret ret = Then(copy_ptr, rest..., translated...);
         memcpy( (void*)base, (void*)copy_ptr, sizeof(T) );
         return ret;
      }
      return Then(base, rest..., translated...);
   };

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * Specialization for transcribing a reference to a name which can be passed as a native value
 *    This type transcribes into a native type which is loaded by value into a
 *    variable on the stack and then passed by reference to the intrinsic.
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<bool is_injected, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<is_injected, Ret, std::tuple<const name&, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<is_injected, Ret, std::tuple<Inputs...>, std::tuple<Translated..., native_to_wasm_t<const name&> >>;
   using then_type = Ret (*)(const name&, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(Inputs... rest, Translated... translated, native_to_wasm_t<const name&> wasm_value) {
      auto value = name(wasm_value);
      return Then(value, rest..., translated...);
   }

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * Specialization for transcribing  a reference type in the native method signature
 *    This type transcribes into an int32  pointer checks the validity of that memory
 *    range before dispatching to the native method
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<bool is_injected, typename T, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<is_injected, Ret, std::tuple<T &, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<is_injected, Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32>>;
   using then_type = Ret (*)(T &, Inputs..., Translated...);

   template<then_type Then, typename U=T>
   static auto translate_one(Inputs... rest, Translated... translated, I32 ptr) -> std::enable_if_t<std::is_const<U>::value, Ret> {
      EOS_ASSERT((U32)ptr != 0, wasm_exception, "references cannot be created for null pointers");
      T &base = *array_ptr_impl<T>((uint32_t)ptr, 1);

      if ( reinterpret_cast<uintptr_t>(&base) % alignof(T) != 0 ) {
         std::remove_const_t<T> copy;
         T* copy_ptr = &copy;
         memcpy( (void*)copy_ptr, (void*)&base, sizeof(T) );
         return Then(*copy_ptr, rest..., translated...);
      }
      return Then(base, rest..., translated...);
   }

   template<then_type Then, typename U=T>
   static auto translate_one(Inputs... rest, Translated... translated, I32 ptr) -> std::enable_if_t<!std::is_const<U>::value, Ret> {
      EOS_ASSERT((U32)ptr != 0, wasm_exception, "reference cannot be created for null pointers");
      T &base = *array_ptr_impl<T>((uint32_t)ptr, 1);

      if ( reinterpret_cast<uintptr_t>(&base) % alignof(T) != 0 ) {
         std::remove_const_t<T> copy;
         T* copy_ptr = &copy;
         memcpy( (void*)copy_ptr, (void*)&base, sizeof(T) );
         Ret ret = Then(*copy_ptr, rest..., translated...);
         memcpy( (void*)&base, (void*)copy_ptr, sizeof(T) );
         return ret;
      }
      return Then(base, rest..., translated...);
   }

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * forward declaration of a wrapper class to call methods of the class
 */
template<bool is_injected, typename WasmSig, typename Ret, typename MethodSig, typename Cls, typename... Params>
struct intrinsic_function_invoker {
   using impl = intrinsic_invoker_impl<is_injected, Ret, std::tuple<Params...>, std::tuple<>>;

   template<MethodSig Method>
   static Ret wrapper(Params... params) {
      constexpr int cb_ctx_ptr_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(ctx);
      apply_context* ctx;
      asm("mov %%gs:%c[applyContextOffset], %[cPtr]\n"
         : [cPtr] "=r" (ctx)
         : [applyContextOffset] "i" (cb_ctx_ptr_offset)
         );
      return (class_from_wasm<Cls>::value(*ctx).*Method)(params...);
   }

   template<MethodSig Method>
   static const WasmSig *fn() {
      auto fn = impl::template fn<wrapper<Method>>();
      static_assert(std::is_same<WasmSig *, decltype(fn)>::value,
                    "Intrinsic function signature does not match the ABI");
      return fn;
   }
};

template<bool is_injected, typename WasmSig, typename MethodSig, typename Cls, typename... Params>
struct intrinsic_function_invoker<is_injected, WasmSig, void, MethodSig, Cls, Params...> {
   using impl = intrinsic_invoker_impl<is_injected, void_type, std::tuple<Params...>, std::tuple<>>;

   template<MethodSig Method>
   static void_type wrapper(Params... params) {
      constexpr int cb_ctx_ptr_offset = OFFSET_OF_CONTROL_BLOCK_MEMBER(ctx);
      apply_context* ctx;
      asm("mov %%gs:%c[applyContextOffset], %[cPtr]\n"
         : [cPtr] "=r" (ctx)
         : [applyContextOffset] "i" (cb_ctx_ptr_offset)
         );
      (class_from_wasm<Cls>::value(*ctx).*Method)(params...);
      return void_type();
   }

   template<MethodSig Method>
   static const WasmSig *fn() {
      auto fn = impl::template fn<wrapper<Method>>();
      static_assert(std::is_same<WasmSig *, decltype(fn)>::value,
                    "Intrinsic function signature does not match the ABI");
      return fn;
   }
};

template<bool, typename, typename>
struct intrinsic_function_invoker_wrapper;

template<bool is_injected, typename WasmSig, typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<is_injected, WasmSig, Ret (Cls::*)(Params...)> {
   static_assert( !(std::is_pointer_v<Ret> && alignof(std::remove_pointer_t<void_ret_wrapper_t<Ret>>) != 1) &&
                  !(std::is_lvalue_reference_v<Ret> && alignof(std::remove_reference_t<void_ret_wrapper_t<Ret>>) != 1),
                  "intrinsics should only return a reference or pointer with single byte alignment");
   using type = intrinsic_function_invoker<is_injected, WasmSig, Ret, Ret (Cls::*)(Params...), Cls, Params...>;
};

template<bool is_injected, typename WasmSig, typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<is_injected, WasmSig, Ret (Cls::*)(Params...) const> {
   static_assert( !(std::is_pointer_v<Ret> && alignof(std::remove_pointer_t<void_ret_wrapper_t<Ret>>) != 1) &&
                  !(std::is_lvalue_reference_v<Ret> && alignof(std::remove_reference_t<void_ret_wrapper_t<Ret>>) != 1),
                  "intrinsics should only return a reference or pointer with single byte alignment");
   using type = intrinsic_function_invoker<is_injected, WasmSig, Ret, Ret (Cls::*)(Params...) const, Cls, Params...>;
};

template<bool is_injected, typename WasmSig, typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<is_injected, WasmSig, Ret (Cls::*)(Params...) volatile> {
   static_assert( !(std::is_pointer_v<Ret> && alignof(std::remove_pointer_t<void_ret_wrapper_t<Ret>>) != 1) &&
                  !(std::is_lvalue_reference_v<Ret> && alignof(std::remove_reference_t<void_ret_wrapper_t<Ret>>) != 1),
                  "intrinsics should only return a reference or pointer with single byte alignment");
   using type = intrinsic_function_invoker<is_injected, WasmSig, Ret, Ret (Cls::*)(Params...) volatile, Cls, Params...>;
};

template<bool is_injected, typename WasmSig, typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<is_injected, WasmSig, Ret (Cls::*)(Params...) const volatile> {
   static_assert( !(std::is_pointer_v<Ret> && alignof(std::remove_pointer_t<void_ret_wrapper_t<Ret>>) != 1) &&
                  !(std::is_lvalue_reference_v<Ret> && alignof(std::remove_reference_t<void_ret_wrapper_t<Ret>>) != 1),
                  "intrinsics should only return a reference or pointer with single byte alignment");
   using type = intrinsic_function_invoker<is_injected, WasmSig, Ret, Ret (Cls::*)(Params...) const volatile, Cls, Params...>;
};

#define _ADD_PAREN_1(...) ((__VA_ARGS__)) _ADD_PAREN_2
#define _ADD_PAREN_2(...) ((__VA_ARGS__)) _ADD_PAREN_1
#define _ADD_PAREN_1_END
#define _ADD_PAREN_2_END
#define _WRAPPED_SEQ(SEQ) BOOST_PP_CAT(_ADD_PAREN_1 SEQ, _END)

#define __INTRINSIC_NAME(LABEL, SUFFIX) LABEL##SUFFIX
#define _INTRINSIC_NAME(LABEL, SUFFIX) __INTRINSIC_NAME(LABEL,SUFFIX)

#define _REGISTER_EOSVMOC_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG)\
   static eosio::chain::eosvmoc::intrinsic _INTRINSIC_NAME(__intrinsic_fn, __COUNTER__) EOSVMOC_INTRINSIC_INIT_PRIORITY (\
      MOD "." NAME,\
      eosio::chain::webassembly::eosvmoc::wasm_function_type_provider<WASM_SIG>::type(),\
      (void *)eosio::chain::webassembly::eosvmoc::intrinsic_function_invoker_wrapper<std::string_view(MOD) != "env", WASM_SIG, SIG>::type::fn<&CLS::METHOD>(),\
      ::boost::hana::index_if(eosio::chain::eosvmoc::intrinsic_table, ::boost::hana::equal.to(BOOST_HANA_STRING(MOD "." NAME))).value()\
   );\


} } } }// eosio::chain::webassembly::eosvmoc
