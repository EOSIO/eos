#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/wasm_eosio_validation.hpp>
#include <eosio/chain/wasm_eosio_binary_ops.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include "IR/Module.h"
#include "IR/Operators.h"
#include "WASM/WASM.h"

namespace eosio { namespace chain { namespace wasm_validations {
using namespace IR;

void noop_validation_visitor::validate( const Module& m ) {
   // just pass
}

void memories_validation_visitor::validate( const Module& m ) {
   if ( m.memories.defs.size() && m.memories.defs[0].type.size.min > wasm_constraints::maximum_linear_memory/(64*1024) )
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract initial memory size must be less than or equal to ${k}KiB", 
            ("k", wasm_constraints::maximum_linear_memory/1024));
}

void data_segments_validation_visitor::validate(const Module& m ) {
   for ( const DataSegment& ds : m.dataSegments ) {
      if ( ds.baseOffset.type != InitializerExpression::Type::i32_const )
         FC_THROW_EXCEPTION( wasm_execution_error, "Smart contract has unexpected memory base offset type" );

      if ( static_cast<uint32_t>( ds.baseOffset.i32 ) + ds.data.size() > wasm_constraints::maximum_linear_memory_init )
         FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract data segments must lie in first ${k}KiB", 
               ("k", wasm_constraints::maximum_linear_memory_init/1024));
   }
}

void tables_validation_visitor::validate( const Module& m ) {
   if ( m.tables.defs.size() && m.tables.defs[0].type.size.min > wasm_constraints::maximum_table_elements )
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract table limited to ${t} elements", 
            ("t", wasm_constraints::maximum_table_elements));
}

void globals_validation_visitor::validate( const Module& m ) {
   unsigned mutable_globals_total_size = 0;
   for(const GlobalDef& global_def : m.globals.defs) {
      if(!global_def.type.isMutable)
         continue;
      switch(global_def.type.valueType) {
         case ValueType::any:
         case ValueType::num:
            FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract has unexpected global definition value type");
         case ValueType::i64:
         case ValueType::f64:
            mutable_globals_total_size += 4;
         case ValueType::i32:
         case ValueType::f32:
            mutable_globals_total_size += 4;
      }
   }
   if(mutable_globals_total_size > wasm_constraints::maximum_mutable_globals)
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract has more than ${k} bytes of mutable globals",
            ("k", wasm_constraints::maximum_mutable_globals));
}

void maximum_function_stack_visitor::validate( const IR::Module& m ) {
   for(const FunctionDef& func : m.functions.defs) {
      unsigned function_stack_usage = 0;
      for(const ValueType& local : func.nonParameterLocalTypes)
         function_stack_usage += getTypeBitWidth(local)/8;
      for(const ValueType& params : m.types[func.type.index]->parameters)
         function_stack_usage += getTypeBitWidth(params)/8;

      if(function_stack_usage > wasm_constraints::maximum_func_local_bytes)
         FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract function has more than ${k} bytes of stack usage",
            ("k", wasm_constraints::maximum_func_local_bytes));
   }
}

void ensure_apply_exported_visitor::validate( const IR::Module& m ) {
   bool found_it = false;

   for(const Export& exprt : m.exports) {
      if(exprt.name != "apply" && exprt.kind != ObjectKind::function)
         continue;
      if(m.types[m.functions.getType(exprt.index).index] == FunctionType::get(ResultType::none, {ValueType::i64, ValueType::i64})) {
         found_it = true;
         break;
      }
   }

   if(!found_it)
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract's apply function not exported; non-existent; or wrong type");
}

}}} // namespace eosio chain validation
