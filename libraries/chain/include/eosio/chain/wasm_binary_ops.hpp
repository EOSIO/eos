#pragma once

#include <fc/reflect/reflect.hpp>

#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <fc/optional.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace eosio { namespace chain { namespace wasm_ops {

// forward declaration
struct instr;

using wasm_op_ptr	  = std::unique_ptr<instr>;
using wasm_instr_ptr      = std::shared_ptr<instr>;
using wasm_return_t       = fc::optional<std::vector<uint8_t>>; 
using wasm_instr_callback = std::function<std::vector<wasm_instr_ptr>(uint8_t)>;
using code_iterator       = std::vector<uint8_t>::iterator;
using wasm_op_generator   = std::function<wasm_instr_ptr(std::vector<uint8_t>, size_t)>;

//#define BUILD_WASM_OP_STRUCTURES:w

enum code {
   UNREACHABLE    = 0x00,
   NOP            = 0x01,
   BLOCK          = 0x02,
   LOOP           = 0x03,
   IF             = 0x04,
   ELSE           = 0x05,
   END            = 0x0B,
   BR             = 0x0C,
   BR_IF          = 0x0D,
   BR_TABLE       = 0x0E,
   RETURN         = 0x0F,
   CALL           = 0x10,
   CALL_INDIRECT  = 0x11,

   DROP           = 0x1A,
   SELECT         = 0x1B,

   GET_LOCAL      = 0x20,
   SET_LOCAL      = 0x21,
   TEE_LOCAL      = 0x22,
   GET_GLOBAL     = 0x23,
   SET_GLOBAL     = 0x24,

   I32_LOAD       = 0x28,
   I64_LOAD       = 0x29,
   F32_LOAD       = 0x2A,
   F64_LOAD       = 0x2B,
   I32_LOAD8_S    = 0x2C,
   I32_LOAD8_U    = 0x2D,
   I32_LOAD16_S   = 0x2E,
   I32_LOAD16_U   = 0x2F,
   I64_LOAD8_S    = 0x30,
   I64_LOAD8_U    = 0x31,
   I64_LOAD16_S   = 0x32,
   I64_LOAD16_U   = 0x33,
   I64_LOAD32_S   = 0x34,
   I64_LOAD32_U   = 0x35,
   I32_STORE      = 0x36,
   I64_STORE      = 0x37,
   F32_STORE      = 0x38,
   F64_STORE      = 0x39,
   I32_STORE8     = 0x3A,
   I32_STORE16    = 0x3B,
   I64_STORE8     = 0x3C,
   I64_STORE16    = 0x3D,
   I64_STORE32    = 0x3E,
   CURRENT_MEM    = 0x3F,
   GROW_MEM       = 0x40,

   I32_CONST      = 0x41,
   I64_CONST      = 0x42,
   F32_CONST      = 0x43,
   F64_CONST      = 0x44,

   I32_EQZ        = 0x45,
   I32_EQ         = 0x46,
   I32_NE         = 0x47,
   I32_LT_S       = 0x48,
   I32_LT_U       = 0x49,
   I32_GT_S       = 0x4A,
   I32_GT_U       = 0x4B,
   I32_LE_S       = 0x4C,
   I32_LE_U       = 0x4D,
   I32_GE_S       = 0x4E,
   I32_GE_U       = 0x4F,

   I64_EQZ        = 0x50,
   I64_EQ         = 0x51,
   I64_NE         = 0x52,
   I64_LT_S       = 0x53,
   I64_LT_U       = 0x54,
   I64_GT_S       = 0x55,
   I64_GT_U       = 0x56,
   I64_LE_S       = 0x57,
   I64_LE_U       = 0x58,
   I64_GE_S       = 0x59,
   I64_GE_U       = 0x5A,

   F32_EQ         = 0x5B,
   F32_NE         = 0x5C,
   F32_LT         = 0x5D,
   F32_GT         = 0x5E,
   F32_LE         = 0x5F,
   F32_GE         = 0x60,

   F64_EQ         = 0x61,
   F64_NE         = 0x62,
   F64_LT         = 0x63,
   F64_GT         = 0x64,
   F64_LE         = 0x65,
   F64_GE         = 0x66,
   
   I32_CLZ        = 0x67,
   I32_CTZ        = 0x68,
   I32_POPCOUNT   = 0x69,
   I32_ADD        = 0x6A,
   I32_SUB        = 0x6B,
   I32_MUL        = 0x6C,
   I32_DIV_S      = 0x6D,
   I32_DIV_U      = 0x6E,
   I32_REM_S      = 0x6F,
   I32_REM_U      = 0x70,
   I32_AND        = 0x71,
   I32_OR         = 0x72,
   I32_XOR        = 0x73,
   I32_SHL        = 0x74,
   I32_SHR_S      = 0x75,
   I32_SHR_U      = 0x76,
   I32_ROTL       = 0x77,
   I32_ROTR       = 0x78,

   I64_CLZ        = 0x79,
   I64_CTZ        = 0x7A,
   I64_POPCOUNT   = 0x7B,
   I64_ADD        = 0x7C,
   I64_SUB        = 0x7D,
   I64_MUL        = 0x7E,
   I64_DIV_S      = 0x7F,
   I64_DIV_U      = 0x80,
   I64_REM_S      = 0x81,
   I64_REM_U      = 0x82,
   I64_AND        = 0x83,
   I64_OR         = 0x84,
   I64_XOR        = 0x85,
   I64_SHL        = 0x86,
   I64_SHR_S      = 0x87,
   I64_SHR_U      = 0x88,
   I64_ROTL       = 0x89,
   I64_ROTR       = 0x8A,

   F32_ABS        = 0x8B,
   F32_NEG        = 0x8C,
   F32_CEIL       = 0x8D,
   F32_FLOOR      = 0x8E,
   F32_TRUNC      = 0x8F,
   F32_NEAREST    = 0x90,
   F32_SQRT       = 0x91,
   F32_ADD        = 0x92,
   F32_SUB        = 0x93,
   F32_MUL        = 0x94,
   F32_DIV        = 0x95,
   F32_MIN        = 0x96,
   F32_MAX        = 0x97,
   F32_COPYSIGN   = 0x98,

   F64_ABS        = 0x99,
   F64_NEG        = 0x9A,
   F64_CEIL       = 0x9B,
   F64_FLOOR      = 0x9C,
   F64_TRUNC      = 0x9D,
   F64_NEAREST    = 0x9E,
   F64_SQRT       = 0x9F,
   F64_ADD        = 0xA0,
   F64_SUB        = 0xA1,
   F64_MUL        = 0xA2,
   F64_DIV        = 0xA3,
   F64_MIN        = 0xA4,
   F64_MAX        = 0xA5,
   F64_COPYSIGN   = 0xA6,

   I32_WRAP_I64       = 0xA7,
   I32_TRUNC_S_F32    = 0xA8,
   I32_TRUNC_U_F32    = 0xA9,
   I32_TRUNC_S_F64    = 0xAA,
   I32_TRUNC_U_F64    = 0xAB,
   I64_EXTEND_S_I32   = 0xAC,
   I64_EXTEND_U_I32   = 0xAD,
   I64_TRUNC_S_F32    = 0xAE,
   I64_TRUNC_U_F32    = 0xAF,
   I64_TRUNC_S_F64    = 0xB0,
   I64_TRUNC_U_F64    = 0xB1,
   F32_CONVERT_S_I32  = 0xB2,
   F32_CONVERT_U_I32  = 0xB3,
   F32_CONVERT_S_I64  = 0xB4,
   F32_CONVERT_U_I64  = 0xB5,
   F32_DEMOTE_F64     = 0xB6,
   F64_CONVERT_S_I32  = 0xB7,
   F64_CONVERT_U_I32  = 0xB8,
   F64_CONVERT_S_I64  = 0xB9,
   F64_CONVERT_U_I64  = 0xBA,
   F64_PROMOTE_F32    = 0xBB,
   I32_REINTERPRET_F32 = 0xBC,
   I64_REINTERPRET_F64 = 0xBD,
   F32_REINTERPRET_I32 = 0xBE,
   F64_REINTERPRET_I64 = 0xBF,
}; // code

enum valtype {
   i32 = 0x7F,
   i64 = 0x7E,
   f32 = 0x7D,
   f64 = 0x7C
};

#pragma pack (push)
struct memarg {
   uint32_t a;   // align
   uint32_t o;   // offset
};

struct blocktype {
   uint8_t result = 0x40; // either null (0x40) or valtype
};

template <size_t Params, size_t Results>
struct functype {
   uint8_t code = 0x60;
   std::array<uint8_t, Params>  params;
   std::array<uint8_t, Results> results;
};

struct limits {
   uint8_t code = 0x00; // either 0x00 or 0x01
   uint32_t min = 0;
   uint32_t max = -1;   // any size
};

struct memtype {
   limits lim;
};

struct elemtype {
   uint8_t code = 0x70; // anyfunc
};

struct tabletype {
   elemtype et;
   limits   lim;
};

struct mut {
   uint8_t code = 0x00; // either 0x00 const or 0x01 var
};

struct globaltype {
   valtype t;
   mut     m;
};

struct instr {
   uint8_t code;
   virtual std::string to_string() { return "instr"; }
   virtual wasm_return_t visit() = 0;
   virtual std::vector<uint8_t> pack() { return { code }; }
   virtual uint32_t skip_ahead() { return 1; } // amount to skip past operands
   /*
   template <typename Stream>
   friend Stream& operator >> ( Stream& out, const instr& in ) {
      return out << code;
   }
*/
};

