#pragma once

#include "Inline/BasicTypes.h"
#include "IR.h"
#include "Types.h"
#include "Inline/Serialization.h"

namespace IR
{
	// Structures for operator immediates

	struct NoImm {};
	struct MemoryImm {};

	struct ControlStructureImm
	{
		ResultType resultType;
	};

	struct BranchImm
	{
		U32 targetDepth;
	};

	struct BranchTableImm
	{
		Uptr defaultTargetDepth;
		Uptr branchTableIndex; // An index into the FunctionDef's branchTables array.
	};

	template<typename Value>
	struct LiteralImm
	{
		Value value;
	};

	template<bool isGlobal>
	struct GetOrSetVariableImm
	{
		U32 variableIndex;
	};

	struct CallImm
	{
		U32 functionIndex;
	};

	struct CallIndirectImm
	{
		IndexedFunctionType type;
	};

	template<Uptr naturalAlignmentLog2>
	struct LoadOrStoreImm
	{
		U8 alignmentLog2;
		U32 offset;
	};

	#if ENABLE_SIMD_PROTOTYPE
	template<Uptr numLanes>
	struct LaneIndexImm
	{
		U8 laneIndex;
	};
	
	template<Uptr numLanes>
	struct ShuffleImm
	{
		U8 laneIndices[numLanes];
	};
	#endif

	#if ENABLE_THREADING_PROTOTYPE
	struct LaunchThreadImm {};
	
	template<Uptr naturalAlignmentLog2>
	struct AtomicLoadOrStoreImm
	{
		U8 alignmentLog2;
		U32 offset;
	};
	#endif

	// Enumate the WebAssembly operators

	#define ENUM_CONTROL_OPERATORS(visitOp) \
		visitOp(0x02,block,"block",ControlStructureImm,CONTROL) \
		visitOp(0x03,loop,"loop",ControlStructureImm,CONTROL) \
		visitOp(0x04,if_,"if",ControlStructureImm,CONTROL) \
		visitOp(0x05,else_,"else",NoImm,CONTROL) \
		visitOp(0x0b,end,"end",NoImm,CONTROL)

	#define ENUM_PARAMETRIC_OPERATORS(visitOp) \
		visitOp(0x00,unreachable,"unreachable",NoImm,UNREACHABLE) \
		visitOp(0x0c,br,"br",BranchImm,BR) \
		visitOp(0x0d,br_if,"br_if",BranchImm,BR_IF) \
		visitOp(0x0e,br_table,"br_table",BranchTableImm,BR_TABLE) \
		visitOp(0x0f,return_,"return",NoImm,RETURN) \
		visitOp(0x10,call,"call",CallImm,CALL) \
		visitOp(0x11,call_indirect,"call_indirect",CallIndirectImm,CALLINDIRECT) \
		visitOp(0x1a,drop,"drop",NoImm,DROP) \
		visitOp(0x1b,select,"select",NoImm,SELECT) \
		\
		visitOp(0x20,get_local,"get_local",GetOrSetVariableImm<false>,GETLOCAL) \
		visitOp(0x21,set_local,"set_local",GetOrSetVariableImm<false>,SETLOCAL) \
		visitOp(0x22,tee_local,"tee_local",GetOrSetVariableImm<false>,TEELOCAL) \
		visitOp(0x23,get_global,"get_global",GetOrSetVariableImm<true>,GETGLOBAL) \
		visitOp(0x24,set_global,"set_global",GetOrSetVariableImm<true>,SETGLOBAL)
	
	/*
		Possible signatures used by ENUM_NONCONTROL_NONPARAMETRIC_OPERATORS:
		NULLARY(T) : () -> T
		UNARY(T,U) : T -> U
		BINARY(T,U) : (T,T) -> U
		LOAD(T) : i32 -> T
		STORE(T) : (i32,T) -> ()
		VECTORSELECT(V) : (V,V,V) -> V
        REPLACELANE(S,V) : (V,S) -> V
		COMPAREEXCHANGE(T) : (i32,T,T) -> T
		WAIT(T) : (i32,T,f64) -> i32
		LAUNCHTHREAD : (i32,i32,i32) -> ()
		ATOMICRMW : (i32,T) -> T
	*/

   #define ENUM_MEMORY_OPERATORS(visitOp) \
		visitOp(0x3f,current_memory,"current_memory",MemoryImm,NULLARY(i32)) \
		visitOp(0x40,grow_memory,"grow_memory",MemoryImm,UNARY(i32,i32))

   #define ENUM_NONCONTROL_NONPARAMETRIC_OPERATORS(visitOp) \
      ENUM_NONFLOAT_NONCONTROL_NONPARAMETRIC_OPERATORS(visitOp) \
      ENUM_FLOAT_NONCONTROL_NONPARAMETRIC_OPERATORS(visitOp)

