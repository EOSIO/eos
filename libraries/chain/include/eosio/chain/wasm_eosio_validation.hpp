#pragma once

#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/controller.hpp>
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
   struct allowlist_validator {
      static constexpr bool kills = true;
      static constexpr bool post = false;
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
         // just pass
      }
   };
   
   template <typename T>
   struct large_offset_validator {
      static constexpr bool kills = true;
      static constexpr bool post = false;
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
         // cast to a type that has a memarg field
         T* memarg_instr = reinterpret_cast<T*>(inst);
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

   struct blocklist_validator {
      static constexpr bool kills = true;
      static constexpr bool post = false;
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
         FC_THROW_EXCEPTION(wasm_execution_error, "Error, blocklisted opcode ${op} ", 
            ("op", inst->to_string()));
      }
   };
   
   struct nested_validator {
      static constexpr bool kills = false;
      static constexpr bool post = false;
      static bool disabled;
      static uint16_t depth;
      static void init(bool disable) { disabled = disable; depth = 0; }
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
         if (!disabled) {
            if ( inst->get_code() == wasm_ops::end_code && depth > 0 ) {
               depth--;
               return;
            }
            depth++;
            EOS_ASSERT(depth < 1024, wasm_execution_error, "Nested depth exceeded");
         }
      }
   };

   // add opcode specific constraints here
   // so far we only block list
   struct op_constrainers : wasm_ops::op_types<blocklist_validator> {
      using block_t           = wasm_ops::block                   <allowlist_validator, nested_validator>;
      using loop_t            = wasm_ops::loop                    <allowlist_validator, nested_validator>;
      using if__t             = wasm_ops::if_                     <allowlist_validator, nested_validator>;
      using else__t           = wasm_ops::else_                   <allowlist_validator, nested_validator>;
      
      using end_t             = wasm_ops::end                     <allowlist_validator, nested_validator>;
      using unreachable_t     = wasm_ops::unreachable             <allowlist_validator>;
      using br_t              = wasm_ops::br                      <allowlist_validator>;
      using br_if_t           = wasm_ops::br_if                   <allowlist_validator>;
      using br_table_t        = wasm_ops::br_table                <allowlist_validator>;
      using return__t         = wasm_ops::return_                 <allowlist_validator>;
      using call_t            = wasm_ops::call                    <allowlist_validator>;
      using call_indirect_t   = wasm_ops::call_indirect           <allowlist_validator>;
      using drop_t            = wasm_ops::drop                    <allowlist_validator>;
      using select_t          = wasm_ops::select                  <allowlist_validator>;

      using get_local_t       = wasm_ops::get_local               <allowlist_validator>;
      using set_local_t       = wasm_ops::set_local               <allowlist_validator>;
      using tee_local_t       = wasm_ops::tee_local               <allowlist_validator>;
      using get_global_t      = wasm_ops::get_global              <allowlist_validator>;
      using set_global_t      = wasm_ops::set_global              <allowlist_validator>;

      using grow_memory_t     = wasm_ops::grow_memory             <allowlist_validator>;
      using current_memory_t  = wasm_ops::current_memory          <allowlist_validator>;

      using nop_t             = wasm_ops::nop                     <allowlist_validator>;
      using i32_load_t        = wasm_ops::i32_load                <large_offset_validator<wasm_ops::op_types<>::i32_load_t>, allowlist_validator>;
      using i64_load_t        = wasm_ops::i64_load                <large_offset_validator<wasm_ops::op_types<>::i64_load_t>, allowlist_validator>;
      using f32_load_t        = wasm_ops::f32_load                <large_offset_validator<wasm_ops::op_types<>::f32_load_t>, allowlist_validator>;
      using f64_load_t        = wasm_ops::f64_load                <large_offset_validator<wasm_ops::op_types<>::f64_load_t>, allowlist_validator>;
      using i32_load8_s_t     = wasm_ops::i32_load8_s             <large_offset_validator<wasm_ops::op_types<>::i32_load8_s_t>, allowlist_validator>;
      using i32_load8_u_t     = wasm_ops::i32_load8_u             <large_offset_validator<wasm_ops::op_types<>::i32_load8_u_t>, allowlist_validator>;
      using i32_load16_s_t    = wasm_ops::i32_load16_s            <large_offset_validator<wasm_ops::op_types<>::i32_load16_s_t>, allowlist_validator>;
      using i32_load16_u_t    = wasm_ops::i32_load16_u            <large_offset_validator<wasm_ops::op_types<>::i32_load16_u_t>, allowlist_validator>;
      using i64_load8_s_t     = wasm_ops::i64_load8_s             <large_offset_validator<wasm_ops::op_types<>::i64_load8_s_t>, allowlist_validator>;
      using i64_load8_u_t     = wasm_ops::i64_load8_u             <large_offset_validator<wasm_ops::op_types<>::i64_load8_u_t>, allowlist_validator>;
      using i64_load16_s_t    = wasm_ops::i64_load16_s            <large_offset_validator<wasm_ops::op_types<>::i64_load16_s_t>, allowlist_validator>;
      using i64_load16_u_t    = wasm_ops::i64_load16_u            <large_offset_validator<wasm_ops::op_types<>::i64_load16_u_t>, allowlist_validator>;
      using i64_load32_s_t    = wasm_ops::i64_load32_s            <large_offset_validator<wasm_ops::op_types<>::i64_load32_s_t>, allowlist_validator>;
      using i64_load32_u_t    = wasm_ops::i64_load32_u            <large_offset_validator<wasm_ops::op_types<>::i64_load32_u_t>, allowlist_validator>;
      using i32_store_t       = wasm_ops::i32_store               <large_offset_validator<wasm_ops::op_types<>::i32_store_t>, allowlist_validator>;
      using i64_store_t       = wasm_ops::i64_store               <large_offset_validator<wasm_ops::op_types<>::i64_store_t>, allowlist_validator>;
      using f32_store_t       = wasm_ops::f32_store               <large_offset_validator<wasm_ops::op_types<>::f32_store_t>, allowlist_validator>;
      using f64_store_t       = wasm_ops::f64_store               <large_offset_validator<wasm_ops::op_types<>::f64_store_t>, allowlist_validator>;
      using i32_store8_t      = wasm_ops::i32_store8              <large_offset_validator<wasm_ops::op_types<>::i32_store8_t>, allowlist_validator>;
      using i32_store16_t     = wasm_ops::i32_store16             <large_offset_validator<wasm_ops::op_types<>::i32_store16_t>, allowlist_validator>;
      using i64_store8_t      = wasm_ops::i64_store8              <large_offset_validator<wasm_ops::op_types<>::i64_store8_t>, allowlist_validator>;
      using i64_store16_t     = wasm_ops::i64_store16             <large_offset_validator<wasm_ops::op_types<>::i64_store16_t>, allowlist_validator>;
      using i64_store32_t     = wasm_ops::i64_store32             <large_offset_validator<wasm_ops::op_types<>::i64_store32_t>, allowlist_validator>;

      using i32_const_t       = wasm_ops::i32_const               <allowlist_validator>;
      using i64_const_t       = wasm_ops::i64_const               <allowlist_validator>;
      using f32_const_t       = wasm_ops::f32_const               <allowlist_validator>;
      using f64_const_t       = wasm_ops::f64_const               <allowlist_validator>;
      
      using i32_eqz_t         = wasm_ops::i32_eqz                 <allowlist_validator>;
      using i32_eq_t          = wasm_ops::i32_eq                  <allowlist_validator>;
      using i32_ne_t          = wasm_ops::i32_ne                  <allowlist_validator>;
      using i32_lt_s_t        = wasm_ops::i32_lt_s                <allowlist_validator>;
      using i32_lt_u_t        = wasm_ops::i32_lt_u                <allowlist_validator>;
      using i32_gt_s_t        = wasm_ops::i32_gt_s                <allowlist_validator>;
      using i32_gt_u_t        = wasm_ops::i32_gt_u                <allowlist_validator>;
      using i32_le_s_t        = wasm_ops::i32_le_s                <allowlist_validator>;
      using i32_le_u_t        = wasm_ops::i32_le_u                <allowlist_validator>;
      using i32_ge_s_t        = wasm_ops::i32_ge_s                <allowlist_validator>;
      using i32_ge_u_t        = wasm_ops::i32_ge_u                <allowlist_validator>;

      using i32_clz_t         = wasm_ops::i32_clz                 <allowlist_validator>;
      using i32_ctz_t         = wasm_ops::i32_ctz                 <allowlist_validator>;
      using i32_popcnt_t      = wasm_ops::i32_popcnt              <allowlist_validator>;

      using i32_add_t         = wasm_ops::i32_add                 <allowlist_validator>; 
      using i32_sub_t         = wasm_ops::i32_sub                 <allowlist_validator>; 
      using i32_mul_t         = wasm_ops::i32_mul                 <allowlist_validator>; 
      using i32_div_s_t       = wasm_ops::i32_div_s               <allowlist_validator>; 
      using i32_div_u_t       = wasm_ops::i32_div_u               <allowlist_validator>; 
      using i32_rem_s_t       = wasm_ops::i32_rem_s               <allowlist_validator>; 
      using i32_rem_u_t       = wasm_ops::i32_rem_u               <allowlist_validator>; 
      using i32_and_t         = wasm_ops::i32_and                 <allowlist_validator>; 
      using i32_or_t          = wasm_ops::i32_or                  <allowlist_validator>; 
      using i32_xor_t         = wasm_ops::i32_xor                 <allowlist_validator>; 
      using i32_shl_t         = wasm_ops::i32_shl                 <allowlist_validator>; 
      using i32_shr_s_t       = wasm_ops::i32_shr_s               <allowlist_validator>; 
      using i32_shr_u_t       = wasm_ops::i32_shr_u               <allowlist_validator>; 
      using i32_rotl_t        = wasm_ops::i32_rotl                <allowlist_validator>; 
      using i32_rotr_t        = wasm_ops::i32_rotr                <allowlist_validator>; 

      using i64_eqz_t         = wasm_ops::i64_eqz                 <allowlist_validator>;
      using i64_eq_t          = wasm_ops::i64_eq                  <allowlist_validator>;
      using i64_ne_t          = wasm_ops::i64_ne                  <allowlist_validator>;
      using i64_lt_s_t        = wasm_ops::i64_lt_s                <allowlist_validator>;
      using i64_lt_u_t        = wasm_ops::i64_lt_u                <allowlist_validator>;
      using i64_gt_s_t        = wasm_ops::i64_gt_s                <allowlist_validator>;
      using i64_gt_u_t        = wasm_ops::i64_gt_u                <allowlist_validator>;
      using i64_le_s_t        = wasm_ops::i64_le_s                <allowlist_validator>;
      using i64_le_u_t        = wasm_ops::i64_le_u                <allowlist_validator>;
      using i64_ge_s_t        = wasm_ops::i64_ge_s                <allowlist_validator>;
      using i64_ge_u_t        = wasm_ops::i64_ge_u                <allowlist_validator>;

      using i64_clz_t         = wasm_ops::i64_clz                 <allowlist_validator>;
      using i64_ctz_t         = wasm_ops::i64_ctz                 <allowlist_validator>;
      using i64_popcnt_t      = wasm_ops::i64_popcnt              <allowlist_validator>;

      using i64_add_t         = wasm_ops::i64_add                 <allowlist_validator>; 
      using i64_sub_t         = wasm_ops::i64_sub                 <allowlist_validator>; 
      using i64_mul_t         = wasm_ops::i64_mul                 <allowlist_validator>; 
      using i64_div_s_t       = wasm_ops::i64_div_s               <allowlist_validator>; 
      using i64_div_u_t       = wasm_ops::i64_div_u               <allowlist_validator>; 
      using i64_rem_s_t       = wasm_ops::i64_rem_s               <allowlist_validator>; 
      using i64_rem_u_t       = wasm_ops::i64_rem_u               <allowlist_validator>; 
      using i64_and_t         = wasm_ops::i64_and                 <allowlist_validator>; 
      using i64_or_t          = wasm_ops::i64_or                  <allowlist_validator>; 
      using i64_xor_t         = wasm_ops::i64_xor                 <allowlist_validator>; 
      using i64_shl_t         = wasm_ops::i64_shl                 <allowlist_validator>; 
      using i64_shr_s_t       = wasm_ops::i64_shr_s               <allowlist_validator>; 
      using i64_shr_u_t       = wasm_ops::i64_shr_u               <allowlist_validator>; 
      using i64_rotl_t        = wasm_ops::i64_rotl                <allowlist_validator>; 
      using i64_rotr_t        = wasm_ops::i64_rotr                <allowlist_validator>; 

      using f32_eq_t                = wasm_ops::f32_eq            <allowlist_validator>;
      using f32_ne_t                = wasm_ops::f32_ne            <allowlist_validator>;
      using f32_lt_t                = wasm_ops::f32_lt            <allowlist_validator>;
      using f32_gt_t                = wasm_ops::f32_gt            <allowlist_validator>;
      using f32_le_t                = wasm_ops::f32_le            <allowlist_validator>;
      using f32_ge_t                = wasm_ops::f32_ge            <allowlist_validator>;
      using f64_eq_t                = wasm_ops::f64_eq            <allowlist_validator>;
      using f64_ne_t                = wasm_ops::f64_ne            <allowlist_validator>;
      using f64_lt_t                = wasm_ops::f64_lt            <allowlist_validator>;
      using f64_gt_t                = wasm_ops::f64_gt            <allowlist_validator>;
      using f64_le_t                = wasm_ops::f64_le            <allowlist_validator>;
      using f64_ge_t                = wasm_ops::f64_ge            <allowlist_validator>;
      using f32_abs_t               = wasm_ops::f32_abs           <allowlist_validator>;
      using f32_neg_t               = wasm_ops::f32_neg           <allowlist_validator>;
      using f32_ceil_t              = wasm_ops::f32_ceil          <allowlist_validator>;
      using f32_floor_t             = wasm_ops::f32_floor         <allowlist_validator>;
      using f32_trunc_t             = wasm_ops::f32_trunc         <allowlist_validator>;
      using f32_nearest_t           = wasm_ops::f32_nearest       <allowlist_validator>;
      using f32_sqrt_t              = wasm_ops::f32_sqrt          <allowlist_validator>;
      using f32_add_t               = wasm_ops::f32_add           <allowlist_validator>;
      using f32_sub_t               = wasm_ops::f32_sub           <allowlist_validator>;
      using f32_mul_t               = wasm_ops::f32_mul           <allowlist_validator>;
      using f32_div_t               = wasm_ops::f32_div           <allowlist_validator>;
      using f32_min_t               = wasm_ops::f32_min           <allowlist_validator>;
      using f32_max_t               = wasm_ops::f32_max           <allowlist_validator>;
      using f32_copysign_t          = wasm_ops::f32_copysign      <allowlist_validator>;
      using f64_abs_t               = wasm_ops::f64_abs           <allowlist_validator>;
      using f64_neg_t               = wasm_ops::f64_neg           <allowlist_validator>;
      using f64_ceil_t              = wasm_ops::f64_ceil          <allowlist_validator>;
      using f64_floor_t             = wasm_ops::f64_floor         <allowlist_validator>;
      using f64_trunc_t             = wasm_ops::f64_trunc         <allowlist_validator>;
      using f64_nearest_t           = wasm_ops::f64_nearest       <allowlist_validator>;
      using f64_sqrt_t              = wasm_ops::f64_sqrt          <allowlist_validator>;
      using f64_add_t               = wasm_ops::f64_add           <allowlist_validator>;
      using f64_sub_t               = wasm_ops::f64_sub           <allowlist_validator>;
      using f64_mul_t               = wasm_ops::f64_mul           <allowlist_validator>;
      using f64_div_t               = wasm_ops::f64_div           <allowlist_validator>;
      using f64_min_t               = wasm_ops::f64_min           <allowlist_validator>;
      using f64_max_t               = wasm_ops::f64_max           <allowlist_validator>;
      using f64_copysign_t          = wasm_ops::f64_copysign      <allowlist_validator>;

      using i32_trunc_s_f32_t       = wasm_ops::i32_trunc_s_f32   <allowlist_validator>;
      using i32_trunc_u_f32_t       = wasm_ops::i32_trunc_u_f32   <allowlist_validator>;
      using i32_trunc_s_f64_t       = wasm_ops::i32_trunc_s_f64   <allowlist_validator>;
      using i32_trunc_u_f64_t       = wasm_ops::i32_trunc_u_f64   <allowlist_validator>;
      using i64_trunc_s_f32_t       = wasm_ops::i64_trunc_s_f32   <allowlist_validator>;
      using i64_trunc_u_f32_t       = wasm_ops::i64_trunc_u_f32   <allowlist_validator>;
      using i64_trunc_s_f64_t       = wasm_ops::i64_trunc_s_f64   <allowlist_validator>;
      using i64_trunc_u_f64_t       = wasm_ops::i64_trunc_u_f64   <allowlist_validator>;
      using f32_convert_s_i32_t     = wasm_ops::f32_convert_s_i32 <allowlist_validator>;
      using f32_convert_u_i32_t     = wasm_ops::f32_convert_u_i32 <allowlist_validator>;
      using f32_convert_s_i64_t     = wasm_ops::f32_convert_s_i64 <allowlist_validator>;
      using f32_convert_u_i64_t     = wasm_ops::f32_convert_u_i64 <allowlist_validator>;
      using f32_demote_f64_t        = wasm_ops::f32_demote_f64    <allowlist_validator>;
      using f64_convert_s_i32_t     = wasm_ops::f64_convert_s_i32 <allowlist_validator>;
      using f64_convert_u_i32_t     = wasm_ops::f64_convert_u_i32 <allowlist_validator>;
      using f64_convert_s_i64_t     = wasm_ops::f64_convert_s_i64 <allowlist_validator>;
      using f64_convert_u_i64_t     = wasm_ops::f64_convert_u_i64 <allowlist_validator>;
      using f64_promote_f32_t       = wasm_ops::f64_promote_f32   <allowlist_validator>;

      using i32_wrap_i64_t    = wasm_ops::i32_wrap_i64            <allowlist_validator>;
      using i64_extend_s_i32_t = wasm_ops::i64_extend_s_i32       <allowlist_validator>;
      using i64_extend_u_i32_t = wasm_ops::i64_extend_u_i32       <allowlist_validator>;
      // TODO, make sure these are just pointer reinterprets
      using i32_reinterpret_f32_t = wasm_ops::i32_reinterpret_f32 <allowlist_validator>;
      using f32_reinterpret_i32_t = wasm_ops::f32_reinterpret_i32 <allowlist_validator>;
      using i64_reinterpret_f64_t = wasm_ops::i64_reinterpret_f64 <allowlist_validator>;
      using f64_reinterpret_i64_t = wasm_ops::f64_reinterpret_i64 <allowlist_validator>;

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
         wasm_binary_validation( const eosio::chain::controller& control, IR::Module& mod ) : _module( &mod ) {
            // initialize validators here
            nested_validator::init(!control.is_producing_block());
         }

         void validate() {
            _module_validators.validate( *_module );
            for ( auto& fd : _module->functions.defs ) {
               wasm_ops::EOSIO_OperatorDecoderStream<op_constrainers> decoder(fd.code);
               while ( decoder ) {
                  wasm_ops::instruction_stream new_code(0);
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
