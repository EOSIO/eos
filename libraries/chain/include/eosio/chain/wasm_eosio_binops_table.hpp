#pragma once


#include <eosio/chain/wasm_binary_ops.hpp>
#include <boost/preprocessor/seq/fold_right.hpp>
#include <type_traits>


namespace eosio { namespace chain { namespace wasm_ops {

template <class Op_Types>
struct wasm_ops_table {
};

using wasm_reinterpreter = std::function<instr*(uint8_t)>;

template <class Op_Types, uint8_t Code = error> 
struct which_pointer{
   using pointer_t = typename Op_Types::error_t;
   static instr* reinterpret( char* block ) { return reinterpret_cast<pointer_t>(block); };
};

#define GENERATE_SPECIALIZATION( r, P, OP )                                                    \
   template <class Op_Types>                                                                   \
   struct which_pointer <Op_Types, OP > {                                                      \
      using P = typename Op_Types:: BOOST_PP_CAT(OP,_t;)                                       \
      static instr* reinterpret( char* block ) { return reinterpret_cast< P >(block); };       \
   };

BOOST_PP_SEQ_FOR_EACH( GENERATE_SPECIALIZATION, instr_t, WASM_OP_SEQ )

// construct the table
template <uint8_t Code, class Op_Types>
struct generate_op_table;

// the base case
template <class Op_Types>
struct generate_op_table <unreachable, Op_Types> {
};

}}} // namespace eosio, chain, wasm_ops