	#define ENUM_NONFLOAT_NONCONTROL_NONPARAMETRIC_OPERATORS(visitOp) \
		visitOp(0x01,nop,"nop",NoImm,NULLARY(none)) \
		\
		ENUM_MEMORY_OPERATORS(visitOp) \
		\
		visitOp(0x28,i32_load,"i32.load",LoadOrStoreImm<2>,LOAD(i32)) \
		visitOp(0x29,i64_load,"i64.load",LoadOrStoreImm<3>,LOAD(i64)) \
		visitOp(0x2c,i32_load8_s,"i32.load8_s",LoadOrStoreImm<0>,LOAD(i32)) \
		visitOp(0x2d,i32_load8_u,"i32.load8_u",LoadOrStoreImm<0>,LOAD(i32)) \
		visitOp(0x2e,i32_load16_s,"i32.load16_s",LoadOrStoreImm<1>,LOAD(i32)) \
		visitOp(0x2f,i32_load16_u,"i32.load16_u",LoadOrStoreImm<1>,LOAD(i32)) \
		visitOp(0x30,i64_load8_s,"i64.load8_s",LoadOrStoreImm<0>,LOAD(i64)) \
		visitOp(0x31,i64_load8_u,"i64.load8_u",LoadOrStoreImm<0>,LOAD(i64)) \
		visitOp(0x32,i64_load16_s,"i64.load16_s",LoadOrStoreImm<1>,LOAD(i64)) \
		visitOp(0x33,i64_load16_u,"i64.load16_u",LoadOrStoreImm<1>,LOAD(i64)) \
		visitOp(0x34,i64_load32_s,"i64.load32_s",LoadOrStoreImm<2>,LOAD(i64)) \
		visitOp(0x35,i64_load32_u,"i64.load32_u",LoadOrStoreImm<2>,LOAD(i64)) \
		\
		visitOp(0x36,i32_store,"i32.store",LoadOrStoreImm<2>,STORE(i32)) \
		visitOp(0x37,i64_store,"i64.store",LoadOrStoreImm<3>,STORE(i64)) \
		visitOp(0x3a,i32_store8,"i32.store8",LoadOrStoreImm<0>,STORE(i32)) \
		visitOp(0x3b,i32_store16,"i32.store16",LoadOrStoreImm<1>,STORE(i32)) \
		visitOp(0x3c,i64_store8,"i64.store8",LoadOrStoreImm<0>,STORE(i64)) \
		visitOp(0x3d,i64_store16,"i64.store16",LoadOrStoreImm<1>,STORE(i64)) \
		visitOp(0x3e,i64_store32,"i64.store32",LoadOrStoreImm<2>,STORE(i64)) \
		\
		visitOp(0x41,i32_const,"i32.const",LiteralImm<I32>,NULLARY(i32)) \
		visitOp(0x42,i64_const,"i64.const",LiteralImm<I64>,NULLARY(i64)) \
		\
		visitOp(0x45,i32_eqz,"i32.eqz",NoImm,UNARY(i32,i32)) \
		visitOp(0x46,i32_eq,"i32.eq",NoImm,BINARY(i32,i32)) \
		visitOp(0x47,i32_ne,"i32.ne",NoImm,BINARY(i32,i32)) \
		visitOp(0x48,i32_lt_s,"i32.lt_s",NoImm,BINARY(i32,i32)) \
		visitOp(0x49,i32_lt_u,"i32.lt_u",NoImm,BINARY(i32,i32)) \
		visitOp(0x4a,i32_gt_s,"i32.gt_s",NoImm,BINARY(i32,i32)) \
		visitOp(0x4b,i32_gt_u,"i32.gt_u",NoImm,BINARY(i32,i32)) \
		visitOp(0x4c,i32_le_s,"i32.le_s",NoImm,BINARY(i32,i32)) \
		visitOp(0x4d,i32_le_u,"i32.le_u",NoImm,BINARY(i32,i32)) \
		visitOp(0x4e,i32_ge_s,"i32.ge_s",NoImm,BINARY(i32,i32)) \
		visitOp(0x4f,i32_ge_u,"i32.ge_u",NoImm,BINARY(i32,i32)) \
		\
		visitOp(0x50,i64_eqz,"i64.eqz",NoImm,UNARY(i64,i32)) \
		visitOp(0x51,i64_eq,"i64.eq",NoImm,BINARY(i64,i32)) \
		visitOp(0x52,i64_ne,"i64.ne",NoImm,BINARY(i64,i32)) \
		visitOp(0x53,i64_lt_s,"i64.lt_s",NoImm,BINARY(i64,i32)) \
		visitOp(0x54,i64_lt_u,"i64.lt_u",NoImm,BINARY(i64,i32)) \
		visitOp(0x55,i64_gt_s,"i64.gt_s",NoImm,BINARY(i64,i32)) \
		visitOp(0x56,i64_gt_u,"i64.gt_u",NoImm,BINARY(i64,i32)) \
		visitOp(0x57,i64_le_s,"i64.le_s",NoImm,BINARY(i64,i32)) \
		visitOp(0x58,i64_le_u,"i64.le_u",NoImm,BINARY(i64,i32)) \
		visitOp(0x59,i64_ge_s,"i64.ge_s",NoImm,BINARY(i64,i32)) \
		visitOp(0x5a,i64_ge_u,"i64.ge_u",NoImm,BINARY(i64,i32)) \
		\
		visitOp(0x67,i32_clz,"i32.clz",NoImm,UNARY(i32,i32)) \
		visitOp(0x68,i32_ctz,"i32.ctz",NoImm,UNARY(i32,i32)) \
		visitOp(0x69,i32_popcnt,"i32.popcnt",NoImm,UNARY(i32,i32)) \
		\
		visitOp(0x6a,i32_add,"i32.add",NoImm,BINARY(i32,i32)) \
		visitOp(0x6b,i32_sub,"i32.sub",NoImm,BINARY(i32,i32)) \
		visitOp(0x6c,i32_mul,"i32.mul",NoImm,BINARY(i32,i32)) \
		visitOp(0x6d,i32_div_s,"i32.div_s",NoImm,BINARY(i32,i32)) \
		visitOp(0x6e,i32_div_u,"i32.div_u",NoImm,BINARY(i32,i32)) \
		visitOp(0x6f,i32_rem_s,"i32.rem_s",NoImm,BINARY(i32,i32)) \
		visitOp(0x70,i32_rem_u,"i32.rem_u",NoImm,BINARY(i32,i32)) \
		visitOp(0x71,i32_and,"i32.and",NoImm,BINARY(i32,i32)) \
		visitOp(0x72,i32_or,"i32.or",NoImm,BINARY(i32,i32)) \
		visitOp(0x73,i32_xor,"i32.xor",NoImm,BINARY(i32,i32)) \
		visitOp(0x74,i32_shl,"i32.shl",NoImm,BINARY(i32,i32)) \
		visitOp(0x75,i32_shr_s,"i32.shr_s",NoImm,BINARY(i32,i32)) \
		visitOp(0x76,i32_shr_u,"i32.shr_u",NoImm,BINARY(i32,i32)) \
		visitOp(0x77,i32_rotl,"i32.rotl",NoImm,BINARY(i32,i32)) \
		visitOp(0x78,i32_rotr,"i32.rotr",NoImm,BINARY(i32,i32)) \
		\
		visitOp(0x79,i64_clz,"i64.clz",NoImm,UNARY(i64,i64)) \
		visitOp(0x7a,i64_ctz,"i64.ctz",NoImm,UNARY(i64,i64)) \
		visitOp(0x7b,i64_popcnt,"i64.popcnt",NoImm,UNARY(i64,i64)) \
		\
		visitOp(0x7c,i64_add,"i64.add",NoImm,BINARY(i64,i64)) \
		visitOp(0x7d,i64_sub,"i64.sub",NoImm,BINARY(i64,i64)) \
		visitOp(0x7e,i64_mul,"i64.mul",NoImm,BINARY(i64,i64)) \
		visitOp(0x7f,i64_div_s,"i64.div_s",NoImm,BINARY(i64,i64)) \
		visitOp(0x80,i64_div_u,"i64.div_u",NoImm,BINARY(i64,i64)) \
		visitOp(0x81,i64_rem_s,"i64.rem_s",NoImm,BINARY(i64,i64)) \
		visitOp(0x82,i64_rem_u,"i64.rem_u",NoImm,BINARY(i64,i64)) \
		visitOp(0x83,i64_and,"i64.and",NoImm,BINARY(i64,i64)) \
		visitOp(0x84,i64_or,"i64.or",NoImm,BINARY(i64,i64)) \
		visitOp(0x85,i64_xor,"i64.xor",NoImm,BINARY(i64,i64)) \
		visitOp(0x86,i64_shl,"i64.shl",NoImm,BINARY(i64,i64)) \
		visitOp(0x87,i64_shr_s,"i64.shr_s",NoImm,BINARY(i64,i64)) \
		visitOp(0x88,i64_shr_u,"i64.shr_u",NoImm,BINARY(i64,i64)) \
		visitOp(0x89,i64_rotl,"i64.rotl",NoImm,BINARY(i64,i64)) \
		visitOp(0x8a,i64_rotr,"i64.rotr",NoImm,BINARY(i64,i64)) \
		\
		visitOp(0xa7,i32_wrap_i64,"i32.wrap/i64",NoImm,UNARY(i64,i32)) \
		visitOp(0xac,i64_extend_s_i32,"i64.extend_s/i32",NoImm,UNARY(i32,i64)) \
		visitOp(0xad,i64_extend_u_i32,"i64.extend_u/i32",NoImm,UNARY(i32,i64)) \
		ENUM_NONFLOAT_SIMD_OPERATORS(visitOp) \
		ENUM_NONFLOAT_THREADING_OPERATORS(visitOp)

