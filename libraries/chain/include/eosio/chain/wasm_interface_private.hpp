#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include "Runtime/Runtime.h"
#include "IR/Types.h"


namespace eosio { namespace chain {

using namespace IR;
using namespace Runtime;

struct module_state {
   ModuleInstance*      instance     = nullptr;
   Module*              module       = nullptr;
   apply_context*       context      = nullptr;
   int                  mem_start    = 0;
   int                  mem_end      = 1<<16;
   vector<char>         init_memory;
   fc::sha256           code_version;
};

struct wasm_interface_impl {
   map<digest_type, module_state> code_cache;
   module_state*                  current_state = nullptr;
};

wasm_interface::wasm_interface()
   :my( new wasm_interface_impl() ) {
}

class intrinsics_accessor {
   public:
      static module_state& get_module_state(wasm_interface& wasm) {
         FC_ASSERT(wasm.my->current_state != nullptr);
         return *wasm.my->current_state;
      }
};

/**
 * class to represent an in-wasm-memory array
 * it is a hint to the transcriber that the next parameter will
 * be a size and that the pair are validated together
 * @tparam T
 */
template<typename T>
struct array_ptr {
   typename std::add_lvalue_reference<T>::type operator*() const {
      return *value;
   }

   T* operator->() const noexcept {
      return value;
   }

   template<typename U>
   operator U*() const {
      return static_cast<U*>(value);
   }

   T* value;
};


/**
 * template that maps native types to WASM VM types
 * @tparam T the native type
 */
template<typename T>
struct native_to_wasm {
   using type = void;
};

/**
 * specialization for maping pointers to int32's
 */
template<typename T>
struct native_to_wasm<T*> {
   using type = I32;
};

/**
 * Mappings for native types
 */
template<> struct native_to_wasm<int32_t>  { using type = I32; };
template<> struct native_to_wasm<uint32_t> { using type = I32; };
template<> struct native_to_wasm<int64_t>  { using type = I64; };
template<> struct native_to_wasm<uint64_t> { using type = I64; };
template<> struct native_to_wasm<name>     { using type = I64; };
template<> struct native_to_wasm<bool>     { using type = I32; };

// convenience alias
template<typename T>
using native_to_wasm_t = typename native_to_wasm<T>::type;

template<typename T>
struct wasm_to_value_type;

template<> struct wasm_to_value_type<I32>  { static constexpr auto value = ValueType::i32; };
template<> struct wasm_to_value_type<I64>  { static constexpr auto value = ValueType::i64; };

template<typename T>
constexpr auto wasm_to_value_type_v = wasm_to_value_type<T>::value;

template<typename T>
struct wasm_to_rvalue_type;
template<> struct wasm_to_rvalue_type<I32>  { static constexpr auto value = ResultType::i32; };
template<> struct wasm_to_rvalue_type<I64>  { static constexpr auto value = ResultType::i64; };

template<typename T>
constexpr auto wasm_to_rvalue_type_v = wasm_to_rvalue_type<T>::value;

/**
 * Forward delclaration of the invoker type which transcribes arguments to/from a native method
 * and injects the appropriate checks
 *
 * @tparam Ret - the return type of the native function
 * @tparam NativeParameters - a std::tuple of the remaining native parameters to transcribe
 * @tparam WasmParameters - a std::tuple of the transribed parameters
 */
template<typename Ret, typename NativeParameters, typename WasmParameters>
struct intrinsic_invoker_impl;

/**
 * specialization fo the fully transcribed signature
 * @tparam Ret - the return type of the native function
 * @tparam Translated - the arguments to the wasm function
 */
template<typename Ret, typename ...Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<>, std::tuple<Translated...>> {
   static const FunctionType* wasm_function_type() {
      return FunctionType::get(wasm_to_rvalue_type_v<Ret>, { wasm_to_value_type_v<Translated> ...  });
   }

   using wasm_method_type = Ret (*)(wasm_interface&, Translated...);

   template<wasm_method_type Method>
   static Ret invoke(Translated... translated) {
      wasm_interface& wasm = wasm_interface::get();
      return native_to_wasm_t<Ret>(Method(wasm, translated...));
   }

   template<wasm_method_type Method>
   static const auto fn() {
      return invoke<Method>;
   }
};

/**
 * specialization of the fully transcribed signature for void return values
 * @tparam Translated - the arguments to the wasm function
 */
template<typename ...Translated>
struct intrinsic_invoker_impl<void, std::tuple<>, std::tuple<Translated...>> {
   static const FunctionType* wasm_function_type() {
      return FunctionType::get(ResultType::none, { wasm_to_value_type_v<Translated> ...  });
   }

