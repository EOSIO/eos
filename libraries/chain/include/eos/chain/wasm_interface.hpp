/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/exceptions.hpp>
#include <eos/chain/message.hpp>
#include <eos/chain/message_handling_contexts.hpp>
#include <Runtime/Runtime.h>
#include "IR/Module.h"

namespace eosio { namespace chain {

class chain_controller;
class wasm_memory;

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
      enum key_type {
         str,
         i64,
         i128i128,
         i64i64i64,
         invalid_key_type
      };
      typedef map<name, key_type> TableMap;
      struct ModuleState {
         Runtime::ModuleInstance* instance = nullptr;
         IR::Module*              module = nullptr;
         int                      mem_start = 0;
         int                      mem_end = 1<<16;
         vector<char>             init_memory;
         fc::sha256               code_version;
         TableMap                 table_key_types;
         bool                     tables_fixed = false;
      };

      static wasm_interface& get();

      void init( apply_context& c );
      void apply( apply_context& c, uint32_t execution_time, bool received_block );
      void validate( apply_context& c );
      void precondition( apply_context& c );

      int64_t current_execution_time();

      static key_type to_key_type(const types::type_name& type_name);
      static std::string to_type_name(key_type key_type);

      apply_context*       current_apply_context = nullptr;
      apply_context*       current_validate_context = nullptr;
      apply_context*       current_precondition_context = nullptr;

      Runtime::MemoryInstance*   current_memory = nullptr;
      Runtime::ModuleInstance*   current_module = nullptr;
      ModuleState*               current_state = nullptr;
      wasm_memory*               current_memory_management = nullptr;
      TableMap*                  table_key_types = nullptr;
      bool                       tables_fixed = false;
      int64_t                    table_storage = 0;

      uint32_t                   checktime_limit = 0;

      int32_t                    per_code_account_max_db_limit_mbytes = config::default_per_code_account_max_db_limit_mbytes;
      uint32_t                   row_overhead_db_limit_bytes = config::default_row_overhead_db_limit_bytes;

   private:
      void load( const account_name& name, const chainbase::database& db );

      char* vm_allocate( int bytes );   
      void  vm_call( const char* name );
      void  vm_validate();
      void  vm_precondition();
      void  vm_apply();
      void  vm_onInit();
      U32   vm_pointer_to_offset( char* );



      map<account_name, ModuleState> instances;
      fc::time_point checktimeStart;

      wasm_interface();
};


} } // eosio::chain