template <uint8_t Code, typename ... Mutators>
struct instr_base : instr {
   instr_base() { code = Code; }
   virtual wasm_return_t visit( ) {//code_iterator start, code_iterator end ) {
      for ( auto m : { Mutators::accept... } )
         m( this );
      
      return {};
   } 
};

// control insructions
template <typename ... Mutators>
struct end : instr_base<END, Mutators...> {
   std::string to_string() { return "end"; }
};

template <typename ... Mutators>
struct unreachable : instr_base<UNREACHABLE, Mutators...> {
   std::string to_string() { return "unreachable"; }
};

template <typename ... Mutators>
struct nop : instr_base<NOP, Mutators...> {
   std::string to_string() { return "nop"; }
};

template <typename ... Mutators>
struct ret : instr_base<RETURN, Mutators...> {
   std::string to_string() { return "return"; }
};

template <uint8_t Code, typename ... Mutators>
struct block_base : instr_base<Code, Mutators...> {
   block_base( std::vector<uint8_t> body ) : in(body) {}
   blocktype rt;
   std::vector<uint8_t> in;
   uint8_t end = END;

   virtual std::vector<uint8_t> pack() { 
      std::vector<uint8_t> ret = instr::pack();
      ret.insert( ret.end(), in.begin(), in.end() );
      ret.push_back( end );
      return ret; 
   }

   virtual uint32_t skip_ahead() {
      return instr::skip_ahead() + sizeof(blocktype); // + in.size() + 1;
   }
};

template <typename ... Mutators>
struct block : block_base<BLOCK, Mutators...> {
   using block_base<BLOCK, Mutators...>::block_base;
   std::string to_string() { return "block"; }
};

template <typename ... Mutators>
struct loop : block_base<LOOP, Mutators...> {
   using block_base<LOOP, Mutators...>::block_base;
   std::string to_string() { return "loop"; }
};

template <typename ... Mutators>
struct if_eps : block_base<IF, Mutators...> {
   using block_base<IF, Mutators...>::block_base;
   std::string to_string() { return "if"; }
};

template <typename ... Mutators>
struct if_else : block_base<IF, Mutators...> {
//   if_else( std::vector<uint8_t> if_body, std::vector<uint8_t> else_body ) : block_base<IF, Mutators...>(if_body), in2(else_body) {}
   using block_base<ELSE, Mutators...>::block_base;
   std::string to_string() { return "if else"; }
};

template <uint8_t Code, typename ... Mutators>
struct br_base : instr_base<Code, Mutators...> {
   br_base( uint32_t label ) : l(label) {}
   uint32_t l; // label to branch to
   virtual std::vector<uint8_t> pack() {
      std::vector<uint8_t> ret = instr::pack();
      ret.insert( ret.end(), { (uint8_t)(l >> 24), (uint8_t)(l >> 16), (uint8_t)(l >> 8), (uint8_t)l } );
      return ret;
   }
   virtual uint32_t skip_ahead() {
      return instr::skip_ahead() + sizeof(l);
   }
};

template <typename ... Mutators>
struct br : br_base<BR, Mutators...> {
   using br_base<BR, Mutators...>::br_base;
   std::string to_string() { return "br"; }
};

template <typename ... Mutators>
struct br_if : br_base<BR_IF, Mutators...> {
   using br_base<BR_IF, Mutators...>::br_base;
   std::string to_string() { return "br_if"; }
};

template <typename ... Mutators>
struct br_table : instr_base<BR_TABLE, Mutators...> {
   br_table( std::vector<uint32_t> labels, uint32_t lN ) : l(labels), lN(lN) {}
   std::string to_string() { return "br_table"; }
   std::vector<uint32_t> l;
   uint32_t lN;
   virtual std::vector<uint8_t> pack() {
      std::vector<uint8_t> ret = instr::pack();
      for ( uint32_t elem : l )
         ret.push_back( elem );
      ret.insert( ret.end(), { (uint8_t)(lN >> 24), (uint8_t)(lN>> 16), (uint8_t)(lN>> 8), (uint8_t)lN } );
      return ret;
   }
   virtual uint32_t skip_ahead() {
      return instr::skip_ahead(); // + l.size() + sizeof(lN);
   }
};

template <uint8_t Code, typename ... Mutators>
struct call_base : instr_base<Code, Mutators...> {
   call_base( uint32_t idx ) : funcidx(idx) {}
   uint32_t funcidx;
   virtual std::vector<uint8_t> pack() {
      std::vector<uint8_t> ret = instr::pack();
      ret.insert( ret.end(), { (uint8_t)(funcidx >> 24), (uint8_t)(funcidx >> 16), (uint8_t)(funcidx >> 8), (uint8_t)funcidx } );
      return ret;
   }
   virtual uint32_t skip_ahead() {
      return instr::skip_ahead() + sizeof(funcidx);
   }
};

template <typename ... Mutators>
struct call : call_base<CALL, Mutators...> {
   using call_base<CALL, Mutators...>::call_base;
   std::string to_string() { return "call"; }
};

template <typename ... Mutators>
struct call_indirect : call_base<CALL_INDIRECT, Mutators...> {
   using call_base<CALL_INDIRECT, Mutators...>::call_base;
   std::string to_string() { return "call_indirect"; }
   uint8_t end = UNREACHABLE;
   virtual std::vector<uint8_t> pack() {
      std::vector<uint8_t> ret = call_base<CALL_INDIRECT, Mutators...>::pack();
      ret.push_back( end );
      return ret;
   }
   virtual uint32_t skip_ahead() {
      return instr::skip_ahead() + 1;
   }
};

// parametric instructions
template <typename ... Mutators>
struct drop : instr_base<DROP, Mutators...> {
   std::string to_string() { return "drop"; }
};

template <typename ... Mutators>
struct select : instr_base<SELECT, Mutators...> {
   std::string to_string() { return "select"; }
};

// variable instructions
template <uint8_t Code, typename ... Mutators>
struct variable_base : instr_base<Code, Mutators...> {
   variable_base( uint32_t idx ) : localidx(idx) {}
   uint32_t localidx;
   virtual std::vector<uint8_t> pack() {
      std::vector<uint8_t> ret = instr::pack();
      ret.insert( ret.end(), { (uint8_t)(localidx >> 24), (uint8_t)(localidx >> 16), (uint8_t)(localidx >> 8), (uint8_t)localidx } );
      return ret;
   }
   virtual uint32_t skip_ahead() {
      return instr::skip_ahead() + sizeof(localidx);
   }
};

template <typename ... Mutators>
struct get_local : instr_base<GET_LOCAL, Mutators...> {
   using variable_base<GET_LOCAL, Mutators...>::variable_base;
   std::string to_string() { return "get_local"; }
};

template <typename ... Mutators>
struct set_local : instr_base<SET_LOCAL, Mutators...> {
   using variable_base<SET_LOCAL, Mutators...>::variable_base;
   std::string to_string() { return "set_local"; }
};

template <typename ... Mutators>
struct tee_local : instr_base<TEE_LOCAL, Mutators...> {
   using variable_base<TEE_LOCAL, Mutators...>::variable_base;
   std::string to_string() { return "tee_local"; }
};

template <typename ... Mutators>
struct get_global : instr_base<GET_GLOBAL, Mutators...> {
   using variable_base<GET_GLOBAL, Mutators...>::variable_base;
   std::string to_string() { return "get_global"; }
};

template <typename ... Mutators>
struct set_global : instr_base<SET_GLOBAL, Mutators...> {
   using variable_base<SET_GLOBAL, Mutators...>::variable_base;
   std::string to_string() { return "set_global"; }
};

// memory instructions
template <uint8_t Code, typename ... Mutators> 
struct mem_base : instr_base<Code, Mutators...> {
   mem_base( memarg m ) : m(m) {}
   memarg m;
   virtual std::vector<uint8_t> pack() {
      std::vector<uint8_t> ret = instr::pack();
      ret.insert( ret.end(), { (uint8_t)(m.a >> 24), (uint8_t)(m.a >> 16), (uint8_t)(m.a >> 8), (uint8_t)m.a,
                               (uint8_t)(m.o >> 24), (uint8_t)(m.o >> 16), (uint8_t)(m.o >> 8), (uint8_t)m.o } );
      return ret;
   }
   virtual uint32_t skip_ahead() {
      return instr::skip_ahead() + sizeof(memarg);
   }
};
template <typename ... Mutators>
struct i32_load : mem_base<I32_LOAD, Mutators...> {
   using mem_base<I32_LOAD, Mutators...>::mem_base;
   std::string to_string() { return "i32_load"; }
};

template <typename ... Mutators>
struct i64_load : mem_base<I64_LOAD, Mutators...> {
   using mem_base<I64_LOAD, Mutators...>::mem_base;
   std::string to_string() { return "i64_load"; }
};

template <typename ... Mutators>
struct f32_load : mem_base<F32_LOAD, Mutators...> {
   using mem_base<F32_LOAD, Mutators...>::mem_base;
   std::string to_string() { return "f32_load"; }
};

template <typename ... Mutators>
struct f64_load : mem_base<F64_LOAD, Mutators...> {
   using mem_base<F64_LOAD, Mutators...>::mem_base;
   std::string to_string() { return "f64_load"; }
};

template <typename ... Mutators>
struct i32_load8_s : mem_base<I32_LOAD8_S, Mutators...> {
   using mem_base<I32_LOAD8_S, Mutators...>::mem_base;
   std::string to_string() { return "i32_load8_s"; }
};

template <typename ... Mutators>
struct i32_load8_u : mem_base<I32_LOAD8_U, Mutators...> {
   using mem_base<I32_LOAD8_U, Mutators...>::mem_base;
   std::string to_string() { return "i32_load8_u"; }
};

