#pragma once

#include <eosio/chain/wasm_eosio_binary_ops.hpp>
#include <fc/exception/exception.hpp>
#include <functional>
#include <vector>
#include <iostream>
#include "IR/Module.h"
#include "IR/Operators.h"
#include "WASM/WASM.h"

namespace eosio { namespace chain { namespace wasm_injections {

   // module validators
   // effectively do nothing and pass
   struct noop_injection_visitor {
      static void inject( IR::Module& m );
      static void initializer();
   };

   struct memories_injection_visitor {
      static void inject( IR::Module& m );
      static void initializer();
   };

   struct data_segments_injection_visitor {
      static void inject( IR::Module& m );
      static void initializer();
   };

   struct tables_injection_visitor {
      static void inject( IR::Module& m );
      static void initializer();
   };

   struct globals_injection_visitor {
      static void inject( IR::Module& m );
      static void initializer();
   };

   struct blacklist_injection_visitor {
      static void inject( IR::Module& m );
      static void initializer();
   };

   using wasm_validate_func = std::function<void(IR::Module&)>;

  
   // just pass 
   struct no_injections_injectors {
      static void inject( IR::Module& m ) {}
   };

   // simple mutator that doesn't actually mutate anything
   // used to verify that a given instruction is valid for execution on our platform
   struct pass_injector {
      static void accept( wasm_ops::instr* inst, uint32_t index, IR::Module& mod ) {
         // just pass
      }
   };
   
   //struct  
   struct instruction_counter {
      static void accept( wasm_ops::instr* inst, uint32_t index, IR::Module& mod ) {
         // maybe change this later to variable weighting for different instruction types
         std::cout << "IC " << icnt << " " << inst->to_string() <<  "\n";
         icnt++;
      }
      static uint32_t icnt;
   };

   struct checktime_injector {
      static void accept( wasm_ops::instr* inst, uint32_t index, IR::Module& mod ) {
//         if (checktime_idx == -1)
//            FC_THROW("Error, this should be run after module injector");
         //std::cout << "HIT A BLOCK " <<  " " << index << "\n";
         wasm_ops::op_types<>::i32_const_t cnt; 
         cnt.field = instruction_counter::icnt;
         wasm_ops::op_types<>::call_t chktm; 
         chktm.field = checktime_idx;
         instruction_counter::icnt = 0;
      }
      static int32_t checktime_idx;
   };

   // add opcode specific constraints here
   // so far we only black list
   struct op_injectors : wasm_ops::op_types<pass_injector> {
      using block_t           = wasm_ops::block<instruction_counter, checktime_injector>;
      using loop_t            = wasm_ops::loop<instruction_counter, checktime_injector>;
      using if__t             = wasm_ops::if_<instruction_counter, checktime_injector>;
      using else__t           = wasm_ops::else_<instruction_counter, checktime_injector>;
      
      using end_t             = wasm_ops::end<instruction_counter>;
      using unreachable_t     = wasm_ops::unreachable<instruction_counter>;
      using br_t              = wasm_ops::br<instruction_counter>;
      using br_if_t           = wasm_ops::br_if<instruction_counter>;
      using br_table_t        = wasm_ops::br_table<instruction_counter>;
      using return__t         = wasm_ops::return_<instruction_counter>;
      using call_t            = wasm_ops::call<instruction_counter>;
      using call_indirect_t   = wasm_ops::call_indirect<instruction_counter>;
      using drop_t            = wasm_ops::drop<instruction_counter>;
      using select_t          = wasm_ops::select<instruction_counter>;

      using get_local_t       = wasm_ops::get_local<instruction_counter>;
      using set_local_t       = wasm_ops::set_local<instruction_counter>;
      using tee_local_t       = wasm_ops::tee_local<instruction_counter>;
      using get_global_t      = wasm_ops::get_global<instruction_counter>;
      using set_global_t      = wasm_ops::set_global<instruction_counter>;

