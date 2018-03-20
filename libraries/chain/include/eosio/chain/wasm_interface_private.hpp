#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/webassembly/wavm.hpp>
#include <eosio/chain/webassembly/binaryen.hpp>

using namespace fc;
using namespace eosio::chain::webassembly;

namespace eosio { namespace chain {

   struct wasm_cache::entry {
      entry(wavm::entry&& wavm, binaryen::entry&& binaryen)
         : wavm(std::forward<wavm::entry>(wavm))
         , binaryen(std::forward<binaryen::entry>(binaryen))
      {
      }

      wavm::entry wavm;
      binaryen::entry binaryen;
   };

   struct wasm_interface_impl {
      optional<common::wasm_context> current_context;
   };

#define _UNWRAP_SEQ(...) __VA_ARGS__

#define _EXPAND_ARGS(CLS, INFO)\
   ( CLS, _UNWRAP_SEQ INFO )

#define _EXPAND_NATIVE_ARGS(CLS, METHOD, NAME, NATIVE_INFO) \
   ( CLS, METHOD, NAME, _UNWRAP_SEQ NATIVE_INFO )

#define _REGISTER_NATIVE_INTRINSIC3(CLS, METHOD, NAME, RETURN, PARAM_LIST, ARGS)\
extern "C" RETURN NAME PARAM_LIST { \
   return class_from_wasm<CLS>::value(wasm_interface::get()).METHOD ARGS; \
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

#define _REGISTER_INTRINSIC_EXPLICIT(CLS, METHOD, WASM_SIG, NATIVE_INFO, NAME, SIG)\
   _REGISTER_WAVM_INTRINSIC(CLS, METHOD, WASM_SIG, BOOST_PP_STRINGIZE(NAME), SIG)\
   _REGISTER_BINARYEN_INTRINSIC(CLS, METHOD, WASM_SIG, BOOST_PP_STRINGIZE(NAME), SIG)\
   _REGISTER_NATIVE_INTRINSIC(CLS, METHOD, NAME, NATIVE_INFO)

#define _REGISTER_INTRINSIC5(CLS, METHOD, WASM_SIG, NATIVE_INFO, NAME, SIG)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, METHOD, WASM_SIG, NATIVE_INFO, NAME, SIG )

#define _REGISTER_INTRINSIC4(CLS, METHOD, WASM_SIG, NATIVE_INFO, NAME)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, METHOD, WASM_SIG, NATIVE_INFO, NAME, decltype(&CLS::METHOD) )

#define _REGISTER_INTRINSIC3(CLS, METHOD, WASM_SIG, NATIVE_INFO)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, METHOD, WASM_SIG, NATIVE_INFO, METHOD, decltype(&CLS::METHOD) )

#define _REGISTER_INTRINSIC2(CLS, METHOD, WASM_SIG)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":" BOOST_PP_STRINGIZE(METHOD) " without native signature");

#define _REGISTER_INTRINSIC1(CLS, METHOD)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":" BOOST_PP_STRINGIZE(METHOD) " without a signature");

#define _REGISTER_INTRINSIC0(CLS, METHOD)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":<unknown> without a method name and signature");

#define _REGISTER_INTRINSIC(R, CLS, INFO)\
   BOOST_PP_CAT(BOOST_PP_OVERLOAD(_REGISTER_INTRINSIC, _UNWRAP_SEQ INFO) _EXPAND_ARGS(CLS, INFO), BOOST_PP_EMPTY())

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
//                   PARAM_LIST  Parentheses-enclosed argument list used for the function definition
//                   ARGS        Parentheses-enclosed argument list used for passing parameters to the api's member function.
//    NAME        name of the intrinsic function (defaults to METHOD)
//    SIG         signature of the api's member function named METHOD
//
// Examples:
//    TODO: add some examples
#define REGISTER_INTRINSICS(CLS, MEMBERS)\
   BOOST_PP_SEQ_FOR_EACH(_REGISTER_INTRINSIC, CLS, _WRAPPED_SEQ(MEMBERS))

} } // eosio::chain
