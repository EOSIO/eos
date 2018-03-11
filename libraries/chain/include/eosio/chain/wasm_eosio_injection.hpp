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
   /*
   static U32 checktimeIndex(const Module& module)
   {
      return module.functions.imports.size() - 1;
   }

   void setTypeSlot(const Module& module, ResultType returnType, const std::vector<ValueType>& parameterTypes)
   {
      if (returnType == ResultType::none && parameterTypes.size() == 1 && parameterTypes[0] == ValueType::i32 )
        typeSlot = module.types.size() - 1;
   }

   void addTypeSlot(Module& module)
   {
      if (typeSlot < 0)
      {
         // add a type for void func(void)
         typeSlot = module.types.size();
         module.types.push_back(FunctionType::get(ResultType::none, std::vector<ValueType>(1, ValueType::i32)));
      }
   }

   void addImport(Module& module)
   {
      if (module.functions.imports.size() == 0 || module.functions.imports.back().exportName.compare(u8"checktime") != 0) {
         if (typeSlot < 0) {
            addTypeSlot(module);
         }

         const U32 functionTypeIndex = typeSlot;
         module.functions.imports.push_back({{functionTypeIndex}, std::move(u8"env"), std::move(u8"checktime")});
      }
   }

   void conditionallyAddCall(Opcode opcode, const ControlStructureImm& imm, Module& module, OperatorEncoderStream& operatorEncoderStream, CodeValidationStream& codeValidationStream)
   {
      switch(opcode)
      {
      case Opcode::loop:
      case Opcode::block:
         addCall(module, operatorEncoderStream, codeValidationStream);
      default:
         break;
      };
   }

   template<typename Imm>
   void conditionallyAddCall(Opcode , const Imm& , Module& , OperatorEncoderStream& , CodeValidationStream& )
   {
   }

   void adjustIfFunctionIndex(Uptr& index, ObjectKind kind)
   {
      if (kind == ObjectKind::function)
         ++index;
   }

   void adjustExportIndex(Module& module)
   {
      // all function exports need to have their index increased to account for inserted definition
      for (auto& exportDef : module.exports)
      {
         adjustIfFunctionIndex(exportDef.index, exportDef.kind);
      }
   }

   void adjustCallIndex(const Module& module, CallImm& imm)
   {
      if (imm.functionIndex >= checktimeIndex(module))
         ++imm.functionIndex;
   }

   template<typename Imm>
   void adjustCallIndex(const Module& , Imm& )
   {
   }
*/

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
      static constexpr bool kills = false;
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
      }
   };

   struct debug_printer {
      static constexpr bool kills = false;
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
         std::cout << "INSTRUCTION : " << inst->to_string() << "\n";
      }
   };

   struct instruction_counter {
      static constexpr bool kills = false;
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
         // maybe change this later to variable weighting for different instruction types
         icnt++;
      }
      static uint32_t icnt;
   };

   struct checktime_injector {
      static constexpr bool kills = false;
      static void accept( wasm_ops::instr* inst, wasm_ops::visitor_arg& arg ) {
//         if (checktime_idx == -1)
//            FC_THROW("Error, this should be run after module injector");
         //std::cout << "HIT A BLOCK " <<  " " << index << "\n";
         wasm_ops::op_types<>::i32_const_t cnt; 
         cnt.field = instruction_counter::icnt;
         wasm_ops::op_types<>::call_t chktm; 
         chktm.field = checktime_idx;
         instruction_counter::icnt = 0;
         // pack the ops for insertion
         std::vector<U8> injected = cnt.pack();
         std::vector<U8> tmp      = chktm.pack();
         injected.insert( injected.end(), tmp.begin(), tmp.end() );
         //arg.new_code->insert( arg.new_code->end(), injected.begin(), injected.end() );
         //arg.function_def->code.insert( arg.function_def->code.begin()+arg.code_index, 
         //      injected.begin(), injected.end() );
      }
      static int32_t checktime_idx;
   };

   // add opcode specific constraints here
   // so far we only black list
   struct op_injectors : wasm_ops::op_types<pass_injector> {
      using block_t           = wasm_ops::block                   <debug_printer, instruction_counter, checktime_injector>;
      using loop_t            = wasm_ops::loop                    <debug_printer, instruction_counter, checktime_injector>;
      using if__t             = wasm_ops::if_                     <debug_printer, instruction_counter, checktime_injector>;
      using else__t           = wasm_ops::else_                   <debug_printer, instruction_counter, checktime_injector>;
      
      using end_t             = wasm_ops::end                     <debug_printer, instruction_counter>;
      using unreachable_t     = wasm_ops::unreachable             <debug_printer, instruction_counter>;
      using br_t              = wasm_ops::br                      <debug_printer, instruction_counter>;
      using br_if_t           = wasm_ops::br_if                   <debug_printer, instruction_counter>;
      using br_table_t        = wasm_ops::br_table                <debug_printer, instruction_counter>;
      using return__t         = wasm_ops::return_                 <debug_printer, instruction_counter>;
      using call_t            = wasm_ops::call                    <debug_printer, instruction_counter>;
      using call_indirect_t   = wasm_ops::call_indirect           <debug_printer, instruction_counter>;
      using drop_t            = wasm_ops::drop                    <debug_printer, instruction_counter>;
      using select_t          = wasm_ops::select                  <debug_printer, instruction_counter>;

      using get_local_t       = wasm_ops::get_local               <debug_printer, instruction_counter>;
      using set_local_t       = wasm_ops::set_local               <debug_printer, instruction_counter>;
      using tee_local_t       = wasm_ops::tee_local               <debug_printer, instruction_counter>;
      using get_global_t      = wasm_ops::get_global              <debug_printer, instruction_counter>;
      using set_global_t      = wasm_ops::set_global              <debug_printer, instruction_counter>;

      using nop_t             = wasm_ops::nop                     <debug_printer, instruction_counter>;
      using i32_load_t        = wasm_ops::i32_load                <debug_printer, instruction_counter>;
      using i64_load_t        = wasm_ops::i64_load                <debug_printer, instruction_counter>;
      using f32_load_t        = wasm_ops::f32_load                <debug_printer, instruction_counter>;
      using f64_load_t        = wasm_ops::f64_load                <debug_printer, instruction_counter>;
      using i32_load8_s_t     = wasm_ops::i32_load8_s             <debug_printer, instruction_counter>;
      using i32_load8_u_t     = wasm_ops::i32_load8_u             <debug_printer, instruction_counter>;
      using i32_load16_s_t    = wasm_ops::i32_load16_s            <debug_printer, instruction_counter>;
      using i32_load16_u_t    = wasm_ops::i32_load16_u            <debug_printer, instruction_counter>;
      using i64_load8_s_t     = wasm_ops::i64_load8_s             <debug_printer, instruction_counter>;
      using i64_load8_u_t     = wasm_ops::i64_load8_u             <debug_printer, instruction_counter>;
      using i64_load16_s_t    = wasm_ops::i64_load16_s            <debug_printer, instruction_counter>;
      using i64_load16_u_t    = wasm_ops::i64_load16_u            <debug_printer, instruction_counter>;
      using i64_load32_s_t    = wasm_ops::i64_load32_s            <debug_printer, instruction_counter>;
      using i64_load32_u_t    = wasm_ops::i64_load32_u            <debug_printer, instruction_counter>;
      using i32_store_t       = wasm_ops::i32_store               <debug_printer, instruction_counter>;
      using i64_store_t       = wasm_ops::i64_store               <debug_printer, instruction_counter>;
      using f32_store_t       = wasm_ops::f32_store               <debug_printer, instruction_counter>;
      using f64_store_t       = wasm_ops::f64_store               <debug_printer, instruction_counter>;
      using i32_store8_t      = wasm_ops::i32_store8              <debug_printer, instruction_counter>;
      using i32_store16_t     = wasm_ops::i32_store16             <debug_printer, instruction_counter>;
      using i64_store8_t      = wasm_ops::i64_store8              <debug_printer, instruction_counter>;
      using i64_store16_t     = wasm_ops::i64_store16             <debug_printer, instruction_counter>;
      using i64_store32_t     = wasm_ops::i64_store32             <debug_printer, instruction_counter>;

      using i32_const_t       = wasm_ops::i32_const               <debug_printer, instruction_counter>;
      using i64_const_t       = wasm_ops::i64_const               <debug_printer, instruction_counter>;
      using f32_const_t       = wasm_ops::f32_const               <debug_printer, instruction_counter>;
      using f64_const_t       = wasm_ops::f64_const               <debug_printer, instruction_counter>;
      
      using i32_eqz_t         = wasm_ops::i32_eqz                 <debug_printer, instruction_counter>;
      using i32_eq_t          = wasm_ops::i32_eq                  <debug_printer, instruction_counter>;
      using i32_ne_t          = wasm_ops::i32_ne                  <debug_printer, instruction_counter>;
      using i32_lt_s_t        = wasm_ops::i32_lt_s                <debug_printer, instruction_counter>;
      using i32_lt_u_t        = wasm_ops::i32_lt_u                <debug_printer, instruction_counter>;
      using i32_gt_s_t        = wasm_ops::i32_gt_s                <debug_printer, instruction_counter>;
      using i32_gt_u_t        = wasm_ops::i32_gt_u                <debug_printer, instruction_counter>;
      using i32_le_s_t        = wasm_ops::i32_le_s                <debug_printer, instruction_counter>;
      using i32_le_u_t        = wasm_ops::i32_le_u                <debug_printer, instruction_counter>;
      using i32_ge_s_t        = wasm_ops::i32_ge_s                <debug_printer, instruction_counter>;
      using i32_ge_u_t        = wasm_ops::i32_ge_u                <debug_printer, instruction_counter>;

      using i32_clz_t         = wasm_ops::i32_clz                 <debug_printer, instruction_counter>;
      using i32_ctz_t         = wasm_ops::i32_ctz                 <debug_printer, instruction_counter>;
      using i32_popcnt_t      = wasm_ops::i32_popcnt              <debug_printer, instruction_counter>;

      using i32_add_t         = wasm_ops::i32_add                 <debug_printer, instruction_counter>; 
      using i32_sub_t         = wasm_ops::i32_sub                 <debug_printer, instruction_counter>; 
      using i32_mul_t         = wasm_ops::i32_mul                 <debug_printer, instruction_counter>; 
      using i32_div_s_t       = wasm_ops::i32_div_s               <debug_printer, instruction_counter>; 
      using i32_div_u_t       = wasm_ops::i32_div_u               <debug_printer, instruction_counter>; 
      using i32_rem_s_t       = wasm_ops::i32_rem_s               <debug_printer, instruction_counter>; 
      using i32_rem_u_t       = wasm_ops::i32_rem_u               <debug_printer, instruction_counter>; 
      using i32_and_t         = wasm_ops::i32_and                 <debug_printer, instruction_counter>; 
      using i32_or_t          = wasm_ops::i32_or                  <debug_printer, instruction_counter>; 
      using i32_xor_t         = wasm_ops::i32_xor                 <debug_printer, instruction_counter>; 
      using i32_shl_t         = wasm_ops::i32_shl                 <debug_printer, instruction_counter>; 
      using i32_shr_s_t       = wasm_ops::i32_shr_s               <debug_printer, instruction_counter>; 
      using i32_shr_u_t       = wasm_ops::i32_shr_u               <debug_printer, instruction_counter>; 
      using i32_rotl_t        = wasm_ops::i32_rotl                <debug_printer, instruction_counter>; 
      using i32_rotr_t        = wasm_ops::i32_rotr                <debug_printer, instruction_counter>; 

      using i64_eqz_t         = wasm_ops::i64_eqz                 <debug_printer, instruction_counter>;
      using i64_eq_t          = wasm_ops::i64_eq                  <debug_printer, instruction_counter>;
      using i64_ne_t          = wasm_ops::i64_ne                  <debug_printer, instruction_counter>;
      using i64_lt_s_t        = wasm_ops::i64_lt_s                <debug_printer, instruction_counter>;
      using i64_lt_u_t        = wasm_ops::i64_lt_u                <debug_printer, instruction_counter>;
      using i64_gt_s_t        = wasm_ops::i64_gt_s                <debug_printer, instruction_counter>;
      using i64_gt_u_t        = wasm_ops::i64_gt_u                <debug_printer, instruction_counter>;
      using i64_le_s_t        = wasm_ops::i64_le_s                <debug_printer, instruction_counter>;
      using i64_le_u_t        = wasm_ops::i64_le_u                <debug_printer, instruction_counter>;
      using i64_ge_s_t        = wasm_ops::i64_ge_s                <debug_printer, instruction_counter>;
      using i64_ge_u_t        = wasm_ops::i64_ge_u                <debug_printer, instruction_counter>;

      using i64_clz_t         = wasm_ops::i64_clz                 <debug_printer, instruction_counter>;
      using i64_ctz_t         = wasm_ops::i64_ctz                 <debug_printer, instruction_counter>;
      using i64_popcnt_t      = wasm_ops::i64_popcnt              <debug_printer, instruction_counter>;

      using i64_add_t         = wasm_ops::i64_add                 <debug_printer, instruction_counter>; 
      using i64_sub_t         = wasm_ops::i64_sub                 <debug_printer, instruction_counter>; 
      using i64_mul_t         = wasm_ops::i64_mul                 <debug_printer, instruction_counter>; 
      using i64_div_s_t       = wasm_ops::i64_div_s               <debug_printer, instruction_counter>; 
      using i64_div_u_t       = wasm_ops::i64_div_u               <debug_printer, instruction_counter>; 
      using i64_rem_s_t       = wasm_ops::i64_rem_s               <debug_printer, instruction_counter>; 
      using i64_rem_u_t       = wasm_ops::i64_rem_u               <debug_printer, instruction_counter>; 
      using i64_and_t         = wasm_ops::i64_and                 <debug_printer, instruction_counter>; 
      using i64_or_t          = wasm_ops::i64_or                  <debug_printer, instruction_counter>; 
      using i64_xor_t         = wasm_ops::i64_xor                 <debug_printer, instruction_counter>; 
      using i64_shl_t         = wasm_ops::i64_shl                 <debug_printer, instruction_counter>; 
      using i64_shr_s_t       = wasm_ops::i64_shr_s               <debug_printer, instruction_counter>; 
      using i64_shr_u_t       = wasm_ops::i64_shr_u               <debug_printer, instruction_counter>; 
      using i64_rotl_t        = wasm_ops::i64_rotl                <debug_printer, instruction_counter>; 
      using i64_rotr_t        = wasm_ops::i64_rotr                <debug_printer, instruction_counter>; 

      using i32_wrap_i64_t    = wasm_ops::i32_wrap_i64            <debug_printer, instruction_counter>;
      using i64_extend_s_i32_t = wasm_ops::i64_extend_s_i32       <debug_printer, instruction_counter>;
      using i64_extend_u_i32_t = wasm_ops::i64_extend_u_i32       <debug_printer, instruction_counter>;
      // TODO, make sure these are just pointer reinterprets
      using i32_reinterpret_f32_t = wasm_ops::i32_reinterpret_f32 <debug_printer, instruction_counter>;
      using f32_reinterpret_i32_t = wasm_ops::f32_reinterpret_i32 <debug_printer, instruction_counter>;
      using i64_reinterpret_f64_t = wasm_ops::i64_reinterpret_f64 <debug_printer, instruction_counter>;
      using f64_reinterpret_i64_t = wasm_ops::f64_reinterpret_i64 <debug_printer, instruction_counter>;

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
            for ( auto& fd : mod.functions.defs ) {
               wasm_ops::EOSIO_OperatorDecoderStream<op_injectors> decoder(fd.code);
               std::vector<U8> new_code;
               while ( decoder ) {
                  auto op = decoder.decodeOp();
                  std::vector<U8>::iterator decoded_index = (fd.code.begin() + decoder.index());
                  op->visit( { &mod, &new_code, &fd, decoder.index() } );
               }

               for ( int i=0; i < new_code.size(); i++ )
                  std::cout <<std::hex << uint32_t(new_code[i]) << ", ";
               std::cout << "\n";
               for ( int i=0; i < fd.code.size(); i++ )
                  std::cout << std::hex << uint32_t(fd.code[i]) << ", ";
               std::cout << "\n";

               fd.code = new_code;
            }
         }
      private:
         static std::string op_string;
         static standard_module_injectors _module_injectors;
   };

}}} // namespace wasm_constraints, chain, eosio
