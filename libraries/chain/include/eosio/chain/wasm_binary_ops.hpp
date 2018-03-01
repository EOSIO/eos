#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/subseq.hpp>
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
using namespace fc;
using wasm_op_ptr	  = std::unique_ptr<instr>;
using wasm_instr_ptr      = std::shared_ptr<instr>;
using wasm_return_t       = fc::optional<std::vector<uint8_t>>; 
using wasm_instr_callback = std::function<std::vector<wasm_instr_ptr>(uint8_t)>;
using code_iterator       = std::vector<uint8_t>::iterator;
using wasm_op_generator   = std::function<wasm_instr_ptr(std::vector<uint8_t>, size_t)>;
 //  BOOST_PP_CAT(_, OP)( char* block ) {   \
 //  }\

// helper types for construction
struct no_param_op {
   static constexpr bool has_data = false;
};
struct memarg;
struct mem_param_op {
   static constexpr bool has_data = true;
   typedef memarg t;
};
struct blocktype;
struct block_param_op {
   static constexpr bool has_data = true;
   typedef blocktype t;
};
struct uint32_param_op {
   static constexpr bool has_data = true;
   typedef uint32_t t;
};
struct uint64_param_op {
   static constexpr bool has_data = true;
   typedef uint64_t t;
};

#define CONSTRUCT_OP( r, DATA, OP )       \
template <typename ... Mutators>          \
struct BOOST_PP_CAT(_, OP) : instr_base<Mutators...> {  \
   BOOST_PP_CAT(_, OP)( char* block ){     \
      if constexpr ( DATA ::has_data ) {   \
         field = *((field*)(block));      \
      }                                   \
   }                                      \
   uint8_t code = OP;                     \
   if constexpr ( DATA ::has_data ){       \
      DATA ::t field;                      \
   }                                      \
   std::string to_string() { return #OP; }\
}; 
/*
#define CONSTRUCT_OP( r, DATA, OP )       \
template <typename ... Mutators>          \
struct BOOST_PP_CAT(_, OP) : instr_base<Mutators...> {  \
   BOOST_PP_CAT(_, OP)( char* block ) {   \
   }\
   uint8_t code = OP;                     \
   DATA;                                  \
   std::string to_string() { return #OP; }\
}; 
#define CONSTRUCT_OP( r, DATA, OP )       \
template <typename ... Mutators>          \
struct BOOST_PP_CAT(_, OP) : instr_base<Mutators...> {  \
   BOOST_PP_CAT(_, OP)( char* block ) {   \
   }\
   uint8_t code = OP;                     \
   DATA;                                  \
   std::string to_string() { return #OP; }\
}; 
#define CONSTRUCT_OP( r, DATA, OP )       \
template <typename ... Mutators>          \
struct BOOST_PP_CAT(_, OP) : instr_base<Mutators...> {  \
   BOOST_PP_CAT(_, OP)( char* block ) {   \
   }\
   uint8_t code = OP;                     \
   DATA;                                  \
   std::string to_string() { return #OP; }\
}; 
*/
#define WASM_OP_SEQ_LEN 169
#define WASM_OP_SEQ  (end)          \
                     (unreachable)  \
                     (nop)          \
                     (ret)          \
                     (drop)         \
                     (select)       \
                     (br_table)     \
                     (i32_eqz)      \
                     (i32_eq)       \
                     (i32_ne)       \
                     (i32_lt_s)        \
                     (i32_lt_u)        \
                     (i32_gt_s)        \
                     (i32_gt_u)        \
                     (i32_le_s)        \
                     (i32_le_u)        \
                     (i32_ge_s)        \
                     (i32_ge_u)        \
                     (i64_eqz)         \
                     (i64_eq)       \
                     (i64_ne)       \
                     (i64_lt_s)        \
                     (i64_lt_u)        \
                     (i64_gt_s)        \
                     (i64_gt_u)        \
                     (i64_le_s)        \
                     (i64_le_u)        \
                     (i64_ge_s)        \
                     (i64_ge_u)        \
                     (f32_eq)       \
                     (f32_ne)       \
                     (f32_lt)       \
                     (f32_gt)       \
                     (f32_le)       \
                     (f32_ge)       \
                     (f64_eq)       \
                     (f64_ne)       \
                     (f64_lt)       \
                     (f64_gt)       \
                     (f64_le)       \
                     (f64_ge)       \
                     (i32_clz)         \
                     (i32_ctz)         \
                     (i32_popcount)       \
                     (i32_add)         \
                     (i32_sub)         \
                     (i32_mul)         \
                     (i32_div_s)       \
                     (i32_div_u)       \
                     (i32_rem_s)       \
                     (i32_rem_u)       \
                     (i32_and)         \
                     (i32_or)       \
                     (i32_xor)         \
                     (i32_shl)         \
                     (i32_shr_s)       \
                     (i32_shr_u)       \
                     (i32_rotl)        \
                     (i32_rotr)        \
                     (i64_clz)         \
                     (i64_ctz)         \
                     (i64_popcount)       \
                     (i64_add)         \
                     (i64_sub)         \
                     (i64_mul)         \
                     (i64_div_s)       \
                     (i64_div_u)       \
                     (i64_rem_s)       \
                     (i64_rem_u)       \
                     (i64_and)         \
                     (i64_or)       \
                     (i64_xor)         \
                     (i64_shl)         \
                     (i64_shr_s)       \
                     (i64_shr_u)       \
                     (i64_rotl)        \
                     (i64_rotr)        \
                     (f32_abs)         \
                     (f32_neg)         \
                     (f32_ceil)        \
                     (f32_floor)       \
                     (f32_trunc)       \
                     (f32_nearest)        \
                     (f32_sqrt)        \
                     (f32_add)         \
                     (f32_sub)         \
                     (f32_mul)         \
                     (f32_div)         \
                     (f32_min)         \
                     (f32_max)         \
                     (f32_copysign)       \
                     (f64_abs)         \
                     (f64_neg)         \
                     (f64_ceil)        \
                     (f64_floor)       \
                     (f64_trunc)       \
                     (f64_nearest)        \
                     (f64_sqrt)        \
                     (f64_add)         \
                     (f64_sub)         \
                     (f64_mul)         \
                     (f64_div)         \
                     (f64_min)         \
                     (f64_max)         \
                     (f64_copysign)       \
                     (i32_wrap_i64)       \
                     (i32_trunc_s_f32)       \
                     (i32_trunc_u_f32)       \
                     (i32_trunc_s_f64)       \
                     (i32_trunc_u_f64)       \
                     (i64_extend_s_i32)         \
                     (i64_extend_u_i32)         \
                     (i64_trunc_s_f32)       \
                     (i64_trunc_u_f32)       \
                     (i64_trunc_s_f64)       \
                     (i64_trunc_u_f64)       \
                     (f32_convert_s_i32)        \
                     (f32_convert_u_i32)        \
                     (f32_convert_s_i64)        \
                     (f32_convert_u_i64)        \
                     (f32_demote_f64)        \
                     (f64_convert_s_i32)        \
                     (f64_convert_u_i32)        \
                     (f64_convert_s_i64)        \
                     (f64_convert_u_i64)        \
                     (f64_promote_f32)       \
                     (i32_reinterpret_f32)         \
                     (i64_reinterpret_f64)         \
                     (f32_reinterpret_i32)         \
                     (f64_reinterpret_i64)         \
                     (block)        \
                     (loop)         \
                     (if_eps)       \
                     (if_else)      \
                     (br)           \
                     (br_if)        \
                     (call)         \
                     (get_local)    \
                     (set_local)    \
                     (tee_local)    \
                     (get_global)   \
                     (set_global)   \
                     (i32_const)    \
                     (f32_const)    \
                     (i32_load)     \
                     (i64_load)     \
                     (f32_load)     \
                     (f64_load)     \
                     (i32_load8_s)     \
                     (i32_load8_u)     \
                     (i32_load16_s)     \
                     (i32_load16_u)     \
                     (i64_load8_s)     \
                     (i64_load8_u)     \
                     (i64_load16_s)     \
                     (i64_load16_u)     \
                     (i64_load32_s)     \
                     (i64_load32_u)     \
                     (i32_store)     \
                     (i64_store)     \
                     (f32_store)     \
                     (f64_store)     \
                     (i32_store8)     \
                     (i32_store16)     \
                     (i64_store8)     \
                     (i64_store16)     \
                     (i64_store32)   \
                     (i64_const)     \
                     (f64_const)  

enum code {
   unreachable    = 0x00,
   nop            = 0x01,
   block          = 0x02,
   loop           = 0x03,
   if_eps         = 0x04,
   if_else        = 0x05,
   end            = 0x0B,
   br             = 0x0C,
   br_if          = 0x0D,
   br_table       = 0x0E,
   ret            = 0x0F,
   call           = 0x10,
   call_indirect  = 0x11,
   drop           = 0x1A,
   select         = 0x1B,
   get_local      = 0x20,
   set_local      = 0x21,
   tee_local      = 0x22,
   get_global     = 0x23,
   set_global     = 0x24,
   i32_load       = 0x28,
   i64_load       = 0x29,
   f32_load       = 0x2A,
   f64_load       = 0x2B,
   i32_load8_s    = 0x2C,
   i32_load8_u    = 0x2D,
   i32_load16_s   = 0x2E,
   i32_load16_u   = 0x2F,
   i64_load8_s    = 0x30,
   i64_load8_u    = 0x31,
   i64_load16_s   = 0x32,
   i64_load16_u   = 0x33,
   i64_load32_s   = 0x34,
   i64_load32_u   = 0x35,
   i32_store      = 0x36,
   i64_store      = 0x37,
   f32_store      = 0x38,
   f64_store      = 0x39,
   i32_store8     = 0x3A,
   i32_store16    = 0x3B,
   i64_store8     = 0x3C,
   i64_store16    = 0x3D,
   i64_store32    = 0x3E,
   current_memory    = 0x3F,
   grow_memory       = 0x40,
   i32_const      = 0x41,
   i64_const      = 0x42,
   f32_const      = 0x43,
   f64_const      = 0x44,
   i32_eqz        = 0x45,
   i32_eq         = 0x46,
   i32_ne         = 0x47,
   i32_lt_s       = 0x48,
   i32_lt_u       = 0x49,
   i32_gt_s       = 0x4A,
   i32_gt_u       = 0x4B,
   i32_le_s       = 0x4C,
   i32_le_u       = 0x4D,
   i32_ge_s       = 0x4E,
   i32_ge_u       = 0x4F,
   i64_eqz        = 0x50,
   i64_eq         = 0x51,
   i64_ne         = 0x52,
   i64_lt_s       = 0x53,
   i64_lt_u       = 0x54,
   i64_gt_s       = 0x55,
   i64_gt_u       = 0x56,
   i64_le_s       = 0x57,
   i64_le_u       = 0x58,
   i64_ge_s       = 0x59,
   i64_ge_u       = 0x5A,
   f32_eq         = 0x5B,
   f32_ne         = 0x5C,
   f32_lt         = 0x5D,
   f32_gt         = 0x5E,
   f32_le         = 0x5F,
   f32_ge         = 0x60,
   f64_eq         = 0x61,
   f64_ne         = 0x62,
   f64_lt         = 0x63,
   f64_gt         = 0x64,
   f64_le         = 0x65,
   f64_ge         = 0x66,
   i32_clz        = 0x67,
   i32_ctz        = 0x68,
   i32_popcount   = 0x69,
   i32_add        = 0x6A,
   i32_sub        = 0x6B,
   i32_mul        = 0x6C,
   i32_div_s      = 0x6D,
   i32_div_u      = 0x6E,
   i32_rem_s      = 0x6F,
   i32_rem_u      = 0x70,
   i32_and        = 0x71,
   i32_or         = 0x72,
   i32_xor        = 0x73,
   i32_shl        = 0x74,
   i32_shr_s      = 0x75,
   i32_shr_u      = 0x76,
   i32_rotl       = 0x77,
   i32_rotr       = 0x78,
   i64_clz        = 0x79,
   i64_ctz        = 0x7A,
   i64_popcount   = 0x7B,
   i64_add        = 0x7C,
   i64_sub        = 0x7D,
   i64_mul        = 0x7E,
   i64_div_s      = 0x7F,
   i64_div_u      = 0x80,
   i64_rem_s      = 0x81,
   i64_rem_u      = 0x82,
   i64_and        = 0x83,
   i64_or         = 0x84,
   i64_xor        = 0x85,
   i64_shl        = 0x86,
   i64_shr_s      = 0x87,
   i64_shr_u      = 0x88,
   i64_rotl       = 0x89,
   i64_rotr       = 0x8A,
   f32_abs        = 0x8B,
   f32_neg        = 0x8C,
   f32_ceil       = 0x8D,
   f32_floor      = 0x8E,
   f32_trunc      = 0x8F,
   f32_nearest    = 0x90,
   f32_sqrt       = 0x91,
   f32_add        = 0x92,
   f32_sub        = 0x93,
   f32_mul        = 0x94,
   f32_div        = 0x95,
   f32_min        = 0x96,
   f32_max        = 0x97,
   f32_copysign   = 0x98,
   f64_abs        = 0x99,
   f64_neg        = 0x9A,
   f64_ceil       = 0x9B,
   f64_floor      = 0x9C,
   f64_trunc      = 0x9D,
   f64_nearest    = 0x9E,
   f64_sqrt       = 0x9F,
   f64_add        = 0xA0,
   f64_sub        = 0xA1,
   f64_mul        = 0xA2,
   f64_div        = 0xA3,
   f64_min        = 0xA4,
   f64_max        = 0xA5,
   f64_copysign   = 0xA6,
   i32_wrap_i64       = 0xA7,
   i32_trunc_s_f32    = 0xA8,
   i32_trunc_u_f32    = 0xA9,
   i32_trunc_s_f64    = 0xAA,
   i32_trunc_u_f64    = 0xAB,
   i64_extend_s_i32   = 0xAC,
   i64_extend_u_i32   = 0xAD,
   i64_trunc_s_f32    = 0xAE,
   i64_trunc_u_f32    = 0xAF,
   i64_trunc_s_f64    = 0xB0,
   i64_trunc_u_f64    = 0xB1,
   f32_convert_s_i32  = 0xB2,
   f32_convert_u_i32  = 0xB3,
   f32_convert_s_i64  = 0xB4,
   f32_convert_u_i64  = 0xB5,
   f32_demote_f64     = 0xB6,
   f64_convert_s_i32  = 0xB7,
   f64_convert_u_i32  = 0xB8,
   f64_convert_s_i64  = 0xB9,
   f64_convert_u_i64  = 0xBA,
   f64_promote_f32    = 0xBB,
   i32_reinterpret_f32 = 0xBC,
   i64_reinterpret_f64 = 0xBD,
   f32_reinterpret_i32 = 0xBE,
   f64_reinterpret_i64 = 0xBF,
   error               = 0xFF,
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
};

template <typename ... Mutators>
struct instr_base : instr {
   virtual wasm_return_t visit( ) {//code_iterator start, code_iterator end ) {
      for ( auto m : { Mutators::accept... } )
         m( this );
      
      return {};
   } 
};

// error
template <typename ... Mutators>
struct _error : instr_base<Mutators...> {
   uint8_t code = error;
   std::string to_string() { return "error"; }
   virtual wasm_return_t visit( ) {//code_iterator start, code_iterator end ) {
      //FC_THROW("ERROR, invalid instruction");  //TODO add exception here
      return {};
   } 
};
template <typename ... Mutators>
struct _call_indirect : instr_base<Mutators...> {
   uint8_t code = call_indirect;
   uint32_t funcidx;
   uint8_t end = unreachable;
   std::string to_string() { return "call_indirect"; }
};
template <typename ... Mutators>
struct _current_memory : instr_base<Mutators...> {
   uint8_t code = current_memory;
   uint8_t end = unreachable;
   std::string to_string() { return "current_memory"; }
};
template <typename ... Mutators>
struct _grow_memory : instr_base<Mutators...>{
   uint8_t code = grow_memory;
   uint8_t end = unreachable;
   std::string to_string() { return "grow_memory"; }
};


// construct the ops
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP, no_param_op,             BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 0, 130 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP, block_param_op, BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 130, 4 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP, uint32_param_op,   BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 134, 10 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP, mem_param_op,     BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 144, 23 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP, uint64_param op,   BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 167, 2 ) )

