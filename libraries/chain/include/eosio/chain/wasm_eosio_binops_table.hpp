#pragma once


#include <eosio/chain/wasm_binary_ops.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/push_back.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <type_traits>


namespace eosio { namespace chain { namespace wasm_ops {

using wasm_ref_func = std::function<instr*()>;

// forward declaration
template <class Op_Types, uint8_t Code> 
struct which_pointer;

template <class Op_Types>
struct wasm_op_table {

// construct a set of wasm ops that we can take the address from 
#define CACHED_OP( r, PRE, OP ) \
      static typename Op_Types::BOOST_PP_CAT(OP,_t) BOOST_PP_CAT(PRE,OP);
      BOOST_PP_SEQ_FOR_EACH( CACHED_OP, cached_, WASM_OP_SEQ )
#undef CACHED_OP

// copied straight from boost documentation 
#define PRED(r, state) \
   BOOST_PP_NOT_EQUAL( \
      BOOST_PP_TUPLE_ELEM(2, 0, state), \
      BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 1, state)) \
   ) 
#define OP(r, state) \
   ( \
      BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 0, state)), \
      BOOST_PP_TUPLE_ELEM(2, 1, state) \
   ) 
#define MACRO(r, state) which_pointer<Op_Types, BOOST_PP_TUPLE_ELEM(2, 0, state)>::func_ptr,
  
   static wasm_ref_func default_ref_func() { return &cached_error; } 
#define GENERATE_REINTERPRETERS( r, P, OP ) \
   static wasm_ref_func BOOST_PP_CAT(OP, _ref_func)() { return &BOOST_PP_CAT(P, OP); }
   BOOST_PP_SEQ_FOR_EACH( GENERATE_REINTERPRETERS, cached_, WASM_OP_SEQ )
#undef GENERATE_REINTERPRETERS

   enum { op_table_size = 0xFF };
   //static constexpr std::array<instr*, 0xFF> op_table = GenTable<Op_Types>(); //{ BOOST_PP_FOR((0, 255), PRED, OP, MACRO) which_pointer<Op_Types, 0xFF>::instr_ptr };
   static const instr* op_table[op_table_size]; // = { BOOST_PP_FOR((0, 253), PRED, OP, MACRO) which_pointer<Op_Types, 0xFF>::instr_ptr };
};

template <class Op_Types>
const instr* wasm_op_table<Op_Types>::op_table[op_table_size] = { BOOST_PP_FOR((0, 253), PRED, OP, MACRO) which_pointer<Op_Types, 0xFF>::func_ptr };

#if 1
// default to error
template <class Op_Types, uint8_t Code = error_code> 
struct which_pointer {
   static constexpr wasm_ref_func func_ptr = wasm_op_table<Op_Types>::default_ref_func;
};

// create specializations
#define GENERATE_SPECIALIZATION( r, P, OP )                                \
      template <class Op_Types>                                            \
      struct which_pointer <Op_Types, BOOST_PP_CAT(OP, _code) > {          \
         static constexpr wasm_ref_func P = wasm_op_table<Op_Types>::BOOST_PP_CAT(OP, _ref_func); \
      };    
BOOST_PP_SEQ_FOR_EACH( GENERATE_SPECIALIZATION, func_ptr, WASM_OP_SEQ )
#undef GENERATE_SPECIALIZATION
#endif



}}} // namespace eosio, chain, wasm_ops
