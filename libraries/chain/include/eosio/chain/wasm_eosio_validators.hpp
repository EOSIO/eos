#pragma once

#include <functional>
#include <vector>
#include <iostream>
#include "IR/Module.h"
#include "IR/Operators.h"
#include "WASM/WASM.h"

namespace eosio { namespace chain { namespace wasm_constraints {
   // effectively do nothing and pass
   struct noop_validator {
      static void validate( IR::Module& m );
   };

   struct memories_validator {
      static void validate( IR::Module& m );
   };

   struct data_segments_validator {
      static void validate( IR::Module& m );
   };

   struct tables_validator {
      static void validate( IR::Module& m );
   };

   struct globals_validator {
      static void validate( IR::Module& m );
   };

   struct blacklist_validator {
      static void validate( IR::Module& m );
   };

   using wasm_validate_func = std::function<void(IR::Module&)>;

   template <typename ... Validators>
   struct constraints_validators {
      static void validate( IR::Module& m ) {
         for ( auto validator : { Validators::validate... } )
            validator( m );
      }
   };
   
   // just pass 
   struct no_constraints_validators {
      static void validate( IR::Module& m ) {}
   };
   
   // inherit from this class and define own validators 
   class wasm_binary_validation {
      using standard_constraints_validators = constraints_validators< memories_validator,
                                                                      data_segments_validator,
                                                                      tables_validator,
                                                                      globals_validator>;
      public:
         static bool validate( const char* wasm_binary, size_t wasm_binary_size ) {
            /*
            IR::Module* mod = new IR::Module();
            Serialization::MemoryInputStream stream((const U8* )wasm_binary, wasm_binary_size);
            WASM::serialize( stream, *mod );
            _validators.validate( *mod );
            */
         }
      private:
         static constraints_validators<standard_constraints_validators> _validators;
   };

}}} // namespace wasm_constraints, chain, eosio
