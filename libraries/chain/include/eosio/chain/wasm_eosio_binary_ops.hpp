#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/subseq.hpp>
#include <boost/preprocessor/seq/remove.hpp>
#include <boost/preprocessor/seq/push_back.hpp>
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

#include "IR/Operators.h"
#include "IR/Module.h"

namespace eosio { namespace chain { namespace wasm_ops {

class instruction_stream {
   public:
      instruction_stream(size_t size) : idx(0) {
         data.resize(size);
      }
      void operator<< (const char c) { 
         if (idx >= data.size())
            data.resize(data.size()*2);
         data[idx++] = static_cast<U8>(c);
      }
      void set(size_t i, const char* arr) {
         if (i+idx >= data.size())
            data.resize(data.size()*2+i);
         memcpy((char*)&data[idx], arr, i);
         idx += i;
      }
      size_t get_index() { return idx; }
      std::vector<U8> get() {
         std::vector<U8> ret = data;
         ret.resize(idx);
         return ret;
      }
//   private:
      size_t idx;
      std::vector<U8> data;
};

// forward declaration
struct instr;
using namespace fc;
using wasm_op_ptr   = std::unique_ptr<instr>;
using wasm_instr_ptr      = std::shared_ptr<instr>;
using wasm_return_t       = std::vector<uint8_t>; 
using wasm_instr_callback = std::function<std::vector<wasm_instr_ptr>(uint8_t)>;
using code_vector         = std::vector<uint8_t>;
using code_iterator       = std::vector<uint8_t>::iterator;
using wasm_op_generator   = std::function<wasm_instr_ptr(std::vector<uint8_t>, size_t)>;

#pragma pack (push)
struct memarg {
   uint32_t a;   // align
   uint32_t o;   // offset
};

struct blocktype {
   uint8_t result = 0x40; // either null (0x40) or valtype
};

struct memoryoptype {
   uint8_t end = 0x00;
};
struct branchtabletype {
   uint64_t target_depth;
   uint64_t table_index;
};

// using for instructions with no fields
struct voidtype {};


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

inline void pack( instruction_stream* stream, uint32_t field ) {
   const char packed[] = { char(field), char(field >> 8), char(field >> 16), char(field >> 24) };
   stream->set(sizeof(packed), packed);
}
inline void pack( instruction_stream* stream, uint64_t field ) {
   const char packed[] = { char(field), char(field >> 8), char(field >> 16), char(field >> 24), 
                           char(field >> 32), char(field >> 40), char(field >> 48), char(field >> 56) };
   stream->set(sizeof(packed), packed);
}
inline void pack( instruction_stream* stream, blocktype field ) {
   const char packed[] = { char(field.result) };
   stream->set(sizeof(packed), packed);
}
inline void pack( instruction_stream* stream,  memoryoptype field ) {
   const char packed[] = { char(field.end) };
   stream->set(sizeof(packed), packed);
}
inline void pack( instruction_stream* stream, memarg field ) {
   const char packed[] = { char(field.a), char(field.a >> 8), char(field.a >> 16), char(field.a >> 24), 
                           char(field.o), char(field.o >> 8), char(field.o >> 16), char(field.o >> 24)};
   stream->set(sizeof(packed), packed);

}
inline void pack( instruction_stream* stream, branchtabletype field ) {
   const char packed[] = { char(field.target_depth), char(field.target_depth >> 8), char(field.target_depth >> 16), char(field.target_depth >> 24), 
            char(field.target_depth >> 32), char(field.target_depth >> 40), char(field.target_depth >> 48), char(field.target_depth >> 56), 
            char(field.table_index), char(field.table_index >> 8), char(field.table_index >> 16), char(field.table_index >> 24), 
            char(field.table_index >> 32), char(field.table_index >> 40), char(field.table_index >> 48), char(field.table_index >> 56) };
   stream->set(sizeof(packed), packed);
}

template <typename Field>
struct field_specific_params {
   static constexpr int skip_ahead = sizeof(uint16_t) + sizeof(Field);
   static auto unpack( char* opcode, Field& f ) { f = *reinterpret_cast<Field*>(opcode); }
   static void pack(instruction_stream* stream, Field& f) { return eosio::chain::wasm_ops::pack(stream, f); }
   static auto to_string(Field& f) { return std::string(" ")+
                                       eosio::chain::wasm_ops::to_string(f); }
};
template <>
struct field_specific_params<voidtype> {
   static constexpr int skip_ahead = sizeof(uint16_t);
   static auto unpack( char* opcode, voidtype& f ) {}
   static void pack(instruction_stream* stream, voidtype& f) {}
   static auto to_string(voidtype& f) { return ""; }
}; 

#define CONSTRUCT_OP_HAS_DATA( r, DATA, OP )                                                        \
template <typename ... Mutators>                                                                    \
struct OP final : instr_base<Mutators...> {                                                         \
   uint16_t code = BOOST_PP_CAT(OP,_code);                                                          \
   DATA field;                                                                                      \
   uint16_t get_code() override { return BOOST_PP_CAT(OP,_code); }                                  \
   int skip_ahead() override { return field_specific_params<DATA>::skip_ahead; }                    \
   void unpack( char* opcode ) override {                                                           \
      field_specific_params<DATA>::unpack( opcode, field );                                         \
   }                                                                                                \
   void pack(instruction_stream* stream) override {                                                 \
      stream->set(2, (const char*)&code);                                                           \
      field_specific_params<DATA>::pack( stream, field );                                           \
   }                                                                                                \
   std::string to_string() override {                                                               \
      return std::string(BOOST_PP_STRINGIZE(OP))+field_specific_params<DATA>::to_string( field );   \
   }                                                                                                \
};

#define WASM_OP_SEQ  (error)                \
                     (end)                  \
                     (unreachable)          \
                     (nop)                  \
                     (else_)                \
                     (return_)              \
                     (drop)                 \
                     (select)               \
                     (i32_eqz)              \
                     (i32_eq)               \
                     (i32_ne)               \
                     (i32_lt_s)             \
                     (i32_lt_u)             \
                     (i32_gt_s)             \
                     (i32_gt_u)             \
                     (i32_le_s)             \
                     (i32_le_u)             \
                     (i32_ge_s)             \
                     (i32_ge_u)             \
                     (i64_eqz)              \
                     (i64_eq)               \
                     (i64_ne)               \
                     (i64_lt_s)             \
                     (i64_lt_u)             \
                     (i64_gt_s)             \
                     (i64_gt_u)             \
                     (i64_le_s)             \
                     (i64_le_u)             \
                     (i64_ge_s)             \
                     (i64_ge_u)             \
                     (f32_eq)               \
                     (f32_ne)               \
                     (f32_lt)               \
                     (f32_gt)               \
                     (f32_le)               \
                     (f32_ge)               \
                     (f64_eq)               \
                     (f64_ne)               \
                     (f64_lt)               \
                     (f64_gt)               \
                     (f64_le)               \
                     (f64_ge)               \
                     (i32_clz)              \
                     (i32_ctz)              \
                     (i32_popcnt)           \
                     (i32_add)              \
                     (i32_sub)              \
                     (i32_mul)              \
                     (i32_div_s)            \
                     (i32_div_u)            \
                     (i32_rem_s)            \
                     (i32_rem_u)            \
                     (i32_and)              \
                     (i32_or)               \
                     (i32_xor)              \
                     (i32_shl)              \
                     (i32_shr_s)            \
                     (i32_shr_u)            \
                     (i32_rotl)             \
                     (i32_rotr)             \
                     (i64_clz)              \
                     (i64_ctz)              \
                     (i64_popcnt)           \
                     (i64_add)              \
                     (i64_sub)              \
                     (i64_mul)              \
                     (i64_div_s)            \
                     (i64_div_u)            \
                     (i64_rem_s)            \
                     (i64_rem_u)            \
                     (i64_and)              \
                     (i64_or)               \
                     (i64_xor)              \
                     (i64_shl)              \
                     (i64_shr_s)            \
                     (i64_shr_u)            \
                     (i64_rotl)             \
                     (i64_rotr)             \
                     (f32_abs)              \
                     (f32_neg)              \
                     (f32_ceil)             \
                     (f32_floor)            \
                     (f32_trunc)            \
                     (f32_nearest)          \
                     (f32_sqrt)             \
                     (f32_add)              \
                     (f32_sub)              \
                     (f32_mul)              \
                     (f32_div)              \
                     (f32_min)              \
                     (f32_max)              \
                     (f32_copysign)         \
                     (f64_abs)              \
                     (f64_neg)              \
                     (f64_ceil)             \
                     (f64_floor)            \
                     (f64_trunc)            \
                     (f64_nearest)          \
                     (f64_sqrt)             \
                     (f64_add)              \
                     (f64_sub)              \
                     (f64_mul)              \
                     (f64_div)              \
                     (f64_min)              \
                     (f64_max)              \
                     (f64_copysign)         \
                     (i32_wrap_i64)         \
                     (i32_trunc_s_f32)      \
                     (i32_trunc_u_f32)      \
                     (i32_trunc_s_f64)      \
                     (i32_trunc_u_f64)      \
                     (i64_extend_s_i32)     \
                     (i64_extend_u_i32)     \
                     (i64_trunc_s_f32)      \
                     (i64_trunc_u_f32)      \
                     (i64_trunc_s_f64)      \
                     (i64_trunc_u_f64)      \
                     (f32_convert_s_i32)    \
                     (f32_convert_u_i32)    \
                     (f32_convert_s_i64)    \
                     (f32_convert_u_i64)    \
                     (f32_demote_f64)       \
                     (f64_convert_s_i32)    \
                     (f64_convert_u_i32)    \
                     (f64_convert_s_i64)    \
                     (f64_convert_u_i64)    \
                     (f64_promote_f32)      \
                     (i32_reinterpret_f32)  \
                     (i64_reinterpret_f64)  \
                     (f32_reinterpret_i32)  \
                     (f64_reinterpret_i64)  \
                     (grow_memory)          \
                     (current_memory)       \
/* BLOCK TYPE OPS */                        \
                     (block)                \
                     (loop)                 \
                     (if_)                  \
/* 32bit OPS */                             \
                     (br)                   \
                     (br_if)                \
                     (call)                 \
                     (call_indirect)        \
                     (get_local)            \
                     (set_local)            \
                     (tee_local)            \
                     (get_global)           \
                     (set_global)           \
                     (i32_const)            \
                     (f32_const)            \
/* memarg OPS */                            \
                     (i32_load)             \
                     (i64_load)             \
                     (f32_load)             \
                     (f64_load)             \
                     (i32_load8_s)          \
                     (i32_load8_u)          \
                     (i32_load16_s)         \
                     (i32_load16_u)         \
                     (i64_load8_s)          \
                     (i64_load8_u)          \
                     (i64_load16_s)         \
                     (i64_load16_u)         \
                     (i64_load32_s)         \
                     (i64_load32_u)         \
                     (i32_store)            \
                     (i64_store)            \
                     (f32_store)            \
                     (f64_store)            \
                     (i32_store8)           \
                     (i32_store16)          \
                     (i64_store8)           \
                     (i64_store16)          \
                     (i64_store32)          \
/* 64bit OPS */                             \
                     (i64_const)            \
                     (f64_const)            \
/* branchtable op */                        \
                     (br_table)             \

enum code {
   unreachable_code           = 0x00,
   nop_code                   = 0x01,
   block_code                 = 0x02,
   loop_code                  = 0x03,
   if__code                   = 0x04,
   else__code                 = 0x05,
   end_code                   = 0x0B,
   br_code                    = 0x0C,
   br_if_code                 = 0x0D,
   br_table_code              = 0x0E,
   return__code               = 0x0F,
   call_code                  = 0x10,
   call_indirect_code         = 0x11,
   drop_code                  = 0x1A,
   select_code                = 0x1B,
   get_local_code             = 0x20,
   set_local_code             = 0x21,
   tee_local_code             = 0x22,
   get_global_code            = 0x23,
   set_global_code            = 0x24,
   i32_load_code              = 0x28,
   i64_load_code              = 0x29,
   f32_load_code              = 0x2A,
   f64_load_code              = 0x2B,
   i32_load8_s_code           = 0x2C,
   i32_load8_u_code           = 0x2D,
   i32_load16_s_code          = 0x2E,
   i32_load16_u_code          = 0x2F,
   i64_load8_s_code           = 0x30,
   i64_load8_u_code           = 0x31,
   i64_load16_s_code          = 0x32,
   i64_load16_u_code          = 0x33,
   i64_load32_s_code          = 0x34,
   i64_load32_u_code          = 0x35,
   i32_store_code             = 0x36,
   i64_store_code             = 0x37,
   f32_store_code             = 0x38,
   f64_store_code             = 0x39,
   i32_store8_code            = 0x3A,
   i32_store16_code           = 0x3B,
   i64_store8_code            = 0x3C,
   i64_store16_code           = 0x3D,
   i64_store32_code           = 0x3E,
   current_memory_code        = 0x3F,
   grow_memory_code           = 0x40,
   i32_const_code             = 0x41,
   i64_const_code             = 0x42,
   f32_const_code             = 0x43,
   f64_const_code             = 0x44,
   i32_eqz_code               = 0x45,
   i32_eq_code                = 0x46,
   i32_ne_code                = 0x47,
   i32_lt_s_code              = 0x48,
   i32_lt_u_code              = 0x49,
   i32_gt_s_code              = 0x4A,
   i32_gt_u_code              = 0x4B,
   i32_le_s_code              = 0x4C,
   i32_le_u_code              = 0x4D,
   i32_ge_s_code              = 0x4E,
   i32_ge_u_code              = 0x4F,
   i64_eqz_code               = 0x50,
   i64_eq_code                = 0x51,
   i64_ne_code                = 0x52,
   i64_lt_s_code              = 0x53,
   i64_lt_u_code              = 0x54,
   i64_gt_s_code              = 0x55,
   i64_gt_u_code              = 0x56,
   i64_le_s_code              = 0x57,
   i64_le_u_code              = 0x58,
   i64_ge_s_code              = 0x59,
   i64_ge_u_code              = 0x5A,
   f32_eq_code                = 0x5B,
   f32_ne_code                = 0x5C,
   f32_lt_code                = 0x5D,
   f32_gt_code                = 0x5E,
   f32_le_code                = 0x5F,
   f32_ge_code                = 0x60,
   f64_eq_code                = 0x61,
   f64_ne_code                = 0x62,
   f64_lt_code                = 0x63,
   f64_gt_code                = 0x64,
   f64_le_code                = 0x65,
   f64_ge_code                = 0x66,
   i32_clz_code               = 0x67,
   i32_ctz_code               = 0x68,
   i32_popcnt_code            = 0x69,
   i32_add_code               = 0x6A,
   i32_sub_code               = 0x6B,
   i32_mul_code               = 0x6C,
   i32_div_s_code             = 0x6D,
   i32_div_u_code             = 0x6E,
   i32_rem_s_code             = 0x6F,
   i32_rem_u_code             = 0x70,
   i32_and_code               = 0x71,
   i32_or_code                = 0x72,
   i32_xor_code               = 0x73,
   i32_shl_code               = 0x74,
   i32_shr_s_code             = 0x75,
   i32_shr_u_code             = 0x76,
   i32_rotl_code              = 0x77,
   i32_rotr_code              = 0x78,
   i64_clz_code               = 0x79,
   i64_ctz_code               = 0x7A,
   i64_popcnt_code            = 0x7B,
   i64_add_code               = 0x7C,
   i64_sub_code               = 0x7D,
   i64_mul_code               = 0x7E,
   i64_div_s_code             = 0x7F,
   i64_div_u_code             = 0x80,
   i64_rem_s_code             = 0x81,
   i64_rem_u_code             = 0x82,
   i64_and_code               = 0x83,
   i64_or_code                = 0x84,
   i64_xor_code               = 0x85,
   i64_shl_code               = 0x86,
   i64_shr_s_code             = 0x87,
   i64_shr_u_code             = 0x88,
   i64_rotl_code              = 0x89,
   i64_rotr_code              = 0x8A,
   f32_abs_code               = 0x8B,
   f32_neg_code               = 0x8C,
   f32_ceil_code              = 0x8D,
   f32_floor_code             = 0x8E,
   f32_trunc_code             = 0x8F,
   f32_nearest_code           = 0x90,
   f32_sqrt_code              = 0x91,
   f32_add_code               = 0x92,
   f32_sub_code               = 0x93,
   f32_mul_code               = 0x94,
   f32_div_code               = 0x95,
   f32_min_code               = 0x96,
   f32_max_code               = 0x97,
   f32_copysign_code          = 0x98,
   f64_abs_code               = 0x99,
   f64_neg_code               = 0x9A,
   f64_ceil_code              = 0x9B,
   f64_floor_code             = 0x9C,
   f64_trunc_code             = 0x9D,
   f64_nearest_code           = 0x9E,
   f64_sqrt_code              = 0x9F,
   f64_add_code               = 0xA0,
   f64_sub_code               = 0xA1,
   f64_mul_code               = 0xA2,
   f64_div_code               = 0xA3,
   f64_min_code               = 0xA4,
   f64_max_code               = 0xA5,
   f64_copysign_code          = 0xA6,
   i32_wrap_i64_code          = 0xA7,
   i32_trunc_s_f32_code       = 0xA8,
   i32_trunc_u_f32_code       = 0xA9,
   i32_trunc_s_f64_code       = 0xAA,
   i32_trunc_u_f64_code       = 0xAB,
   i64_extend_s_i32_code      = 0xAC,
   i64_extend_u_i32_code      = 0xAD,
   i64_trunc_s_f32_code       = 0xAE,
   i64_trunc_u_f32_code       = 0xAF,
   i64_trunc_s_f64_code       = 0xB0,
   i64_trunc_u_f64_code       = 0xB1,
   f32_convert_s_i32_code     = 0xB2,
   f32_convert_u_i32_code     = 0xB3,
   f32_convert_s_i64_code     = 0xB4,
   f32_convert_u_i64_code     = 0xB5,
   f32_demote_f64_code        = 0xB6,
   f64_convert_s_i32_code     = 0xB7,
   f64_convert_u_i32_code     = 0xB8,
   f64_convert_s_i64_code     = 0xB9,
   f64_convert_u_i64_code     = 0xBA,
   f64_promote_f32_code       = 0xBB,
   i32_reinterpret_f32_code   = 0xBC,
   i64_reinterpret_f64_code   = 0xBD,
   f32_reinterpret_i32_code   = 0xBE,
   f64_reinterpret_i64_code   = 0xBF,
   error_code                 = 0xFF,
}; // code

struct visitor_arg {
   IR::Module*         module;
   instruction_stream* new_code;
   IR::FunctionDef*    function_def;
   size_t              start_index;
};

struct instr {
   virtual std::string to_string() { return "instr"; }
   virtual uint16_t get_code() = 0;
   virtual void visit( visitor_arg&& arg ) = 0;
   virtual int skip_ahead() = 0;
   virtual void unpack( char* opcode ) = 0;
   virtual void pack( instruction_stream* stream ) = 0;
   virtual bool is_kill() = 0;
   virtual bool is_post() = 0;
};

// base injector and utility classes for encoding if we should reflect the instr to the new code block
template <typename Mutator, typename ... Mutators>
struct propagate_should_kill {
   static constexpr bool value = Mutator::kills || propagate_should_kill<Mutators...>::value;
};
template <typename Mutator>
struct propagate_should_kill<Mutator> {
   static constexpr bool value = Mutator::kills;
};
template <typename Mutator, typename ... Mutators>
struct propagate_post_injection {
   static constexpr bool value = Mutator::post || propagate_post_injection<Mutators...>::value;
};
template <typename Mutator>
struct propagate_post_injection<Mutator> {
   static constexpr bool value = Mutator::post;
};
template <typename ... Mutators>
struct instr_base : instr {
   bool is_post() override { return propagate_post_injection<Mutators...>::value; }
   bool is_kill() override { return propagate_should_kill<Mutators...>::value; }
   virtual void visit( visitor_arg&& arg ) override {
      for ( auto m : { Mutators::accept... } ) {
         m(this, arg);
      }
   } 
};

// construct the instructions
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, voidtype,         BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 0, 133 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, blocktype,        BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 133, 3 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, uint32_t,         BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 136, 11 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, memarg,           BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 147, 23 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, uint64_t,         BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 170, 2 ) )
BOOST_PP_SEQ_FOR_EACH( CONSTRUCT_OP_HAS_DATA, branchtabletype,  BOOST_PP_SEQ_SUBSEQ( WASM_OP_SEQ, 172, 1 ) )
#undef CONSTRUCT_OP_HAS_DATA

#pragma pack (pop)

struct nop_mutator {
   static constexpr bool kills = false;
   static constexpr bool post = false;
   static void accept( instr* inst, visitor_arg& arg ) {}
};

// class to inherit from to attach specific mutators and have default behavior for all specified types
template < typename Mutator = nop_mutator, typename ... Mutators>
struct op_types {
#define GEN_TYPE( r, T, OP ) \
   using BOOST_PP_CAT( OP, _t ) = OP < T , BOOST_PP_CAT(T, s) ...>;
   BOOST_PP_SEQ_FOR_EACH( GEN_TYPE, Mutator, WASM_OP_SEQ )
#undef GEN_TYPE
}; // op_types


/** 
 * Section for cached ops
 */
template <class Op_Types>
class cached_ops {
#define GEN_FIELD( r, P, OP ) \
   static std::unique_ptr<typename Op_Types::BOOST_PP_CAT(OP,_t)> BOOST_PP_CAT(P, OP);
   BOOST_PP_SEQ_FOR_EACH( GEN_FIELD, cached_, WASM_OP_SEQ )
#undef GEN_FIELD

