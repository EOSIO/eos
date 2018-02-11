#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include "IR/Module.h"
#include "IR/Operators.h"

namespace eosio { namespace chain {

using namespace IR;

struct nop_opcode_visitor {
   typedef void Result;

   #define VISIT_OPCODE(opcode,name,nameString,Imm,...) \
         virtual void name(Imm) {}
	ENUM_OPERATORS(VISIT_OPCODE)
	#undef VISIT_OPCODE

   void unknown(Opcode) {
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract encountered unknown opcode");
   }
};

struct eosio_constraints_visitor : public nop_opcode_visitor {
   ///Make this some sort of visitor enum to reduce chance of copy pasta errors (but
   // the override declaration makes it somewhat safe)

   //While it's possible to access beyond 1MiB by giving an offset that's 1MiB-1 and
   // an 8 byte data type, that's fine. There will be enough of a guard on the end
   // of 1MiB where it's not a problem
   void fail_large_offset(U32 offset) {
      if(offset >= wasm_constraints::maximum_linear_memory)
         FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract used an invalid large memory store/load offset");
   }
   void i32_load     (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void i64_load     (LoadOrStoreImm<3> imm) override { fail_large_offset(imm.offset); }
   void i32_load8_s  (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i32_load8_u  (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i32_load16_s (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i32_load16_u (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i64_load8_s  (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i64_load8_u  (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i64_load16_s (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i64_load16_u (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i64_load32_s (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void i64_load32_u (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void i32_store    (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void i64_store    (LoadOrStoreImm<3> imm) override { fail_large_offset(imm.offset); }
   void i32_store8   (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i32_store16  (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i64_store8   (LoadOrStoreImm<0> imm) override { fail_large_offset(imm.offset); }
   void i64_store16  (LoadOrStoreImm<1> imm) override { fail_large_offset(imm.offset); }
   void i64_store32  (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }

   void f32_load     (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void f64_load     (LoadOrStoreImm<3> imm) override { fail_large_offset(imm.offset); }
   void f32_store    (LoadOrStoreImm<2> imm) override { fail_large_offset(imm.offset); }
   void f64_store    (LoadOrStoreImm<3> imm) override { fail_large_offset(imm.offset); }

   #define VISIT_OPCODE(opcode,name,nameString,Imm,...) \
      void name(Imm) override { FC_THROW_EXCEPTION(wasm_execution_error, "Smart contracts may not use WASM memory operators"); }
	ENUM_MEMORY_OPERATORS(VISIT_OPCODE);
	#undef VISIT_OPCODE
   
};

void validate_eosio_wasm_constraints(const Module& m) {
   if(m.memories.defs.size() && m.memories.defs[0].type.size.min > wasm_constraints::maximum_linear_memory/(64*1024))
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract initial memory size must be less than or equal to ${k}KiB", ("k", wasm_constraints::maximum_linear_memory/1024));

   for(const DataSegment& ds : m.dataSegments) {
      if(ds.baseOffset.type != InitializerExpression::Type::i32_const)
         FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract has unexpected memory base offset type");
      if(static_cast<uint32_t>(ds.baseOffset.i32) + ds.data.size() > wasm_constraints::maximum_linear_memory_init)
         FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract data segments must lie in first ${k}KiB", ("k", wasm_constraints::maximum_linear_memory_init/1024));
   }

   if(m.tables.defs.size() && m.tables.defs[0].type.size.min > wasm_constraints::maximum_table_elements)
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract table limited to ${t} elements", ("t", wasm_constraints::maximum_table_elements));

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
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract has more than ${k} bytes of mutable globals", ("k", wasm_constraints::maximum_mutable_globals));

   //Some of the OperatorDecoderStream users inside of WAVM track the control stack and quit parsing from
   // OperatorDecoderStream when the control stack is empty (since that would indicate unreachable code).
   // Not doing that here, yet, since it's not clear it's required for the purpose of the validation
   eosio_constraints_visitor visitor;
   for(const FunctionDef& fd : m.functions.defs) {
      OperatorDecoderStream decoder(fd.code);
      while(decoder) {
         decoder.decodeOp(visitor);
      }
   }
}

}}