      using nop_t             = wasm_ops::nop<instruction_counter>;
      using i32_load_t        = wasm_ops::i32_load<instruction_counter>;
      using i64_load_t        = wasm_ops::i64_load<instruction_counter>;
      using f32_load_t        = wasm_ops::f32_load<instruction_counter>;
      using f64_load_t        = wasm_ops::f64_load<instruction_counter>;
      using i32_load8_s_t     = wasm_ops::i32_load8_s<instruction_counter>;
      using i32_load8_u_t     = wasm_ops::i32_load8_u<instruction_counter>;
      using i32_load16_s_t    = wasm_ops::i32_load16_s<instruction_counter>;
      using i32_load16_u_t    = wasm_ops::i32_load16_u<instruction_counter>;
      using i64_load8_s_t     = wasm_ops::i64_load8_s<instruction_counter>;
      using i64_load8_u_t     = wasm_ops::i64_load8_u<instruction_counter>;
      using i64_load16_s_t    = wasm_ops::i64_load16_s<instruction_counter>;
      using i64_load16_u_t    = wasm_ops::i64_load16_u<instruction_counter>;
      using i64_load32_s_t    = wasm_ops::i64_load32_s<instruction_counter>;
      using i64_load32_u_t    = wasm_ops::i64_load32_u<instruction_counter>;
      using i32_store_t       = wasm_ops::i32_store<instruction_counter>;
      using i64_store_t       = wasm_ops::i64_store<instruction_counter>;
      using f32_store_t       = wasm_ops::f32_store<instruction_counter>;
      using f64_store_t       = wasm_ops::f64_store<instruction_counter>;
      using i32_store8_t      = wasm_ops::i32_store8<instruction_counter>;
      using i32_store16_t     = wasm_ops::i32_store16<instruction_counter>;
      using i64_store8_t      = wasm_ops::i64_store8<instruction_counter>;
      using i64_store16_t     = wasm_ops::i64_store16<instruction_counter>;
      using i64_store32_t     = wasm_ops::i64_store32<instruction_counter>;

      using i32_const_t       = wasm_ops::i32_const<instruction_counter>;
      using i64_const_t       = wasm_ops::i64_const<instruction_counter>;
      using f32_const_t       = wasm_ops::f32_const<instruction_counter>;
      using f64_const_t       = wasm_ops::f64_const<instruction_counter>;
      
      using i32_eqz_t         = wasm_ops::i32_eqz<instruction_counter>;
      using i32_eq_t          = wasm_ops::i32_eq<instruction_counter>;
      using i32_ne_t          = wasm_ops::i32_ne<instruction_counter>;
      using i32_lt_s_t        = wasm_ops::i32_lt_s<instruction_counter>;
      using i32_lt_u_t        = wasm_ops::i32_lt_u<instruction_counter>;
      using i32_gt_s_t        = wasm_ops::i32_gt_s<instruction_counter>;
      using i32_gt_u_t        = wasm_ops::i32_gt_u<instruction_counter>;
      using i32_le_s_t        = wasm_ops::i32_le_s<instruction_counter>;
      using i32_le_u_t        = wasm_ops::i32_le_u<instruction_counter>;
      using i32_ge_s_t        = wasm_ops::i32_ge_s<instruction_counter>;
      using i32_ge_u_t        = wasm_ops::i32_ge_u<instruction_counter>;

      using i32_clz_t         = wasm_ops::i32_clz<instruction_counter>;
      using i32_ctz_t         = wasm_ops::i32_ctz<instruction_counter>;
      using i32_popcnt_t      = wasm_ops::i32_popcnt<instruction_counter>;

      using i32_add_t         = wasm_ops::i32_add<instruction_counter>; 
      using i32_sub_t         = wasm_ops::i32_sub<instruction_counter>; 
      using i32_mul_t         = wasm_ops::i32_mul<instruction_counter>; 
      using i32_div_s_t       = wasm_ops::i32_div_s<instruction_counter>; 
      using i32_div_u_t       = wasm_ops::i32_div_u<instruction_counter>; 
      using i32_rem_s_t       = wasm_ops::i32_rem_s<instruction_counter>; 
      using i32_rem_u_t       = wasm_ops::i32_rem_u<instruction_counter>; 
      using i32_and_t         = wasm_ops::i32_and<instruction_counter>; 
      using i32_or_t          = wasm_ops::i32_or<instruction_counter>; 
      using i32_xor_t         = wasm_ops::i32_xor<instruction_counter>; 
      using i32_shl_t         = wasm_ops::i32_shl<instruction_counter>; 
      using i32_shr_s_t       = wasm_ops::i32_shr_s<instruction_counter>; 
      using i32_shr_u_t       = wasm_ops::i32_shr_u<instruction_counter>; 
      using i32_rotl_t        = wasm_ops::i32_rotl<instruction_counter>; 
      using i32_rotr_t        = wasm_ops::i32_rotr<instruction_counter>; 

