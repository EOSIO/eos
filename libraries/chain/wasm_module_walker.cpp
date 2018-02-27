#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include "IR/Module.h"
#include "IR/Operators.h"

#include <functional>
#include <vector>
#include <unordered_map>
#include <iostream>

namespace eosio { namespace chain {

using namespace IR;
/*
struct wasm_opcode_no_disposition_exception {
   string opcode_name;
};
*/
/*
template <typename ... CONSTRAINTS>
struct contraints {
   static constexpr std::array<wasm_validate_func, sizeof...(CONSTRAINTS)> value = { CONSTRAINTS::validate ... };
};
*/
struct wasm_mutator {
   std::vector<U8> on_opcode( U8 op ) { 
      return {op};
   }
};

struct checktime_injection_mutator : wasm_mutator {
   checktime_injection_mutator() : count(0) {}
   std::vector<U8> on_opcode( U8 op ) { 
      count++;
      return {op};
   }

   uint32_t count;
};


#if 0
template <typename PRE_CONSTRAINTS>
struct wasm_module_walker {
   wasm_module_walker( Module& m ) : mod(m) {}
   void pre_validate() {
      for ( auto constraint : pre_validators )
         constraint( mod );     
   }
/*
   void post_validate() {
      for ( wasm_walker_cb constraint : post_validators )
         constraint( mod );     
   }
   */
/*
   void rewrite() {
      for ( const FunctionDef& fd : mod.functions.defs ) {
         for ( U8 opcode : fd.code )
            for ( wasm_mutator rewriter : rewriters)
               rewriter->on_opcode(opcode);
      }
   }
   */
   
   PRE_CONSTRAINTS pre_validators;   
   /*
   std::vector<wasm_walker_cb> post_validators;
   */
   std::vector<wasm_mutator*> rewriters;
   Module& mod;
};
#endif

}} // namespace eosio, namespace chain