   static std::vector<instr*> _cached_ops;
   public:
   static std::vector<instr*>* get_cached_ops() {
#define PUSH_BACK_OP( r, T, OP ) \
         _cached_ops[BOOST_PP_CAT(OP,_code)] = BOOST_PP_CAT(T, OP).get();
      if ( _cached_ops.empty() ) {
         // prefill with error
         _cached_ops.resize( 256, cached_error.get() );
         BOOST_PP_SEQ_FOR_EACH( PUSH_BACK_OP, cached_ , WASM_OP_SEQ )
      }
#undef PUSH_BACK_OP
      return &_cached_ops;
   }
};

template <class Op_Types>
std::vector<instr*> cached_ops<Op_Types>::_cached_ops; 

#define INIT_FIELD( r, P, OP ) \
   template <class Op_Types>   \
   std::unique_ptr<typename Op_Types::BOOST_PP_CAT(OP,_t)> cached_ops<Op_Types>::BOOST_PP_CAT(P, OP) = std::make_unique<typename Op_Types::BOOST_PP_CAT(OP,_t)>();
   BOOST_PP_SEQ_FOR_EACH( INIT_FIELD, cached_, WASM_OP_SEQ )

template <class Op_Types>
std::vector<instr*>* get_cached_ops_vec() {
 #define GEN_FIELD( r, P, OP ) \
   static std::unique_ptr<typename Op_Types::BOOST_PP_CAT(OP,_t)> BOOST_PP_CAT(P, OP) = std::make_unique<typename Op_Types::BOOST_PP_CAT(OP,_t)>();
   BOOST_PP_SEQ_FOR_EACH( GEN_FIELD, cached_, WASM_OP_SEQ )
 #undef GEN_FIELD
   static std::vector<instr*> _cached_ops;

#define PUSH_BACK_OP( r, T, OP ) \
      _cached_ops[BOOST_PP_CAT(OP,_code)] = BOOST_PP_CAT(T, OP).get();

   if ( _cached_ops.empty() ) {
      // prefill with error
      _cached_ops.resize( 256, cached_error.get() );
      BOOST_PP_SEQ_FOR_EACH( PUSH_BACK_OP, cached_ , WASM_OP_SEQ )
   }
#undef PUSH_BACK_OP
   return &_cached_ops;
}
using namespace IR;

// Decodes an operator from an input stream and dispatches by opcode.
// This code is from wasm-jit/Include/IR/Operators.h
template <class Op_Types>
struct EOSIO_OperatorDecoderStream
{
   EOSIO_OperatorDecoderStream(const std::vector<U8>& codeBytes)
   : start(codeBytes.data()), nextByte(codeBytes.data()), end(codeBytes.data()+codeBytes.size()) {
     if(!_cached_ops)
        _cached_ops = cached_ops<Op_Types>::get_cached_ops();
   }

