#pragma once

#include <eosio/chain/wasm_eosio_rewriters.hpp>
#include <functional>
#include <vector>
#include <iostream>
#include "IR/Module.h"
#include "IR/Operators.h"
#include "WASM/WASM.h"

namespace eosio { namespace chain { namespace wasm_constraints {

   // module validators
   // effectively do nothing and pass
   struct noop_validation_visitor {
      static void validate( IR::Module& m );
   };

   struct memories_validation_visitor {
      static void validate( IR::Module& m );
   };

   struct data_segments_validation_visitor {
      static void validate( IR::Module& m );
   };

   struct tables_validation_visitor {
      static void validate( IR::Module& m );
   };

   struct globals_validation_visitor {
      static void validate( IR::Module& m );
   };

   struct blacklist_validation_visitor {
      static void validate( IR::Module& m );
   };

   using wasm_validate_func = std::function<void(IR::Module&)>;

  
   // just pass 
   struct no_constraints_validators {
      static void validate( IR::Module& m ) {}
   };

   // instruction validators
   // simple mutator that doesn't actually mutate anything
   // used to verify that a given instruction is valid for execution on our platform
   struct whitelist_validator {
      static wasm_ops::wasm_return_t accept( wasm_ops::instr* inst ) {
         // just pass
      }
   };

   struct wasm_opcode_no_disposition_exception {
      std::string opcode_name;
   };

   struct blacklist_validator {
      static wasm_ops::wasm_return_t accept( wasm_ops::instr* inst ) {
         throw wasm_opcode_no_disposition_exception { inst->to_string() };   
      }
   };

   // add opcode specific constraints here
   // so far we only black list
   struct op_constrainers : wasm_ops::op_types<blacklist_validator> {
      using block_t        = wasm_ops::block<whitelist_validator>;
      using loop_t         = wasm_ops::loop<whitelist_validator>;
      using if__t          = wasm_ops::if_<whitelist_validator>;
      using else__t        = wasm_ops::else_<whitelist_validator>;
      
      using end_t          = wasm_ops::end<whitelist_validator>;
      using unreachable_t  = wasm_ops::unreachable<whitelist_validator>;
      using br_t           = wasm_ops::br<whitelist_validator>;
      using br_if_t        = wasm_ops::br_if<whitelist_validator>;
      using br_table_t     = wasm_ops::br_table<whitelist_validator>;
      using return_t       = wasm_ops::return_<whitelist_validator>;
      using call_t         = wasm_ops::call<whitelist_validator>;
      using call_indirect_t =wasm_ops:: call_indirect<whitelist_validator>;
      using drop_t         = wasm_ops::drop<whitelist_validator>;
      using select_t       = wasm_ops::select<whitelist_validator>;

      using get_local_t    = wasm_ops::get_local<whitelist_validator>;
      using set_local_t    = wasm_ops::set_local<whitelist_validator>;
      using tee_local_t    = wasm_ops::tee_local<whitelist_validator>;
      using get_global_t   = wasm_ops::get_global<whitelist_validator>;
      using set_global_t   = wasm_ops::set_global<whitelist_validator>;

      using nop_t          = wasm_ops::nop<whitelist_validator>;
      using i32_const_t    = wasm_ops::i32_const<whitelist_validator>;
      using i64_const_t    = wasm_ops::i64_const<whitelist_validator>;
      using f32_const_t    = wasm_ops::f32_const<whitelist_validator>;
      using f64_const_t    = wasm_ops::f64_const<whitelist_validator>;
      
      using i32_eqz_t      = wasm_ops::i32_eqz<whitelist_validator>;
      using i32_eq_t       = wasm_ops::i32_eq<whitelist_validator>;
      using i32_ne_t       = wasm_ops::i32_ne<whitelist_validator>;
      using i32_lt_s_t     = wasm_ops::i32_lt_s<whitelist_validator>;
      using i32_lt_u_t     = wasm_ops::i32_lt_u<whitelist_validator>;
      using i32_gt_s_t     = wasm_ops::i32_gt_s<whitelist_validator>;
      using i32_gt_u_t     = wasm_ops::i32_gt_u<whitelist_validator>;
      using i32_le_s_t     = wasm_ops::i32_le_s<whitelist_validator>;
      using i32_le_u_t     = wasm_ops::i32_le_u<whitelist_validator>;
      using i32_ge_s_t     = wasm_ops::i32_ge_s<whitelist_validator>;
      using i32_ge_u_t     = wasm_ops::i32_ge_u<whitelist_validator>;

      using i32_clz_t      = wasm_ops::i32_clz<whitelist_validator>;
      using i32_ctz_t      = wasm_ops::i32_ctz<whitelist_validator>;
      using i32_popcnt_t   = wasm_ops::i32_popcnt<whitelist_validator>;