   #define ENUM_FLOAT_NONCONTROL_NONPARAMETRIC_OPERATORS(visitOp) \
      visitOp(0x2a,f32_load,"f32.load",LoadOrStoreImm<2>,LOAD(f32)) \
      visitOp(0x2b,f64_load,"f64.load",LoadOrStoreImm<3>,LOAD(f64)) \
      \
      visitOp(0x38,f32_store,"f32.store",LoadOrStoreImm<2>,STORE(f32)) \
      visitOp(0x39,f64_store,"f64.store",LoadOrStoreImm<3>,STORE(f64)) \
      \
      visitOp(0x43,f32_const,"f32.const",LiteralImm<F32>,NULLARY(f32)) \
      visitOp(0x44,f64_const,"f64.const",LiteralImm<F64>,NULLARY(f64)) \
      \
      visitOp(0x5b,f32_eq,"f32.eq",NoImm,BINARY(f32,i32)) \
      visitOp(0x5c,f32_ne,"f32.ne",NoImm,BINARY(f32,i32)) \
      visitOp(0x5d,f32_lt,"f32.lt",NoImm,BINARY(f32,i32)) \
      visitOp(0x5e,f32_gt,"f32.gt",NoImm,BINARY(f32,i32)) \
      visitOp(0x5f,f32_le,"f32.le",NoImm,BINARY(f32,i32)) \
      visitOp(0x60,f32_ge,"f32.ge",NoImm,BINARY(f32,i32)) \
      \
      visitOp(0x61,f64_eq,"f64.eq",NoImm,BINARY(f64,i32)) \
      visitOp(0x62,f64_ne,"f64.ne",NoImm,BINARY(f64,i32)) \
      visitOp(0x63,f64_lt,"f64.lt",NoImm,BINARY(f64,i32)) \
      visitOp(0x64,f64_gt,"f64.gt",NoImm,BINARY(f64,i32)) \
      visitOp(0x65,f64_le,"f64.le",NoImm,BINARY(f64,i32)) \
      visitOp(0x66,f64_ge,"f64.ge",NoImm,BINARY(f64,i32)) \
      \
      visitOp(0x8b,f32_abs,"f32.abs",NoImm,UNARY(f32,f32)) \
      visitOp(0x8c,f32_neg,"f32.neg",NoImm,UNARY(f32,f32)) \
      visitOp(0x8d,f32_ceil,"f32.ceil",NoImm,UNARY(f32,f32)) \
      visitOp(0x8e,f32_floor,"f32.floor",NoImm,UNARY(f32,f32)) \
      visitOp(0x8f,f32_trunc,"f32.trunc",NoImm,UNARY(f32,f32)) \
      visitOp(0x90,f32_nearest,"f32.nearest",NoImm,UNARY(f32,f32)) \
      visitOp(0x91,f32_sqrt,"f32.sqrt",NoImm,UNARY(f32,f32)) \
      \
      visitOp(0x92,f32_add,"f32.add",NoImm,BINARY(f32,f32)) \
      visitOp(0x93,f32_sub,"f32.sub",NoImm,BINARY(f32,f32)) \
      visitOp(0x94,f32_mul,"f32.mul",NoImm,BINARY(f32,f32)) \
      visitOp(0x95,f32_div,"f32.div",NoImm,BINARY(f32,f32)) \
      visitOp(0x96,f32_min,"f32.min",NoImm,BINARY(f32,f32)) \
      visitOp(0x97,f32_max,"f32.max",NoImm,BINARY(f32,f32)) \
      visitOp(0x98,f32_copysign,"f32.copysign",NoImm,BINARY(f32,f32)) \
      \
      visitOp(0x99,f64_abs,"f64.abs",NoImm,UNARY(f64,f64)) \
      visitOp(0x9a,f64_neg,"f64.neg",NoImm,UNARY(f64,f64)) \
      visitOp(0x9b,f64_ceil,"f64.ceil",NoImm,UNARY(f64,f64)) \
      visitOp(0x9c,f64_floor,"f64.floor",NoImm,UNARY(f64,f64)) \
      visitOp(0x9d,f64_trunc,"f64.trunc",NoImm,UNARY(f64,f64)) \
      visitOp(0x9e,f64_nearest,"f64.nearest",NoImm,UNARY(f64,f64)) \
      visitOp(0x9f,f64_sqrt,"f64.sqrt",NoImm,UNARY(f64,f64)) \
      \
      visitOp(0xa0,f64_add,"f64.add",NoImm,BINARY(f64,f64)) \
      visitOp(0xa1,f64_sub,"f64.sub",NoImm,BINARY(f64,f64)) \
      visitOp(0xa2,f64_mul,"f64.mul",NoImm,BINARY(f64,f64)) \
      visitOp(0xa3,f64_div,"f64.div",NoImm,BINARY(f64,f64)) \
      visitOp(0xa4,f64_min,"f64.min",NoImm,BINARY(f64,f64)) \
      visitOp(0xa5,f64_max,"f64.max",NoImm,BINARY(f64,f64)) \
      visitOp(0xa6,f64_copysign,"f64.copysign",NoImm,BINARY(f64,f64)) \
      \
      visitOp(0xa8,i32_trunc_s_f32,"i32.trunc_s/f32",NoImm,UNARY(f32,i32)) \
      visitOp(0xa9,i32_trunc_u_f32,"i32.trunc_u/f32",NoImm,UNARY(f32,i32)) \
      visitOp(0xaa,i32_trunc_s_f64,"i32.trunc_s/f64",NoImm,UNARY(f64,i32)) \
      visitOp(0xab,i32_trunc_u_f64,"i32.trunc_u/f64",NoImm,UNARY(f64,i32)) \
      visitOp(0xae,i64_trunc_s_f32,"i64.trunc_s/f32",NoImm,UNARY(f32,i64)) \
      visitOp(0xaf,i64_trunc_u_f32,"i64.trunc_u/f32",NoImm,UNARY(f32,i64)) \
      visitOp(0xb0,i64_trunc_s_f64,"i64.trunc_s/f64",NoImm,UNARY(f64,i64)) \
      visitOp(0xb1,i64_trunc_u_f64,"i64.trunc_u/f64",NoImm,UNARY(f64,i64)) \
      visitOp(0xb2,f32_convert_s_i32,"f32.convert_s/i32",NoImm,UNARY(i32,f32)) \
      visitOp(0xb3,f32_convert_u_i32,"f32.convert_u/i32",NoImm,UNARY(i32,f32)) \
      visitOp(0xb4,f32_convert_s_i64,"f32.convert_s/i64",NoImm,UNARY(i64,f32)) \
      visitOp(0xb5,f32_convert_u_i64,"f32.convert_u/i64",NoImm,UNARY(i64,f32)) \
      visitOp(0xb6,f32_demote_f64,"f32.demote/f64",NoImm,UNARY(f64,f32)) \
      visitOp(0xb7,f64_convert_s_i32,"f64.convert_s/i32",NoImm,UNARY(i32,f64)) \
      visitOp(0xb8,f64_convert_u_i32,"f64.convert_u/i32",NoImm,UNARY(i32,f64)) \
      visitOp(0xb9,f64_convert_s_i64,"f64.convert_s/i64",NoImm,UNARY(i64,f64)) \
      visitOp(0xba,f64_convert_u_i64,"f64.convert_u/i64",NoImm,UNARY(i64,f64)) \
      visitOp(0xbb,f64_promote_f32,"f64.promote/f32",NoImm,UNARY(f32,f64)) \
      visitOp(0xbc,i32_reinterpret_f32,"i32.reinterpret/f32",NoImm,UNARY(f32,i32)) \
      visitOp(0xbd,i64_reinterpret_f64,"i64.reinterpret/f64",NoImm,UNARY(f64,i64)) \
      visitOp(0xbe,f32_reinterpret_i32,"f32.reinterpret/i32",NoImm,UNARY(i32,f32)) \
      visitOp(0xbf,f64_reinterpret_i64,"f64.reinterpret/i64",NoImm,UNARY(i64,f64)) \
      ENUM_FLOAT_SIMD_OPERATORS(visitOp) \
      ENUM_FLOAT_THREADING_OPERATORS(visitOp)