   using wasm_method_type = void (*)(wasm_interface&, Translated...);

   template<wasm_method_type Method>
   static void invoke(Translated... translated) {
      wasm_interface& wasm = wasm_interface::get();
      Method(wasm, translated...);
   }

   template<wasm_method_type Method>
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
template<typename Ret, typename Input, typename... Inputs, typename... Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<Input, Inputs...>, std::tuple<Translated...>> {
   using translated_type = native_to_wasm_t<Input>;
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., translated_type>>;
   static const FunctionType* wasm_function_type() { return next_step::wasm_function_type(); }
   using then_type = Ret (*)(wasm_interface&, Input, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface& wasm, Inputs... rest, Translated... translated, translated_type last) {
      auto native = Input(last);
      return Then(wasm, native, rest..., translated...);
   };

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * Specialization for transcribing  a array_ptr type in the native method signature
 * This type transcribes into 2 wasm parameters: a pointer and length and checks the validity of that memory
 * range before dispatching to the native method
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<typename T, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<array_ptr<T>, size_t, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32, I32>>;
   static const FunctionType* wasm_function_type() { return next_step::wasm_function_type(); }
   using then_type = Ret(*)(wasm_interface&, array_ptr<T>, size_t, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface& wasm, Inputs... rest, Translated... translated, I32 ptr, I32 size) {
      auto mem = getDefaultMemory(intrinsics_accessor::get_module_state(wasm).instance);
      size_t length = size_t(size);
      T* base = memoryArrayPtr<T>( mem, ptr, length );
      return Then(wasm, array_ptr<T>{base}, length, rest..., translated...);
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
template<typename T, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<T*, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32>>;
   static const FunctionType* wasm_function_type() { return next_step::wasm_function_type(); }
   using then_type = Ret (*)(wasm_interface&, T*, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface& wasm, Inputs... rest, Translated... translated, I32 ptr) {
      auto mem = getDefaultMemory(intrinsics_accessor::get_module_state(wasm).instance);
      T* base = memoryArrayPtr<T>( mem, ptr, 1 );
      return Then(wasm, base, rest..., translated...);
   };

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>;
   }
};

/**
 * Specialization for transcribing  a reference type in the native method signature
 * This type transcribes into an int32  pointer checks the validity of that memory
 * range before dispatching to the native method
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<typename T, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<T&, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32>>;
   static const FunctionType* wasm_function_type() { return next_step::wasm_function_type(); }
   using then_type = Ret (*)(wasm_interface&, T&, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface& wasm, Inputs... rest, Translated... translated, I32 ptr) {
      // references cannot be created for null pointers
      FC_ASSERT(ptr != 0);
      auto mem = getDefaultMemory(intrinsics_accessor::get_module_state(wasm).instance);
      T& base = memoryRef<T>( mem, ptr );
      return Then(wasm, base, rest..., translated...);
   }

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>;
   }
};

/**
 * forward declaration of a wrapper class to call methods of the class
 */
template<typename T>
struct intrinsic_function_invoker;

template<typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker<Ret (Cls::*)(Params...)> {
   using impl = intrinsic_invoker_impl<Ret, std::tuple<Params...>, std::tuple<>>;
   static const FunctionType* wasm_function_type() { return impl::wasm_function_type(); }

   template< Ret (Cls::*Method)(Params...) >
   static Ret wrapper(wasm_interface& wasm, Params... params) {
      return (Cls(wasm).*Method)(params...);
   }

   template< Ret (Cls::*Method)(Params...) >
   static const auto fn() {
      return impl::template fn<wrapper<Method>>();
   }
};

#define _REGISTER_INTRINSIC(R, DATA, METHOD)\
   BOOST_PP_TUPLE_ELEM(2, 0, DATA).emplace( #METHOD, \
      std::make_unique<Intrinsics::Function>(\
         #METHOD,\
         eosio::chain::intrinsic_function_invoker<decltype(&BOOST_PP_TUPLE_ELEM(2, 1, DATA)::METHOD)>::wasm_function_type(),\
         (void *)eosio::chain::intrinsic_function_invoker<decltype(&BOOST_PP_TUPLE_ELEM(2, 1, DATA)::METHOD)>::fn<&BOOST_PP_TUPLE_ELEM(2, 1, DATA)::METHOD>()\
      )\
   );

#define REGISTER_INTRINSICS(DATA, MEMBERS)\
   BOOST_PP_SEQ_FOR_EACH(_REGISTER_INTRINSIC, DATA, MEMBERS)

} };