      using i32_add_t      = wasm_ops::i32_add<whitelist_validator>; 
      using i32_sub_t      = wasm_ops::i32_sub<whitelist_validator>; 
      using i32_mul_t      = wasm_ops::i32_mul<whitelist_validator>; 
      using i32_div_s_t    = wasm_ops::i32_div_s<whitelist_validator>; 
      using i32_div_u_t    = wasm_ops::i32_div_u<whitelist_validator>; 
      using i32_rem_s_t    = wasm_ops::i32_rem_s<whitelist_validator>; 
      using i32_rem_u_t    = wasm_ops::i32_rem_u<whitelist_validator>; 
      using i32_and_t      = wasm_ops::i32_and<whitelist_validator>; 
      using i32_or_t       = wasm_ops::i32_or<whitelist_validator>; 
      using i32_xor_t      = wasm_ops::i32_xor<whitelist_validator>; 
      using i32_shl_t      = wasm_ops::i32_shl<whitelist_validator>; 
      using i32_shr_s_t    = wasm_ops::i32_shr_s<whitelist_validator>; 
      using i32_shr_u_t    = wasm_ops::i32_shr_u<whitelist_validator>; 
      using i32_rotl_t     = wasm_ops::i32_rotl<whitelist_validator>; 
      using i32_rotr_t     = wasm_ops::i32_rotr<whitelist_validator>; 

      using i64_eqz_t      = wasm_ops::i64_eqz<whitelist_validator>;
      using i64_eq_t       = wasm_ops::i64_eq<whitelist_validator>;
      using i64_ne_t       = wasm_ops::i64_ne<whitelist_validator>;
      using i64_lt_s_t     = wasm_ops::i64_lt_s<whitelist_validator>;
      using i64_lt_u_t     = wasm_ops::i64_lt_u<whitelist_validator>;
      using i64_gt_s_t     = wasm_ops::i64_gt_s<whitelist_validator>;
      using i64_gt_u_t     = wasm_ops::i64_gt_u<whitelist_validator>;
      using i64_le_s_t     = wasm_ops::i64_le_s<whitelist_validator>;
      using i64_le_u_t     = wasm_ops::i64_le_u<whitelist_validator>;
      using i64_ge_s_t     = wasm_ops::i64_ge_s<whitelist_validator>;
      using i64_ge_u_t     = wasm_ops::i64_ge_u<whitelist_validator>;

      using i64_clz_t      = wasm_ops::i64_clz<whitelist_validator>;
      using i64_ctz_t      = wasm_ops::i64_ctz<whitelist_validator>;
      using i64_popcnt_t   = wasm_ops::i64_popcnt<whitelist_validator>;

      using i64_add_t      = wasm_ops::i64_add<whitelist_validator>; 
      using i64_sub_t      = wasm_ops::i64_sub<whitelist_validator>; 
      using i64_mul_t      = wasm_ops::i64_mul<whitelist_validator>; 
      using i64_div_s_t    = wasm_ops::i64_div_s<whitelist_validator>; 
      using i64_div_u_t    = wasm_ops::i64_div_u<whitelist_validator>; 
      using i64_rem_s_t    = wasm_ops::i64_rem_s<whitelist_validator>; 
      using i64_rem_u_t    = wasm_ops::i64_rem_u<whitelist_validator>; 
      using i64_and_t      = wasm_ops::i64_and<whitelist_validator>; 
      using i64_or_t       = wasm_ops::i64_or<whitelist_validator>; 
      using i64_xor_t      = wasm_ops::i64_xor<whitelist_validator>; 
      using i64_shl_t      = wasm_ops::i64_shl<whitelist_validator>; 
      using i64_shr_s_t    = wasm_ops::i64_shr_s<whitelist_validator>; 
      using i64_shr_u_t    = wasm_ops::i64_shr_u<whitelist_validator>; 
      using i64_rotl_t     = wasm_ops::i64_rotl<whitelist_validator>; 
      using i64_rotr_t     = wasm_ops::i64_rotr<whitelist_validator>; 

      using i32_wrap_i64_t = wasm_ops::i32_wrap_i64<whitelist_validator>;
      using i64_extend_s_i32_t = wasm_ops::i64_extend_s_i32<whitelist_validator>;
      using i64_extend_u_i32_t = wasm_ops::i64_extend_u_i32<whitelist_validator>;
      // TODO, make sure these are just pointer reinterprets
      using i32_reinterpret_f32_t = wasm_ops::i32_reinterpret_f32<whitelist_validator>;
      using f32_reinterpret_i32_t = wasm_ops::f32_reinterpret_i32<whitelist_validator>;
      using i64_reinterpret_f64_t = wasm_ops::i64_reinterpret_f64<whitelist_validator>;
      using f64_reinterpret_i64_t = wasm_ops::f64_reinterpret_i64<whitelist_validator>;

   }; // op_constrainers



   template <typename ... Visitors>
   struct constraints_validators {
      static void validate( IR::Module& m ) {
         for ( auto validator : { Visitors::validate... } )
            validator( m );
      }
   };
 
   // inherit from this class and define your own validators 
   class wasm_binary_validation {
      using standard_module_constraints_validators = constraints_validators< memories_validation_visitor,
                                                                      data_segments_validation_visitor,
                                                                      tables_validation_visitor,
                                                                      globals_validation_visitor>;
      public:
         static void validate( IR::Module& mod ) {
            _module_validators.validate( mod );
            for ( const auto& fd : mod.functions.defs ) {
               wasm_ops::EOSIO_OperatorDecoderStream<op_constrainers> decoder(fd.code);
               while ( decoder ) {
                  decoder.decodeOp()->visit();
               }
            }
         }
      private:
         static constraints_validators<standard_module_constraints_validators> _module_validators;
   };

}}} // namespace wasm_constraints, chain, eosio
