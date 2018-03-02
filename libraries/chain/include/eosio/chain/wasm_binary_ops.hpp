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
using wasm_return_t       = std::vector<uint8_t>; 
using wasm_instr_callback = std::function<std::vector<wasm_instr_ptr>(uint8_t)>;
using code_iterator       = std::vector<uint8_t>::iterator;
using wasm_op_generator   = std::function<wasm_instr_ptr(std::vector<uint8_t>, size_t)>;

struct no_param_op {
   static constexpr bool has_param = false;
};

struct block_param_op {
};

#define CONSTRUCT_OP( r, DATA, OP )                            \
template <typename ... Mutators>                               \
struct OP : instr_base<Mutators...> {                          \
   OP() = default;                                             \
   OP( char* block ) {}                                        \
   uint8_t code = BOOST_PP_CAT(OP,_code);                      \
   uint8_t get_code() { return BOOST_PP_CAT(OP,_code); }       \
   char* skip_ahead( char* block ) { return ++block; }         \
   std::string to_string() { return BOOST_PP_STRINGIZE(OP); }  \
};
//   char* unpack( char* block ) { return ++block; }             \
//   std::vector<char> pack() { return { code }; }                    \
//};

#define CONSTRUCT_OP_HAS_DATA( r, DATA, OP )                                        \
template <typename ... Mutators>                                                    \
struct OP : instr_base<Mutators...> {                                               \
   OP() = default;                                                                  \
   OP( char* block ) { code = *((uint8_t*)(block++)); field = *((DATA*)(block)); }  \
   uint8_t code = BOOST_PP_CAT(OP,_code);                                           \
   DATA field;                                                                      \
   uint8_t get_code() { return BOOST_PP_CAT(OP,_code); }                            \
   char* skip_ahead( char* block) { return block + 1 + sizeof(DATA); }              \
   std::string to_string() { return BOOST_PP_STRINGIZE(OP); }                       \
};
/*
   char* unpack( char* block ) { return block + 1 + sizeof(DATA); }                 \
   std::vector<char> pack() {                                                            \
      std::vector<char> ret_vec = { code };                                      \
      std::vector<char> field_vec = fc::raw::pack(field);                        \
      return ret_vec.insert( ret_vec.end(), field_vec.begin(), field_vec.end() );   \
   }                                                                                \
};
*/

#define WASM_OP_SEQ  (error)        \
                     (end)          \
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
                     (f64_const)    \
                     (call_indirect) \
                     (grow_memory) \
                     (current_memory) 

