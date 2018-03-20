#pragma once

#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/exceptions.hpp>
#include <wasm-interpreter.h>
#include <softfloat_types.h>


namespace eosio { namespace chain { namespace webassembly { namespace binaryen {

using namespace fc;
using namespace wasm;
using namespace eosio::chain::webassembly::common;


using linear_memory_type = fc::array<char, wasm_constraints::maximum_linear_memory>;
using call_indirect_table_type = vector<Name>;

struct intrinsic_registrator {
   using intrinsic_fn = Literal(*)(LiteralList&);

   static auto& get_map(){
      static map<string, intrinsic_fn> _map;
      return _map;
   };

   intrinsic_registrator(const char* name, intrinsic_fn fn)
   {
      get_map()[string(name)] = fn;
   }
};

using import_lut_type = unordered_map<uintptr_t, intrinsic_registrator::intrinsic_fn>;

// create own module instance to access memorySize
class interpreter_instance : public ModuleInstance {
   public:
      using ModuleInstance::ModuleInstance;

      void set_accessible_pages(uint32_t num_pages) {
         memorySize = num_pages;
      }
};

struct interpreter_interface : ModuleInstance::ExternalInterface {
   interpreter_interface(linear_memory_type& memory, call_indirect_table_type& table, import_lut_type& import_lut, const uint32_t& sbrk_bytes)
   :memory(memory),table(table),import_lut(import_lut), sbrk_bytes(sbrk_bytes)
   {}

   void importGlobals(std::map<Name, Literal>& globals, Module& wasm) override
   {

   }

   void init(Module& wasm, ModuleInstance& instance) override {

   }

   Literal callImport(Import *import, LiteralList &args) override
   {
      auto fn_iter = import_lut.find((uintptr_t)import);
      EOS_ASSERT(fn_iter != import_lut.end(), wasm_execution_error, "unknown import ${m}:${n}", ("m", import->module.c_str())("n", import->module.c_str()));
      return fn_iter->second(args);
   }

   Literal callTable(Index index, LiteralList& arguments, WasmType result, ModuleInstance& instance) override
   {
      EOS_ASSERT(index < table.size(), wasm_execution_error, "callIndirect: bad pointer");
      auto* func = instance.wasm.getFunctionOrNull(table[index]);
      EOS_ASSERT(func, wasm_execution_error, "callIndirect: uninitialized element");
      EOS_ASSERT(func->params.size() == arguments.size(), wasm_execution_error, "callIndirect: bad # of arguments");

      for (size_t i = 0; i < func->params.size(); i++) {
         EOS_ASSERT(func->params[i] == arguments[i].type, wasm_execution_error, "callIndirect: bad argument type");
      }
      EOS_ASSERT(func->result == result, wasm_execution_error, "callIndirect: bad result type");
      return instance.callFunctionInternal(func->name, arguments);
   }

   void trap(const char* why) override {
      FC_THROW_EXCEPTION(wasm_execution_error, why);
   }

   void assert_memory_is_accessible(uint32_t offset, size_t size) {
      #warning this is wrong
      if(offset + size > sbrk_bytes)
      FC_THROW_EXCEPTION(wasm_execution_error, "access violation");
   }

   char* get_validated_pointer(uint32_t offset, size_t size) {
      assert_memory_is_accessible(offset, size);
      return memory.data + offset;
   }

   template <typename T>
   static bool aligned_for(const void* address) {
      return 0 == (reinterpret_cast<uintptr_t>(address) & (std::alignment_of<T>::value - 1));
   }

   template<typename T>
   T load_memory(uint32_t offset) {
      char *base = get_validated_pointer(offset, sizeof(T));
      if (aligned_for<T>(base)) {
         return *reinterpret_cast<T*>(base);
      } else {
         T temp;
         memcpy(&temp, base, sizeof(T));
         return temp;
      }
   }