template <typename ... Mutators>
struct i32_load16_s : mem_base<I32_LOAD16_S, Mutators...> {
   using mem_base<I32_LOAD16_S, Mutators...>::mem_base;
   std::string to_string() { return "i32_load16_s"; }
};

template <typename ... Mutators>
struct i32_load16_u : mem_base<I32_LOAD16_U, Mutators...> {
   using mem_base<I32_LOAD16_U, Mutators...>::mem_base;
   std::string to_string() { return "i32_load16_u"; }
};

template <typename ... Mutators>
struct i64_load8_s : mem_base<I64_LOAD8_S, Mutators...> {
   using mem_base<I64_LOAD8_S, Mutators...>::mem_base;
   std::string to_string() { return "i64_load8_s"; }
};

template <typename ... Mutators>
struct i64_load8_u : mem_base<I64_LOAD8_U, Mutators...> {
   using mem_base<I64_LOAD8_U, Mutators...>::mem_base;
   std::string to_string() { return "i64_load8_u"; }
};

template <typename ... Mutators>
struct i64_load16_s : mem_base<I64_LOAD16_S, Mutators...> {
   using mem_base<I64_LOAD16_S, Mutators...>::mem_base;
   std::string to_string() { return "i64_load16_s"; }
};

template <typename ... Mutators>
struct i64_load16_u : mem_base<I64_LOAD16_U, Mutators...> {
   using mem_base<I64_LOAD16_U, Mutators...>::mem_base;
   std::string to_string() { return "i64_load16_u"; }
};

template <typename ... Mutators>
struct i64_load32_s : mem_base<I64_LOAD32_S, Mutators...> {
   using mem_base<I64_LOAD32_S, Mutators...>::mem_base;
   std::string to_string() { return "i64_load32_s"; }
};

template <typename ... Mutators>
struct i64_load32_u : mem_base<I64_LOAD32_U, Mutators...> {
   using mem_base<I64_LOAD32_U, Mutators...>::mem_base;
   std::string to_string() { return "i64_load32_u"; }
};

template <typename ... Mutators>
struct i32_store : mem_base<I32_STORE, Mutators...> {
   using mem_base<I32_STORE, Mutators...>::mem_base;
   std::string to_string() { return "i32_store"; }
};

template <typename ... Mutators>
struct i64_store : mem_base<I64_STORE, Mutators...> {
   using mem_base<I64_STORE, Mutators...>::mem_base;
   std::string to_string() { return "i64_store"; }
};

template <typename ... Mutators>
struct f32_store : mem_base<F32_STORE, Mutators...> {
   using mem_base<F32_STORE, Mutators...>::mem_base;
   std::string to_string() { return "f32_store"; }
};

template <typename ... Mutators>
struct f64_store : mem_base<F64_STORE, Mutators...> {
   using mem_base<F64_STORE, Mutators...>::mem_base;
   std::string to_string() { return "f64_store"; }
};

template <typename ... Mutators>
struct i32_store8 : mem_base<I32_STORE8, Mutators...> {
   using mem_base<I32_STORE8, Mutators...>::mem_base;
   std::string to_string() { return "i32_store8"; }
};

template <typename ... Mutators>
struct i32_store16 : mem_base<I32_STORE16, Mutators...> {
   using mem_base<I32_STORE16, Mutators...>::mem_base;
   std::string to_string() { return "i32_store16"; }
};

template <typename ... Mutators>
struct i64_store8 : mem_base<I64_STORE8, Mutators...> {
   using mem_base<I64_STORE8, Mutators...>::mem_base;
   std::string to_string() { return "i64_store8"; }
};

template <typename ... Mutators>
struct i64_store16 : mem_base<I64_STORE16, Mutators...> {
   using mem_base<I64_STORE16, Mutators...>::mem_base;
   std::string to_string() { return "i64_store16"; }
};

template <typename ... Mutators>
struct i64_store32 : mem_base<I64_STORE32, Mutators...> {
   using mem_base<I64_STORE32, Mutators...>::mem_base;
   std::string to_string() { return "i64_store32"; }
};

template <typename ... Mutators>
struct current_memory : instr_base<CURRENT_MEM, Mutators...> {
   uint8_t end = UNREACHABLE;
   std::string to_string() { return "current_memory"; }
   virtual uint32_t skip_ahead() {
      return instr::skip_ahead() + 1;
   }
};

template <typename ... Mutators>
struct grow_memory : instr_base<GROW_MEM> {
   uint8_t end = UNREACHABLE;
   std::string to_string() { return "grow_memory"; }
   virtual uint32_t skip_ahead() {
      return instr::skip_ahead() + 1;
   }
};

// numeric instructions
template <uint8_t Code, typename ... Mutators> 
struct const_base : instr_base<Code, Mutators...> {
   const_base( uint32_t n ) : n(n) {}
   uint32_t n;
   virtual std::vector<uint8_t> pack() {
      std::vector<uint8_t> ret = instr::pack();
      ret.insert( ret.end(), { (uint8_t)(n >> 24), (uint8_t)(n >> 16), (uint8_t)(n >> 8), (uint8_t)n } );
      return ret;
   }
   virtual uint32_t skip_ahead() {
      return instr::skip_ahead() + sizeof(n);
   }
};

template <typename ... Mutators>
struct i32_const : const_base<I32_CONST, Mutators...> {
   using const_base<I32_CONST, Mutators...>::const_base;
   std::string to_string() { return "i32_const"; }
};

template <typename ... Mutators>
struct f32_const : const_base<F32_CONST, Mutators...> {
   using const_base<F32_CONST, Mutators...>::const_base;
   std::string to_string() { return "f32_const"; }
};

template <typename ... Mutators>
struct i64_const : const_base<I64_CONST, Mutators...> {
   using const_base<I64_CONST, Mutators...>::const_base;
   std::string to_string() { return "i64_const"; }
};

template <typename ... Mutators>
struct f64_const : const_base<F64_CONST, Mutators...> {
   using const_base<F64_CONST, Mutators...>::const_base;
   std::string to_string() { return "f64_const"; }
};

template <typename ... Mutators>
struct i32_eqz  : instr_base<I32_EQZ, Mutators...> {
   std::string to_string() { return "i32_eqz"; }
};
template <typename ... Mutators>
struct i32_eq   : instr_base<I32_EQ, Mutators...> {
   std::string to_string() { return "i32_eq"; }
};
template <typename ... Mutators>
struct i32_ne   : instr_base<I32_NE, Mutators...> {
   std::string to_string() { return "i32_ne"; }
};
template <typename ... Mutators>
struct i32_lt_s : instr_base<I32_LT_S, Mutators...> {
   std::string to_string() { return "i32_lt_s"; }
};
template <typename ... Mutators>
struct i32_lt_u : instr_base<I32_LT_U, Mutators...> {
   std::string to_string() { return "i32_lt_u"; }
};
template <typename ... Mutators>
struct i32_gt_s : instr_base<I32_GT_S, Mutators...> {
   std::string to_string() { return "i32_gt_s"; }
};
template <typename ... Mutators>
struct i32_gt_u : instr_base<I32_GT_U, Mutators...> {
   std::string to_string() { return "i32_gt_u"; }
};
template <typename ... Mutators>
struct i32_le_s : instr_base<I32_LE_S, Mutators...> {
   std::string to_string() { return "i32_le_s"; }
};
template <typename ... Mutators>
struct i32_le_u : instr_base<I32_LE_U, Mutators...> {
   std::string to_string() { return "i32_le_u"; }
};
template <typename ... Mutators>
struct i32_ge_s : instr_base<I32_GE_S, Mutators...> {
   std::string to_string() { return "i32_ge_s"; }
};
template <typename ... Mutators>
struct i32_ge_u : instr_base<I32_GE_U, Mutators...> {
   std::string to_string() { return "i32_ge_u"; }
};

template <typename ... Mutators>
struct i64_eqz  : instr_base<I64_EQZ, Mutators...> {
   std::string to_string() { return "i64_eqz"; }
};
template <typename ... Mutators>
struct i64_eq   : instr_base<I64_EQ, Mutators...> {
   std::string to_string() { return "i64_eq"; }
};
template <typename ... Mutators>
struct i64_ne   : instr_base<I64_NE, Mutators...> {
   std::string to_string() { return "i64_ne"; }
};
template <typename ... Mutators>
struct i64_lt_s : instr_base<I64_LT_S, Mutators...> {
   std::string to_string() { return "i64_lt_s"; }
};
template <typename ... Mutators>
struct i64_lt_u : instr_base<I64_LT_U, Mutators...> {
   std::string to_string() { return "i64_lt_u"; }
};
template <typename ... Mutators>
struct i64_gt_s : instr_base<I64_GT_S, Mutators...> {
   std::string to_string() { return "i64_gt_s"; }
};
template <typename ... Mutators>
struct i64_gt_u : instr_base<I64_GT_U, Mutators...> {
   std::string to_string() { return "i64_gt_u"; }
};
template <typename ... Mutators>
struct i64_le_s : instr_base<I64_LE_S, Mutators...> {
   std::string to_string() { return "i64_le_s"; }
};
template <typename ... Mutators>
struct i64_le_u : instr_base<I64_LE_U, Mutators...> {
   std::string to_string() { return "i64_le_u"; }
};
template <typename ... Mutators>
struct i64_ge_s : instr_base<I64_GE_S, Mutators...> {
   std::string to_string() { return "i64_ge_s"; }
};
template <typename ... Mutators>
struct i64_ge_u : instr_base<I64_GE_U, Mutators...> {
   std::string to_string() { return "i64_ge_u"; }
};