enum code {
   unreachable_code    = 0x00,
   nop_code            = 0x01,
   block_code          = 0x02,
   loop_code           = 0x03,
   if_eps_code         = 0x04,
   if_else_code        = 0x05,
   end_code            = 0x0B,
   br_code             = 0x0C,
   br_if_code          = 0x0D,
   br_table_code       = 0x0E,
   ret_code            = 0x0F,
   call_code           = 0x10,
   call_indirect_code  = 0x11,
   drop_code           = 0x1A,
   select_code         = 0x1B,
   get_local_code      = 0x20,
   set_local_code      = 0x21,
   tee_local_code      = 0x22,
   get_global_code     = 0x23,
   set_global_code     = 0x24,
   i32_load_code       = 0x28,
   i64_load_code       = 0x29,
   f32_load_code       = 0x2A,
   f64_load_code       = 0x2B,
   i32_load8_s_code    = 0x2C,
   i32_load8_u_code    = 0x2D,
   i32_load16_s_code   = 0x2E,
   i32_load16_u_code   = 0x2F,
   i64_load8_s_code    = 0x30,
   i64_load8_u_code    = 0x31,
   i64_load16_s_code   = 0x32,
   i64_load16_u_code   = 0x33,
   i64_load32_s_code   = 0x34,
   i64_load32_u_code   = 0x35,
   i32_store_code      = 0x36,
   i64_store_code      = 0x37,
   f32_store_code      = 0x38,
   f64_store_code      = 0x39,
   i32_store8_code     = 0x3A,
   i32_store16_code    = 0x3B,
   i64_store8_code     = 0x3C,
   i64_store16_code    = 0x3D,
   i64_store32_code    = 0x3E,
   current_memory_code    = 0x3F,
   grow_memory_code       = 0x40,
   i32_const_code      = 0x41,
   i64_const_code      = 0x42,
   f32_const_code      = 0x43,
   f64_const_code      = 0x44,
   i32_eqz_code        = 0x45,
   i32_eq_code         = 0x46,
   i32_ne_code         = 0x47,
   i32_lt_s_code       = 0x48,
   i32_lt_u_code       = 0x49,
   i32_gt_s_code       = 0x4A,
   i32_gt_u_code       = 0x4B,
   i32_le_s_code       = 0x4C,
   i32_le_u_code       = 0x4D,
   i32_ge_s_code       = 0x4E,
   i32_ge_u_code       = 0x4F,
   i64_eqz_code        = 0x50,
   i64_eq_code         = 0x51,
   i64_ne_code         = 0x52,
   i64_lt_s_code       = 0x53,
   i64_lt_u_code       = 0x54,
   i64_gt_s_code       = 0x55,
   i64_gt_u_code       = 0x56,
   i64_le_s_code       = 0x57,
   i64_le_u_code       = 0x58,
   i64_ge_s_code       = 0x59,
   i64_ge_u_code       = 0x5A,
   f32_eq_code         = 0x5B,
   f32_ne_code         = 0x5C,
   f32_lt_code         = 0x5D,
   f32_gt_code         = 0x5E,
   f32_le_code         = 0x5F,
   f32_ge_code         = 0x60,
   f64_eq_code         = 0x61,
   f64_ne_code         = 0x62,
   f64_lt_code         = 0x63,
   f64_gt_code         = 0x64,
   f64_le_code         = 0x65,
   f64_ge_code         = 0x66,
   i32_clz_code        = 0x67,
   i32_ctz_code        = 0x68,
   i32_popcount_code   = 0x69,
   i32_add_code        = 0x6A,
   i32_sub_code        = 0x6B,
   i32_mul_code        = 0x6C,
   i32_div_s_code      = 0x6D,
   i32_div_u_code      = 0x6E,
   i32_rem_s_code      = 0x6F,
   i32_rem_u_code      = 0x70,
   i32_and_code        = 0x71,
   i32_or_code         = 0x72,
   i32_xor_code        = 0x73,
   i32_shl_code        = 0x74,
   i32_shr_s_code      = 0x75,
   i32_shr_u_code      = 0x76,
   i32_rotl_code       = 0x77,
   i32_rotr_code       = 0x78,
   i64_clz_code        = 0x79,
   i64_ctz_code        = 0x7A,
   i64_popcount_code   = 0x7B,
   i64_add_code        = 0x7C,
   i64_sub_code        = 0x7D,
   i64_mul_code        = 0x7E,
   i64_div_s_code      = 0x7F,
   i64_div_u_code      = 0x80,
   i64_rem_s_code      = 0x81,
   i64_rem_u_code      = 0x82,
   i64_and_code        = 0x83,
   i64_or_code         = 0x84,
   i64_xor_code        = 0x85,
   i64_shl_code        = 0x86,
   i64_shr_s_code      = 0x87,
   i64_shr_u_code      = 0x88,
   i64_rotl_code       = 0x89,
   i64_rotr_code       = 0x8A,
   f32_abs_code        = 0x8B,
   f32_neg_code        = 0x8C,
   f32_ceil_code       = 0x8D,
   f32_floor_code      = 0x8E,
   f32_trunc_code      = 0x8F,
   f32_nearest_code    = 0x90,
   f32_sqrt_code       = 0x91,
   f32_add_code        = 0x92,
   f32_sub_code        = 0x93,
   f32_mul_code        = 0x94,
   f32_div_code        = 0x95,
   f32_min_code        = 0x96,
   f32_max_code        = 0x97,
   f32_copysign_code   = 0x98,
   f64_abs_code        = 0x99,
   f64_neg_code        = 0x9A,
   f64_ceil_code       = 0x9B,
   f64_floor_code      = 0x9C,
   f64_trunc_code      = 0x9D,
   f64_nearest_code    = 0x9E,
   f64_sqrt_code       = 0x9F,
   f64_add_code        = 0xA0,
   f64_sub_code        = 0xA1,
   f64_mul_code        = 0xA2,
   f64_div_code        = 0xA3,
   f64_min_code        = 0xA4,
   f64_max_code        = 0xA5,
   f64_copysign_code   = 0xA6,
   i32_wrap_i64_code       = 0xA7,
   i32_trunc_s_f32_code    = 0xA8,
   i32_trunc_u_f32_code    = 0xA9,
   i32_trunc_s_f64_code    = 0xAA,
   i32_trunc_u_f64_code    = 0xAB,
   i64_extend_s_i32_code   = 0xAC,
   i64_extend_u_i32_code   = 0xAD,
   i64_trunc_s_f32_code    = 0xAE,
   i64_trunc_u_f32_code    = 0xAF,
   i64_trunc_s_f64_code    = 0xB0,
   i64_trunc_u_f64_code    = 0xB1,
   f32_convert_s_i32_code  = 0xB2,
   f32_convert_u_i32_code  = 0xB3,
   f32_convert_s_i64_code  = 0xB4,
   f32_convert_u_i64_code  = 0xB5,
   f32_demote_f64_code     = 0xB6,
   f64_convert_s_i32_code  = 0xB7,
   f64_convert_u_i32_code  = 0xB8,
   f64_convert_s_i64_code  = 0xB9,
   f64_convert_u_i64_code  = 0xBA,
   f64_promote_f32_code    = 0xBB,
   i32_reinterpret_f32_code = 0xBC,
   i64_reinterpret_f64_code = 0xBD,
   f32_reinterpret_i32_code = 0xBE,
   f64_reinterpret_i64_code = 0xBF,
   error_code               = 0xFF,
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
   virtual std::string to_string() { return "instr"; }
   virtual uint8_t get_code() = 0;
   virtual wasm_return_t visit() = 0;
   virtual char* skip_ahead( char* block ) = 0;
};