	#if !ENABLE_SIMD_PROTOTYPE
	#define ENUM_NONFLOAT_SIMD_OPERATORS(visitOp)
   #define ENUM_FLOAT_SIMD_OPERATORS(visitOp)
	#else
	#define SIMDOP(simdOpIndex) (0xfd00|simdOpIndex)
	#define ENUM_NONFLOAT_SIMD_OPERATORS(visitOp) \
		visitOp(SIMDOP(0),v128_const,"v128.const",LiteralImm<V128>,NULLARY(v128)) \
		visitOp(SIMDOP(1),v128_load,"v128.load",LoadOrStoreImm<4>,LOAD(v128)) \
		visitOp(SIMDOP(2),v128_store,"v128.store",LoadOrStoreImm<4>,STORE(v128)) \
		\
		visitOp(SIMDOP(3),i8x16_splat,"i8x16.splat",NoImm,UNARY(i32,v128)) \
		visitOp(SIMDOP(4),i16x8_splat,"i16x8.splat",NoImm,UNARY(i32,v128)) \
		visitOp(SIMDOP(5),i32x4_splat,"i32x4.splat",NoImm,UNARY(i32,v128)) \
		visitOp(SIMDOP(6),i64x2_splat,"i64x2.splat",NoImm,UNARY(i64,v128)) \
		\
		visitOp(SIMDOP(9),i8x16_extract_lane_s,"i8x16.extract_lane_s",LaneIndexImm<16>,UNARY(v128,i32)) \
		visitOp(SIMDOP(10),i8x16_extract_lane_u,"i8x16.extract_lane_u",LaneIndexImm<16>,UNARY(v128,i32)) \
		visitOp(SIMDOP(11),i16x8_extract_lane_s,"i16x8.extract_lane_s",LaneIndexImm<8>,UNARY(v128,i32)) \
		visitOp(SIMDOP(12),i16x8_extract_lane_u,"i16x8.extract_lane_u",LaneIndexImm<8>,UNARY(v128,i32)) \
		visitOp(SIMDOP(13),i32x4_extract_lane,"i32x4.extract_lane",LaneIndexImm<4>,UNARY(v128,i32)) \
		visitOp(SIMDOP(14),i64x2_extract_lane,"i64x2.extract_lane",LaneIndexImm<2>,UNARY(v128,i64)) \
		\
		visitOp(SIMDOP(17),i8x16_replace_lane,"i8x16.replace_lane",LaneIndexImm<16>,REPLACELANE(i32,v128)) \
		visitOp(SIMDOP(18),i16x8_replace_lane,"i16x8.replace_lane",LaneIndexImm<8>,REPLACELANE(i32,v128)) \
		visitOp(SIMDOP(19),i32x4_replace_lane,"i32x4.replace_lane",LaneIndexImm<4>,REPLACELANE(i32,v128)) \
		visitOp(SIMDOP(20),i64x2_replace_lane,"i64x2.replace_lane",LaneIndexImm<2>,REPLACELANE(i64,v128)) \
		\
		visitOp(SIMDOP(23),v8x16_shuffle,"v8x16.shuffle",ShuffleImm<16>,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(24),i8x16_add,"i8x16.add",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(25),i16x8_add,"i16x8.add",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(26),i32x4_add,"i32x4.add",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(27),i64x2_add,"i64x2.add",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(28),i8x16_sub,"i8x16.sub",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(29),i16x8_sub,"i16x8.sub",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(30),i32x4_sub,"i32x4.sub",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(31),i64x2_sub,"i64x2.sub",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(32),i8x16_mul,"i8x16.mul",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(33),i16x8_mul,"i16x8.mul",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(34),i32x4_mul,"i32x4.mul",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(35),i8x16_neg,"i8x16.neg",NoImm,UNARY(v128,v128)) \
		visitOp(SIMDOP(36),i16x8_neg,"i16x8.neg",NoImm,UNARY(v128,v128)) \
		visitOp(SIMDOP(37),i32x4_neg,"i32x4.neg",NoImm,UNARY(v128,v128)) \
		visitOp(SIMDOP(38),i64x2_neg,"i64x2.neg",NoImm,UNARY(v128,v128)) \
		\
		visitOp(SIMDOP(39),i8x16_add_saturate_s,"i8x16.add_saturate_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(40),i8x16_add_saturate_u,"i8x16.add_saturate_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(41),i16x8_add_saturate_s,"i16x8.add_saturate_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(42),i16x8_add_saturate_u,"i16x8.add_saturate_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(43),i8x16_sub_saturate_s,"i8x16.sub_saturate_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(44),i8x16_sub_saturate_u,"i8x16.sub_saturate_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(45),i16x8_sub_saturate_s,"i16x8.sub_saturate_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(46),i16x8_sub_saturate_u,"i16x8.sub_saturate_u",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(47),i8x16_shl,"i8x16.shl",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(48),i16x8_shl,"i16x8.shl",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(49),i32x4_shl,"i32x4.shl",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(50),i64x2_shl,"i64x2.shl",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(51),i8x16_shr_s,"i8x16.shr_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(52),i8x16_shr_u,"i8x16.shr_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(53),i16x8_shr_s,"i16x8.shr_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(54),i16x8_shr_u,"i16x8.shr_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(55),i32x4_shr_s,"i32x4.shr_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(56),i32x4_shr_u,"i32x4.shr_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(57),i64x2_shr_s,"i64x2.shr_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(58),i64x2_shr_u,"i64x2.shr_u",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(59),v128_and,"v128.and",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(60),v128_or,"v128.or",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(61),v128_xor,"v128.xor",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(62),v128_not,"v128.not",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(63),v128_bitselect,"v128.bitselect",NoImm,VECTORSELECT(v128)) \
		\
		visitOp(SIMDOP(64),i8x16_any_true,"i8x16.any_true",NoImm,UNARY(v128,i32)) \
		visitOp(SIMDOP(65),i16x8_any_true,"i16x8.any_true",NoImm,UNARY(v128,i32)) \
		visitOp(SIMDOP(66),i32x4_any_true,"i32x4.any_true",NoImm,UNARY(v128,i32)) \
		visitOp(SIMDOP(67),i64x2_any_true,"i64x2.any_true",NoImm,UNARY(v128,i32)) \
		\
		visitOp(SIMDOP(68),i8x16_all_true,"i8x16.all_true",NoImm,UNARY(v128,i32)) \
		visitOp(SIMDOP(69),i16x8_all_true,"i16x8.all_true",NoImm,UNARY(v128,i32)) \
		visitOp(SIMDOP(70),i32x4_all_true,"i32x4.all_true",NoImm,UNARY(v128,i32)) \
		visitOp(SIMDOP(71),i64x2_all_true,"i64x2.all_true",NoImm,UNARY(v128,i32)) \
		\
		visitOp(SIMDOP(72),i8x16_eq,"i8x16.eq",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(73),i16x8_eq,"i16x8.eq",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(74),i32x4_eq,"i32x4.eq",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(77),i8x16_ne,"i8x16.ne",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(78),i16x8_ne,"i16x8.ne",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(79),i32x4_ne,"i32x4.ne",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(82),i8x16_lt_s,"i8x16.lt_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(83),i8x16_lt_u,"i8x16.lt_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(84),i16x8_lt_s,"i16x8.lt_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(85),i16x8_lt_u,"i16x8.lt_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(86),i32x4_lt_s,"i32x4.lt_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(87),i32x4_lt_u,"i32x4.lt_u",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(90),i8x16_le_s,"i8x16.le_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(91),i8x16_le_u,"i8x16.le_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(92),i16x8_le_s,"i16x8.le_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(93),i16x8_le_u,"i16x8.le_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(94),i32x4_le_s,"i32x4.le_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(95),i32x4_le_u,"i32x4.le_u",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(98),i8x16_gt_s,"i8x16.gt_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(99),i8x16_gt_u,"i8x16.gt_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(100),i16x8_gt_s,"i16x8.gt_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(101),i16x8_gt_u,"i16x8.gt_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(102),i32x4_gt_s,"i32x4.gt_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(103),i32x4_gt_u,"i32x4.gt_u",NoImm,BINARY(v128,v128)) \
		\
		visitOp(SIMDOP(106),i8x16_ge_s,"i8x16.ge_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(107),i8x16_ge_u,"i8x16.ge_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(108),i16x8_ge_s,"i16x8.ge_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(109),i16x8_ge_u,"i16x8.ge_u",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(110),i32x4_ge_s,"i32x4.ge_s",NoImm,BINARY(v128,v128)) \
		visitOp(SIMDOP(111),i32x4_ge_u,"i32x4.ge_u",NoImm,BINARY(v128,v128))
   #define ENUM_FLOAT_SIMD_OPERATORS(visitOp) \
      visitOp(SIMDOP(7),f32x4_splat,"f32x4.splat",NoImm,UNARY(f32,v128)) \
      visitOp(SIMDOP(8),f64x2_splat,"f64x2.splat",NoImm,UNARY(f64,v128)) \
      \
      visitOp(SIMDOP(15),f32x4_extract_lane,"f32x4.extract_lane",LaneIndexImm<4>,UNARY(v128,f32)) \
      visitOp(SIMDOP(16),f64x2_extract_lane,"f64x2.extract_lane",LaneIndexImm<2>,UNARY(v128,f64)) \
      \
      visitOp(SIMDOP(21),f32x4_replace_lane,"f32x4.replace_lane",LaneIndexImm<4>,REPLACELANE(f32,v128)) \
      visitOp(SIMDOP(22),f64x2_replace_lane,"f64x2.replace_lane",LaneIndexImm<2>,REPLACELANE(f64,v128)) \
      \
      visitOp(SIMDOP(75),f32x4_eq,"f32x4.eq",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(76),f64x2_eq,"f64x2.eq",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(80),f32x4_ne,"f32x4.ne",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(81),f64x2_ne,"f64x2.ne",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(88),f32x4_lt,"f32x4.lt",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(89),f64x2_lt,"f64x2.lt",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(96),f32x4_le,"f32x4.le",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(97),f64x2_le,"f64x2.le",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(104),f32x4_gt,"f32x4.gt",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(105),f64x2_gt,"f64x2.gt",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(112),f32x4_ge,"f32x4.ge",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(113),f64x2_ge,"f64x2.ge",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(114),f32x4_neg,"f32x4.neg",NoImm,UNARY(v128,v128)) \
      visitOp(SIMDOP(115),f64x2_neg,"f64x2.neg",NoImm,UNARY(v128,v128)) \
      \
      visitOp(SIMDOP(116),f32x4_abs,"f32x4.abs",NoImm,UNARY(v128,v128)) \
      visitOp(SIMDOP(117),f64x2_abs,"f64x2.abs",NoImm,UNARY(v128,v128)) \
      \
      visitOp(SIMDOP(118),f32x4_min,"f32x4.min",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(119),f64x2_min,"f64x2.min",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(120),f32x4_max,"f32x4.max",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(121),f64x2_max,"f64x2.max",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(122),f32x4_add,"f32x4.add",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(123),f64x2_add,"f64x2.add",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(124),f32x4_sub,"f32x4.sub",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(125),f64x2_sub,"f64x2.sub",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(126),f32x4_div,"f32x4.div",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(127),f64x2_div,"f64x2.div",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(128),f32x4_mul,"f32x4.mul",NoImm,BINARY(v128,v128)) \
      visitOp(SIMDOP(129),f64x2_mul,"f64x2.mul",NoImm,BINARY(v128,v128)) \
      \
      visitOp(SIMDOP(130),f32x4_sqrt,"f32x4.sqrt",NoImm,UNARY(v128,v128)) \
      visitOp(SIMDOP(131),f64x2_sqrt,"f64x2.sqrt",NoImm,UNARY(v128,v128)) \
      \
      visitOp(SIMDOP(132),f32x4_convert_s_i32x4,"f32x4.convert_s/i32x4",NoImm,UNARY(v128,v128)) \
      visitOp(SIMDOP(133),f32x4_convert_u_i32x4,"f32x4.convert_u/i32x4",NoImm,UNARY(v128,v128)) \
      visitOp(SIMDOP(134),f64x2_convert_s_i64x2,"f64x2.convert_s/i64x2",NoImm,UNARY(v128,v128)) \
      visitOp(SIMDOP(135),f64x2_convert_u_i64x2,"f64x2.convert_u/i64x2",NoImm,UNARY(v128,v128)) \
      \
      visitOp(SIMDOP(136),i32x4_trunc_s_f32x4_sat,"i32x4.trunc_s/f32x4:sat",NoImm,UNARY(v128,v128)) \
      visitOp(SIMDOP(137),i32x4_trunc_u_f32x4_sat,"i32x4.trunc_u/f32x4:sat",NoImm,UNARY(v128,v128)) \
      visitOp(SIMDOP(138),i64x2_trunc_s_f64x2_sat,"i64x2.trunc_s/f64x2:sat",NoImm,UNARY(v128,v128)) \
      visitOp(SIMDOP(139),i64x2_trunc_u_f64x2_sat,"i64x2.trunc_u/f64x2:sat",NoImm,UNARY(v128,v128))
	#endif

