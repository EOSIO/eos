#pragma once
#include <eosio/blockchain/exceptions.hpp>
#include <eosio/blockchain/types.hpp>

namespace Runtime {
   struct MemoryInstance;
}

namespace eosio { namespace blockchain {

   class apply_context;

   /**
    * @class wasm_interface
    *
    * EOS.IO uses the wasm-jit library to evaluate web assembly code. This library relies
    * upon a singlton thread-local interface which means there can be only one instance 
    * per thread. 
    *
    */
   class wasm_interface {
      public:
         static wasm_interface& get();

         Runtime::MemoryInstance* memory()const;
         uint32_t                 memory_size()const;

         /**
          * Will initalize the given code or used a cached version if one exists for
          * the given codeid.  
          *
          * @param wasmcode - code in wasm format 
          */
         void load( digest_type codeid, const char* wasmcode, size_t codesize );

         /**
          * If there exists a cached version of the code for codeid then it is released
          */
         void unload( digest_type codeid );


         /**
          * Calls the init() method on the currently loaded code
          *
          * @param context - the interface by which the contract can interact
          * with blockchain state.
          */
         void init( apply_context& context );

         /**
          * Calls the apply() method on the currently loaded code
          *
          * @param context - the interface by which the contract can interact
          * with blockchain state.
          */
         void apply( apply_context& context );

      private:
         wasm_interface();
         unique_ptr<struct wasm_interface_impl> my;
   };


} } // eosio::blockchain
