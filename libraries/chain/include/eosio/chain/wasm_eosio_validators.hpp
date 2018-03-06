#pragma once

#include <eosio/chain/wasm_eosio_rewriters.hpp>
#include <functional>
#include <vector>
#include <iostream>
#include "IR/Module.h"
#include "IR/Operators.h"
#include "WASM/WASM.h"

namespace eosio { namespace chain { namespace wasm_constraints {
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

   template <typename ... Visitors>
   struct constraints_validators {
      static void validate( IR::Module& m ) {
         for ( auto validator : { Visitors::validate... } )
            validator( m );
      }
   };
   
   // just pass 
   struct no_constraints_validators {
      static void validate( IR::Module& m ) {}
   };
   
   struct blacklist_validator { 
   struct standard_op_constrainers : op_types<blacklist_validator> { 
      
   };
   // inherit from this class and define your own validators 
   class wasm_binary_validation {
      using standard_module_constraints_validators = constraints_validators< memories_validation_visitor,
                                                                      data_segments_validation_visitor,
                                                                      tables_validation_visitor,
                                                                      globals_validation_visitor>;
      public:
         static void validate( const char* wasm_binary, size_t wasm_binary_size ) {
            IR::Module* mod = new IR::Module();
            Serialization::MemoryInputStream stream((const U8* )wasm_binary, wasm_binary_size);
            WASM::serialize( stream, *mod );
            _validators.validate( *mod );
         }
      private:
         static constraints_validators<standard_module_constraints_validators> _module_validators;
         static instruction_validators<standard_module_constraints_validators> _instr_validators;
   };

}}} // namespace wasm_constraints, chain, eosio