struct callindirecttype {
   uint32_t funcidx;
   uint8_t end = unreachable_code;
};

struct memoryoptype {
   uint8_t end = unreachable_code;
};

template <typename ... Mutators>
struct instr_base : instr {
   virtual wasm_return_t visit( ) {//code_iterator start, code_iterator end ) {
      std::vector<uint8_t> ret_vec = {};
      for ( auto m : { Mutators::accept... } ) {
         std::vector<uint8_t> temp = m(this);
         ret_vec.insert(ret_vec.end(), temp.begin(), temp.end());
      }
      return ret_vec;
   } 
};
/*
// odd specializations that don't fit anywhere
// error
template <typename ... Mutators>
struct error : instr_base<Mutators...> {
   uint8_t code = error_code;
   uint8_t get_code() { return error_code; }
   std::string to_string() { return "error"; }
   char* skip_ahead( char* block ) { return block + sizeof(code); }
};

template <typename ... Mutators>
struct call_indirect : instr_base<Mutators...> {
   uint8_t code = call_indirect_code;
   uint32_t funcidx;
   uint8_t end = unreachable_code;
   std::string to_string() { return "call_indirect"; }
   uint8_t get_code() { return error_code; }
   char* skip_ahead( char* block ) { return block + sizeof(code)+sizeof(funcidx)+sizeof(end); }
};
template <typename ... Mutators>
struct current_memory : instr_base<Mutators...> {
   uint8_t code = current_memory_code;
   uint8_t end = unreachable_code;
   std::string to_string() { return "current_memory"; }
   uint8_t get_code() { return current_memory_code; }
   char* skip_ahead( char* block ) { return block + sizeof(code)+sizeof(end); }
};
template <typename ... Mutators>
struct grow_memory : instr_base<Mutators...>{
   uint8_t code = grow_memory_code;
   uint8_t end = unreachable_code;
   std::string to_string() { return "grow_memory"; }
   uint8_t get_code() { return grow_memory_code; }
   char* skip_ahead( char* block ) { return block + sizeof(code)+sizeof(end); }
};
*/

