#include <eosio/chain/wasm_eosio_binary_ops.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/chain/exceptions.hpp>
#include "IR/Module.h"
#include "IR/Operators.h"

#include <vector>

namespace eosio { namespace chain { namespace wasm_ops {
using namespace IR;

inline std::string to_string( NoImm imm ) {
   return "no imm";
}
inline std::string to_string( MemoryImm imm ) {
   return "memory imm";
}
inline std::string to_string( CallImm imm ) {
   return "call index : "+std::to_string(uint32_t(imm.functionIndex));
}
inline std::string to_string( CallIndirectImm imm ) {
   return "call indirect type : "+std::to_string(uint32_t(imm.type.index));
}
inline std::string to_string( uint32_t field ) {
   return std::string("i32 : ")+std::to_string(field);
}
inline std::string to_string( uint64_t field ) {
   return std::string("i64 : ")+std::to_string(field);
}
inline std::string to_string( blocktype field ) {
   return std::string("blocktype : ")+std::to_string((uint32_t)field.result);
}
inline std::string to_string( memoryoptype field ) {
   return std::string("memoryoptype : ")+std::to_string((uint32_t)field.end);
}
inline std::string to_string( memarg field ) {
   return std::string("memarg : ")+std::to_string(field.a)+std::string(", ")+std::to_string(field.o);
}
inline std::string to_string( branchtabletype field ) {
   return std::string("branchtabletype : ")+std::to_string(field.target_depth)+std::string(", ")+std::to_string(field.table_index);
}

inline std::vector<U8> pack( uint32_t field ) {
   return { U8(field), U8(field >> 8), U8(field >> 16), U8(field >> 24) };
}
inline std::vector<U8> pack( uint64_t field ) {
   return { U8(field), U8(field >> 8), U8(field >> 16), U8(field >> 24), 
            U8(field >> 32), U8(field >> 40), U8(field >> 48), U8(field >> 56) 
          };
}
inline std::vector<U8> pack( blocktype field ) {
   return { U8(field.result) };
}
inline std::vector<U8> pack( memoryoptype field ) {
   return { U8(field.end) };
}
inline std::vector<U8> pack( memarg field ) {
   return { U8(field.a), U8(field.a >> 8), U8(field.a >> 16), U8(field.a >> 24), 
            U8(field.o), U8(field.o >> 8), U8(field.o >> 16), U8(field.o >> 24), 
          };

}
inline std::vector<U8> pack( branchtabletype field ) {
   return { U8(field.target_depth), U8(field.target_depth >> 8), U8(field.target_depth >> 16), U8(field.target_depth >> 24), 
            U8(field.target_depth >> 32), U8(field.target_depth >> 40), U8(field.target_depth >> 48), U8(field.target_depth >> 56), 
            U8(field.table_index), U8(field.table_index >> 8), U8(field.table_index >> 16), U8(field.table_index >> 24), 
            U8(field.table_index >> 32), U8(field.table_index >> 40), U8(field.table_index >> 48), U8(field.table_index >> 56) 

          };
}
}}} // namespace eosio, chain, wasm_ops
