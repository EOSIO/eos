#pragma once

#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wasm_eosio_binary_ops.hpp>
#include <functional>
#include <vector>
#include <iostream>
#include "IR/Module.h"
#include "IR/Operators.h"
#include "WASM/WASM.h"

namespace eosio { namespace chain { namespace wasm_validations {

   // module validators
   // effectively do nothing and pass
   struct noop_validation_visitor {
      static void validate( const IR::Module& m );
   };

   struct memories_validation_visitor {
      static void validate( const IR::Module& m );
   };

   struct data_segments_validation_visitor {
      static void validate( const IR::Module& m );
   };

   struct tables_validation_visitor {
      static void validate( const IR::Module& m );
   };

   struct globals_validation_visitor {
      static void validate( const IR::Module& m );
   };

   struct maximum_function_stack_visitor {
      static void validate( const IR::Module& m );
   };

   struct ensure_apply_exported_visitor {
      static void validate( const IR::Module& m );
   };

   using wasm_validate_func = std::function<void(IR::Module&)>;

  
   // just pass 
   struct no_constraints_validators {
      static void validate( const IR::Module& m ) {}
   };

   // instruction validators
   // simple mutator that doesn't actually mutate anything
   // used to verify that a given instruction is valid for execution on our platform
   // for validators set kills to true, this eliminates the extraneous building
   // of new code that is going to get thrown away any way
   struct whitelist_validator {
      static constexpr bool kills = true;
      static constexpr bool post = false;
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
         // just pass
      }
   };

   struct large_offset_validator {
      static constexpr bool kills = true;
      static constexpr bool post = false;
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
         // cast to a type that has a memarg field
         wasm_ops::op_types<>::i32_load_t* memarg_instr = reinterpret_cast<wasm_ops::op_types<>::i32_load_t*>(inst);
         if(memarg_instr->field.o >= wasm_constraints::maximum_linear_memory)
            FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract used an invalid large memory store/load offset");
      }

   };