template <typename ... Mutators>
struct f32_eq : instr_base<F32_EQ, Mutators...> {
   std::string to_string() { return "f32_eq"; }
};
template <typename ... Mutators>
struct f32_ne : instr_base<F32_NE, Mutators...> {
   std::string to_string() { return "f32_ne"; }
};
template <typename ... Mutators>
struct f32_lt : instr_base<F32_LT, Mutators...> {
   std::string to_string() { return "f32_lt"; }
};
template <typename ... Mutators>
struct f32_gt : instr_base<F32_GT, Mutators...> {
   std::string to_string() { return "f32_gt"; }
};
template <typename ... Mutators>
struct f32_le : instr_base<F32_LE, Mutators...> {
   std::string to_string() { return "f32_le"; }
};
template <typename ... Mutators>
struct f32_ge : instr_base<F32_GE, Mutators...> {
   std::string to_string() { return "f32_ge"; }
};

template <typename ... Mutators>
struct f64_eq : instr_base<F64_EQ, Mutators...> {
   std::string to_string() { return "f64_eq"; }
};
template <typename ... Mutators>
struct f64_ne : instr_base<F64_NE, Mutators...> {
   std::string to_string() { return "f64_ne"; }
};
template <typename ... Mutators>
struct f64_lt : instr_base<F64_LT, Mutators...> {
   std::string to_string() { return "f64_lt"; }
};
template <typename ... Mutators>
struct f64_gt : instr_base<F64_GT, Mutators...> {
   std::string to_string() { return "f64_gt"; }
};
template <typename ... Mutators>
struct f64_le : instr_base<F64_LE, Mutators...> {
   std::string to_string() { return "f64_le"; }
};
template <typename ... Mutators>
struct f64_ge : instr_base<F64_GE, Mutators...> {
   std::string to_string() { return "f64_ge"; }
};

template <typename ... Mutators>
struct i32_popcount : instr_base<I32_POPCOUNT, Mutators...> {
   std::string to_string() { return "i32_popcount"; }
};
template <typename ... Mutators>
struct i32_clz      : instr_base<I32_CLZ, Mutators...> {
   std::string to_string() { return "i32_clz"; }
};
template <typename ... Mutators>
struct i32_ctz      : instr_base<I32_CTZ, Mutators...> {
   std::string to_string() { return "i32_ctz"; }
};
template <typename ... Mutators>
struct i32_add      : instr_base<I32_ADD, Mutators...> {
   std::string to_string() { return "i32_add"; }
};
template <typename ... Mutators>
struct i32_sub      : instr_base<I32_SUB, Mutators...> {
   std::string to_string() { return "i32_sub"; }
};
template <typename ... Mutators>
struct i32_mul      : instr_base<I32_MUL, Mutators...> {
   std::string to_string() { return "i32_mul"; }
};
template <typename ... Mutators>
struct i32_div_s    : instr_base<I32_DIV_S, Mutators...> {
   std::string to_string() { return "i32_div_s"; }
};
template <typename ... Mutators>
struct i32_div_u    : instr_base<I32_DIV_U, Mutators...> {
   std::string to_string() { return "i32_div_u"; }
};
template <typename ... Mutators>
struct i32_rem_s    : instr_base<I32_REM_S, Mutators...> {
   std::string to_string() { return "i32_rem_s"; }
};
template <typename ... Mutators>
struct i32_rem_u    : instr_base<I32_REM_U, Mutators...> {
   std::string to_string() { return "i32_rem_u"; }
};
template <typename ... Mutators>
struct i32_and      : instr_base<I32_AND, Mutators...> {
   std::string to_string() { return "i32_and"; }
};
template <typename ... Mutators>
struct i32_or       : instr_base<I32_OR, Mutators...> {
   std::string to_string() { return "i32_or"; }
};
template <typename ... Mutators>
struct i32_xor      : instr_base<I32_XOR, Mutators...> {
   std::string to_string() { return "i32_xor"; }
};
template <typename ... Mutators>
struct i32_shl      : instr_base<I32_SHL, Mutators...> {
   std::string to_string() { return "i32_shl"; }
};
template <typename ... Mutators>
struct i32_shr_s    : instr_base<I32_SHR_S, Mutators...> {
   std::string to_string() { return "i32_shr_s"; }
};
template <typename ... Mutators>
struct i32_shr_u    : instr_base<I32_SHR_U, Mutators...> {
   std::string to_string() { return "i32_shr_u"; }
};
template <typename ... Mutators>
struct i32_rotl     : instr_base<I32_ROTL, Mutators...> {
   std::string to_string() { return "i32_rotl"; }
};
template <typename ... Mutators>
struct i32_rotr     : instr_base<I32_ROTR, Mutators...> {
   std::string to_string() { return "i32_rotr"; }
};

template <typename ... Mutators>
struct i64_popcount : instr_base<I64_POPCOUNT, Mutators...> {
   std::string to_string() { return "i64_popcount"; }
};
template <typename ... Mutators>
struct i64_clz      : instr_base<I64_CLZ, Mutators...> {
   std::string to_string() { return "i64_clz"; }
};
template <typename ... Mutators>
struct i64_ctz      : instr_base<I64_CTZ, Mutators...> {
   std::string to_string() { return "i64_ctz"; }
};
template <typename ... Mutators>
struct i64_add      : instr_base<I64_ADD, Mutators...> {
   std::string to_string() { return "i64_add"; }
};
template <typename ... Mutators>
struct i64_sub      : instr_base<I64_SUB, Mutators...> {
   std::string to_string() { return "i64_sub"; }
};
template <typename ... Mutators>
struct i64_mul      : instr_base<I64_MUL, Mutators...> {
   std::string to_string() { return "i64_mul"; }
};
template <typename ... Mutators>
struct i64_div_s    : instr_base<I64_DIV_S, Mutators...> {
   std::string to_string() { return "i64_div_s"; }
};
template <typename ... Mutators>
struct i64_div_u    : instr_base<I64_DIV_U, Mutators...> {
   std::string to_string() { return "i64_div_u"; }
};
template <typename ... Mutators>
struct i64_rem_s    : instr_base<I64_REM_S, Mutators...> {
   std::string to_string() { return "i64_rem_s"; }
};
template <typename ... Mutators>
struct i64_rem_u    : instr_base<I64_REM_U, Mutators...> {
   std::string to_string() { return "i64_rem_u"; }
};
template <typename ... Mutators>
struct i64_and      : instr_base<I64_AND, Mutators...> {
   std::string to_string() { return "i64_and"; }
};
template <typename ... Mutators>
struct i64_or       : instr_base<I64_OR, Mutators...> {
   std::string to_string() { return "i64_or"; }
};
template <typename ... Mutators>
struct i64_xor      : instr_base<I64_XOR, Mutators...> {
   std::string to_string() { return "i64_xor"; }
};
template <typename ... Mutators>
struct i64_shl      : instr_base<I64_SHL, Mutators...> {
   std::string to_string() { return "i64_shl"; }
};
template <typename ... Mutators>
struct i64_shr_s    : instr_base<I64_SHR_S, Mutators...> {
   std::string to_string() { return "i64_shr_s"; }
};
template <typename ... Mutators>
struct i64_shr_u    : instr_base<I64_SHR_U, Mutators...> {
   std::string to_string() { return "i64_shr_u"; }
};
template <typename ... Mutators>
struct i64_rotl     : instr_base<I64_ROTL, Mutators...> {
   std::string to_string() { return "i64_rotl"; }
};
template <typename ... Mutators>
struct i64_rotr     : instr_base<I64_ROTR, Mutators...> {
   std::string to_string() { return "i64_rotr"; }
};

template <typename ... Mutators>
struct f32_abs      : instr_base<F32_ABS, Mutators...> {
   std::string to_string() { return "f32_abs"; }
};
template <typename ... Mutators>
struct f32_neg      : instr_base<F32_NEG, Mutators...> {
   std::string to_string() { return "f32_neg"; }
};
template <typename ... Mutators>
struct f32_ceil     : instr_base<F32_CEIL, Mutators...> {
   std::string to_string() { return "f32_ceil"; }
};
template <typename ... Mutators>
struct f32_floor    : instr_base<F32_FLOOR, Mutators...> {
   std::string to_string() { return "f32_floor"; }
};
template <typename ... Mutators>
struct f32_trunc    : instr_base<F32_TRUNC, Mutators...> {
   std::string to_string() { return "f32_trunc"; }
};
template <typename ... Mutators>
struct f32_nearest  : instr_base<F32_NEAREST, Mutators...> {
   std::string to_string() { return "f32_nearest"; }
};
template <typename ... Mutators>
struct f32_sqrt     : instr_base<F32_SQRT, Mutators...> {
   std::string to_string() { return "f32_sqrt"; }
};
template <typename ... Mutators>
struct f32_add      : instr_base<F32_ADD, Mutators...> {
   std::string to_string() { return "f32_add"; }
};
template <typename ... Mutators>
struct f32_sub      : instr_base<F32_SUB, Mutators...> {
   std::string to_string() { return "f32_sub"; }
};
template <typename ... Mutators>
struct f32_mul      : instr_base<F32_MUL, Mutators...> {
   std::string to_string() { return "f32_mul"; }
};
template <typename ... Mutators>
struct f32_div      : instr_base<F32_DIV, Mutators...> {
   std::string to_string() { return "f32_div"; }
};
template <typename ... Mutators>
struct f32_min      : instr_base<F32_MIN, Mutators...> {
   std::string to_string() { return "f32_min"; }
};
template <typename ... Mutators>
struct f32_max      : instr_base<F32_MAX, Mutators...> {
   std::string to_string() { return "f32_max"; }
};
template <typename ... Mutators>
struct f32_copysign : instr_base<F32_COPYSIGN, Mutators...> {
   std::string to_string() { return "f32_copysign"; }
};

