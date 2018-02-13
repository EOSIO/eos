#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/webassembly/jit.hpp>

using namespace fc;
using namespace eosio::chain::webassembly;

namespace eosio { namespace chain {

   struct wasm_cache::entry {
      entry(jit::entry&& jit)
         : jit(std::forward<jit::entry>(jit))
      {
      }

      jit::entry jit;
   };

   struct wasm_interface_impl {
      optional<common::wasm_context> current_context;
   };

#define _REGISTER_INTRINSIC_EXPLICIT(CLS, METHOD, WASM_SIG, NAME, SIG)\
   _REGISTER_JIT_INTRINSIC(CLS, METHOD, WASM_SIG, NAME, SIG)

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