   struct debug_printer {
      static constexpr bool kills = false;
      static constexpr bool post = false;
      static void init() {}
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
         std::cout << "INSTRUCTION : " << inst->to_string() << "\n";
      }
   };


   struct wasm_opcode_no_disposition_exception {
      std::string opcode_name;
   };

   struct blacklist_validator {
      static constexpr bool kills = true;
      static constexpr bool post = false;
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
         FC_THROW_EXCEPTION(wasm_execution_error, "Error, blacklisted opcode ${op} ", 
            ("op", inst->to_string()));
      }
   };
   
   // add opcode specific constraints here
   // so far we only black list
   struct op_constrainers : wasm_ops::op_types<blacklist_validator> {
      using block_t           = wasm_ops::block                   <whitelist_validator>;
      using loop_t            = wasm_ops::loop                    <whitelist_validator>;
      using if__t             = wasm_ops::if_                     <whitelist_validator>;
      using else__t           = wasm_ops::else_                   <whitelist_validator>;
      
      using end_t             = wasm_ops::end                     <whitelist_validator>;
      using unreachable_t     = wasm_ops::unreachable             <whitelist_validator>;
      using br_t              = wasm_ops::br                      <whitelist_validator>;
      using br_if_t           = wasm_ops::br_if                   <whitelist_validator>;
      using br_table_t        = wasm_ops::br_table                <whitelist_validator>;
      using return__t         = wasm_ops::return_                 <whitelist_validator>;
      using call_t            = wasm_ops::call                    <whitelist_validator>;
      using call_indirect_t   = wasm_ops::call_indirect           <whitelist_validator>;
      using drop_t            = wasm_ops::drop                    <whitelist_validator>;
      using select_t          = wasm_ops::select                  <whitelist_validator>;

      using get_local_t       = wasm_ops::get_local               <whitelist_validator>;
      using set_local_t       = wasm_ops::set_local               <whitelist_validator>;
      using tee_local_t       = wasm_ops::tee_local               <whitelist_validator>;
      using get_global_t      = wasm_ops::get_global              <whitelist_validator>;
      using set_global_t      = wasm_ops::set_global              <whitelist_validator>;

      using grow_memory_t     = wasm_ops::grow_memory             <whitelist_validator>;
      using current_memory_t  = wasm_ops::current_memory          <whitelist_validator>;

      using nop_t             = wasm_ops::nop                     <whitelist_validator>;
      using i32_load_t        = wasm_ops::i32_load                <large_offset_validator, whitelist_validator>;
      using i64_load_t        = wasm_ops::i64_load                <large_offset_validator, whitelist_validator>;
      using f32_load_t        = wasm_ops::f32_load                <large_offset_validator, whitelist_validator>;
      using f64_load_t        = wasm_ops::f64_load                <large_offset_validator, whitelist_validator>;
      using i32_load8_s_t     = wasm_ops::i32_load8_s             <large_offset_validator, whitelist_validator>;
      using i32_load8_u_t     = wasm_ops::i32_load8_u             <large_offset_validator, whitelist_validator>;
      using i32_load16_s_t    = wasm_ops::i32_load16_s            <large_offset_validator, whitelist_validator>;
      using i32_load16_u_t    = wasm_ops::i32_load16_u            <large_offset_validator, whitelist_validator>;
      using i64_load8_s_t     = wasm_ops::i64_load8_s             <large_offset_validator, whitelist_validator>;
      using i64_load8_u_t     = wasm_ops::i64_load8_u             <large_offset_validator, whitelist_validator>;
      using i64_load16_s_t    = wasm_ops::i64_load16_s            <large_offset_validator, whitelist_validator>;
      using i64_load16_u_t    = wasm_ops::i64_load16_u            <large_offset_validator, whitelist_validator>;
      using i64_load32_s_t    = wasm_ops::i64_load32_s            <large_offset_validator, whitelist_validator>;
      using i64_load32_u_t    = wasm_ops::i64_load32_u            <large_offset_validator, whitelist_validator>;
      using i32_store_t       = wasm_ops::i32_store               <large_offset_validator, whitelist_validator>;
      using i64_store_t       = wasm_ops::i64_store               <large_offset_validator, whitelist_validator>;
      using f32_store_t       = wasm_ops::f32_store               <large_offset_validator, whitelist_validator>;
      using f64_store_t       = wasm_ops::f64_store               <large_offset_validator, whitelist_validator>;
      using i32_store8_t      = wasm_ops::i32_store8              <large_offset_validator, whitelist_validator>;
      using i32_store16_t     = wasm_ops::i32_store16             <large_offset_validator, whitelist_validator>;
      using i64_store8_t      = wasm_ops::i64_store8              <large_offset_validator, whitelist_validator>;
      using i64_store16_t     = wasm_ops::i64_store16             <large_offset_validator, whitelist_validator>;
      using i64_store32_t     = wasm_ops::i64_store32             <large_offset_validator, whitelist_validator>;

      using i32_const_t       = wasm_ops::i32_const               <whitelist_validator>;
      using i64_const_t       = wasm_ops::i64_const               <whitelist_validator>;
      using f32_const_t       = wasm_ops::f32_const               <whitelist_validator>;
      using f64_const_t       = wasm_ops::f64_const               <whitelist_validator>;
      
      using i32_eqz_t         = wasm_ops::i32_eqz                 <whitelist_validator>;
      using i32_eq_t          = wasm_ops::i32_eq                  <whitelist_validator>;
      using i32_ne_t          = wasm_ops::i32_ne                  <whitelist_validator>;
      using i32_lt_s_t        = wasm_ops::i32_lt_s                <whitelist_validator>;
      using i32_lt_u_t        = wasm_ops::i32_lt_u                <whitelist_validator>;
      using i32_gt_s_t        = wasm_ops::i32_gt_s                <whitelist_validator>;
      using i32_gt_u_t        = wasm_ops::i32_gt_u                <whitelist_validator>;
      using i32_le_s_t        = wasm_ops::i32_le_s                <whitelist_validator>;
      using i32_le_u_t        = wasm_ops::i32_le_u                <whitelist_validator>;
      using i32_ge_s_t        = wasm_ops::i32_ge_s                <whitelist_validator>;
      using i32_ge_u_t        = wasm_ops::i32_ge_u                <whitelist_validator>;

      using i32_clz_t         = wasm_ops::i32_clz                 <whitelist_validator>;
      using i32_ctz_t         = wasm_ops::i32_ctz                 <whitelist_validator>;
      using i32_popcnt_t      = wasm_ops::i32_popcnt              <whitelist_validator>;

      using i32_add_t         = wasm_ops::i32_add                 <whitelist_validator>; 
      using i32_sub_t         = wasm_ops::i32_sub                 <whitelist_validator>; 
      using i32_mul_t         = wasm_ops::i32_mul                 <whitelist_validator>; 
      using i32_div_s_t       = wasm_ops::i32_div_s               <whitelist_validator>; 
      using i32_div_u_t       = wasm_ops::i32_div_u               <whitelist_validator>; 
      using i32_rem_s_t       = wasm_ops::i32_rem_s               <whitelist_validator>; 
      using i32_rem_u_t       = wasm_ops::i32_rem_u               <whitelist_validator>; 
      using i32_and_t         = wasm_ops::i32_and                 <whitelist_validator>; 
      using i32_or_t          = wasm_ops::i32_or                  <whitelist_validator>; 
      using i32_xor_t         = wasm_ops::i32_xor                 <whitelist_validator>; 
      using i32_shl_t         = wasm_ops::i32_shl                 <whitelist_validator>; 
      using i32_shr_s_t       = wasm_ops::i32_shr_s               <whitelist_validator>; 
      using i32_shr_u_t       = wasm_ops::i32_shr_u               <whitelist_validator>; 
      using i32_rotl_t        = wasm_ops::i32_rotl                <whitelist_validator>; 
      using i32_rotr_t        = wasm_ops::i32_rotr                <whitelist_validator>; 

      using i64_eqz_t         = wasm_ops::i64_eqz                 <whitelist_validator>;
      using i64_eq_t          = wasm_ops::i64_eq                  <whitelist_validator>;
      using i64_ne_t          = wasm_ops::i64_ne                  <whitelist_validator>;
      using i64_lt_s_t        = wasm_ops::i64_lt_s                <whitelist_validator>;
      using i64_lt_u_t        = wasm_ops::i64_lt_u                <whitelist_validator>;
      using i64_gt_s_t        = wasm_ops::i64_gt_s                <whitelist_validator>;
      using i64_gt_u_t        = wasm_ops::i64_gt_u                <whitelist_validator>;
      using i64_le_s_t        = wasm_ops::i64_le_s                <whitelist_validator>;
      using i64_le_u_t        = wasm_ops::i64_le_u                <whitelist_validator>;
      using i64_ge_s_t        = wasm_ops::i64_ge_s                <whitelist_validator>;
      using i64_ge_u_t        = wasm_ops::i64_ge_u                <whitelist_validator>;

      using i64_clz_t         = wasm_ops::i64_clz                 <whitelist_validator>;
      using i64_ctz_t         = wasm_ops::i64_ctz                 <whitelist_validator>;
      using i64_popcnt_t      = wasm_ops::i64_popcnt              <whitelist_validator>;

      using i64_add_t         = wasm_ops::i64_add                 <whitelist_validator>; 
      using i64_sub_t         = wasm_ops::i64_sub                 <whitelist_validator>; 
      using i64_mul_t         = wasm_ops::i64_mul                 <whitelist_validator>; 
      using i64_div_s_t       = wasm_ops::i64_div_s               <whitelist_validator>; 
      using i64_div_u_t       = wasm_ops::i64_div_u               <whitelist_validator>; 
      using i64_rem_s_t       = wasm_ops::i64_rem_s               <whitelist_validator>; 
      using i64_rem_u_t       = wasm_ops::i64_rem_u               <whitelist_validator>; 
      using i64_and_t         = wasm_ops::i64_and                 <whitelist_validator>; 
      using i64_or_t          = wasm_ops::i64_or                  <whitelist_validator>; 
      using i64_xor_t         = wasm_ops::i64_xor                 <whitelist_validator>; 
      using i64_shl_t         = wasm_ops::i64_shl                 <whitelist_validator>; 
      using i64_shr_s_t       = wasm_ops::i64_shr_s               <whitelist_validator>; 
      using i64_shr_u_t       = wasm_ops::i64_shr_u               <whitelist_validator>; 
      using i64_rotl_t        = wasm_ops::i64_rotl                <whitelist_validator>; 
      using i64_rotr_t        = wasm_ops::i64_rotr                <whitelist_validator>; 

      using f32_eq_t                = wasm_ops::f32_eq            <whitelist_validator>;
      using f32_ne_t                = wasm_ops::f32_ne            <whitelist_validator>;
      using f32_lt_t                = wasm_ops::f32_lt            <whitelist_validator>;
      using f32_gt_t                = wasm_ops::f32_gt            <whitelist_validator>;
      using f32_le_t                = wasm_ops::f32_le            <whitelist_validator>;
      using f32_ge_t                = wasm_ops::f32_ge            <whitelist_validator>;
      using f64_eq_t                = wasm_ops::f64_eq            <whitelist_validator>;
      using f64_ne_t                = wasm_ops::f64_ne            <whitelist_validator>;
      using f64_lt_t                = wasm_ops::f64_lt            <whitelist_validator>;
      using f64_gt_t                = wasm_ops::f64_gt            <whitelist_validator>;
      using f64_le_t                = wasm_ops::f64_le            <whitelist_validator>;
      using f64_ge_t                = wasm_ops::f64_ge            <whitelist_validator>;
      using f32_abs_t               = wasm_ops::f32_abs           <whitelist_validator>;
      using f32_neg_t               = wasm_ops::f32_neg           <whitelist_validator>;
      using f32_ceil_t              = wasm_ops::f32_ceil          <whitelist_validator>;
      using f32_floor_t             = wasm_ops::f32_floor         <whitelist_validator>;
      using f32_trunc_t             = wasm_ops::f32_trunc         <whitelist_validator>;
      using f32_nearest_t           = wasm_ops::f32_nearest       <whitelist_validator>;
      using f32_sqrt_t              = wasm_ops::f32_sqrt          <whitelist_validator>;
      using f32_add_t               = wasm_ops::f32_add           <whitelist_validator>;
      using f32_sub_t               = wasm_ops::f32_sub           <whitelist_validator>;
      using f32_mul_t               = wasm_ops::f32_mul           <whitelist_validator>;
      using f32_div_t               = wasm_ops::f32_div           <whitelist_validator>;
      using f32_min_t               = wasm_ops::f32_min           <whitelist_validator>;
      using f32_max_t               = wasm_ops::f32_max           <whitelist_validator>;
      using f32_copysign_t          = wasm_ops::f32_copysign      <whitelist_validator>;
      using f64_abs_t               = wasm_ops::f64_abs           <whitelist_validator>;
      using f64_neg_t               = wasm_ops::f64_neg           <whitelist_validator>;
      using f64_ceil_t              = wasm_ops::f64_ceil          <whitelist_validator>;
      using f64_floor_t             = wasm_ops::f64_floor         <whitelist_validator>;
      using f64_trunc_t             = wasm_ops::f64_trunc         <whitelist_validator>;
      using f64_nearest_t           = wasm_ops::f64_nearest       <whitelist_validator>;
      using f64_sqrt_t              = wasm_ops::f64_sqrt          <whitelist_validator>;
      using f64_add_t               = wasm_ops::f64_add           <whitelist_validator>;
      using f64_sub_t               = wasm_ops::f64_sub           <whitelist_validator>;
      using f64_mul_t               = wasm_ops::f64_mul           <whitelist_validator>;
      using f64_div_t               = wasm_ops::f64_div           <whitelist_validator>;
      using f64_min_t               = wasm_ops::f64_min           <whitelist_validator>;
      using f64_max_t               = wasm_ops::f64_max           <whitelist_validator>;
      using f64_copysign_t          = wasm_ops::f64_copysign      <whitelist_validator>;

      using i32_trunc_s_f32_t       = wasm_ops::i32_trunc_s_f32   <whitelist_validator>;
      using i32_trunc_u_f32_t       = wasm_ops::i32_trunc_u_f32   <whitelist_validator>;
      using i32_trunc_s_f64_t       = wasm_ops::i32_trunc_s_f64   <whitelist_validator>;
      using i32_trunc_u_f64_t       = wasm_ops::i32_trunc_u_f64   <whitelist_validator>;
      using i64_trunc_s_f32_t       = wasm_ops::i64_trunc_s_f32   <whitelist_validator>;
      using i64_trunc_u_f32_t       = wasm_ops::i64_trunc_u_f32   <whitelist_validator>;
      using i64_trunc_s_f64_t       = wasm_ops::i64_trunc_s_f64   <whitelist_validator>;
      using i64_trunc_u_f64_t       = wasm_ops::i64_trunc_u_f64   <whitelist_validator>;
      using f32_convert_s_i32_t     = wasm_ops::f32_convert_s_i32 <whitelist_validator>;
      using f32_convert_u_i32_t     = wasm_ops::f32_convert_u_i32 <whitelist_validator>;
      using f32_convert_s_i64_t     = wasm_ops::f32_convert_s_i64 <whitelist_validator>;
      using f32_convert_u_i64_t     = wasm_ops::f32_convert_u_i64 <whitelist_validator>;
      using f32_demote_f64_t        = wasm_ops::f32_demote_f64    <whitelist_validator>;
      using f64_convert_s_i32_t     = wasm_ops::f64_convert_s_i32 <whitelist_validator>;
      using f64_convert_u_i32_t     = wasm_ops::f64_convert_u_i32 <whitelist_validator>;
      using f64_convert_s_i64_t     = wasm_ops::f64_convert_s_i64 <whitelist_validator>;
      using f64_convert_u_i64_t     = wasm_ops::f64_convert_u_i64 <whitelist_validator>;
      using f64_promote_f32_t       = wasm_ops::f64_promote_f32   <whitelist_validator>;

      using i32_wrap_i64_t    = wasm_ops::i32_wrap_i64            <whitelist_validator>;
      using i64_extend_s_i32_t = wasm_ops::i64_extend_s_i32       <whitelist_validator>;
      using i64_extend_u_i32_t = wasm_ops::i64_extend_u_i32       <whitelist_validator>;
      // TODO, make sure these are just pointer reinterprets
      using i32_reinterpret_f32_t = wasm_ops::i32_reinterpret_f32 <whitelist_validator>;
      using f32_reinterpret_i32_t = wasm_ops::f32_reinterpret_i32 <whitelist_validator>;
      using i64_reinterpret_f64_t = wasm_ops::i64_reinterpret_f64 <whitelist_validator>;
      using f64_reinterpret_i64_t = wasm_ops::f64_reinterpret_i64 <whitelist_validator>;

   }; // op_constrainers



   template <typename ... Visitors>
   struct constraints_validators {
      static void validate( const IR::Module& m ) {
         for ( auto validator : { Visitors::validate... } )
            validator( m );
      }
   };
 
   // inherit from this class and define your own validators 
   class wasm_binary_validation {
      using standard_module_constraints_validators = constraints_validators< memories_validation_visitor,
                                                                             data_segments_validation_visitor,
                                                                             tables_validation_visitor,
                                                                             globals_validation_visitor,
                                                                             maximum_function_stack_visitor,
                                                                             ensure_apply_exported_visitor>;
      public:
         wasm_binary_validation( IR::Module& mod ) : _module( &mod ) {
            // initialize validators here
         }

         void validate() {
            _module_validators.validate( *_module );
            for ( auto& fd : _module->functions.defs ) {
               wasm_ops::EOSIO_OperatorDecoderStream<op_constrainers> decoder(fd.code);
               while ( decoder ) {
                  std::vector<U8> new_code;
                  auto op = decoder.decodeOp();
                  op->visit( { _module, &new_code, &fd, decoder.index() } );
               }
            }
         }
      private:
         IR::Module* _module;
         static standard_module_constraints_validators _module_validators;
   };

}}} // namespace wasm_constraints, chain, eosio
