#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/webassembly/wavm.hpp>
#include <eosio/chain/webassembly/binaryen.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>

#include "IR/Module.h"
#include "Runtime/Intrinsics.h"
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "IR/Validate.h"

using namespace fc;
using namespace eosio::chain::webassembly;
using namespace IR;
using namespace Runtime;

namespace eosio { namespace chain {

   extern apply_context* native_apply_context;

   struct wasm_interface_impl {
      wasm_interface_impl(wasm_interface::vm_type vm) {
         if(vm == wasm_interface::vm_type::wavm)
            runtime_interface = std::make_unique<webassembly::wavm::wavm_runtime>();
         else if(vm == wasm_interface::vm_type::binaryen)
            runtime_interface = std::make_unique<webassembly::binaryen::binaryen_runtime>();
         else
            FC_THROW("wasm_interface_impl fall through");
      }
      
      std::vector<uint8_t> parse_initial_memory(const Module& module) {
         std::vector<uint8_t> mem_image;

         for(const DataSegment& data_segment : module.dataSegments) {
            FC_ASSERT(data_segment.baseOffset.type == InitializerExpression::Type::i32_const);
            FC_ASSERT(module.memories.defs.size());
            const U32 base_offset = data_segment.baseOffset.i32;
            const Uptr memory_size = (module.memories.defs[0].type.size.min << IR::numBytesPerPageLog2);
            if(base_offset >= memory_size || base_offset + data_segment.data.size() > memory_size)
               FC_THROW_EXCEPTION(wasm_execution_error, "WASM data segment outside of valid memory range");
            if(base_offset + data_segment.data.size() > mem_image.size())
               mem_image.resize(base_offset + data_segment.data.size(), 0x00);
            memcpy(mem_image.data() + base_offset, data_segment.data.data(), data_segment.data.size());
         }

         return mem_image;
      }

      std::unique_ptr<wasm_instantiated_module_interface>& get_instantiated_module(const digest_type& code_id, const shared_vector<char>& code) {
         auto it = instantiation_cache.find(code_id);
         if(it == instantiation_cache.end()) {
            IR::Module module;
            try {
               Serialization::MemoryInputStream stream((const U8*)code.data(), code.size());
               WASM::serialize(stream, module);
            } catch(Serialization::FatalSerializationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            }

            wasm_injections::wasm_binary_injection injector(module);
            injector.inject();
            
            std::vector<U8> bytes;
            try {
               Serialization::ArrayOutputStream outstream;
               WASM::serialize(outstream, module);
               bytes = outstream.getBytes();
            } catch(Serialization::FatalSerializationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            }

            it = instantiation_cache.emplace(code_id, runtime_interface->instantiate_module((const char*)bytes.data(), bytes.size(), parse_initial_memory(module))).first;
         }
         return it->second;
      }

      std::unique_ptr<wasm_runtime_interface> runtime_interface;
      map<digest_type, std::unique_ptr<wasm_instantiated_module_interface>> instantiation_cache;
   };

#define _UNWRAP_SEQ(...) __VA_ARGS__

#define _EXPAND_ARGS(CLS, MOD, INFO)\
   ( CLS, MOD, _UNWRAP_SEQ INFO )

#define _EXPAND_NATIVE_ARGS(CLS, METHOD, NAME, NATIVE_INFO) \
   ( CLS, METHOD, NAME, _UNWRAP_SEQ NATIVE_INFO )

#define _REGISTER_NATIVE_INTRINSIC3(CLS, METHOD, NAME, RETURN, PARAM_LIST, ARGS)\
extern "C" RETURN NAME PARAM_LIST { \
   return class_from_wasm<CLS>::value(*native_apply_context).METHOD ARGS; \
}

#define _REGISTER_NATIVE_INTRINSIC2(CLS, METHOD, NAME, RETURN, PARAM_LIST)\
   static_assert(false, "Cannot create native binding for " BOOST_PP_STRINGIZE(CLS) "::" BOOST_PP_STRINGIZE(METHOD) " without argument list"); \

// Empty native binding list means don't create one
#define _REGISTER_NATIVE_INTRINSIC1(CLS, METHOD, NAME, RETURN)

#define _REGISTER_NATIVE_INTRINSIC0(CLS, METHOD, NAME)

#define INDIRECT_UNWRAP_SEQ(ARG) _UNWRAP_SEQ __VA_ARGS__

#define MY_OVERLOAD(PREFIX, NATIVE_INFO)\
   MY_CAT(PREFIX, BOOST_PP_VARIADIC_SIZE(NATIVE_INFO))

#define MY_CAT(a, b) MY_CAT_I(a, b)
#define MY_CAT_I(a, b) a ## b

#define _REGISTER_NATIVE_INTRINSIC(CLS, METHOD, NAME, NATIVE_INFO)\
   MY_CAT(MY_OVERLOAD(_REGISTER_NATIVE_INTRINSIC, _UNWRAP_SEQ NATIVE_INFO) _EXPAND_NATIVE_ARGS(CLS, METHOD, NAME, NATIVE_INFO), BOOST_PP_EMPTY())

#define _REGISTER_INTRINSIC_EXPLICIT(CLS, MOD, METHOD, WASM_SIG, NATIVE_INFO, NAME, SIG)\
   _REGISTER_WAVM_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, BOOST_PP_STRINGIZE(NAME), SIG)\
   _REGISTER_BINARYEN_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, BOOST_PP_STRINGIZE(NAME), SIG)\
   _REGISTER_NATIVE_INTRINSIC(CLS, METHOD, NAME, NATIVE_INFO)