   operator bool() const { return nextByte < end; }

   instr* decodeOp() {
      assert(nextByte + sizeof(IR::Opcode) <= end);
      IR::Opcode opcode = *(IR::Opcode*)nextByte;  
      switch(opcode)
      {
      #define VISIT_OPCODE(opcode,name,nameString,Imm,...) \
         case IR::Opcode::name: \
         { \
            assert(nextByte + sizeof(IR::OpcodeAndImm<IR::Imm>) <= end); \
            IR::OpcodeAndImm<IR::Imm>* encodedOperator = (IR::OpcodeAndImm<IR::Imm>*)nextByte; \
            nextByte += sizeof(IR::OpcodeAndImm<IR::Imm>); \
            auto op = _cached_ops->at(BOOST_PP_CAT(name, _code)); \
            op->unpack( reinterpret_cast<char*>(&(encodedOperator->imm)) ); \
            return op;  \
         }
      ENUM_OPERATORS(VISIT_OPCODE)
      #undef VISIT_OPCODE
      default:
         nextByte += sizeof(IR::Opcode);
         return _cached_ops->at(error_code); 
      }
   }

   instr* decodeOpWithoutConsume() {
      const U8* savedNextByte = nextByte;
      instr* result = decodeOp();
      nextByte = savedNextByte;
      return result;
   }
   inline uint32_t index() { return nextByte - start; }
private:
   // cached ops to take the address of 
   static const std::vector<instr*>* _cached_ops;
   const U8* start;
   const U8* nextByte;
   const U8* end;
};

template <class Op_Types>
const std::vector<instr*>* EOSIO_OperatorDecoderStream<Op_Types>::_cached_ops;

}}} // namespace eosio, chain, wasm_ops

FC_REFLECT_TEMPLATE( (typename T), eosio::chain::wasm_ops::block< T >, (code)(rt) )
FC_REFLECT( eosio::chain::wasm_ops::memarg, (a)(o) )
FC_REFLECT( eosio::chain::wasm_ops::blocktype, (result) )
FC_REFLECT( eosio::chain::wasm_ops::memoryoptype, (end) )
