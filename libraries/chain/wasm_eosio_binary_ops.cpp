#include <eosio/chain/wasm_eosio_binary_ops.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include "IR/Module.h"
#include "IR/Operators.h"

#include <vector>

namespace eosio { namespace chain { namespace wasm_ops {
using namespace IR;

std::string to_string( NoImm imm ) {
   return "no imm";
}
std::string to_string( MemoryImm imm ) {
   return "memory imm";
}
std::string to_string( CallImm imm ) {
   return "call index : "+std::to_string(uint32_t(imm.functionIndex));
}
std::string to_string( CallIndirectImm imm ) {
   return "call indirect type : "+std::to_string(uint32_t(imm.type.index));
}
std::string to_string( uint32_t field ) {
   return std::string("i32 : ")+std::to_string(field);
}
std::string to_string( uint64_t field ) {
   return std::string("i64 : ")+std::to_string(field);
}
std::string to_string( blocktype field ) {
   return std::string("blocktype : ")+std::to_string((uint32_t)field.result);
}
std::string to_string( memoryoptype field ) {
   return std::string("memoryoptype : ")+std::to_string((uint32_t)field.end);
}
std::string to_string( memarg field ) {
   return std::string("memarg : ")+std::to_string(field.a)+std::string(", ")+std::to_string(field.o);
}
std::string to_string( callindirecttype field ) {
   return std::string("callindirecttype : ")+
      std::to_string(field.funcidx)+std::string(", ")+std::to_string((uint32_t)field.end);
}

std::vector<U8> pack( uint32_t field ) {
   return { U8(field >> 24), U8(field >> 16), U8(field >> 8), U8(field) };
}
std::vector<U8> pack( uint64_t field ) {
   return { U8(field >> 56), U8(field >> 48), U8(field >> 40), U8(field >> 32), 
            U8(field >> 24), U8(field >> 16), U8(field >> 8), U8(field) };
}
std::vector<U8> pack( blocktype field ) {
   return { U8(field.result) };
}
std::vector<U8> pack( memoryoptype field ) {
   return { U8(field.end) };
}
std::vector<U8> pack( memarg field ) {
   return { U8(field.a >> 24), U8(field.a >> 16), U8(field.a >> 8), U8(field.a),
            U8(field.o >> 24), U8(field.o >> 16), U8(field.o >> 8), U8(field.o) };
}
std::vector<U8> pack( callindirecttype field ) {
   return { U8(field.funcidx >> 24), U8(field.funcidx >> 16), U8(field.funcidx >> 8), U8(field.funcidx),
            U8(field.end) };
}
}}} // namespace eosio, chain, wasm_ops
