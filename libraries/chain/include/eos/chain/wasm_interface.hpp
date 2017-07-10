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
      struct ModuleState {
         Runtime::ModuleInstance* instance     = nullptr;
         IR::Module*              module       = nullptr;
         int                      mem_start    = 0;
         int                      mem_end      = 1<<16;
         vector<char>             init_memory;
         fc::sha256               code_version;
      };

      static wasm_interface& get();

      void init( apply_context& c );
      void apply( apply_context& c );
      void validate( message_validate_context& c );
      void precondition( precondition_validate_context& c );

      apply_context*                  current_apply_context        = nullptr;
      message_validate_context*       current_validate_context     = nullptr;
      precondition_validate_context*  current_precondition_context = nullptr;

      Runtime::MemoryInstance*   current_memory  = nullptr;
      Runtime::ModuleInstance*   current_module  = nullptr;
      ModuleState*               current_state   = nullptr;

   private:
      void load( const AccountName& name, const chainbase::database& db );

      char* vm_allocate( int bytes );   
      void  vm_call( const char* name );
      void  vm_validate();
      void  vm_precondition();
      void  vm_apply();
      void  vm_onInit();
      U32   vm_pointer_to_offset( char* );



      map<AccountName, ModuleState> instances;


      wasm_interface();
};


} } // eos::chain
