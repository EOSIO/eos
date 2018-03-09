#pragma once

#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/wasm_eosio_binary_ops.hpp>
#include <functional>


template <typename Pre_Validators, typename Post_Validators> //, typename Rewriters>
struct wasm_module_walker {
   wasm_module_walker( IR::Module& m ) : mod(m) {}

   void pre_validate() {
      Pre_Validators::validate( mod );
   }
   
   void post_validate() {
      Post_Validators::validate( mod );
   }


/*
   void rewrite() {
      for ( const FunctionDef& fd : mod.functions.defs ) {
         for ( U8 opcode : fd.code )
            for ( wasm_mutator rewriter : rewriters)
               rewriter->on_opcode(opcode);
      }
   }
   */
   
   /*
   std::vector<wasm_walker_cb> post_validators;
   */
//   std::vector<wasm_mutator*> rewriters;
   IR::Module& mod;
};


