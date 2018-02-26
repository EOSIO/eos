#include <eosio/chain/wasm_eosio_instruction_count.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include "IR/Module.h"
#include "IR/Operators.h"

namespace eosio { namespace chain {

using namespace IR;

struct InstructionCount {
   InstructionCount(LiteralImm<I32>* i32_imm) : i32_imm(i32_imm) {
   }

   uint32_t count = 0;
   LiteralImm<I32>* i32_imm;
};

typedef std::vector<InstructionCount> InstructionCounts;

struct opcode_instruction_count_visitor {
   typedef void Result;

#define VISIT_OPCODE(opcode,name,nameString,Imm,...) \
   virtual void name(Imm& ) { \
      if (!instructionCounts.empty()) \
         ++instructionCounts.back().count; \
      if (expect_count) FC_THROW_EXCEPTION(wasm_execution_error, "Checktime instruction count not present"); \
   }
ENUM_OPERATORS(VISIT_OPCODE)
#undef VISIT_OPCODE

   void unknown(Opcode& ) {
      FC_THROW_EXCEPTION(wasm_execution_error, "Smart contract encountered unknown opcode");
   }

   InstructionCounts instructionCounts;
   // expect the LiteralImm<I32> to be the first parameter
   bool expect_count = true;
};

struct special_instruction_count_visitor : public opcode_instruction_count_visitor {
   void block(ControlStructureImm& imm) override {
      opcode_instruction_count_visitor::block(imm);
      expect_count = true;
   }

   void loop(ControlStructureImm& imm) override {
      opcode_instruction_count_visitor::loop(imm);
      expect_count = true;
   }

   void i32_const(LiteralImm<I32>& imm) override {
      // start the new InstructionCount for this call
      if (expect_count) {
         instructionCounts.emplace_back(&imm);
         expect_count = false;
      }

      opcode_instruction_count_visitor::i32_const(imm);
   }

   void end(NoImm& imm) override {
      if (instructionCounts.empty())
         FC_THROW_EXCEPTION(wasm_execution_error, "Smart Contract encountered end instruction with nothing to end");

      uint32_t residule_count = 0;
      {
         auto& instructionCount = instructionCounts.back();
         if (instructionCount.i32_imm) {
            instructionCount.i32_imm->value = instructionCount.count;
         }
         else {
            residule_count = instructionCount.count;
         }
      }
      instructionCounts.pop_back();

      if (residule_count)
         instructionCounts.back().count += residule_count;
   }

   void if_(ControlStructureImm& imm) {
      opcode_instruction_count_visitor::if_(imm);
      // no count structure, but still need a stack object to track the count
      instructionCounts.emplace_back(nullptr);
   }
};

void wasm_esio_instruction_count::inject_instruction_count(const IR::Module& module) {
   for(const FunctionDef& fd : module.functions.defs) {
      special_instruction_count_visitor visitor;
      OperatorDecoderStream decoder(fd.code);
      while(decoder)
         decoder.decodeOp(visitor);
   }
}

}}
