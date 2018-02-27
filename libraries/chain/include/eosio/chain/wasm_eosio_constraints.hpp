#pragma once

#include <functional>
#include <vector>
#include <iostream>

namespace IR {
    struct Module;
};

namespace eosio { namespace chain { namespace wasm_constraints {
   //Be aware that some of these are required to be a multiple of some internal number
   constexpr unsigned maximum_linear_memory      = 1024*1024;  //bytes
   constexpr unsigned maximum_mutable_globals    = 1024;       //bytes
   constexpr unsigned maximum_table_elements     = 1024;       //elements
   constexpr unsigned maximum_linear_memory_init = 64*1024;    //bytes

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
   struct nop_constraints_validators {
      static void validate( IR::Module& m ) {}
   };

   bool is_whitelisted();
  
} // namespace wasm_constraints

//Throws if something in the module violates
void validate_eosio_wasm_constraints(const IR::Module& m);

//Throws if an opcode is neither whitelisted nor blacklisted
void check_wasm_opcode_dispositions();

}}