   template<typename T>
   void store_memory(uint32_t offset, T value) {
      char *base = get_validated_pointer(offset, sizeof(T));
      if (aligned_for<T>(base)) {
         *reinterpret_cast<T*>(base) = value;
      } else {
         memcpy(base, &value, sizeof(T));
      }
   }

   void growMemory(Address, Address) override
   {
      FC_THROW_EXCEPTION(wasm_execution_error, "grow memory is not supported");
   }

   int8_t load8s(Address addr) override { return load_memory<int8_t>(addr); }
   uint8_t load8u(Address addr) override { return load_memory<uint8_t>(addr); }
   int16_t load16s(Address addr) override { return load_memory<int16_t>(addr); }
   uint16_t load16u(Address addr) override { return load_memory<uint16_t>(addr); }
   int32_t load32s(Address addr) override { return load_memory<int32_t>(addr); }
   uint32_t load32u(Address addr) override { return load_memory<uint32_t>(addr); }
   int64_t load64s(Address addr) override { return load_memory<int64_t>(addr); }
   uint64_t load64u(Address addr) override { return load_memory<uint64_t>(addr); }

   void store8(Address addr, int8_t value) override { store_memory(addr, value); }
   void store16(Address addr, int16_t value) override { store_memory(addr, value); }
   void store32(Address addr, int32_t value) override { store_memory(addr, value); }
   void store64(Address addr, int64_t value) override { store_memory(addr, value); }

   linear_memory_type&          memory;
   call_indirect_table_type&    table;
   import_lut_type&             import_lut;
   const uint32_t&              sbrk_bytes;
};

struct info;
class entry {
public:
   unique_ptr<Module>                  module;
   linear_memory_type                  memory;
   call_indirect_table_type            table;
   import_lut_type                     import_lut;
   interpreter_interface*              interface;
   interpreter_instance*               instance;
   uint32_t                            sbrk_bytes;

   void reset(const info& );
   void prepare( const info& );

   void call(const string& entry_point, LiteralList& args, apply_context& context);
   void call_apply(apply_context&);
   void call_error(apply_context&);

   int sbrk(int num_bytes);

   static const entry& get(wasm_interface& wasm);

   static entry build(const char* wasm_binary, size_t wasm_binary_size);

private:
   entry(unique_ptr<Module>&& module, linear_memory_type&& memory, call_indirect_table_type&& table, import_lut_type&& import_lut, uint32_t sbrk_bytes);

};

struct info {
   info( const entry& binaryen )
   {
      default_sbrk_bytes = binaryen.sbrk_bytes;
      const char* start = binaryen.memory.data;
      const char* high_watermark = start + (binaryen.module->memory.initial * Memory::kPageSize);
      while (high_watermark > start) {
         if (*high_watermark) {
            break;
         }
         high_watermark--;
      }
      mem_image.resize(high_watermark - start);
      memcpy(mem_image.data(), start, high_watermark - start);
   }