      using i64_eqz_t         = wasm_ops::i64_eqz<instruction_counter>;
      using i64_eq_t          = wasm_ops::i64_eq<instruction_counter>;
      using i64_ne_t          = wasm_ops::i64_ne<instruction_counter>;
      using i64_lt_s_t        = wasm_ops::i64_lt_s<instruction_counter>;
      using i64_lt_u_t        = wasm_ops::i64_lt_u<instruction_counter>;
      using i64_gt_s_t        = wasm_ops::i64_gt_s<instruction_counter>;
      using i64_gt_u_t        = wasm_ops::i64_gt_u<instruction_counter>;
      using i64_le_s_t        = wasm_ops::i64_le_s<instruction_counter>;
      using i64_le_u_t        = wasm_ops::i64_le_u<instruction_counter>;
      using i64_ge_s_t        = wasm_ops::i64_ge_s<instruction_counter>;
      using i64_ge_u_t        = wasm_ops::i64_ge_u<instruction_counter>;

      using i64_clz_t         = wasm_ops::i64_clz<instruction_counter>;
      using i64_ctz_t         = wasm_ops::i64_ctz<instruction_counter>;
      using i64_popcnt_t      = wasm_ops::i64_popcnt<instruction_counter>;

      using i64_add_t         = wasm_ops::i64_add<instruction_counter>; 
      using i64_sub_t         = wasm_ops::i64_sub<instruction_counter>; 
      using i64_mul_t         = wasm_ops::i64_mul<instruction_counter>; 
      using i64_div_s_t       = wasm_ops::i64_div_s<instruction_counter>; 
      using i64_div_u_t       = wasm_ops::i64_div_u<instruction_counter>; 
      using i64_rem_s_t       = wasm_ops::i64_rem_s<instruction_counter>; 
      using i64_rem_u_t       = wasm_ops::i64_rem_u<instruction_counter>; 
      using i64_and_t         = wasm_ops::i64_and<instruction_counter>; 
      using i64_or_t          = wasm_ops::i64_or<instruction_counter>; 
      using i64_xor_t         = wasm_ops::i64_xor<instruction_counter>; 
      using i64_shl_t         = wasm_ops::i64_shl<instruction_counter>; 
      using i64_shr_s_t       = wasm_ops::i64_shr_s<instruction_counter>; 
      using i64_shr_u_t       = wasm_ops::i64_shr_u<instruction_counter>; 
      using i64_rotl_t        = wasm_ops::i64_rotl<instruction_counter>; 
      using i64_rotr_t        = wasm_ops::i64_rotr<instruction_counter>; 

      using i32_wrap_i64_t    = wasm_ops::i32_wrap_i64<instruction_counter>;
      using i64_extend_s_i32_t = wasm_ops::i64_extend_s_i32<instruction_counter>;
      using i64_extend_u_i32_t = wasm_ops::i64_extend_u_i32<instruction_counter>;
      // TODO, make sure these are just pointer reinterprets
      using i32_reinterpret_f32_t = wasm_ops::i32_reinterpret_f32<instruction_counter>;
      using f32_reinterpret_i32_t = wasm_ops::f32_reinterpret_i32<instruction_counter>;
      using i64_reinterpret_f64_t = wasm_ops::i64_reinterpret_f64<instruction_counter>;
      using f64_reinterpret_i64_t = wasm_ops::f64_reinterpret_i64<instruction_counter>;

   }; // op_injectors



   template <typename ... Visitors>
   struct module_injectors {
      static void inject( IR::Module& m ) {
         for ( auto injector : { Visitors::inject... } )
            injector( m );
      }
      static void init() {
         // place initial values for static fields of injectors here
         for ( auto initializer : { Visitors::initializer... } )
            initializer();
      }
   };
 
   // inherit from this class and define your own injectors 
   class wasm_binary_injection {
      using standard_module_injectors = module_injectors< noop_injection_visitor >;

      public:
         wasm_binary_injection() { 
            _module_injectors.init();
            // initialize static fields of injectors
            instruction_counter::icnt = 0;
            checktime_injector::checktime_idx = -1;
         }

         static void inject( IR::Module& mod ) {
            _module_injectors.inject( mod );
            for ( const auto& fd : mod.functions.defs ) {
               wasm_ops::EOSIO_OperatorDecoderStream<op_injectors> decoder(fd.code);
               while ( decoder )
                  decoder.decodeOp()->visit( decoder.index(), mod );
            }
         }
      private:
         static standard_module_injectors _module_injectors;
   };

}}} // namespace wasm_constraints, chain, eosio
