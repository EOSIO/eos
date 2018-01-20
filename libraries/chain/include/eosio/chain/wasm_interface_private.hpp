#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <fc/optional.hpp>
#include "Runtime/Runtime.h"
#include "IR/Types.h"

#include <boost/multiprecision/cpp_dec_float.hpp>

using namespace IR;
using namespace Runtime;
using namespace fc;

namespace eosio { namespace chain {

using wasm_double = boost::multiprecision::cpp_dec_float_50;

struct wasm_cache::entry {
   entry(ModuleInstance *instance, Module *module)
      : instance(instance), module(module) 
   {
   }

   ModuleInstance* instance;
   Module* module;
};

struct wasm_context {
   wasm_context(wasm_cache::entry &code, apply_context& ctx) : code(code), context(ctx)
   {
      //initialize to minimum bytes and limit this to 32 bit space
      sbrk_bytes = (1 << IR::numBytesPerPageLog2) > UINT32_MAX ? UINT32_MAX : 1 << IR::numBytesPerPageLog2;
   }
   wasm_cache::entry& code;
   apply_context& context;
   uint32_t sbrk_bytes;
};

struct wasm_interface_impl {
   optional<wasm_context> current_context;

   void call(const string &entry_point, const vector <Value> &args, wasm_cache::entry &code, apply_context &context);
};

class intrinsics_accessor {
   public:
   static wasm_context &get_context(wasm_interface &wasm) {
      FC_ASSERT(wasm.my->current_context.valid());
      return *wasm.my->current_context;
   }
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
 * be a size and that the pair are validated together
 * @tparam T
 */
template<typename T>
struct array_ptr {
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
struct native_to_wasm<wasm_double> {
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
auto convert_native_to_wasm(const wasm_interface &wasm, T val) {
   return native_to_wasm_t<T>(val);
}

auto convert_native_to_wasm(const wasm_interface &wasm, const name &val) {
   return native_to_wasm_t<const name &>(val.value);
}

auto convert_native_to_wasm(const wasm_interface &wasm, const fc::time_point_sec &val) {
   return native_to_wasm_t<const fc::time_point_sec &>(val.sec_since_epoch());
}

auto convert_native_to_wasm(wasm_interface &wasm, char* ptr) {
   auto mem = getDefaultMemory(intrinsics_accessor::get_context(wasm).code.instance);
   if(!mem)
      Runtime::causeException(Exception::Cause::accessViolation);
   char* base = (char*)getMemoryBaseAddress(mem);
   char* top_of_memory = base + IR::numBytesPerPage*Runtime::getMemoryNumPages(mem);
   if(ptr < base || ptr >= top_of_memory)
      Runtime::causeException(Exception::Cause::accessViolation);
   return (int)(ptr - base);
}

template<typename T>
auto convert_wasm_to_native(native_to_wasm_t<T> val) {
   return T(val);
}

template<>
auto convert_wasm_to_native<wasm_double>(I64 val) {
   return wasm_double(*reinterpret_cast<wasm_double *>(&val));
}

template<typename T>
struct wasm_to_value_type;

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
template<typename Ret, typename NativeParameters, typename WasmParameters>
struct intrinsic_invoker_impl;

/**
 * Specialization for the fully transcribed signature
 * @tparam Ret - the return type of the native function
 * @tparam Translated - the arguments to the wasm function
 */
template<typename Ret, typename ...Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<>, std::tuple<Translated...>> {
   using next_method_type        = Ret (*)(wasm_interface &, Translated...);

   template<next_method_type Method>
   static native_to_wasm_t<Ret> invoke(Translated... translated) {
      wasm_interface &wasm = wasm_interface::get();
      return convert_native_to_wasm(wasm, Method(wasm, translated...));
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
template<typename ...Translated>
struct intrinsic_invoker_impl<void_type, std::tuple<>, std::tuple<Translated...>> {
   using next_method_type        = void_type (*)(wasm_interface &, Translated...);

   template<next_method_type Method>
   static void invoke(Translated... translated) {
      wasm_interface &wasm = wasm_interface::get();
      Method(wasm, translated...);
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
template<typename Ret, typename Input, typename... Inputs, typename... Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<Input, Inputs...>, std::tuple<Translated...>> {
   using translated_type = native_to_wasm_t<Input>;
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., translated_type>>;
   using then_type = Ret (*)(wasm_interface &, Input, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, Translated... translated, translated_type last) {
      auto native = convert_wasm_to_native<Input>(last);
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
   using then_type = Ret(*)(wasm_interface &, array_ptr<T>, size_t, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, Translated... translated, I32 ptr, I32 size) {
      auto mem = getDefaultMemory(intrinsics_accessor::get_context(wasm).code.instance);
      size_t length = size_t(size);
      if(!mem || ptr+sizeof(T)*length >= IR::numBytesPerPage*Runtime::getMemoryNumPages(mem))
         Runtime::causeException(Exception::Cause::accessViolation);
      T *base = (T*)(getMemoryBaseAddress(mem)+ptr);
      return Then(wasm, array_ptr<T>{base}, length, rest..., translated...);
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
template<typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<null_terminated_ptr, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32>>;
   using then_type = Ret(*)(wasm_interface &, null_terminated_ptr, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, Translated... translated, I32 ptr) {
      auto mem = getDefaultMemory(intrinsics_accessor::get_context(wasm).code.instance);
      if(!mem)
         Runtime::causeException(Exception::Cause::accessViolation);
      char *base          = (char*)(getMemoryBaseAddress(mem) + ptr);
      char *p             = base;
      char *top_of_memory = (char*)(getMemoryBaseAddress(mem) + IR::numBytesPerPage*Runtime::getMemoryNumPages(mem));
      while(p < top_of_memory)
         if(*p++ == '\0')
            return Then(wasm, null_terminated_ptr{base}, rest..., translated...);
      Runtime::causeException(Exception::Cause::accessViolation);
   };

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * Specialization for transcribing  a pair of array_ptr types in the native method signature that share size
 * This type transcribes into 3 wasm parameters: 2 pointers and length and checks the validity of those memory
 * ranges before dispatching to the native method
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<typename T, typename U, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<array_ptr<T>, array_ptr<U>, size_t, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32, I32, I32>>;
   using then_type = Ret(*)(wasm_interface &, array_ptr<T>, array_ptr<U>, size_t, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, Translated... translated, I32 ptr_t, I32 ptr_u, I32 size) {
      auto mem = getDefaultMemory(intrinsics_accessor::get_context(wasm).code.instance);
      size_t length = size_t(size);
      if(!mem || ptr_t+sizeof(T)*length >= IR::numBytesPerPage*Runtime::getMemoryNumPages(mem)
              || ptr_u+sizeof(U)*length >= IR::numBytesPerPage*Runtime::getMemoryNumPages(mem))
         Runtime::causeException(Exception::Cause::accessViolation);
      T *base_t = (T*)(getMemoryBaseAddress(mem)+ptr_t);
      U *base_u = (U*)(getMemoryBaseAddress(mem)+ptr_u);
      return Then(wasm, array_ptr<T>{base_t}, array_ptr<U>{base_u}, length, rest..., translated...);
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
template<typename Ret>
struct intrinsic_invoker_impl<Ret, std::tuple<array_ptr<char>, int, size_t>, std::tuple<>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<>, std::tuple<I32, I32, I32>>;
   using then_type = Ret(*)(wasm_interface &, array_ptr<char>, int, size_t);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, I32 ptr, I32 value, I32 size) {
      auto mem = getDefaultMemory(intrinsics_accessor::get_context(wasm).code.instance);
      size_t length = size_t(size);
      if(!mem || ptr+sizeof(char)*length >= IR::numBytesPerPage*Runtime::getMemoryNumPages(mem))
         Runtime::causeException(Exception::Cause::accessViolation);
      char *base = (char*)(getMemoryBaseAddress(mem)+ptr);
      return Then(wasm, array_ptr<char>{base}, value, length);
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
struct intrinsic_invoker_impl<Ret, std::tuple<T *, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32>>;
   using then_type = Ret (*)(wasm_interface &, T *, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, Translated... translated, I32 ptr) {
      auto mem = getDefaultMemory(intrinsics_accessor::get_context(wasm).code.instance);
      if(!mem || ptr+sizeof(T) >= IR::numBytesPerPage*Runtime::getMemoryNumPages(mem))
         Runtime::causeException(Exception::Cause::accessViolation);
      T *base = (T*)(getMemoryBaseAddress(mem)+ptr);
      return Then(wasm, base, rest..., translated...);
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
template<typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<const name&, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., native_to_wasm_t<const name&> >>;
   using then_type = Ret (*)(wasm_interface &, const name&, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, Translated... translated, native_to_wasm_t<const name&> wasm_value) {
      auto value = name(wasm_value);
      return Then(wasm, value, rest..., translated...);
   }

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * Specialization for transcribing a reference to a fc::time_point_sec which can be passed as a native value
 *    This type transcribes into a native type which is loaded by value into a
 *    variable on the stack and then passed by reference to the intrinsic.
 *
 * @tparam Ret - the return type of the native method
 * @tparam Inputs - the remaining native parameters to transcribe
 * @tparam Translated - the list of transcribed wasm parameters
 */
template<typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<const fc::time_point_sec&, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., native_to_wasm_t<const fc::time_point_sec&> >>;
   using then_type = Ret (*)(wasm_interface &, const fc::time_point_sec&, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, Translated... translated, native_to_wasm_t<const fc::time_point_sec&> wasm_value) {
      auto value = fc::time_point_sec(wasm_value);
      return Then(wasm, value, rest..., translated...);
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
template<typename T, typename Ret, typename... Inputs, typename ...Translated>
struct intrinsic_invoker_impl<Ret, std::tuple<T &, Inputs...>, std::tuple<Translated...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>, std::tuple<Translated..., I32>>;
   using then_type = Ret (*)(wasm_interface &, T &, Inputs..., Translated...);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, Translated... translated, I32 ptr) {
      // references cannot be created for null pointers
      FC_ASSERT(ptr != 0);
      auto mem = getDefaultMemory(intrinsics_accessor::get_context(wasm).code.instance);
      if(!mem || ptr+sizeof(T) >= IR::numBytesPerPage*Runtime::getMemoryNumPages(mem))
         Runtime::causeException(Exception::Cause::accessViolation);
      T &base = *(T*)(getMemoryBaseAddress(mem)+ptr);
      return Then(wasm, base, rest..., translated...);
   }

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * forward declaration of a wrapper class to call methods of the class
 */
template<typename WasmSig, typename Ret, typename MethodSig, typename Cls, typename... Params>
struct intrinsic_function_invoker {
   using impl = intrinsic_invoker_impl<Ret, std::tuple<Params...>, std::tuple<>>;

   template<MethodSig Method>
   static Ret wrapper(wasm_interface &wasm, Params... params) {
      return (class_from_wasm<Cls>::value(wasm).*Method)(params...);
   }

   template<MethodSig Method>
   static const WasmSig *fn() {
      auto fn = impl::template fn<wrapper<Method>>();
      static_assert(std::is_same<WasmSig *, decltype(fn)>::value,
                    "Intrinsic function signature does not match the ABI");
      return fn;
   }
};

template<typename WasmSig, typename MethodSig, typename Cls, typename... Params>
struct intrinsic_function_invoker<WasmSig, void, MethodSig, Cls, Params...> {
   using impl = intrinsic_invoker_impl<void_type, std::tuple<Params...>, std::tuple<>>;

   template<MethodSig Method>
   static void_type wrapper(wasm_interface &wasm, Params... params) {
      (class_from_wasm<Cls>::value(wasm).*Method)(params...);
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

template<typename, typename>
struct intrinsic_function_invoker_wrapper;

template<typename WasmSig, typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<WasmSig, Ret (Cls::*)(Params...)> {
   using type = intrinsic_function_invoker<WasmSig, Ret, Ret (Cls::*)(Params...), Cls, Params...>;
};

template<typename WasmSig, typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<WasmSig, Ret (Cls::*)(Params...) const> {
   using type = intrinsic_function_invoker<WasmSig, Ret, Ret (Cls::*)(Params...) const, Cls, Params...>;
};

template<typename WasmSig, typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<WasmSig, Ret (Cls::*)(Params...) volatile> {
   using type = intrinsic_function_invoker<WasmSig, Ret, Ret (Cls::*)(Params...) volatile, Cls, Params...>;
};

template<typename WasmSig, typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<WasmSig, Ret (Cls::*)(Params...) const volatile> {
   using type = intrinsic_function_invoker<WasmSig, Ret, Ret (Cls::*)(Params...) const volatile, Cls, Params...>;
};

#define _ADD_PAREN_1(...) ((__VA_ARGS__)) _ADD_PAREN_2
#define _ADD_PAREN_2(...) ((__VA_ARGS__)) _ADD_PAREN_1
#define _ADD_PAREN_1_END
#define _ADD_PAREN_2_END
#define _WRAPPED_SEQ(SEQ) BOOST_PP_CAT(_ADD_PAREN_1 SEQ, _END)

#define __INTRINSIC_NAME(LABEL, SUFFIX) LABEL##SUFFIX
#define _INTRINSIC_NAME(LABEL, SUFFIX) __INTRINSIC_NAME(LABEL,SUFFIX)

#define _REGISTER_INTRINSIC_EXPLICIT(CLS, METHOD, WASM_SIG, NAME, SIG)\
   static Intrinsics::Function _INTRINSIC_NAME(__intrinsic_fn, __COUNTER__) (\
      "env." NAME,\
      eosio::chain::wasm_function_type_provider<WASM_SIG>::type(),\
      (void *)eosio::chain::intrinsic_function_invoker_wrapper<WASM_SIG, SIG>::type::fn<&CLS::METHOD>()\
   );\


#define _REGISTER_INTRINSIC4(CLS, METHOD, WASM_SIG, NAME, SIG)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, METHOD, WASM_SIG, NAME, SIG )

#define _REGISTER_INTRINSIC3(CLS, METHOD, WASM_SIG, NAME)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, METHOD, WASM_SIG, NAME, decltype(&CLS::METHOD) )

#define _REGISTER_INTRINSIC2(CLS, METHOD, WASM_SIG)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, METHOD, WASM_SIG, BOOST_PP_STRINGIZE(METHOD), decltype(&CLS::METHOD) )

#define _REGISTER_INTRINSIC1(CLS, METHOD)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":" BOOST_PP_STRINGIZE(METHOD) " without a signature");

#define _REGISTER_INTRINSIC0(CLS, METHOD)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":<unknown> without a method name and signature");

#define _UNWRAP_SEQ(...) __VA_ARGS__

#define _EXPAND_ARGS(CLS, INFO)\
   ( CLS, _UNWRAP_SEQ INFO )

#define _REGISTER_INTRINSIC(R, CLS, INFO)\
   BOOST_PP_CAT(BOOST_PP_OVERLOAD(_REGISTER_INTRINSIC, _UNWRAP_SEQ INFO) _EXPAND_ARGS(CLS, INFO), BOOST_PP_EMPTY())

#define REGISTER_INTRINSICS(CLS, MEMBERS)\
   BOOST_PP_SEQ_FOR_EACH(_REGISTER_INTRINSIC, CLS, _WRAPPED_SEQ(MEMBERS))

} } // eosio::chain
