#pragma once

#include <stdint.h>
#include <vector>
#include <memory>
#include <functional>
#include <iterator>
#include <utility>
#include <string>
#include <unordered_set>

namespace eosio { namespace chain { namespace wasm_ops {

// forward declaration
struct instr;

using wasm_instr_ptr      = std::shared_ptr<instr>;
using wasm_instr_callback = std::function<std::vector<wasm_instr_ptr>(uint8_t)>;
using code_iterator       = std::vector<uint8_t>::iterator;

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
};

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
   std::string to_string() { return "instr"; }
   /*
   std::vector<wasm_instr_ptr> visit( code_iterator start, code_iterator end ) {
      return std::vector<wasm_instr_ptr>();
   }
   */
};

template <uint8_t Code, typename ... Mutators>
struct instr_base : instr {
   instr_base() { code = Code; }
   void visit( code_iterator start, code_iterator end ) {
      for ( auto m : { Mutators()... } )
         m.accept();
   } 
};

// control insructions
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

template <typename ... Mutators>
struct block : instr_base<BLOCK, Mutators...> {
   block( std::vector<uint8_t> body ) : in(body) {}
   blocktype rt;
   std::vector<uint8_t> in;
   uint8_t end = END;

   std::string to_string() { return "block"; }
};

template <typename ... Mutators>
struct loop : instr_base<LOOP, Mutators...> {
   loop( std::vector<uint8_t> body ) : in(body) {}
   blocktype rt;
   std::vector<uint8_t> in;
   uint8_t end = END;
};

template <typename ... Mutators>
struct if_eps : instr_base<IF, Mutators...> {
   if_eps( std::vector<uint8_t> body ) : in(body) {}
   blocktype rt;
   std::vector<uint8_t> in;
   uint8_t end = END;
};

template <typename ... Mutators>
struct if_else : instr_base<IF, Mutators...> {
   if_else( std::vector<uint8_t> ifbody, std::vector<uint8_t> elsebody ) : in1(ifbody), in2(elsebody) {}
   blocktype rt;
   std::vector<uint8_t> in1;
   uint8_t _else = ELSE;
   std::vector<uint8_t> in2;
   uint8_t end = END;
};

template <typename ... Mutators>
struct br : instr_base<BR, Mutators...> {
   uint32_t l; // label to branch to
};

template <typename ... Mutators>
struct br_if : instr_base<BR_IF, Mutators...> {
   uint32_t l; // label to branch to
};

template <size_t Labels, typename ... Mutators>
struct br_table : instr_base<BR_TABLE, Mutators...> {
   std::array<uint32_t, Labels> l;
   uint32_t lN;
};

template <typename ... Mutators>
struct call : instr_base<CALL, Mutators...> {
   uint32_t funcidx;
};

template <typename ... Mutators>
struct call_indirect : instr_base<CALL_INDIRECT, Mutators...> {
   uint32_t funcidx;
   uint8_t end = UNREACHABLE;
};

// parametric instructions
template <typename ... Mutators>
struct drop : instr_base<DROP, Mutators...> {};

template <typename ... Mutators>
struct select : instr_base<SELECT, Mutators...> {};

// variable instructions
template <typename ... Mutators>
struct get_local : instr_base<GET_LOCAL, Mutators...> {
   uint32_t localidx;
};

template <typename ... Mutators>
struct set_local : instr_base<SET_LOCAL, Mutators...> {
   uint32_t localidx;
};

template <typename ... Mutators>
struct tee_local : instr_base<TEE_LOCAL, Mutators...> {
   uint32_t localidx;
};

template <typename ... Mutators>
struct get_global : instr_base<GET_GLOBAL, Mutators...> {
   uint32_t localidx;
};

template <typename ... Mutators>
struct set_global : instr_base<SET_GLOBAL, Mutators...> {
   uint32_t localidx;
};

// memory instructions
template <typename ... Mutators>
struct i32_load : instr_base<I32_LOAD, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_load : instr_base<I64_LOAD, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct f32_load : instr_base<F32_LOAD, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct f64_load : instr_base<F64_LOAD, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i32_load8_s : instr_base<I32_LOAD8_S, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i32_load8_u : instr_base<I32_LOAD8_U, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i32_load16_s : instr_base<I32_LOAD16_S, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i32_load16_u : instr_base<I32_LOAD16_U, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_load8_s : instr_base<I64_LOAD8_S, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_load8_u : instr_base<I64_LOAD8_U, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_load16_s : instr_base<I64_LOAD16_S, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_load16_u : instr_base<I64_LOAD16_U, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_load32_s : instr_base<I64_LOAD32_S, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_load32_u : instr_base<I64_LOAD32_U, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i32_store : instr_base<I32_STORE, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_store : instr_base<I64_STORE, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct f32_store : instr_base<F32_STORE, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct f64_store : instr_base<F64_STORE, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i32_store8 : instr_base<I32_STORE8, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i32_store16 : instr_base<I32_STORE16, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_store8 : instr_base<I64_STORE8, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_store16 : instr_base<I64_STORE16, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct i64_store32 : instr_base<I64_STORE32, Mutators...> {
   memarg m;
};

template <typename ... Mutators>
struct current_memory : instr_base<CURRENT_MEM, Mutators...> {
   uint8_t end = UNREACHABLE;
};

template <typename ... Mutators>
struct grow_memory : instr_base<GROW_MEM> {
   uint8_t end = UNREACHABLE;
};

// numeric instructions
struct i32_const : instr_base<I32_CONST> {
   uint32_t n;
};

struct f32_const : instr_base<F32_CONST> {
   uint32_t n;
};

struct i64_const : instr_base<I64_CONST> {
   uint32_t n;
};

struct f64_const : instr_base<F64_CONST> {
   uint32_t n;
};

struct i32_eqz  : instr_base<I32_EQZ> {};
struct i32_eq   : instr_base<I32_EQ> {};
struct i32_ne   : instr_base<I32_NE> {};
struct i32_lt_s : instr_base<I32_LT_S> {};
struct i32_lt_u : instr_base<I32_LT_U> {};
struct i32_gt_s : instr_base<I32_GT_S> {};
struct i32_gt_u : instr_base<I32_GT_U> {};
struct i32_le_s : instr_base<I32_LE_S> {};
struct i32_le_u : instr_base<I32_LE_U> {};
struct i32_ge_s : instr_base<I32_GE_S> {};
struct i32_ge_u : instr_base<I32_GE_U> {};

struct i64_eqz  : instr_base<I64_EQZ> {};
struct i64_eq   : instr_base<I64_EQ> {};
struct i64_ne   : instr_base<I64_NE> {};
struct i64_lt_s : instr_base<I64_LT_S> {};
struct i64_lt_u : instr_base<I64_LT_U> {};
struct i64_gt_s : instr_base<I64_GT_S> {};
struct i64_gt_u : instr_base<I64_GT_U> {};
struct i64_le_s : instr_base<I64_LE_S> {};
struct i64_le_u : instr_base<I64_LE_U> {};
struct i64_ge_s : instr_base<I64_GE_S> {};
struct i64_ge_u : instr_base<I64_GE_U> {};

struct f32_eq : instr_base<F32_EQ> {};
struct f32_ne : instr_base<F32_NE> {};
struct f32_lt : instr_base<F32_LT> {};
struct f32_gt : instr_base<F32_GT> {};
struct f32_le : instr_base<F32_LE> {};
struct f32_ge : instr_base<F32_GE> {};

struct f64_eq : instr_base<F64_EQ> {};
struct f64_ne : instr_base<F64_NE> {};
struct f64_lt : instr_base<F64_LT> {};
struct f64_gt : instr_base<F64_GT> {};
struct f64_le : instr_base<F64_LE> {};
struct f64_ge : instr_base<F64_GE> {};

struct i32_popcount : instr_base<I32_POPCOUNT> {};
struct i32_clz      : instr_base<I32_CLZ> {};
struct i32_ctz      : instr_base<I32_CTZ> {};
struct i32_add      : instr_base<I32_ADD> {};
struct i32_sub      : instr_base<I32_SUB> {};
struct i32_mul      : instr_base<I32_MUL> {};
struct i32_div_s    : instr_base<I32_DIV_S> {};
struct i32_div_u    : instr_base<I32_DIV_U> {};
struct i32_rem_s    : instr_base<I32_REM_S> {};
struct i32_rem_u    : instr_base<I32_REM_U> {};
struct i32_and      : instr_base<I32_AND> {};
struct i32_or       : instr_base<I32_OR> {};
struct i32_xor      : instr_base<I32_XOR> {};
struct i32_shl      : instr_base<I32_SHL> {};
struct i32_shr_s    : instr_base<I32_SHR_S> {};
struct i32_shr_u    : instr_base<I32_SHR_U> {};
struct i32_rotl     : instr_base<I32_ROTL> {};
struct i32_rotr     : instr_base<I32_ROTR> {};

struct i64_popcount : instr_base<I64_POPCOUNT> {};
struct i64_clz      : instr_base<I64_CLZ> {};
struct i64_ctz      : instr_base<I64_CTZ> {};
struct i64_add      : instr_base<I64_ADD> {};
struct i64_sub      : instr_base<I64_SUB> {};
struct i64_mul      : instr_base<I64_MUL> {};
struct i64_div_s    : instr_base<I64_DIV_S> {};
struct i64_div_u    : instr_base<I64_DIV_U> {};
struct i64_rem_s    : instr_base<I64_REM_S> {};
struct i64_rem_u    : instr_base<I64_REM_U> {};
struct i64_and      : instr_base<I64_AND> {};
struct i64_or       : instr_base<I64_OR> {};
struct i64_xor      : instr_base<I64_XOR> {};
struct i64_shl      : instr_base<I64_SHL> {};
struct i64_shr_s    : instr_base<I64_SHR_S> {};
struct i64_shr_u    : instr_base<I64_SHR_U> {};
struct i64_rotl     : instr_base<I64_ROTL> {};
struct i64_rotr     : instr_base<I64_ROTR> {};

struct f32_abs      : instr_base<F32_ABS> {};
struct f32_neg      : instr_base<F32_NEG> {};
struct f32_ceil     : instr_base<F32_CEIL> {};
struct f32_floor    : instr_base<F32_FLOOR> {};
struct f32_trunc    : instr_base<F32_TRUNC> {};
struct f32_nearest  : instr_base<F32_NEAREST> {};
struct f32_sqrt     : instr_base<F32_SQRT> {};
struct f32_add      : instr_base<F32_ADD> {};
struct f32_sub      : instr_base<F32_SUB> {};
struct f32_mul      : instr_base<F32_MUL> {};
struct f32_div      : instr_base<F32_DIV> {};
struct f32_min      : instr_base<F32_MIN> {};
struct f32_max      : instr_base<F32_MAX> {};
struct f32_copysign : instr_base<F32_COPYSIGN> {};

struct f64_abs      : instr_base<F64_ABS> {};
struct f64_neg      : instr_base<F64_NEG> {};
struct f64_ceil     : instr_base<F64_CEIL> {};
struct f64_floor    : instr_base<F64_FLOOR> {};
struct f64_trunc    : instr_base<F64_TRUNC> {};
struct f64_nearest  : instr_base<F64_NEAREST> {};
struct f64_sqrt     : instr_base<F64_SQRT> {};
struct f64_add      : instr_base<F64_ADD> {};
struct f64_sub      : instr_base<F64_SUB> {};
struct f64_mul      : instr_base<F64_MUL> {};
struct f64_div      : instr_base<F64_DIV> {};
struct f64_min      : instr_base<F64_MIN> {};
struct f64_max      : instr_base<F64_MAX> {};
struct f64_copysign : instr_base<F64_COPYSIGN> {};

struct i32_wrap_i64    : instr_base<I32_WRAP_I64> {};
struct i32_trunc_s_f32 : instr_base<I32_TRUNC_S_F32> {};
struct i32_trunc_u_f32 : instr_base<I32_TRUNC_U_F32> {};
struct i32_trunc_s_f64 : instr_base<I32_TRUNC_S_F64> {};
struct i32_trunc_u_f64 : instr_base<I32_TRUNC_U_F64> {};
struct i64_extend_s_i32 : instr_base<I64_EXTEND_S_I32> {};
struct i64_extend_u_i32 : instr_base<I64_EXTEND_U_I32> {};
struct i64_trunc_s_f32 : instr_base<I64_TRUNC_S_F32> {};
struct i64_trunc_u_f32 : instr_base<I64_TRUNC_U_F32> {};
struct i64_trunc_s_f64 : instr_base<I64_TRUNC_S_F64> {};
struct i64_trunc_u_f64 : instr_base<I64_TRUNC_U_F64> {};
struct f32_convert_s_i32 : instr_base<F32_CONVERT_S_I32> {};
struct f32_convert_u_i32 : instr_base<F32_CONVERT_U_I32> {};
struct f32_convert_s_i64 : instr_base<F32_CONVERT_S_I64> {};
struct f32_convert_u_i64 : instr_base<F32_CONVERT_U_I64> {};
struct f32_demote_f64 : instr_base<F32_DEMOTE_F64> {};
struct f64_convert_s_i32 : instr_base<F64_CONVERT_S_I32> {};
struct f64_convert_u_i32 : instr_base<F64_CONVERT_U_I32> {};
struct f64_convert_s_i64 : instr_base<F64_CONVERT_S_I64> {};
struct f64_convert_u_i64 : instr_base<F64_CONVERT_U_I64> {};
struct f64_promote_f32 : instr_base<F64_PROMOTE_F32> {};
struct i32_reinterpret_f32 : instr_base<I32_REINTERPRET_F32> {};
struct i64_reinterpret_f64 : instr_base<I64_REINTERPRET_F64> {};
struct f32_reinterpret_i32 : instr_base<F32_REINTERPRET_I32> {};
struct f64_reinterpret_i64 : instr_base<F64_REINTERPRET_I64> {};

// expressions
template <size_t Instrs>
struct expr {
   std::array<instr*, Instrs> in;
   uint8_t end = END;
};
#pragma pack (pop)


}}} // namespace eosio, chain, wasm_ops