	#if !ENABLE_THREADING_PROTOTYPE
	#define ENUM_NONFLOAT_THREADING_OPERATORS(visitOp)
   #define ENUM_FLOAT_THREADING_OPERATORS(visitOp)
	#else
	#define ENUM_NONFLOAT_THREADING_OPERATORS(visitOp) \
		visitOp(0xc0,i32_extend_s_i8,"i32.extend_s/i8",NoImm,UNARY(i32,i32)) \
		visitOp(0xc1,i32_extend_s_i16,"i32.extend_s/i16",NoImm,UNARY(i32,i32)) \
		visitOp(0xc2,i64_extend_s_i8,"i64.extend_s/i8",NoImm,UNARY(i64,i64)) \
		visitOp(0xc3,i64_extend_s_i16,"i64.extend_s/i16",NoImm,UNARY(i64,i64)) \
		\
		visitOp(0xfe01,wake,"wake",AtomicLoadOrStoreImm<2>,BINARY(i32,i32)) \
		visitOp(0xfe02,i32_wait,"i32.wait",AtomicLoadOrStoreImm<2>,WAIT(i32)) \
		visitOp(0xfe03,i64_wait,"i64.wait",AtomicLoadOrStoreImm<3>,WAIT(i64)) \
		visitOp(0xfe04,launch_thread,"launch_thread",LaunchThreadImm,LAUNCHTHREAD) \
		\
		visitOp(0xfe09,i32_atomic_rmw_xchg,"i32.atomic.rmw.xchg",AtomicLoadOrStoreImm<2>,ATOMICRMW(i32)) \
		visitOp(0xfe0a,i64_atomic_rmw_xchg,"i64.atomic.rmw.xchg",AtomicLoadOrStoreImm<3>,ATOMICRMW(i64)) \
		visitOp(0xfe0c,i32_atomic_rmw8_u_xchg,"i32.atomic.rmw8_u.xchg",AtomicLoadOrStoreImm<0>,ATOMICRMW(i32)) \
		visitOp(0xfe0e,i32_atomic_rmw16_u_xchg,"i32.atomic.rmw16_u.xchg",AtomicLoadOrStoreImm<1>,ATOMICRMW(i32)) \
		visitOp(0xfe10,i64_atomic_rmw8_u_xchg,"i64.atomic.rmw8_u.xchg",AtomicLoadOrStoreImm<0>,ATOMICRMW(i64)) \
		visitOp(0xfe12,i64_atomic_rmw16_u_xchg,"i64.atomic.rmw16_u.xchg",AtomicLoadOrStoreImm<1>,ATOMICRMW(i64)) \
		visitOp(0xfe14,i64_atomic_rmw32_u_xchg,"i64.atomic.rmw32_u.xchg",AtomicLoadOrStoreImm<2>,ATOMICRMW(i64)) \
		\
		visitOp(0xfe15,i32_atomic_rmw_cmpxchg,"i32.atomic.rmw.cmpxchg",AtomicLoadOrStoreImm<2>,COMPAREEXCHANGE(i32)) \
		visitOp(0xfe16,i64_atomic_rmw_cmpxchg,"i64.atomic.rmw.cmpxchg",AtomicLoadOrStoreImm<3>,COMPAREEXCHANGE(i64)) \
		visitOp(0xfe18,i32_atomic_rmw8_u_cmpxchg,"i32.atomic.rmw8_u.cmpxchg",AtomicLoadOrStoreImm<0>,COMPAREEXCHANGE(i32)) \
		visitOp(0xfe1a,i32_atomic_rmw16_u_cmpxchg,"i32.atomic.rmw16_u.cmpxchg",AtomicLoadOrStoreImm<1>,COMPAREEXCHANGE(i32)) \
		visitOp(0xfe1c,i64_atomic_rmw8_u_cmpxchg,"i64.atomic.rmw8_u.cmpxchg",AtomicLoadOrStoreImm<0>,COMPAREEXCHANGE(i64)) \
		visitOp(0xfe1e,i64_atomic_rmw16_u_cmpxchg,"i64.atomic.rmw16_u.cmpxchg",AtomicLoadOrStoreImm<1>,COMPAREEXCHANGE(i64)) \
		visitOp(0xfe20,i64_atomic_rmw32_u_cmpxchg,"i64.atomic.rmw32_u.cmpxchg",AtomicLoadOrStoreImm<2>,COMPAREEXCHANGE(i64)) \
		\
		visitOp(0xfe21,i32_atomic_load,"i32.atomic.load",AtomicLoadOrStoreImm<2>,LOAD(i32)) \
		visitOp(0xfe22,i64_atomic_load,"i64.atomic.load",AtomicLoadOrStoreImm<3>,LOAD(i64)) \
		visitOp(0xfe23,i32_atomic_load8_u,"i32.atomic.load8_u",AtomicLoadOrStoreImm<0>,LOAD(i32)) \
		visitOp(0xfe24,i32_atomic_load16_u,"i32.atomic.load16_u",AtomicLoadOrStoreImm<1>,LOAD(i32)) \
		visitOp(0xfe25,i64_atomic_load8_u,"i64.atomic.load8_u",AtomicLoadOrStoreImm<0>,LOAD(i64)) \
		visitOp(0xfe26,i64_atomic_load16_u,"i64.atomic.load16_u",AtomicLoadOrStoreImm<1>,LOAD(i64)) \
		visitOp(0xfe27,i64_atomic_load32_u,"i64.atomic.load32_u",AtomicLoadOrStoreImm<2>,LOAD(i64)) \
		visitOp(0xfe28,i32_atomic_store,"i32.atomic.store",AtomicLoadOrStoreImm<2>,STORE(i32)) \
		visitOp(0xfe29,i64_atomic_store,"i64.atomic.store",AtomicLoadOrStoreImm<3>,STORE(i64)) \
		visitOp(0xfe2c,i32_atomic_store8,"i32.atomic.store8",AtomicLoadOrStoreImm<0>,STORE(i32)) \
		visitOp(0xfe2d,i32_atomic_store16,"i32.atomic.store16",AtomicLoadOrStoreImm<1>,STORE(i32)) \
		visitOp(0xfe2e,i64_atomic_store8,"i64.atomic.store8",AtomicLoadOrStoreImm<0>,STORE(i64)) \
		visitOp(0xfe2f,i64_atomic_store16,"i64.atomic.store16",AtomicLoadOrStoreImm<1>,STORE(i64)) \
		visitOp(0xfe30,i64_atomic_store32,"i64.atomic.store32",AtomicLoadOrStoreImm<2>,STORE(i64)) \
		\
		visitOp(0xfe3f,i32_atomic_rmw_add,"i32.atomic.rmw.add",AtomicLoadOrStoreImm<2>,ATOMICRMW(i32)) \
		visitOp(0xfe40,i64_atomic_rmw_add,"i64.atomic.rmw.add",AtomicLoadOrStoreImm<3>,ATOMICRMW(i64)) \
		visitOp(0xfe42,i32_atomic_rmw8_u_add,"i32.atomic.rmw8_u.add",AtomicLoadOrStoreImm<0>,ATOMICRMW(i32)) \
		visitOp(0xfe44,i32_atomic_rmw16_u_add,"i32.atomic.rmw16_u.add",AtomicLoadOrStoreImm<1>,ATOMICRMW(i32)) \
		visitOp(0xfe46,i64_atomic_rmw8_u_add,"i64.atomic.rmw8_u.add",AtomicLoadOrStoreImm<0>,ATOMICRMW(i64)) \
		visitOp(0xfe48,i64_atomic_rmw16_u_add,"i64.atomic.rmw16_u.add",AtomicLoadOrStoreImm<1>,ATOMICRMW(i64)) \
		visitOp(0xfe4a,i64_atomic_rmw32_u_add,"i64.atomic.rmw32_u.add",AtomicLoadOrStoreImm<2>,ATOMICRMW(i64)) \
		\
		visitOp(0xfe4b,i32_atomic_rmw_sub,"i32.atomic.rmw.sub",AtomicLoadOrStoreImm<2>,ATOMICRMW(i32)) \
		visitOp(0xfe4c,i64_atomic_rmw_sub,"i64.atomic.rmw.sub",AtomicLoadOrStoreImm<3>,ATOMICRMW(i64)) \
		visitOp(0xfe4e,i32_atomic_rmw8_u_sub,"i32.atomic.rmw8_u.sub",AtomicLoadOrStoreImm<0>,ATOMICRMW(i32)) \
		visitOp(0xfe50,i32_atomic_rmw16_u_sub,"i32.atomic.rmw16_u.sub",AtomicLoadOrStoreImm<1>,ATOMICRMW(i32)) \
		visitOp(0xfe52,i64_atomic_rmw8_u_sub,"i64.atomic.rmw8_u.sub",AtomicLoadOrStoreImm<0>,ATOMICRMW(i64)) \
		visitOp(0xfe54,i64_atomic_rmw16_u_sub,"i64.atomic.rmw16_u.sub",AtomicLoadOrStoreImm<1>,ATOMICRMW(i64)) \
		visitOp(0xfe56,i64_atomic_rmw32_u_sub,"i64.atomic.rmw32_u.sub",AtomicLoadOrStoreImm<2>,ATOMICRMW(i64)) \
		\
		visitOp(0xfe57,i32_atomic_rmw_and,"i32.atomic.rmw.and",AtomicLoadOrStoreImm<2>,ATOMICRMW(i32)) \
		visitOp(0xfe58,i64_atomic_rmw_and,"i64.atomic.rmw.and",AtomicLoadOrStoreImm<3>,ATOMICRMW(i64)) \
		visitOp(0xfe5a,i32_atomic_rmw8_u_and,"i32.atomic.rmw8_u.and",AtomicLoadOrStoreImm<0>,ATOMICRMW(i32)) \
		visitOp(0xfe5c,i32_atomic_rmw16_u_and,"i32.atomic.rmw16_u.and",AtomicLoadOrStoreImm<1>,ATOMICRMW(i32)) \
		visitOp(0xfe5e,i64_atomic_rmw8_u_and,"i64.atomic.rmw8_u.and",AtomicLoadOrStoreImm<0>,ATOMICRMW(i64)) \
		visitOp(0xfe60,i64_atomic_rmw16_u_and,"i64.atomic.rmw16_u.and",AtomicLoadOrStoreImm<1>,ATOMICRMW(i64)) \
		visitOp(0xfe62,i64_atomic_rmw32_u_and,"i64.atomic.rmw32_u.and",AtomicLoadOrStoreImm<2>,ATOMICRMW(i64)) \
		\
		visitOp(0xfe63,i32_atomic_rmw_or,"i32.atomic.rmw.or",AtomicLoadOrStoreImm<2>,ATOMICRMW(i32)) \
		visitOp(0xfe64,i64_atomic_rmw_or,"i64.atomic.rmw.or",AtomicLoadOrStoreImm<3>,ATOMICRMW(i64)) \
		visitOp(0xfe66,i32_atomic_rmw8_u_or,"i32.atomic.rmw8_u.or",AtomicLoadOrStoreImm<0>,ATOMICRMW(i32)) \
		visitOp(0xfe68,i32_atomic_rmw16_u_or,"i32.atomic.rmw16_u.or",AtomicLoadOrStoreImm<1>,ATOMICRMW(i32)) \
		visitOp(0xfe6a,i64_atomic_rmw8_u_or,"i64.atomic.rmw8_u.or",AtomicLoadOrStoreImm<0>,ATOMICRMW(i64)) \
		visitOp(0xfe6c,i64_atomic_rmw16_u_or,"i64.atomic.rmw16_u.or",AtomicLoadOrStoreImm<1>,ATOMICRMW(i64)) \
		visitOp(0xfe6e,i64_atomic_rmw32_u_or,"i64.atomic.rmw32_u.or",AtomicLoadOrStoreImm<2>,ATOMICRMW(i64)) \
		\
		visitOp(0xfe6f,i32_atomic_rmw_xor,"i32.atomic.rmw.xor",AtomicLoadOrStoreImm<2>,ATOMICRMW(i32)) \
		visitOp(0xfe70,i64_atomic_rmw_xor,"i64.atomic.rmw.xor",AtomicLoadOrStoreImm<3>,ATOMICRMW(i64)) \
		visitOp(0xfe72,i32_atomic_rmw8_u_xor,"i32.atomic.rmw8_u.xor",AtomicLoadOrStoreImm<0>,ATOMICRMW(i32)) \
		visitOp(0xfe74,i32_atomic_rmw16_u_xor,"i32.atomic.rmw16_u.xor",AtomicLoadOrStoreImm<1>,ATOMICRMW(i32)) \
		visitOp(0xfe76,i64_atomic_rmw8_u_xor,"i64.atomic.rmw8_u.xor",AtomicLoadOrStoreImm<0>,ATOMICRMW(i64)) \
		visitOp(0xfe78,i64_atomic_rmw16_u_xor,"i64.atomic.rmw16_u.xor",AtomicLoadOrStoreImm<1>,ATOMICRMW(i64)) \
		visitOp(0xfe7a,i64_atomic_rmw32_u_xor,"i64.atomic.rmw32_u.xor",AtomicLoadOrStoreImm<2>,ATOMICRMW(i64))
   #define ENUM_FLOAT_THREADING_OPERATORS(visitOp) \
      visitOp(0xfe2a,f32_atomic_store,"f32.atomic.store",AtomicLoadOrStoreImm<2>,STORE(f32)) \
      visitOp(0xfe2b,f64_atomic_store,"f64.atomic.store",AtomicLoadOrStoreImm<3>,STORE(f64))
	#endif