template <typename ... Mutators>
struct f64_abs      : instr_base<F64_ABS, Mutators...> {
   std::string to_string() { return "f64_abs"; }
};
template <typename ... Mutators>
struct f64_neg      : instr_base<F64_NEG, Mutators...> {
   std::string to_string() { return "f64_neg"; }
};
template <typename ... Mutators>
struct f64_ceil     : instr_base<F64_CEIL, Mutators...> {
   std::string to_string() { return "f64_ceil"; }
};
template <typename ... Mutators>
struct f64_floor    : instr_base<F64_FLOOR, Mutators...> {
   std::string to_string() { return "f64_floor"; }
};
template <typename ... Mutators>
struct f64_trunc    : instr_base<F64_TRUNC, Mutators...> {
   std::string to_string() { return "f64_trunc"; }
};
template <typename ... Mutators>
struct f64_nearest  : instr_base<F64_NEAREST, Mutators...> {
   std::string to_string() { return "f64_nearest"; }
};
template <typename ... Mutators>
struct f64_sqrt     : instr_base<F64_SQRT, Mutators...> {
   std::string to_string() { return "f64_sqrt"; }
};
template <typename ... Mutators>
struct f64_add      : instr_base<F64_ADD, Mutators...> {
   std::string to_string() { return "f64_add"; }
};
template <typename ... Mutators>
struct f64_sub      : instr_base<F64_SUB, Mutators...> {
   std::string to_string() { return "f64_sub"; }
};
template <typename ... Mutators>
struct f64_mul      : instr_base<F64_MUL, Mutators...> {
   std::string to_string() { return "f64_mul"; }
};
template <typename ... Mutators>
struct f64_div      : instr_base<F64_DIV, Mutators...> {
   std::string to_string() { return "f64_div"; }
};
template <typename ... Mutators>
struct f64_min      : instr_base<F64_MIN, Mutators...> {
   std::string to_string() { return "f64_min"; }
};
template <typename ... Mutators>
struct f64_max      : instr_base<F64_MAX, Mutators...> {
   std::string to_string() { return "f64_max"; }
};
template <typename ... Mutators>
struct f64_copysign : instr_base<F64_COPYSIGN, Mutators...> {
   std::string to_string() { return "f64_copysign"; }
};

template <typename ... Mutators>
struct i32_wrap_i64    : instr_base<I32_WRAP_I64, Mutators...> {
   std::string to_string() { return "i32_wrap_i64"; }
};
template <typename ... Mutators>
struct i32_trunc_s_f32 : instr_base<I32_TRUNC_S_F32, Mutators...> {
   std::string to_string() { return "i32_trunc_s_f32"; }
};
template <typename ... Mutators>
struct i32_trunc_u_f32 : instr_base<I32_TRUNC_U_F32, Mutators...> {
   std::string to_string() { return "i32_trunc_u_f32"; }
};
template <typename ... Mutators>
struct i32_trunc_s_f64 : instr_base<I32_TRUNC_S_F64, Mutators...> {
   std::string to_string() { return "i32_trunc_s_f64"; }
};
template <typename ... Mutators>
struct i32_trunc_u_f64 : instr_base<I32_TRUNC_U_F64, Mutators...> {
   std::string to_string() { return "i32_trunc_u_f64"; }
};
template <typename ... Mutators>
struct i64_extend_s_i32 : instr_base<I64_EXTEND_S_I32, Mutators...> {
   std::string to_string() { return "i64_extend_s_i32"; }
};
template <typename ... Mutators>
struct i64_extend_u_i32 : instr_base<I64_EXTEND_U_I32, Mutators...> {
   std::string to_string() { return "i64_extend_u_i32"; }
};
template <typename ... Mutators>
struct i64_trunc_s_f32 : instr_base<I64_TRUNC_S_F32, Mutators...> {
   std::string to_string() { return "i64_trunc_s_f32"; }
};
template <typename ... Mutators>
struct i64_trunc_u_f32 : instr_base<I64_TRUNC_U_F32, Mutators...> {
   std::string to_string() { return "i64_trunc_u_f32"; }
};
template <typename ... Mutators>
struct i64_trunc_s_f64 : instr_base<I64_TRUNC_S_F64, Mutators...> {
   std::string to_string() { return "i64_trunc_s_f64"; }
};
template <typename ... Mutators>
struct i64_trunc_u_f64 : instr_base<I64_TRUNC_U_F64, Mutators...> {
   std::string to_string() { return "i64_trunc_u_f64"; }
};
template <typename ... Mutators>
struct f32_convert_s_i32 : instr_base<F32_CONVERT_S_I32, Mutators...> {
   std::string to_string() { return "f32_convert_s_i32"; }
};
template <typename ... Mutators>
struct f32_convert_u_i32 : instr_base<F32_CONVERT_U_I32, Mutators...> {
   std::string to_string() { return "f32_convert_u_i32"; }
};
template <typename ... Mutators>
struct f32_convert_s_i64 : instr_base<F32_CONVERT_S_I64, Mutators...> {
   std::string to_string() { return "f32_convert_s_i64"; }
};
template <typename ... Mutators>
struct f32_convert_u_i64 : instr_base<F32_CONVERT_U_I64, Mutators...> {
   std::string to_string() { return "f32_convert_u_i64"; }
};
template <typename ... Mutators>
struct f32_demote_f64 : instr_base<F32_DEMOTE_F64, Mutators...> {
   std::string to_string() { return "f32_demote_f64"; }
};
template <typename ... Mutators>
struct f64_convert_s_i32 : instr_base<F64_CONVERT_S_I32, Mutators...> {
   std::string to_string() { return "f64_convert_s_i32"; }
};
template <typename ... Mutators>
struct f64_convert_u_i32 : instr_base<F64_CONVERT_U_I32, Mutators...> {
   std::string to_string() { return "f64_convert_u_i32"; }
};
template <typename ... Mutators>
struct f64_convert_s_i64 : instr_base<F64_CONVERT_S_I64, Mutators...> {
   std::string to_string() { return "f64_convert_s_i64"; }
};
template <typename ... Mutators>
struct f64_convert_u_i64 : instr_base<F64_CONVERT_U_I64, Mutators...> {
   std::string to_string() { return "f64_convert_u_i64"; }
};
template <typename ... Mutators>
struct f64_promote_f32 : instr_base<F64_PROMOTE_F32, Mutators...> {
   std::string to_string() { return "f64_promote_f32"; }
};
template <typename ... Mutators>
struct i32_reinterpret_f32 : instr_base<I32_REINTERPRET_F32, Mutators...> {
   std::string to_string() { return "i32_reinterpret_f32"; }
};
template <typename ... Mutators>
struct i64_reinterpret_f64 : instr_base<I64_REINTERPRET_F64, Mutators...> {
   std::string to_string() { return "i64_reinterpret_f64"; }
};
template <typename ... Mutators>
struct f32_reinterpret_i32 : instr_base<F32_REINTERPRET_I32, Mutators...> {
   std::string to_string() { return "f32_reinterpret_i32"; }
};
template <typename ... Mutators>
struct f64_reinterpret_i64 : instr_base<F64_REINTERPRET_I64, Mutators...> {
   std::string to_string() { return "f64_reinterpret_i64"; }
};

// expressions
template <size_t Instrs>
struct expr {
   std::array<instr*, Instrs> in;
   uint8_t end = END;
};
#pragma pack (pop)

struct nop_mutator {
   static void accept( instr* inst ) {}
};

// class to inherit from to attach specific mutators and have default behavior for all specified types
template < typename Mutator = nop_mutator, typename ... Mutators>
struct op_types {
   using end_t = end< Mutator, Mutators...>;
   using unreachable_t = unreachable< Mutator, Mutators...>;
   using nop_t = nop<Mutator, Mutators...>;
   using block_t = block<Mutator, Mutators...>;
   using loop_t = loop<Mutator, Mutators...>;
   using if_eps_t = if_eps<Mutator, Mutators...>;
   using if_else_t = if_else<Mutator, Mutators...>;
   using br_t = br<Mutator, Mutators...>;
   using br_if_t = br_if<Mutator, Mutators...>;
   using br_table_t = br_table<Mutator, Mutators...>;
   using return_t = ret<Mutator, Mutators...>;
   using call_t = call<Mutator, Mutators...>;
   using call_indirect_t = call_indirect<Mutator, Mutators...>;

   using drop_t = drop<Mutator, Mutators...>;
   using select_t = select<Mutator, Mutators...>;

   using get_local_t = get_local<Mutator, Mutators...>;
   using set_local_t = set_local<Mutator, Mutators...>;
   using tee_local_t = tee_local<Mutator, Mutators...>;
   using get_global_t = get_global<Mutator, Mutators...>;
   using set_global_t = set_global<Mutator, Mutators...>;

