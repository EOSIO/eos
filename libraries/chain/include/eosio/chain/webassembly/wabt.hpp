#pragma once

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/apply_context.hpp>
#include <softfloat_types.h>

//wabt includes
#include <src/error-handler.h>
#include <src/binary-reader.h>
#include <src/common.h>
#include <src/interp.h>

namespace eosio { namespace chain { namespace webassembly { namespace wabt_runtime {

using namespace fc;
using namespace wabt;
using namespace wabt::interp;
using namespace eosio::chain::webassembly::common;

struct wabt_apply_instance_vars {
   Memory* memory;
   apply_context& ctx;

   char* get_validated_pointer(uint32_t offset, uint32_t size) {
      EOS_ASSERT(memory, wasm_execution_error, "access violation");
      EOS_ASSERT(offset + size <= memory->data.size() && offset + size >= offset, wasm_execution_error, "access violation");
      return memory->data.data() + offset;
   }
};

struct intrinsic_registrator {
   using intrinsic_fn = TypedValue(*)(wabt_apply_instance_vars&, const TypedValues&);

   struct intrinsic_func_info {
      FuncSignature sig;
      intrinsic_fn func;
   }; 

   static auto& get_map(){
      static map<string, map<string, intrinsic_func_info>> _map;
      return _map;
   };

   intrinsic_registrator(const char* mod, const char* name, const FuncSignature& sig, intrinsic_fn fn) {
      get_map()[string(mod)][string(name)] = intrinsic_func_info{sig, fn};
   }
};

class wabt_runtime : public eosio::chain::wasm_runtime_interface {
   public:
      wabt_runtime();
      std::unique_ptr<wasm_instantiated_module_interface> instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t> initial_memory) override;