	#define ENUM_NONCONTROL_OPERATORS(visitOp) \
		ENUM_PARAMETRIC_OPERATORS(visitOp) \
		ENUM_NONCONTROL_NONPARAMETRIC_OPERATORS(visitOp)

  #define ENUM_NONFLOAT_NONCONTROL_OPERATORS(visitOp) \
      ENUM_PARAMETRIC_OPERATORS(visitOp) \
      ENUM_NONFLOAT_NONCONTROL_NONPARAMETRIC_OPERATORS(visitOp)

	#define ENUM_OPERATORS(visitOp) \
		ENUM_NONCONTROL_OPERATORS(visitOp) \
		ENUM_CONTROL_OPERATORS(visitOp)

   #define ENUM_NONFLOAT_OPERATORS(visitOp) \
      ENUM_NONFLOAT_NONCONTROL_OPERATORS(visitOp) \
      ENUM_CONTROL_OPERATORS(visitOp)

	enum class Opcode : U16
	{
		#define VISIT_OPCODE(opcode,name,...) name = opcode,
		ENUM_OPERATORS(VISIT_OPCODE)
		#undef VISIT_OPCODE

		maxSingleByteOpcode = 0xcf,
	};

	PACKED_STRUCT(
	template<typename Imm>
	struct OpcodeAndImm
	{
		Opcode opcode;
		Imm imm;
	});