#pragma pack (pop)

struct nop_mutator {
   static void accept( instr* inst ) {}
};

#define GEN_TYPE( r, T, OP ) \
   using BOOST_PP_CAT( OP, _t ) = BOOST_PP_CAT(_, OP) < T , BOOST_PP_CAT(T, s) ...>;

// class to inherit from to attach specific mutators and have default behavior for all specified types
template < typename Mutator = nop_mutator, typename ... Mutators>
struct op_types {
   BOOST_PP_SEQ_FOR_EACH( GEN_TYPE, Mutator, WASM_OP_SEQ )
}; // op_types

}}} // namespace eosio, chain, wasm_ops
/*
#define REFLECT_OP( r, FIELD, OP ) \
FC_REFLECT_TEMPLATE( (typename T), eosio::chain::wasm_ops::BOOST_PP_CAT(_,OP)< T >, (code)( FIELD ) )
BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, ,   BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 0, 130 ) )
BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, rt, BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 130, 4 ) )
BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, n,  BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 134, 10 ) )
BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, m,  BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 144, 23 ) )
BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, n,  BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 167, 2 ) )
*/
FC_REFLECT_TEMPLATE( (typename T), eosio::chain::wasm_ops::_nop< T >, (code) )
FC_REFLECT_TEMPLATE( (typename T), eosio::chain::wasm_ops::_block< T >, (code)(rt) )