// construct the ops
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP, no_param_op ,                    BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 0, 131 ) )
//BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP, block_param_op ,                    BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 0, 130 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, blocktype,  BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 131, 4 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, uint32_t,   BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 135, 10 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, memarg,     BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 145, 23 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, uint64_t,   BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 168, 2 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, callindirecttype,   BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 170, 1 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, memoryoptype,   BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 171, 2 ) )
#undef CONSTRUCT_OP
#undef CONSTRUCT_OP_HAS_DATA
#pragma pack (pop)

struct nop_mutator {
   static wasm_return_t accept( instr* inst ) {}
};

#define GEN_TYPE( r, T, OP ) \
   using BOOST_PP_CAT( OP, _t ) = OP < T , BOOST_PP_CAT(T, s) ...>;

// class to inherit from to attach specific mutators and have default behavior for all specified types
template < typename Mutator = nop_mutator, typename ... Mutators>
struct op_types {
   BOOST_PP_SEQ_FOR_EACH( GEN_TYPE, Mutator, WASM_OP_SEQ )
}; // op_types

// simple struct to house a set of instaniated ops that we can take the reference from
template <class Op_Types>
struct cached_ops {
 #define GEN_FIELD( r, P, OP ) \
   typename Op_Types::BOOST_PP_CAT(OP,_t) BOOST_PP_CAT(P, OP);
   BOOST_PP_SEQ_FOR_EACH( GEN_FIELD, cached_, WASM_OP_SEQ )
 #undef GEN_FIELD
};

// function to dynamically get the instr pointer we want
template <class Op_Types>
instr* get_instr_from_op( uint8_t code ) {
   static cached_ops<Op_Types> _cached_ops;

#define GEN_CASE( r, P, OP )     \
   case BOOST_PP_CAT(OP, _code): \
      return &_cached_ops.BOOST_PP_CAT(P, OP);   

   switch ( code ) {
   BOOST_PP_SEQ_FOR_EACH( GEN_CASE, cached_, WASM_OP_SEQ )
   }
#undef GEN_CASE
}

}}} // namespace eosio, chain, wasm_ops

#define REFLECT_OP( r, FIELD, OP ) \
FC_REFLECT_TEMPLATE( (typename T), eosio::chain::wasm_ops::OP< T >, (code) )
/*
BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, ,   BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 0, 130 ) )
BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, rt, BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 130, 4 ) )
BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, n,  BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 134, 10 ) )
BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, m,  BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 144, 23 ) )
BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, n,  BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 167, 2 ) )
*/
FC_REFLECT_TEMPLATE( (typename T), eosio::chain::wasm_ops::nop< T >, (code) )
//BOOST_PP_SEQ_FOR_EACH( REFLECT_OP, , BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 0, 130 ) )
#undef REFLECT_OP
FC_REFLECT_TEMPLATE( (typename T), eosio::chain::wasm_ops::block< T >, (code)(rt) )
FC_REFLECT( eosio::chain::wasm_ops::memarg, (a)(o) )
FC_REFLECT( eosio::chain::wasm_ops::blocktype, (result) )
//FC_REFLECT( eosio::chain::wasm_ops::functype, (code)(params)(results) )
FC_REFLECT( eosio::chain::wasm_ops::limits, (code)(min)(max) )
FC_REFLECT( eosio::chain::wasm_ops::memtype, (lim) )
FC_REFLECT( eosio::chain::wasm_ops::elemtype, (code) )
FC_REFLECT( eosio::chain::wasm_ops::tabletype, (et)(lim) )
FC_REFLECT( eosio::chain::wasm_ops::mut, (code) )
FC_REFLECT( eosio::chain::wasm_ops::globaltype, (t)(m) )
FC_REFLECT( eosio::chain::wasm_ops::callindirecttype, (funcidx)(end) )
FC_REFLECT( eosio::chain::wasm_ops::memoryoptype, (end) )