	// Specialize for the empty immediate structs so they don't take an extra byte of space.
	template<>
	struct OpcodeAndImm<NoImm>
	{
		union
		{
			Opcode opcode;
			NoImm imm;
		};
	};
	template<>
	struct OpcodeAndImm<MemoryImm>
	{
		union
		{
			Opcode opcode;
			MemoryImm imm;
		};
	};

	// Decodes an operator from an input stream and dispatches by opcode.
	struct OperatorDecoderStream
	{
		OperatorDecoderStream(const std::vector<U8>& codeBytes)
		: nextByte(codeBytes.data()), end(codeBytes.data()+codeBytes.size()) {}

		operator bool() const { return nextByte < end; }

		template<typename Visitor>
		typename Visitor::Result decodeOp(Visitor& visitor)
		{
			assert(nextByte + sizeof(Opcode) <= end);
			Opcode opcode = *(Opcode*)nextByte;
			switch(opcode)
			{
			#define VISIT_OPCODE(opcode,name,nameString,Imm,...) \
				case Opcode::name: \
				{ \
					assert(nextByte + sizeof(OpcodeAndImm<Imm>) <= end); \
					OpcodeAndImm<Imm>* encodedOperator = (OpcodeAndImm<Imm>*)nextByte; \
					nextByte += sizeof(OpcodeAndImm<Imm>); \
					return visitor.name(encodedOperator->imm); \
				}
			ENUM_OPERATORS(VISIT_OPCODE)
			#undef VISIT_OPCODE
			default:
				nextByte += sizeof(Opcode);
				return visitor.unknown(opcode);
			}
		}

		template<typename Visitor>
		typename Visitor::Result decodeOpWithoutConsume(Visitor& visitor)
		{
			const U8* savedNextByte = nextByte;
			typename Visitor::Result result = decodeOp(visitor);
			nextByte = savedNextByte;
			return result;
		}

	private:

		const U8* nextByte;
		const U8* end;
	};

	// Encodes an operator to an output stream.
	struct OperatorEncoderStream
	{
		OperatorEncoderStream(Serialization::OutputStream& inByteStream): byteStream(inByteStream) {}

		#define VISIT_OPCODE(_,name,nameString,Imm,...) \
			void name(Imm imm = {}) \
			{ \
				OpcodeAndImm<Imm>* encodedOperator = (OpcodeAndImm<Imm>*)byteStream.advance(sizeof(OpcodeAndImm<Imm>)); \
				encodedOperator->opcode = Opcode::name; \
				encodedOperator->imm = imm; \
			}
		ENUM_OPERATORS(VISIT_OPCODE)
		#undef VISIT_OPCODE

	private:
		Serialization::OutputStream& byteStream;
	};

	IR_API const char* getOpcodeName(Opcode opcode);
}