   // a clean image of the memory used to sanitize things on checkin
   vector<char> mem_image;
   uint32_t default_sbrk_bytes;
};



/**
 * class to represent an in-wasm-memory array
 * it is a hint to the transcriber that the next parameter will
 * be a size (data bytes length) and that the pair are validated together
 * This triggers the template specialization of intrinsic_invoker_impl
 * @tparam T
 */
template<typename T>
inline array_ptr<T> array_ptr_impl (wasm_interface& wasm, uint32_t ptr, size_t length)
{
   auto &interface = entry::get(wasm).interface;
   return array_ptr<T>((T*)(interface->get_validated_pointer(ptr, length)));
}

/**
 * class to represent an in-wasm-memory char array that must be null terminated
 */
inline null_terminated_ptr null_terminated_ptr_impl(wasm_interface& wasm, uint32_t ptr)
{
   auto &interface = entry::get(wasm).interface;
   char *value = interface->get_validated_pointer(ptr, 1);
   const char* p = value;
   const char* const top_of_memory = interface->memory.data + interface->sbrk_bytes;
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
T convert_literal_to_native(Literal& v);

template<>
inline float64_t convert_literal_to_native<float64_t>(Literal& v) {
   auto val = v.getf64();
   return reinterpret_cast<float64_t&>(val);
}

template<>
inline int64_t convert_literal_to_native<int64_t>(Literal& v) {
   return v.geti64();
}

template<>
inline uint64_t convert_literal_to_native<uint64_t>(Literal& v) {
   return v.geti64();
}

template<>
inline int32_t convert_literal_to_native<int32_t>(Literal& v) {
   return v.geti32();
}

template<>
inline uint32_t convert_literal_to_native<uint32_t>(Literal& v) {
   return v.geti32();
}

template<>
inline bool convert_literal_to_native<bool>(Literal& v) {
   return v.geti32();
}

template<>
inline name convert_literal_to_native<name>(Literal& v) {
   int64_t val = v.geti64();
   return name(val);
}

template<typename T>
inline auto convert_native_to_literal(const wasm_interface &, T val) {
   return Literal(val);
}

inline auto convert_native_to_literal(const wasm_interface &, const float64_t& val) {
   return Literal( *((double*)(&val)) );
}

inline auto convert_native_to_literal(const wasm_interface &, const name &val) {
   return Literal(val.value);
}

inline auto convert_native_to_literal(const wasm_interface &, const fc::time_point_sec &val) {
   return Literal(val.sec_since_epoch());
}

inline auto convert_native_to_literal(wasm_interface &wasm, char* ptr) {
   const auto& mem = entry::get(wasm).interface->memory;
   const char* base = mem.data;
   const char* top_of_memory = base + mem.size();
   EOS_ASSERT(ptr >= base && ptr < top_of_memory, wasm_execution_error, "returning pointer not in linear memory");
   return Literal((int)(ptr - base));
}

struct void_type {
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
   using next_method_type        = Ret (*)(wasm_interface&, LiteralList&, int);

   template<next_method_type Method>
   static Literal invoke(LiteralList& args) {
      wasm_interface &wasm = wasm_interface::get();
      return convert_native_to_literal(wasm, Method(wasm, args, args.size() - 1));
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
   using next_method_type        = void_type (*)(wasm_interface&, LiteralList&, int);

   template<next_method_type Method>
   static Literal invoke(LiteralList& args) {
      wasm_interface &wasm = wasm_interface::get();
      Method(wasm, args, args.size() - 1);
      return Literal();
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
   using then_type = Ret (*)(wasm_interface &, Input, Inputs..., LiteralList&, int);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, LiteralList& args, int offset) {
      auto& last = args.at(offset);
      auto native = convert_literal_to_native<Input>(last);
      return Then(wasm, native, rest..., args, offset - 1);
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
   using then_type = Ret(*)(wasm_interface &, array_ptr<T>, size_t, Inputs..., LiteralList&, int);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, LiteralList& args, int offset) {
      uint32_t ptr = args.at(offset - 1).geti32();
      size_t length = args.at(offset).geti32();
      return Then(wasm, array_ptr_impl<T>(wasm, ptr, length), length, rest..., args, offset - 2);
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
   using then_type = Ret(*)(wasm_interface &, null_terminated_ptr, Inputs..., LiteralList&, int);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, LiteralList& args, int offset) {
      uint32_t ptr = args.at(offset).geti32();
      return Then(wasm, null_terminated_ptr_impl(wasm, ptr), rest..., args, offset - 1);
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
   using then_type = Ret(*)(wasm_interface &, array_ptr<T>, array_ptr<U>, size_t, Inputs..., LiteralList&, int);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, LiteralList& args, int offset) {
      uint32_t ptr_t = args.at(offset - 2).geti32();
      uint32_t ptr_u = args.at(offset - 1).geti32();
      size_t length = args.at(offset).geti32();
      return Then(wasm, array_ptr_impl<T>(wasm, ptr_t, length), array_ptr_impl<U>(wasm, ptr_u, length), length, args, offset - 3);
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
   using then_type = Ret(*)(wasm_interface &, array_ptr<char>, int, size_t, LiteralList&, int);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, LiteralList& args, int offset) {
      uint32_t ptr = args.at(offset - 2).geti32();
      uint32_t value = args.at(offset - 1).geti32();
      size_t length = args.at(offset).geti32();
      return Then(wasm, array_ptr_impl<char>(wasm, ptr, length), value, length, args, offset - 3);
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
   using then_type = Ret (*)(wasm_interface &, T *, Inputs..., LiteralList&, int);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, LiteralList& args, int offset) {
      uint32_t ptr = args.at(offset).geti32();
      T* base = array_ptr_impl<T>(wasm, ptr, sizeof(T));
      return Then(wasm, base, rest..., args, offset - 1);
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
   using then_type = Ret (*)(wasm_interface &, const name&, Inputs..., LiteralList&, int);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, LiteralList& args, int offset) {
      uint64_t wasm_value = args.at(offset).geti64();
      auto value = name(wasm_value);
      return Then(wasm, value, rest..., args, offset - 1);
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
   using then_type = Ret (*)(wasm_interface &, const fc::time_point_sec&, Inputs..., LiteralList&, int);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, LiteralList& args, int offset) {
      uint32_t wasm_value = args.at(offset).geti32();
      auto value = fc::time_point_sec(wasm_value);
      return Then(wasm, value, rest..., args, offset - 1);
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
   using then_type = Ret (*)(wasm_interface &, T &, Inputs..., LiteralList&, int);

   template<then_type Then>
   static Ret translate_one(wasm_interface &wasm, Inputs... rest, LiteralList& args, int offset) {
      // references cannot be created for null pointers
      uint32_t ptr = args.at(offset).geti32();
      FC_ASSERT(ptr != 0);
      T* base = array_ptr_impl<T>(wasm, ptr, sizeof(T));
      return Then(wasm, *base, rest..., args, offset - 1);
   }

   template<then_type Then>
   static const auto fn() {
      return next_step::template fn<translate_one<Then>>();
   }
};

/**
 * forward declaration of a wrapper class to call methods of the class
 */
template<typename Ret, typename MethodSig, typename Cls, typename... Params>
struct intrinsic_function_invoker {
   using impl = intrinsic_invoker_impl<Ret, std::tuple<Params...>>;

   template<MethodSig Method>
   static Ret wrapper(wasm_interface &wasm, Params... params, LiteralList&, int) {
      return (class_from_wasm<Cls>::value(wasm).*Method)(params...);
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
   static void_type wrapper(wasm_interface &wasm, Params... params, LiteralList& args, int offset) {
      (class_from_wasm<Cls>::value(wasm).*Method)(params...);
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

#define _ADD_PAREN_1(...) ((__VA_ARGS__)) _ADD_PAREN_2
#define _ADD_PAREN_2(...) ((__VA_ARGS__)) _ADD_PAREN_1
#define _ADD_PAREN_1_END
#define _ADD_PAREN_2_END
#define _WRAPPED_SEQ(SEQ) BOOST_PP_CAT(_ADD_PAREN_1 SEQ, _END)

#define __INTRINSIC_NAME(LABEL, SUFFIX) LABEL##SUFFIX
#define _INTRINSIC_NAME(LABEL, SUFFIX) __INTRINSIC_NAME(LABEL,SUFFIX)

#define _REGISTER_BINARYEN_INTRINSIC(CLS, METHOD, WASM_SIG, NAME, SIG)\
   static eosio::chain::webassembly::binaryen::intrinsic_registrator _INTRINSIC_NAME(__binaryen_intrinsic_fn, __COUNTER__) (\
      NAME,\
      eosio::chain::webassembly::binaryen::intrinsic_function_invoker_wrapper<SIG>::type::fn<&CLS::METHOD>()\
   );\


} } } }// eosio::chain::webassembly::wavm