   private:
      wabt::ReadBinaryOptions read_binary_options;  //note default ctor will look at each option in feature.def and default to DISABLED for the feature
};

/**
 * class to represent an in-wasm-memory array
 * it is a hint to the transcriber that the next parameter will
 * be a size (data bytes length) and that the pair are validated together
 * This triggers the template specialization of intrinsic_invoker_impl
 * @tparam T
 */
template<typename T>
inline array_ptr<T> array_ptr_impl (wabt_apply_instance_vars& vars, uint32_t ptr, uint32_t length)
{
   EOS_ASSERT( length < INT_MAX/(uint32_t)sizeof(T), binaryen_exception, "length will overflow" );
   return array_ptr<T>((T*)(vars.get_validated_pointer(ptr, length * (uint32_t)sizeof(T))));
}

/**
 * class to represent an in-wasm-memory char array that must be null terminated
 */
inline null_terminated_ptr null_terminated_ptr_impl(wabt_apply_instance_vars& vars, uint32_t ptr)
{
   char *value = vars.get_validated_pointer(ptr, 1);
   const char* p = value;
   const char* const top_of_memory = vars.memory->data.data() + vars.memory->data.size();
   while(p < top_of_memory)
      if(*p++ == '\0')
         return null_terminated_ptr(value);

   FC_THROW_EXCEPTION(wasm_execution_error, "unterminated string");
}


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

template<typename T>
T convert_literal_to_native(const TypedValue& v);

template<>
inline double convert_literal_to_native<double>(const TypedValue& v) {
   return v.get_f64();
}

template<>
inline float convert_literal_to_native<float>(const TypedValue& v) {
   return v.get_f32();
}

template<>
inline int64_t convert_literal_to_native<int64_t>(const TypedValue& v) {
   return v.get_i64();
}

template<>
inline uint64_t convert_literal_to_native<uint64_t>(const TypedValue& v) {
   return v.get_i64();
}

template<>
inline int32_t convert_literal_to_native<int32_t>(const TypedValue& v) {
   return v.get_i32();
}

template<>
inline uint32_t convert_literal_to_native<uint32_t>(const TypedValue& v) {
   return v.get_i32();
}

template<>
inline bool convert_literal_to_native<bool>(const TypedValue& v) {
   return v.get_i32();
}

template<>
inline name convert_literal_to_native<name>(const TypedValue& v) {
   int64_t val = v.get_i64();
   return name(val);
}

inline auto convert_native_to_literal(const wabt_apply_instance_vars&, const uint32_t &val) {
   TypedValue tv(Type::I32);
   tv.set_i32(val);
   return tv;
}

inline auto convert_native_to_literal(const wabt_apply_instance_vars&, const int32_t &val) {
   TypedValue tv(Type::I32);
   tv.set_i32(val);
   return tv;
}

inline auto convert_native_to_literal(const wabt_apply_instance_vars&, const uint64_t &val) {
   TypedValue tv(Type::I64);
   tv.set_i64(val);
   return tv;
}

inline auto convert_native_to_literal(const wabt_apply_instance_vars&, const int64_t &val) {
   TypedValue tv(Type::I64);
   tv.set_i64(val);
   return tv;
}

inline auto convert_native_to_literal(const wabt_apply_instance_vars&, const float &val) {
   TypedValue tv(Type::F32);
   tv.set_f32(val);
   return tv;
}

inline auto convert_native_to_literal(const wabt_apply_instance_vars&, const double &val) {
   TypedValue tv(Type::F64);
   tv.set_f64(val);
   return tv;
}

inline auto convert_native_to_literal(const wabt_apply_instance_vars&, const name &val) {
   TypedValue tv(Type::I64);
   tv.set_i64(val.value);
   return tv;
}

inline auto convert_native_to_literal(const wabt_apply_instance_vars& vars, char* ptr) {
   const char* base = vars.memory->data.data();
   const char* top_of_memory = base + vars.memory->data.size();
   EOS_ASSERT(ptr >= base && ptr < top_of_memory, wasm_execution_error, "returning pointer not in linear memory");
   Value v;
   v.i32 = (int)(ptr - base);
   return TypedValue(Type::I32, v);
}

struct void_type {
};

template<typename T>
struct wabt_to_value_type;

template<>
struct wabt_to_value_type<F32> {
   static constexpr auto value = Type::F32;
};

template<>
struct wabt_to_value_type<F64> {
   static constexpr auto value = Type::F64;
};
template<>
struct wabt_to_value_type<I32> {
   static constexpr auto value = Type::I32;
};
template<>
struct wabt_to_value_type<I64> {
   static constexpr auto value = Type::I64;
};

template<typename T>
constexpr auto wabt_to_value_type_v = wabt_to_value_type<T>::value;

template<typename T>
struct wabt_to_rvalue_type;
template<>
struct wabt_to_rvalue_type<F32> {
   static constexpr auto value = Type::F32;
};
template<>
struct wabt_to_rvalue_type<F64> {
   static constexpr auto value = Type::F64;
};
template<>
struct wabt_to_rvalue_type<I32> {
   static constexpr auto value = Type::I32;
};
template<>
struct wabt_to_rvalue_type<I64> {
   static constexpr auto value = Type::I64;
};
template<>
struct wabt_to_rvalue_type<const name&> {
   static constexpr auto value = Type::I64;
};
template<>
struct wabt_to_rvalue_type<name> {
   static constexpr auto value = Type::I64;
};

template<>
struct wabt_to_rvalue_type<char*> {
   static constexpr auto value = Type::I32;
};

template<typename T>
constexpr auto wabt_to_rvalue_type_v = wabt_to_rvalue_type<T>::value;

template<typename>
struct wabt_function_type_provider;

template<typename Ret, typename ...Args>
struct wabt_function_type_provider<Ret(Args...)> {
   static FuncSignature type() {
      return FuncSignature({wabt_to_value_type_v<Args> ...}, {wabt_to_rvalue_type_v<Ret>});
   }
};
template<typename ...Args>
struct wabt_function_type_provider<void(Args...)> {
   static FuncSignature type() {
      return FuncSignature({wabt_to_value_type_v<Args> ...}, {});
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
template<typename Ret, typename NativeParameters>
struct intrinsic_invoker_impl;

/**
 * Specialization for the fully transcribed signature
 * @tparam Ret - the return type of the native function
 */
template<typename Ret>
struct intrinsic_invoker_impl<Ret, std::tuple<>> {
   using next_method_type        = Ret (*)(wabt_apply_instance_vars&, const TypedValues&, int);

   template<next_method_type Method>
   static TypedValue invoke(wabt_apply_instance_vars& vars, const TypedValues& args) {
      return convert_native_to_literal(vars, Method(vars, args, args.size() - 1));
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
template<>
struct intrinsic_invoker_impl<void_type, std::tuple<>> {
   using next_method_type        = void_type (*)(wabt_apply_instance_vars&, const TypedValues&, int);

   template<next_method_type Method>
   static TypedValue invoke(wabt_apply_instance_vars& vars, const TypedValues& args) {
      Method(vars, args, args.size() - 1);
      return TypedValue(Type::Void);
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
 */
template<typename Ret, typename Input, typename... Inputs>
struct intrinsic_invoker_impl<Ret, std::tuple<Input, Inputs...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>>;
   using then_type = Ret (*)(wabt_apply_instance_vars&, Input, Inputs..., const TypedValues&, int);

   template<then_type Then>
   static Ret translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) {
      auto& last = args.at(offset);
      auto native = convert_literal_to_native<Input>(last);
      return Then(vars, native, rest..., args, (uint32_t)offset - 1);
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
 */
template<typename T, typename Ret, typename... Inputs>
struct intrinsic_invoker_impl<Ret, std::tuple<array_ptr<T>, size_t, Inputs...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>>;
   using then_type = Ret(*)(wabt_apply_instance_vars&, array_ptr<T>, size_t, Inputs..., const TypedValues&, int);

   template<then_type Then, typename U=T>
   static auto translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) -> std::enable_if_t<std::is_const<U>::value, Ret> {
      static_assert(!std::is_pointer<U>::value, "Currently don't support array of pointers");
      uint32_t ptr = args.at((uint32_t)offset - 1).get_i32();
      size_t length = args.at((uint32_t)offset).get_i32();
      T* base = array_ptr_impl<T>(vars, ptr, length);
      if ( reinterpret_cast<uintptr_t>(base) % alignof(T) != 0 ) {
         wlog( "misaligned array of const values" );
         std::vector<std::remove_const_t<T> > copy(length > 0 ? length : 1);
         T* copy_ptr = &copy[0];
         memcpy( (void*)copy_ptr, (void*)base, length * sizeof(T) );
         return Then(vars, static_cast<array_ptr<T>>(copy_ptr), length, rest..., args, (uint32_t)offset - 2);
      }
      return Then(vars, static_cast<array_ptr<T>>(base), length, rest..., args, (uint32_t)offset - 2);
   };

   template<then_type Then, typename U=T>
   static auto translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) -> std::enable_if_t<!std::is_const<U>::value, Ret> {
      static_assert(!std::is_pointer<U>::value, "Currently don't support array of pointers");
      uint32_t ptr = args.at((uint32_t)offset - 1).get_i32();
      size_t length = args.at((uint32_t)offset).get_i32();
      T* base = array_ptr_impl<T>(vars, ptr, length);
      if ( reinterpret_cast<uintptr_t>(base) % alignof(T) != 0 ) {
         wlog( "misaligned array of values" );
         std::vector<std::remove_const_t<T> > copy(length > 0 ? length : 1);
         T* copy_ptr = &copy[0];
         memcpy( (void*)copy_ptr, (void*)base, length * sizeof(T) );
         Ret ret = Then(vars, static_cast<array_ptr<T>>(copy_ptr), length, rest..., args, (uint32_t)offset - 2);  
         memcpy( (void*)base, (void*)copy_ptr, length * sizeof(T) );
         return ret;
      }
      return Then(vars, static_cast<array_ptr<T>>(base), length, rest..., args, (uint32_t)offset - 2);
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
 */
template<typename Ret, typename... Inputs>
struct intrinsic_invoker_impl<Ret, std::tuple<null_terminated_ptr, Inputs...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>>;
   using then_type = Ret(*)(wabt_apply_instance_vars&, null_terminated_ptr, Inputs..., const TypedValues&, int);

   template<then_type Then>
   static Ret translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) {
      uint32_t ptr = args.at((uint32_t)offset).get_i32();
      return Then(vars, null_terminated_ptr_impl(vars, ptr), rest..., args, (uint32_t)offset - 1);
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
 */
template<typename T, typename U, typename Ret, typename... Inputs>
struct intrinsic_invoker_impl<Ret, std::tuple<array_ptr<T>, array_ptr<U>, size_t, Inputs...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>>;
   using then_type = Ret(*)(wabt_apply_instance_vars&, array_ptr<T>, array_ptr<U>, size_t, Inputs..., const TypedValues&, int);

   template<then_type Then>
   static Ret translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) {
      uint32_t ptr_t = args.at((uint32_t)offset - 2).get_i32();
      uint32_t ptr_u = args.at((uint32_t)offset - 1).get_i32();
      size_t length = args.at((uint32_t)offset).get_i32();
      static_assert(std::is_same<std::remove_const_t<T>, char>::value && std::is_same<std::remove_const_t<U>, char>::value, "Currently only support array of (const)chars");
      return Then(vars, array_ptr_impl<T>(vars, ptr_t, length), array_ptr_impl<U>(vars, ptr_u, length), length, args, (uint32_t)offset - 3);
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
 */
template<typename Ret>
struct intrinsic_invoker_impl<Ret, std::tuple<array_ptr<char>, int, size_t>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<>>;
   using then_type = Ret(*)(wabt_apply_instance_vars&, array_ptr<char>, int, size_t, const TypedValues&, int);

   template<then_type Then>
   static Ret translate_one(wabt_apply_instance_vars& vars, const TypedValues& args, int offset) {
      uint32_t ptr = args.at((uint32_t)offset - 2).get_i32();
      uint32_t value = args.at((uint32_t)offset - 1).get_i32();
      size_t length = args.at((uint32_t)offset).get_i32();
      return Then(vars, array_ptr_impl<char>(vars, ptr, length), value, length, args, (uint32_t)offset - 3);
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
 */
template<typename T, typename Ret, typename... Inputs>
struct intrinsic_invoker_impl<Ret, std::tuple<T *, Inputs...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>>;
   using then_type = Ret (*)(wabt_apply_instance_vars&, T *, Inputs..., const TypedValues&, int);

   template<then_type Then, typename U=T>
   static auto translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) -> std::enable_if_t<std::is_const<U>::value, Ret> {
      uint32_t ptr = args.at((uint32_t)offset).get_i32();
      T* base = array_ptr_impl<T>(vars, ptr, 1);
      if ( reinterpret_cast<uintptr_t>(base) % alignof(T) != 0 ) {
         wlog( "misaligned const pointer" );
         std::remove_const_t<T> copy;
         T* copy_ptr = &copy;
         memcpy( (void*)copy_ptr, (void*)base, sizeof(T) );
         return Then(vars, copy_ptr, rest..., args, (uint32_t)offset - 1);
      }
      return Then(vars, base, rest..., args, (uint32_t)offset - 1);
   };

   template<then_type Then, typename U=T>
   static auto translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) -> std::enable_if_t<!std::is_const<U>::value, Ret> {
      uint32_t ptr = args.at((uint32_t)offset).get_i32();
      T* base = array_ptr_impl<T>(vars, ptr, 1);
      if ( reinterpret_cast<uintptr_t>(base) % alignof(T) != 0 ) {
         wlog( "misaligned pointer" );
         T copy;
         memcpy( (void*)&copy, (void*)base, sizeof(T) );
         Ret ret = Then(vars, &copy, rest..., args, (uint32_t)offset - 1);
         memcpy( (void*)base, (void*)&copy, sizeof(T) );
         return ret; 
      }
      return Then(vars, base, rest..., args, (uint32_t)offset - 1);
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
 */
template<typename Ret, typename... Inputs>
struct intrinsic_invoker_impl<Ret, std::tuple<const name&, Inputs...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>>;
   using then_type = Ret (*)(wabt_apply_instance_vars&, const name&, Inputs..., const TypedValues&, int);

   template<then_type Then>
   static Ret translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) {
      uint64_t wasm_value = args.at((uint32_t)offset).get_i64();
      auto value = name(wasm_value);
      return Then(vars, value, rest..., args, (uint32_t)offset - 1);
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
 */
template<typename Ret, typename... Inputs>
struct intrinsic_invoker_impl<Ret, std::tuple<const fc::time_point_sec&, Inputs...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>>;
   using then_type = Ret (*)(wabt_apply_instance_vars&, const fc::time_point_sec&, Inputs..., const TypedValues&, int);

   template<then_type Then>
   static Ret translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) {
      uint32_t wasm_value = args.at((uint32_t)offset).get_i32();
      auto value = fc::time_point_sec(wasm_value);
      return Then(vars, value, rest..., args, (uint32_t)offset - 1);
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
 */
template<typename T, typename Ret, typename... Inputs>
struct intrinsic_invoker_impl<Ret, std::tuple<T &, Inputs...>> {
   using next_step = intrinsic_invoker_impl<Ret, std::tuple<Inputs...>>;
   using then_type = Ret (*)(wabt_apply_instance_vars&, T &, Inputs..., const TypedValues&, int);

   template<then_type Then, typename U=T>
   static auto translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) -> std::enable_if_t<std::is_const<U>::value, Ret> {
      // references cannot be created for null pointers
      uint32_t ptr = args.at((uint32_t)offset).get_i32();
      EOS_ASSERT(ptr != 0, binaryen_exception, "references cannot be created for null pointers");
      T* base = array_ptr_impl<T>(vars, ptr, 1);
      if ( reinterpret_cast<uintptr_t>(base) % alignof(T) != 0 ) {
         wlog( "misaligned const reference" );
         std::remove_const_t<T> copy;
         T* copy_ptr = &copy;
         memcpy( (void*)copy_ptr, (void*)base, sizeof(T) );
         return Then(vars, *copy_ptr, rest..., args, (uint32_t)offset - 1);
      }
      return Then(vars, *base, rest..., args, (uint32_t)offset - 1);
   }

   template<then_type Then, typename U=T>
   static auto translate_one(wabt_apply_instance_vars& vars, Inputs... rest, const TypedValues& args, int offset) -> std::enable_if_t<!std::is_const<U>::value, Ret> {
      // references cannot be created for null pointers
      uint32_t ptr = args.at((uint32_t)offset).get_i32();
      EOS_ASSERT(ptr != 0, binaryen_exception, "references cannot be created for null pointers");
      T* base = array_ptr_impl<T>(vars, ptr, 1);
      if ( reinterpret_cast<uintptr_t>(base) % alignof(T) != 0 ) {
         wlog( "misaligned reference" );
         T copy;
         memcpy( (void*)&copy, (void*)base, sizeof(T) );
         Ret ret = Then(vars, copy, rest..., args, (uint32_t)offset - 1);
         memcpy( (void*)base, (void*)&copy, sizeof(T) );
         return ret; 
      }
      return Then(vars, *base, rest..., args, (uint32_t)offset - 1);
   }


   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

extern apply_context* fixme_context;

/**
 * forward declaration of a wrapper class to call methods of the class
 */
template<typename Ret, typename MethodSig, typename Cls, typename... Params>
struct intrinsic_function_invoker {
   using impl = intrinsic_invoker_impl<Ret, std::tuple<Params...>>;

   template<MethodSig Method>
   static Ret wrapper(wabt_apply_instance_vars& vars, Params... params, const TypedValues&, int) {
      class_from_wasm<Cls>::value(vars.ctx).checktime();
      return (class_from_wasm<Cls>::value(vars.ctx).*Method)(params...);
   }

   template<MethodSig Method>
   static const intrinsic_registrator::intrinsic_fn fn() {
      return impl::template fn<wrapper<Method>>();
   }
};

template<typename MethodSig, typename Cls, typename... Params>
struct intrinsic_function_invoker<void, MethodSig, Cls, Params...> {
   using impl = intrinsic_invoker_impl<void_type, std::tuple<Params...>>;

   template<MethodSig Method>
   static void_type wrapper(wabt_apply_instance_vars& vars, Params... params, const TypedValues& args, int offset) {
      class_from_wasm<Cls>::value(vars.ctx).checktime();
      (class_from_wasm<Cls>::value(vars.ctx).*Method)(params...);
      return void_type();
   }

   template<MethodSig Method>
   static const intrinsic_registrator::intrinsic_fn fn() {
      return impl::template fn<wrapper<Method>>();
   }

};

template<typename>
struct intrinsic_function_invoker_wrapper;

template<typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<Ret (Cls::*)(Params...)> {
   using type = intrinsic_function_invoker<Ret, Ret (Cls::*)(Params...), Cls, Params...>;
};

template<typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<Ret (Cls::*)(Params...) const> {
   using type = intrinsic_function_invoker<Ret, Ret (Cls::*)(Params...) const, Cls, Params...>;
};

template<typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<Ret (Cls::*)(Params...) volatile> {
   using type = intrinsic_function_invoker<Ret, Ret (Cls::*)(Params...) volatile, Cls, Params...>;
};

template<typename Cls, typename Ret, typename... Params>
struct intrinsic_function_invoker_wrapper<Ret (Cls::*)(Params...) const volatile> {
   using type = intrinsic_function_invoker<Ret, Ret (Cls::*)(Params...) const volatile, Cls, Params...>;
};

#define __INTRINSIC_NAME(LABEL, SUFFIX) LABEL##SUFFIX
#define _INTRINSIC_NAME(LABEL, SUFFIX) __INTRINSIC_NAME(LABEL,SUFFIX)

#define _REGISTER_WABT_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG)\
   static eosio::chain::webassembly::wabt_runtime::intrinsic_registrator _INTRINSIC_NAME(__wabt_intrinsic_fn, __COUNTER__) (\
      MOD,\
      NAME,\
      eosio::chain::webassembly::wabt_runtime::wabt_function_type_provider<WASM_SIG>::type(),\
      eosio::chain::webassembly::wabt_runtime::intrinsic_function_invoker_wrapper<SIG>::type::fn<&CLS::METHOD>()\
   );\

} } } }// eosio::chain::webassembly::wabt_runtime
