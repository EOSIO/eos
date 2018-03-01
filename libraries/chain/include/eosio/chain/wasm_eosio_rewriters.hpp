#pragma once

#include <eosio/chain/wasm_binary_ops.hpp>
#include <iostream>

namespace eosio { namespace chain { namespace wasm_rewriter {
using namespace eosio::chain::wasm_ops;

// simple mutator that doesn't actually mutate anything
// used to verify that a given instruction is valid for execution on our platform
struct whitelist_mutator {
   static wasm_return_t accept( instr* inst ) {
      // just pass
   }
};

struct wasm_opcode_no_disposition_exception {
   std::string opcode_name;
};
struct blacklist_mutator {
   static wasm_return_t accept( instr* inst ) {
      // fail
      throw wasm_opcode_no_disposition_exception { inst->to_string() };   
   }
};

// add opcode specific constraints here
// so far we only black list
struct op_constrainers : op_types<blacklist_mutator> {
   using block_t        = _block<whitelist_mutator>;
   using loop_t         = _loop<whitelist_mutator>;
   using if_t           = _if_eps<whitelist_mutator>;
   using if_else_t      = _if_else<whitelist_mutator>;
   
   using end_t          = _end<whitelist_mutator>;
   using unreachable_t  = _unreachable<whitelist_mutator>;
   using br_t           = _br<whitelist_mutator>;
   using br_if_t        = _br_if<whitelist_mutator>;
   using br_table_t     = _br_table<whitelist_mutator>;
   using return_t       = _ret<whitelist_mutator>;
   using call_t         = _call<whitelist_mutator>;
   using call_indirect_t = _call_indirect<whitelist_mutator>;
   using drop_t         = _drop<whitelist_mutator>;
   using select_t       = _select<whitelist_mutator>;

   using get_local_t    = _get_local<whitelist_mutator>;
   using set_local_t    = _set_local<whitelist_mutator>;
   using tee_local_t    = _tee_local<whitelist_mutator>;
   using get_global_t   = _get_global<whitelist_mutator>;
   using set_global_t   = _set_global<whitelist_mutator>;

   using nop_t          = _nop<whitelist_mutator>;
   using i32_const_t    = _i32_const<whitelist_mutator>;
   using i64_const_t    = _i64_const<whitelist_mutator>;
   using f32_const_t    = _f32_const<whitelist_mutator>;
   using f64_const_t    = _f64_const<whitelist_mutator>;
   
   using i32_eqz_t      = _i32_eqz<whitelist_mutator>;
   using i32_eq_t       = _i32_eq<whitelist_mutator>;
   using i32_ne_t       = _i32_ne<whitelist_mutator>;
   using i32_lt_s_t     = _i32_lt_s<whitelist_mutator>;
   using i32_lt_u_t     = _i32_lt_u<whitelist_mutator>;
   using i32_gt_s_t     = _i32_gt_s<whitelist_mutator>;
   using i32_gt_u_t     = _i32_gt_u<whitelist_mutator>;
   using i32_le_s_t     = _i32_le_s<whitelist_mutator>;
   using i32_le_u_t     = _i32_le_u<whitelist_mutator>;
   using i32_ge_s_t     = _i32_ge_s<whitelist_mutator>;
   using i32_ge_u_t     = _i32_ge_u<whitelist_mutator>;

   using i32_clz_t      = _i32_clz<whitelist_mutator>;
   using i32_ctz_t      = _i32_ctz<whitelist_mutator>;
   using i32_popcount_t = _i32_popcount<whitelist_mutator>;

   using i32_add_t      = _i32_add<whitelist_mutator>; 
   using i32_sub_t      = _i32_sub<whitelist_mutator>; 
   using i32_mul_t      = _i32_mul<whitelist_mutator>; 
   using i32_div_s_t    = _i32_div_s<whitelist_mutator>; 
   using i32_div_u_t    = _i32_div_u<whitelist_mutator>; 
   using i32_rem_s_t    = _i32_rem_s<whitelist_mutator>; 
   using i32_rem_u_t    = _i32_rem_u<whitelist_mutator>; 
   using i32_and_t      = _i32_and<whitelist_mutator>; 
   using i32_or_t       = _i32_or<whitelist_mutator>; 
   using i32_xor_t      = _i32_xor<whitelist_mutator>; 
   using i32_shl_t      = _i32_shl<whitelist_mutator>; 
   using i32_shr_s_t    = _i32_shr_s<whitelist_mutator>; 
   using i32_shr_u_t    = _i32_shr_u<whitelist_mutator>; 
   using i32_rotl_t     = _i32_rotl<whitelist_mutator>; 
   using i32_rotr_t     = _i32_rotr<whitelist_mutator>; 