   using i32_load_t = i32_load<Mutator, Mutators...>;
   using i64_load_t = i64_load<Mutator, Mutators...>;
   using f32_load_t = f32_load<Mutator, Mutators...>;
   using f64_load_t = f64_load<Mutator, Mutators...>;
   using i32_load8_s_t = i32_load8_s<Mutator, Mutators...>;
   using i32_load8_u_t = i32_load8_u<Mutator, Mutators...>;
   using i32_load16_s_t = i32_load16_s<Mutator, Mutators...>;
   using i32_load16_u_t = i32_load16_u<Mutator, Mutators...>;
   using i64_load8_s_t = i64_load8_s<Mutator, Mutators...>;
   using i64_load8_u_t = i64_load8_u<Mutator, Mutators...>;
   using i64_load16_s_t = i64_load16_s<Mutator, Mutators...>;
   using i64_load16_u_t = i64_load16_u<Mutator, Mutators...>;
   using i64_load32_s_t = i64_load32_s<Mutator, Mutators...>;
   using i64_load32_u_t = i64_load32_u<Mutator, Mutators...>;
   using i32_store_t = i32_store<Mutator, Mutators...>;
   using i64_store_t = i64_store<Mutator, Mutators...>;
   using f32_store_t = f32_store<Mutator, Mutators...>;
   using f64_store_t = f64_store<Mutator, Mutators...>;
   using i32_store8_t = i32_store8<Mutator, Mutators...>;
   using i32_store16_t = i32_store16<Mutator, Mutators...>;
   using i64_store8_t = i64_store8<Mutator, Mutators...>;
   using i64_store16_t = i64_store16<Mutator, Mutators...>;
   using i64_store32_t = i64_store32<Mutator, Mutators...>;
   using current_memory_t = current_memory<Mutator, Mutators...>;
   using grow_memory_t = grow_memory<Mutator, Mutators...>;

   using i32_const_t = i32_const<Mutator, Mutators...>;
   using i64_const_t = i64_const<Mutator, Mutators...>;
   using f32_const_t = f32_const<Mutator, Mutators...>;
   using f64_const_t = f64_const<Mutator, Mutators...>;

   using i32_eqz_t = i32_eqz<Mutator, Mutators...>;
   using i32_eq_t = i32_eq<Mutator, Mutators...>;
   using i32_ne_t = i32_ne<Mutator, Mutators...>;
   using i32_lt_s_t = i32_lt_s<Mutator, Mutators...>;
   using i32_lt_u_t = i32_lt_u<Mutator, Mutators...>;
   using i32_gt_s_t = i32_gt_s<Mutator, Mutators...>;
   using i32_gt_u_t = i32_gt_u<Mutator, Mutators...>;
   using i32_le_s_t = i32_le_s<Mutator, Mutators...>;
   using i32_le_u_t = i32_le_u<Mutator, Mutators...>;
   using i32_ge_s_t = i32_ge_s<Mutator, Mutators...>;
   using i32_ge_u_t = i32_ge_u<Mutator, Mutators...>;

   using i64_eqz_t = i64_eqz<Mutator, Mutators...>;
   using i64_eq_t = i64_eq<Mutator, Mutators...>;
   using i64_ne_t = i64_ne<Mutator, Mutators...>;
   using i64_lt_s_t = i64_lt_s<Mutator, Mutators...>;
   using i64_lt_u_t = i64_lt_u<Mutator, Mutators...>;
   using i64_gt_s_t = i64_gt_s<Mutator, Mutators...>;
   using i64_gt_u_t = i64_gt_u<Mutator, Mutators...>;
   using i64_le_s_t = i64_le_s<Mutator, Mutators...>;
   using i64_le_u_t = i64_le_u<Mutator, Mutators...>;
   using i64_ge_s_t = i64_ge_s<Mutator, Mutators...>;
   using i64_ge_u_t = i64_ge_u<Mutator, Mutators...>;

   using f32_eq_t = f32_eq<Mutator, Mutators...>;
   using f32_ne_t = f32_ne<Mutator, Mutators...>;
   using f32_lt_t = f32_lt<Mutator, Mutators...>;
   using f32_gt_t = f32_gt<Mutator, Mutators...>;
   using f32_le_t = f32_le<Mutator, Mutators...>;
   using f32_ge_t = f32_ge<Mutator, Mutators...>;

   using f64_eq_t = f64_eq<Mutator, Mutators...>;
   using f64_ne_t = f64_ne<Mutator, Mutators...>;
   using f64_lt_t = f64_lt<Mutator, Mutators...>;
   using f64_gt_t = f64_gt<Mutator, Mutators...>;
   using f64_le_t = f64_le<Mutator, Mutators...>;
   using f64_ge_t = f64_ge<Mutator, Mutators...>;
   
   using i32_clz_t = i32_clz<Mutator, Mutators...>;
   using i32_ctz_t = i32_ctz<Mutator, Mutators...>;
   using i32_popcount_t = i32_popcount<Mutator, Mutators...>;
   using i32_add_t = i32_add<Mutator, Mutators...>;
   using i32_sub_t = i32_sub<Mutator, Mutators...>;
   using i32_mul_t = i32_mul<Mutator, Mutators...>;
   using i32_div_s_t = i32_div_s<Mutator, Mutators...>;
   using i32_div_u_t = i32_div_u<Mutator, Mutators...>;
   using i32_rem_s_t = i32_rem_s<Mutator, Mutators...>;
   using i32_rem_u_t = i32_rem_u<Mutator, Mutators...>;
   using i32_and_t = i32_and<Mutator, Mutators...>;
   using i32_or_t = i32_or<Mutator, Mutators...>;
   using i32_xor_t = i32_xor<Mutator, Mutators...>;
   using i32_shl_t = i32_shl<Mutator, Mutators...>;
   using i32_shr_s_t = i32_shr_s<Mutator, Mutators...>;
   using i32_shr_u_t = i32_shr_u<Mutator, Mutators...>;
   using i32_rotl_t = i32_rotl<Mutator, Mutators...>;
   using i32_rotr_t = i32_rotr<Mutator, Mutators...>;

   using i64_clz_t = i64_clz<Mutator, Mutators...>;
   using i64_ctz_t = i64_ctz<Mutator, Mutators...>;
   using i64_popcount_t = i64_popcount<Mutator, Mutators...>;
   using i64_add_t = i64_add<Mutator, Mutators...>;
   using i64_sub_t = i64_sub<Mutator, Mutators...>;
   using i64_mul_t = i64_mul<Mutator, Mutators...>;
   using i64_div_s_t = i64_div_s<Mutator, Mutators...>;
   using i64_div_u_t = i64_div_u<Mutator, Mutators...>;
   using i64_rem_s_t = i64_rem_s<Mutator, Mutators...>;
   using i64_rem_u_t = i64_rem_u<Mutator, Mutators...>;
   using i64_and_t = i64_and<Mutator, Mutators...>;
   using i64_or_t = i64_or<Mutator, Mutators...>;
   using i64_xor_t = i64_xor<Mutator, Mutators...>;
   using i64_shl_t = i64_shl<Mutator, Mutators...>;
   using i64_shr_s_t = i64_shr_s<Mutator, Mutators...>;
   using i64_shr_u_t = i64_shr_u<Mutator, Mutators...>;
   using i64_rotl_t = i64_rotl<Mutator, Mutators...>;
   using i64_rotr_t = i64_rotr<Mutator, Mutators...>;

   using f32_abs_t = f32_abs<Mutator, Mutators...>;
   using f32_neg_t = f32_neg<Mutator, Mutators...>;
   using f32_ceil_t = f32_ceil<Mutator, Mutators...>;
   using f32_floor_t = f32_floor<Mutator, Mutators...>;
   using f32_trunc_t = f32_trunc<Mutator, Mutators...>;
   using f32_nearest_t = f32_nearest<Mutator, Mutators...>;
   using f32_sqrt_t = f32_sqrt<Mutator, Mutators...>;
   using f32_add_t = f32_add<Mutator, Mutators...>;
   using f32_sub_t = f32_sub<Mutator, Mutators...>;
   using f32_mul_t = f32_mul<Mutator, Mutators...>;
   using f32_div_t = f32_div<Mutator, Mutators...>;
   using f32_min_t = f32_min<Mutator, Mutators...>;
   using f32_max_t = f32_max<Mutator, Mutators...>;
   using f32_copysign_t = f32_copysign<Mutator, Mutators...>;

   using f64_abs_t = f64_abs<Mutator, Mutators...>;
   using f64_neg_t = f64_neg<Mutator, Mutators...>;
   using f64_ceil_t = f64_ceil<Mutator, Mutators...>;
   using f64_floor_t = f64_floor<Mutator, Mutators...>;
   using f64_trunc_t = f64_trunc<Mutator, Mutators...>;
   using f64_nearest_t = f64_nearest<Mutator, Mutators...>;
   using f64_sqrt_t = f64_sqrt<Mutator, Mutators...>;
   using f64_add_t = f64_add<Mutator, Mutators...>;
   using f64_sub_t = f64_sub<Mutator, Mutators...>;
   using f64_mul_t = f64_mul<Mutator, Mutators...>;
   using f64_div_t = f64_div<Mutator, Mutators...>;
   using f64_min_t = f64_min<Mutator, Mutators...>;
   using f64_max_t = f64_max<Mutator, Mutators...>;
   using f64_copysign_t = f64_copysign<Mutator, Mutators...>;

