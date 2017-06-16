#pragma once
#include <eos/chain/message.hpp>
#include <eos/chain/message_handling_contexts.hpp>
#include <Runtime/Runtime.h>
#include "IR/Module.h"

namespace eos { namespace chain {

class  chain_controller;

/**
 * @class wasm_interface
 *
 * EOS uses the wasm-jit library to evaluate web assembly code. This library relies
 * upon a singlton interface which means there can be only one instance. This interface
 * is designed to wrap that singlton interface and potentially make it thread-local state
 * in the future.
 */
class wasm_interface {
   public:
      static wasm_interface& get();

      enum database_access_type {
        none,
        read_only,
        read_write
      };
   
      void load(const char* wasmbytes, size_t len );
      void apply( apply_context& c );
      void validate( message_validate_context& c );
      void precondition( precondition_validate_context& c );

      apply_context*                  current_apply_context    = nullptr;
      message_validate_context*       current_validate_context = nullptr;
      precondition_validate_context*  current_precondition_context = nullptr;

      Runtime::MemoryInstance*   current_memory  = nullptr;
      Runtime::ModuleInstance*   current_module  = nullptr;
      chain_controller*          current_chain   = nullptr;
      const AccountName*         current_account = nullptr;
      const Message*             current_message = nullptr;
      database_access_type       current_access  = none;

   private:
      char* vm_allocate( int bytes );
      void  vm_apply( const vector<char>& message );
      U32   vm_pointer_to_offset( char* );


      IR::Module*   module = nullptr;
      wasm_interface();
};


} } // eos::chain