#define _REGISTER_INTRINSIC5(CLS, MOD, METHOD, WASM_SIG, NATIVE_INFO, NAME, SIG)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, MOD, METHOD, WASM_SIG, NATIVE_INFO, NAME, SIG )

#define _REGISTER_INTRINSIC4(CLS, MOD, METHOD, WASM_SIG, NATIVE_INFO, NAME)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, MOD, METHOD, WASM_SIG, NATIVE_INFO, NAME, decltype(&CLS::METHOD) )

#define _REGISTER_INTRINSIC3(CLS, MOD, METHOD, WASM_SIG, NATIVE_INFO)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, MOD, METHOD, WASM_SIG, NATIVE_INFO, METHOD, decltype(&CLS::METHOD) )

#define _REGISTER_INTRINSIC2(CLS, MOD, METHOD, WASM_SIG)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":" BOOST_PP_STRINGIZE(METHOD) " without native signature");

#define _REGISTER_INTRINSIC1(CLS, MOD, METHOD)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":" BOOST_PP_STRINGIZE(METHOD) " without a signature");

#define _REGISTER_INTRINSIC0(CLS, MOD, METHOD)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":<unknown> without a method name and signature");

#define _REGISTER_INTRINSIC(R, CLS, INFO)\
   BOOST_PP_CAT(BOOST_PP_OVERLOAD(_REGISTER_INTRINSIC, _UNWRAP_SEQ INFO) _EXPAND_ARGS(CLS, "env", INFO), BOOST_PP_EMPTY())

// This macro registers intrinsic functions with the various wasm virtual machines (wavm, binaryen) and also enables
// their use during native contract debugging.  All of these bindings define mappings between the function with C
// binding that the contract calls and the implementation of that function which is usually a member function of an
// api class.
//
// This macro takes two arguments a class name that refers to the api class and a paren enclosed list of
// functions that the api class implements.
// Each function definition in that list can contain the following parameters:
//    METHOD      method name of the implementing member function in the api class
//    WASM_SIG    signature of the intrinsic function
//    NATIVE_INFO This is a paren enclosed list of arguments for generating the native intrinsic binding, which is
//                a function with extern "C" binding with the same name as the intrinsic. It can either be
//                left empty (and no native intrinsic function will be defined) or passed three parameters:
//                   RETURN      native return value of the intrinsic
//                   PARAM_LIST  Paren-enclosed argument list used for the function definition
//                   ARGS        Paren-enclosed argument list used for passing parameters to the api's member function.
//    NAME        optional name of the intrinsic function (defaults to METHOD)
//    SIG         optional signature of the api's member function named METHOD (defaults to WASM_SIG)
//
// Example:
//    REGISTER_INTRINSICS(system_api,
//       (abort,  void(), () ) // Native intrinsic not defined; system-defined abort() will be used
//       (eosio_assert, void(int, int), (void, (bool condition, null_terminated_ptr str), (condition, str)))
//       (eosio_exit,   void(int ),     (void, (int32_t code), (code)))
//    );
//    This macro call defines three intrinsics bound to the system_api class. abort passes an empty
//    NATIVE_INFO meaning no native-debugging intrinsic function is defined. 

#define REGISTER_INTRINSICS(CLS, MEMBERS)\
   BOOST_PP_SEQ_FOR_EACH(_REGISTER_INTRINSIC, CLS, _WRAPPED_SEQ(MEMBERS))

#define _REGISTER_INJECTED_INTRINSIC(R, CLS, INFO)\
   BOOST_PP_CAT(BOOST_PP_OVERLOAD(_REGISTER_INTRINSIC, _UNWRAP_SEQ INFO) _EXPAND_ARGS(CLS, EOSIO_INJECTED_MODULE_NAME, INFO), BOOST_PP_EMPTY())

#define REGISTER_INJECTED_INTRINSICS(CLS, MEMBERS)\
   BOOST_PP_SEQ_FOR_EACH(_REGISTER_INJECTED_INTRINSIC, CLS, _WRAPPED_SEQ(MEMBERS))

} } // eosio::chain