   using i64_eqz_t      = _i64_eqz<whitelist_mutator>;
   using i64_eq_t       = _i64_eq<whitelist_mutator>;
   using i64_ne_t       = _i64_ne<whitelist_mutator>;
   using i64_lt_s_t     = _i64_lt_s<whitelist_mutator>;
   using i64_lt_u_t     = _i64_lt_u<whitelist_mutator>;
   using i64_gt_s_t     = _i64_gt_s<whitelist_mutator>;
   using i64_gt_u_t     = _i64_gt_u<whitelist_mutator>;
   using i64_le_s_t     = _i64_le_s<whitelist_mutator>;
   using i64_le_u_t     = _i64_le_u<whitelist_mutator>;
   using i64_ge_s_t     = _i64_ge_s<whitelist_mutator>;
   using i64_ge_u_t     = _i64_ge_u<whitelist_mutator>;

   using i64_clz_t      = _i64_clz<whitelist_mutator>;
   using i64_ctz_t      = _i64_ctz<whitelist_mutator>;
   using i64_popcount_t = _i64_popcount<whitelist_mutator>;

   using i64_add_t      = _i64_add<whitelist_mutator>; 
   using i64_sub_t      = _i64_sub<whitelist_mutator>; 
   using i64_mul_t      = _i64_mul<whitelist_mutator>; 
   using i64_div_s_t    = _i64_div_s<whitelist_mutator>; 
   using i64_div_u_t    = _i64_div_u<whitelist_mutator>; 
   using i64_rem_s_t    = _i64_rem_s<whitelist_mutator>; 
   using i64_rem_u_t    = _i64_rem_u<whitelist_mutator>; 
   using i64_and_t      = _i64_and<whitelist_mutator>; 
   using i64_or_t       = _i64_or<whitelist_mutator>; 
   using i64_xor_t      = _i64_xor<whitelist_mutator>; 
   using i64_shl_t      = _i64_shl<whitelist_mutator>; 
   using i64_shr_s_t    = _i64_shr_s<whitelist_mutator>; 
   using i64_shr_u_t    = _i64_shr_u<whitelist_mutator>; 
   using i64_rotl_t     = _i64_rotl<whitelist_mutator>; 
   using i64_rotr_t     = _i64_rotr<whitelist_mutator>; 

   using i32_wrap_i64_t = _i32_wrap_i64<whitelist_mutator>;
   using i64_extend_s_i32_t = _i64_extend_s_i32<whitelist_mutator>;
   using i64_extend_u_i32_t = _i64_extend_u_i32<whitelist_mutator>;
   // TODO, make sure these are just pointer reinterprets
   using i32_reinterpret_f32_t = _i32_reinterpret_f32<whitelist_mutator>;
   using f32_reinterpret_i32_t = _f32_reinterpret_i32<whitelist_mutator>;
   using i64_reinterpret_f64_t = _i64_reinterpret_f64<whitelist_mutator>;
   using f64_reinterpret_i64_t = _f64_reinterpret_i64<whitelist_mutator>;

}; // op_constrainers

// define the rewriters used for injections
struct instruction_count {
   static wasm_return_t accept( instr* inst ) {
      icnt++;
   }
   static uint32_t icnt;
};

uint32_t instruction_count::icnt = 0;

struct checktime_injector {
   static wasm_return_t accept( instr* inst ) {
      if (checktime_idx == -1)
	FC_THROW("Error, this should be run after module rewriter");
      wasm_ops::op_types<>::i32_const_t cnt; 
      cnt.n = instruction_count::icnt;
      wasm_ops::op_types<>::call_t chktm; 
      chktm.n = checktime_idx;
      instruction_count::icnt = 0;
   }
   static int32_t checktime_idx;
};

int32_t checktime_injector::checktime_idx = -1;

struct rewriters : op_types<whitelist_mutator> {

};

}}} //namespace eosio, chain, wasm_rewriter