   using i32_wrap_i64_t = i32_wrap_i64<Mutator, Mutators...>;
   using i32_trunc_s_f32_t = i32_trunc_s_f32<Mutator, Mutators...>;
   using i32_trunc_u_f32_t = i32_trunc_u_f32<Mutator, Mutators...>;
   using i32_trunc_s_f64_t = i32_trunc_s_f64<Mutator, Mutators...>;
   using i32_trunc_u_f64_t = i32_trunc_u_f64<Mutator, Mutators...>;
   using i64_extend_s_i32_t = i64_extend_s_i32<Mutator, Mutators...>;
   using i64_extend_u_i32_t = i64_extend_u_i32<Mutator, Mutators...>;
   using i64_trunc_s_f32_t = i64_trunc_s_f32<Mutator, Mutators...>;
   using i64_trunc_u_f32_t = i64_trunc_u_f32<Mutator, Mutators...>;
   using i64_trunc_s_f64_t = i64_trunc_s_f64<Mutator, Mutators...>;
   using i64_trunc_u_f64_t = i64_trunc_u_f64<Mutator, Mutators...>;
   using f32_convert_s_i32_t = f32_convert_s_i32<Mutator, Mutators...>;
   using f32_convert_u_i32_t = f32_convert_u_i32<Mutator, Mutators...>;
   using f32_convert_s_i64_t = f32_convert_s_i64<Mutator, Mutators...>;
   using f32_convert_u_i64_t = f32_convert_u_i64<Mutator, Mutators...>;
   using f32_demote_f64_t = f32_demote_f64<Mutator, Mutators...>;
   using f64_convert_s_i32_t = f64_convert_s_i32<Mutator, Mutators...>;
   using f64_convert_u_i32_t = f64_convert_u_i32<Mutator, Mutators...>;
   using f64_convert_s_i64_t = f64_convert_s_i64<Mutator, Mutators...>;
   using f64_convert_u_i64_t = f64_convert_u_i64<Mutator, Mutators...>;
   using f64_promote_f32_t = f64_promote_f32<Mutator, Mutators...>;
   using i32_reinterpret_f32_t = i32_reinterpret_f32<Mutator, Mutators...>;
   using i64_reinterpret_f64_t = i64_reinterpret_f64<Mutator, Mutators...>;
   using f32_reinterpret_i32_t = f32_reinterpret_i32<Mutator, Mutators...>;
   using f64_reinterpret_i64_t = f64_reinterpret_i64<Mutator, Mutators...>;
}; // op_types

// function to get cast a memory chunk to the correct type
template <class Op_Types>
wasm_op_ptr get_wasm_op_ptr( char* code ) {
   switch ((uint8_t)*code) {
      case END:
         return reinterpret_cast<*Op_Types::end_t>(code);
      case UNREACHABLE:
         return reinterpret_cast<*Op_Types::unreachable_t>(code);
      case NOP:
         return reinterpret_cast<*Op_Types::nop_t>(code);
      case BLOCK:
         return reinterpret_cast<Op_Types::block_t>(code);
      case LOOP:
         return reinterpret_cast<Op_Types::loop_t>(code);
      case IF:
         return reinterpret_cast<Op_Types::if_eps_t>(code);
      case ELSE:
         return reinterpret_cast<Op_Types::if_else_t>(code);
      case BR:
         return reinterpret_cast<Op_Types::br_t>(code);
      case BR_IF:
         return reinterpret_cast<Op_Types::br_if_t>(code);
      case BR_TABLE:
         return reinterpret_cast<Op_Types::br_table_t>(code);
      case RETURN:
         return reinterpret_cast<Op_Types::return_t>(code);
      case CALL:
         return reinterpret_cast<Op_Types::call_t>(code);
      case CALL_INDIRECT:
         return reinterpret_cast<Op_Types::call_indirect_t>(code);
      case DROP:
         return reinterpret_cast<Op_Types::drop_t>(code);
      case SELECT:
         return reinterpret_cast<Op_Types::select_t>(code);

      case GET_LOCAL:
         return reinterpret_cast<Op_Types::get_local_t>(code);
      case SET_LOCAL:
         return reinterpret_cast<Op_Types::set_local_t>(code);
      case TEE_LOCAL:
         return reinterpret_cast<Op_Types::tee_local_t>(code);
      case GET_GLOBAL:
         return reinterpret_cast<Op_Types::get_global_t>(code);
      case SET_GLOBAL:
         return reinterpret_cast<Op_Types::set_global_t>(code);

      case I32_LOAD:
         return reinterpret_cast<Op_Types::i32_load_t>(code);
      case I64_LOAD:
         return reinterpret_cast<Op_Types::i64_load_t>(code);
      case F32_LOAD:
         return reinterpret_cast<Op_Types::f32_load_t>(code);
      case F64_LOAD:
         return reinterpret_cast<Op_Types::f64_load_t>(code);
      case I32_LOAD8_S:
         return reinterpret_cast<Op_Types::i32_load8_s_t>(code);
      case I32_LOAD8_U:
         return reinterpret_cast<Op_Types::i32_load8_u_t>(code);
      case I32_LOAD16_S:
         return reinterpret_cast<Op_Types::i32_load16_s_t>(code);
      case I32_LOAD16_U:
         return reinterpret_cast<Op_Types::i32_load16_u_t>(code);
      case I64_LOAD8_S:
         return reinterpret_cast<Op_Types::i64_load8_s_t>(code);
      case I64_LOAD8_U:
         return reinterpret_cast<Op_Types::i64_load8_u_t>(code);
      case I64_LOAD16_S:
         return reinterpret_cast<Op_Types::i64_load16_s_t>(code);
      case I64_LOAD16_U:
         return reinterpret_cast<Op_Types::i64_load16_u_t>(code);
      case I64_LOAD32_S:
         return reinterpret_cast<Op_Types::i64_load32_s_t>(code);
      case I64_LOAD32_U:
         return reinterpret_cast<Op_Types::i64_load32_u_t>(code);
      case I32_STORE:
         return reinterpret_cast<Op_Types::i32_store_t>(code);
      case I64_STORE:
         return reinterpret_cast<Op_Types::i64_store_t>(code);
      case F32_STORE:
         return reinterpret_cast<Op_Types::f32_store_t>(code);
      case F64_STORE:
         return reinterpret_cast<Op_Types::f64_store_t>(code);
      case I32_STORE8:
         return reinterpret_cast<Op_Types::i32_store8_t>(code);
      case I32_STORE16:
         return reinterpret_cast<Op_Types::i32_store16_t>(code);
      case I64_STORE8:
         return reinterpret_cast<Op_Types::i64_store8_t>(code);
      case I64_STORE16:
         return reinterpret_cast<Op_Types::i64_store16_t>(code);
      case I64_STORE32:
         return reinterpret_cast<Op_Types::i64_store32_t>(code);
      case CURRENT_MEM:
         return reinterpret_cast<Op_Types::current_memory_t>(code);
      case GROW_MEM:
         return reinterpret_cast<Op_Types::grow_memory_t>(code);

      case I32_CONST:
         return reinterpret_cast<Op_Types::i32_const_t>(code);
      case I64_CONST:
         return reinterpret_cast<Op_Types::i64_const_t>(code);
      case F32_CONST:
         return reinterpret_cast<Op_Types::f32_const_t>(code);
      case F64_CONST:
         return reinterpret_cast<Op_Types::f64_const_t>(code);

      case I32_EQZ:
         return reinterpret_cast<Op_Types::i32_eqz_t>(code);
      case I32_EQ:
         return reinterpret_cast<Op_Types::i32_eq_t>(code);
      case I32_NE:
         return reinterpret_cast<Op_Types::i32_ne_t>(code);
      case I32_LT_S:
         return reinterpret_cast<Op_Types::i32_lt_s_t>(code);
      case I32_LT_U:
         return reinterpret_cast<Op_Types::i32_lt_u_t>(code);
      case I32_GT_S:
         return reinterpret_cast<Op_Types::i32_gt_s_t>(code);
      case I32_GT_U:
         return reinterpret_cast<Op_Types::i32_gt_u_t>(code);
      case I32_LE_S:
         return reinterpret_cast<Op_Types::i32_le_s_t>(code);
      case I32_LE_U:
         return reinterpret_cast<Op_Types::i32_le_u_t>(code);
      case I32_GE_S:
         return reinterpret_cast<Op_Types::i32_ge_s_t>(code);
      case I32_GE_U:
         return reinterpret_cast<Op_Types::i32_ge_u_t>(code);

      case I64_EQZ:
         return reinterpret_cast<Op_Types::i64_eqz_t>(code);
      case I64_EQ:
         return reinterpret_cast<Op_Types::i64_eq_t>(code);
      case I64_NE:
         return reinterpret_cast<Op_Types::i64_ne_t>(code);
      case I64_LT_S:
         return reinterpret_cast<Op_Types::i64_lt_s_t>(code);
      case I64_LT_U:
         return reinterpret_cast<Op_Types::i64_lt_u_t>(code);
      case I64_GT_S:
         return reinterpret_cast<Op_Types::i64_gt_s_t>(code);
      case I64_GT_U:
         return reinterpret_cast<Op_Types::i64_gt_u_t>(code);
      case I64_LE_S:
         return reinterpret_cast<Op_Types::i64_le_s_t>(code);
      case I64_LE_U:
         return reinterpret_cast<Op_Types::i64_le_u_t>(code);
      case I64_GE_S:
         return reinterpret_cast<Op_Types::i64_ge_s_t>(code);
      case I64_GE_U:
         return reinterpret_cast<Op_Types::i64_ge_u_t>(code);

      case F32_EQ:
         return reinterpret_cast<Op_Types::f32_eq_t>(code);
      case F32_NE:
         return reinterpret_cast<Op_Types::f32_ne_t>(code);
      case F32_LT:
         return reinterpret_cast<Op_Types::f32_lt_t>(code);
      case F32_GT:
         return reinterpret_cast<Op_Types::f32_gt_t>(code);
      case F32_LE:
         return reinterpret_cast<Op_Types::f32_le_t>(code);
      case F32_GE:
         return reinterpret_cast<Op_Types::f32_ge_t>(code);

      case F64_EQ:
         return reinterpret_cast<Op_Types::f64_eq_t>(code);
      case F64_NE:
         return reinterpret_cast<Op_Types::f64_ne_t>(code);
      case F64_LT:
         return reinterpret_cast<Op_Types::f64_lt_t>(code);
      case F64_GT:
         return reinterpret_cast<Op_Types::f64_gt_t>(code);
      case F64_LE:
         return reinterpret_cast<Op_Types::f64_le_t>(code);
      case F64_GE:
         return reinterpret_cast<Op_Types::f64_ge_t>(code);
      
      case I32_CLZ:
         return reinterpret_cast<Op_Types::i32_clz_t>(code);
      case I32_CTZ:
         return reinterpret_cast<Op_Types::i32_ctz_t>(code);
      case I32_POPCOUNT:
         return reinterpret_cast<Op_Types::i32_popcount_t>(code);
      case I32_ADD:
         return reinterpret_cast<Op_Types::i32_add_t>(code);
      case I32_SUB:
         return reinterpret_cast<Op_Types::i32_sub_t>(code);
      case I32_MUL:
         return reinterpret_cast<Op_Types::i32_mul_t>(code);
      case I32_DIV_S:
         return reinterpret_cast<Op_Types::i32_div_s_t>(code);
      case I32_DIV_U:
         return reinterpret_cast<Op_Types::i32_div_u_t>(code);
      case I32_REM_S:
         return reinterpret_cast<Op_Types::i32_rem_s_t>(code);
      case I32_REM_U:
         return reinterpret_cast<Op_Types::i32_rem_u_t>(code);
      case I32_AND:
         return reinterpret_cast<Op_Types::i32_and_t>(code);
      case I32_OR:
         return reinterpret_cast<Op_Types::i32_or_t>(code);
      case I32_XOR:
         return reinterpret_cast<Op_Types::i32_xor_t>(code);
      case I32_SHL:
         return reinterpret_cast<Op_Types::i32_shl_t>(code);
      case I32_SHR_S:
         return reinterpret_cast<Op_Types::i32_shr_s_t>(code);
      case I32_SHR_U:
         return reinterpret_cast<Op_Types::i32_shr_u_t>(code);
      case I32_ROTL:
         return reinterpret_cast<Op_Types::i32_rotl_t>(code);
      case I32_ROTR:
         return reinterpret_cast<Op_Types::i32_rotr_t>(code);

      case I64_CLZ:
         return reinterpret_cast<Op_Types::i64_clz_t>(code);
      case I64_CTZ:
         return reinterpret_cast<Op_Types::i64_ctz_t>(code);
      case I64_POPCOUNT:
         return reinterpret_cast<Op_Types::i64_popcount_t>(code);
      case I64_ADD:
         return reinterpret_cast<Op_Types::i64_add_t>(code);
      case I64_SUB:
         return reinterpret_cast<Op_Types::i64_sub_t>(code);
      case I64_MUL:
         return reinterpret_cast<Op_Types::i64_mul_t>(code);
      case I64_DIV_S:
         return reinterpret_cast<Op_Types::i64_div_s_t>(code);
      case I64_DIV_U:
         return reinterpret_cast<Op_Types::i64_div_u_t>(code);
      case I64_REM_S:
         return reinterpret_cast<Op_Types::i64_rem_s_t>(code);
      case I64_REM_U:
         return reinterpret_cast<Op_Types::i64_rem_u_t>(code);
      case I64_AND:
         return reinterpret_cast<Op_Types::i64_and_t>(code);
      case I64_OR:
         return reinterpret_cast<Op_Types::i64_or_t>(code);
      case I64_XOR:
         return reinterpret_cast<Op_Types::i64_xor_t>(code);
      case I64_SHL:
         return reinterpret_cast<Op_Types::i64_shl_t>(code);
      case I64_SHR_S:
         return reinterpret_cast<Op_Types::i64_shr_s_t>(code);
      case I64_SHR_U:
         return reinterpret_cast<Op_Types::i64_shr_u_t>(code);
      case I64_ROTL:
         return reinterpret_cast<Op_Types::i64_rotl_t>(code);
      case I64_ROTR:
         return reinterpret_cast<Op_Types::i64_rotr_t>(code);

      case F32_ABS:
         return reinterpret_cast<Op_Types::f32_abs_t>(code);
      case F32_NEG:
         return reinterpret_cast<Op_Types::f32_neg_t>(code);
      case F32_CEIL:
         return reinterpret_cast<Op_Types::f32_ceil_t>(code);
      case F32_FLOOR:
         return reinterpret_cast<Op_Types::f32_floor_t>(code);
      case F32_TRUNC:
         return reinterpret_cast<Op_Types::f32_trunc_t>(code);
      case F32_NEAREST:
         return reinterpret_cast<Op_Types::f32_nearest_t>(code);
      case F32_SQRT:
         return reinterpret_cast<Op_Types::f32_sqrt_t>(code);
      case F32_ADD:
         return reinterpret_cast<Op_Types::f32_add_t>(code);
      case F32_SUB:
         return reinterpret_cast<Op_Types::f32_sub_t>(code);
      case F32_MUL:
         return reinterpret_cast<Op_Types::f32_mul_t>(code);
      case F32_DIV:
         return reinterpret_cast<Op_Types::f32_div_t>(code);
      case F32_MIN:
         return reinterpret_cast<Op_Types::f32_min_t>(code);
      case F32_MAX:
         return reinterpret_cast<Op_Types::f32_max_t>(code);
      case F32_COPYSIGN:
         return reinterpret_cast<Op_Types::f32_copysign_t>(code);

      case F64_ABS:
         return reinterpret_cast<Op_Types::f64_abs_t>(code);
      case F64_NEG:
         return reinterpret_cast<Op_Types::f64_neg_t>(code);
      case F64_CEIL:
         return reinterpret_cast<Op_Types::f64_ceil_t>(code);
      case F64_FLOOR:
         return reinterpret_cast<Op_Types::f64_floor_t>(code);
      case F64_TRUNC:
         return reinterpret_cast<Op_Types::f64_trunc_t>(code);
      case F64_NEAREST:
         return reinterpret_cast<Op_Types::f64_nearest_t>(code);
      case F64_SQRT:
         return reinterpret_cast<Op_Types::f64_sqrt_t>(code);
      case F64_ADD:
         return reinterpret_cast<Op_Types::f64_add_t>(code);
      case F64_SUB:
         return reinterpret_cast<Op_Types::f64_sub_t>(code);
      case F64_MUL:
         return reinterpret_cast<Op_Types::f64_mul_t>(code);
      case F64_DIV:
         return reinterpret_cast<Op_Types::f64_div_t>(code);
      case F64_MIN:
         return reinterpret_cast<Op_Types::f64_min_t>(code);
      case F64_MAX:
         return reinterpret_cast<Op_Types::f64_max_t>(code);
      case F64_COPYSIGN:
         return reinterpret_cast<Op_Types::f64_copysign_t>(code);

      case I32_WRAP_I64:
         return reinterpret_cast<Op_Types::i32_wrap_i64_t>(code);
      case I32_TRUNC_S_F32:
         return reinterpret_cast<Op_Types::i32_trunc_s_f32_t>(code);
      case I32_TRUNC_U_F32:
         return reinterpret_cast<Op_Types::i32_trunc_u_f32_t>(code);
      case I32_TRUNC_S_F64:
         return reinterpret_cast<Op_Types::i32_trunc_s_f64_t>(code);
      case I32_TRUNC_U_F64:
         return reinterpret_cast<Op_Types::i32_trunc_u_f64_t>(code);
      case I64_EXTEND_S_I32:
         return reinterpret_cast<Op_Types::i64_extend_s_i32_t>(code);
      case I64_EXTEND_U_I32:
         return reinterpret_cast<Op_Types::i64_extend_u_i32_t>(code);
      case I64_TRUNC_S_F32:
         return reinterpret_cast<Op_Types::i64_trunc_s_f32_t>(code);
      case I64_TRUNC_U_F32:
         return reinterpret_cast<Op_Types::i64_trunc_u_f32_t>(code);
      case I64_TRUNC_S_F64:
         return reinterpret_cast<Op_Types::i64_trunc_s_f64_t>(code);
      case I64_TRUNC_U_F64:
         return reinterpret_cast<Op_Types::i64_trunc_u_f64_t>(code);
      case F32_CONVERT_S_I32:
         return reinterpret_cast<Op_Types::f32_convert_s_i32_t>(code);
      case F32_CONVERT_U_I32:
         return reinterpret_cast<Op_Types::f32_convert_u_i32_t>(code);
      case F32_CONVERT_S_I64:
         return reinterpret_cast<Op_Types::f32_convert_s_i64_t>(code);
      case F32_CONVERT_U_I64:
         return reinterpret_cast<Op_Types::f32_convert_u_i64_t>(code);
      case F32_DEMOTE_F64:
         return reinterpret_cast<Op_Types::f32_demote_f64_t>(code);
      case F64_CONVERT_S_I32:
         return reinterpret_cast<Op_Types::f64_convert_s_i32_t>(code);
      case F64_CONVERT_U_I32:
         return reinterpret_cast<Op_Types::f64_convert_u_i32_t>(code);
      case F64_CONVERT_S_I64:
         return reinterpret_cast<Op_Types::f64_convert_s_i64_t>(code);
      case F64_CONVERT_U_I64:
         return reinterpret_cast<Op_Types::f64_convert_u_i64_t>(code);
      case F64_PROMOTE_F32:
         return reinterpret_cast<Op_Types::f64_promote_f32_t>(code);
      case I32_REINTERPRET_F32:
         return reinterpret_cast<Op_Types::i32_reinterpret_f32_t>(code);
      case I64_REINTERPRET_F64:
         return reinterpret_cast<Op_Types::i64_reinterpret_f64_t>(code);
      case F32_REINTERPRET_I32:
         return reinterpret_cast<Op_Types::f32_reinterpret_i32_t>(code);
      case F64_REINTERPRET_I64:
         return reinterpret_cast<Op_Types::f64_reinterpret_i64_t>(code);

   }
}

}}} // namespace eosio, chain, wasm_ops

