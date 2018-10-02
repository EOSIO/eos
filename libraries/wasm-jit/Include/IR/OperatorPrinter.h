#pragma once

#include "Inline/BasicTypes.h"
#include "Inline/Serialization.h"
#include "IR.h"
#include "Module.h"
#include "Operators.h"

namespace IR
{
	struct OperatorPrinter
	{
		typedef std::string Result;

		OperatorPrinter(const Module& inModule,const FunctionDef& inFunctionDef)
		: module(inModule), functionDef(inFunctionDef) {}

		#define VISIT_OPCODE(encoding,name,nameString,Imm,...) \
			std::string name(Imm imm = {}) \
			{ \
				return std::string(nameString) + describeImm(imm); \
			}
		ENUM_OPERATORS(VISIT_OPCODE)
		#undef VISIT_OPCODE

		std::string unknown(Opcode opcode)
		{
			return "<unknown opcode " + std::to_string((Uptr)opcode) + ">";
		}
	private:
		const Module& module;
		const FunctionDef& functionDef;

		std::string describeImm(NoImm) { return ""; }
		std::string describeImm(ControlStructureImm imm) { return std::string(" : ") + asString(imm.resultType); }
		std::string describeImm(BranchImm imm) { return " " + std::to_string(imm.targetDepth); }
		std::string describeImm(BranchTableImm imm)
		{
			std::string result = " " + std::to_string(imm.defaultTargetDepth);
			const char* prefix = " [";
			WAVM_ASSERT_THROW(imm.branchTableIndex < functionDef.branchTables.size());
			for(auto depth : functionDef.branchTables[imm.branchTableIndex]) { result += prefix + std::to_string(depth); prefix = ","; }
			result += "]";
			return result;
		}
		template<typename NativeValue>
		std::string describeImm(LiteralImm<NativeValue> imm) { return " " + asString(imm.value); }
		template<bool isGlobal>
		std::string describeImm(GetOrSetVariableImm<isGlobal> imm) { return " " + std::to_string(imm.variableIndex); }
		std::string describeImm(CallImm imm)
		{
			const std::string typeString = imm.functionIndex >= module.functions.size()
				? "<invalid function index>"
				: asString(module.types[module.functions.getType(imm.functionIndex).index]);
			return " " + std::to_string(imm.functionIndex) + " " + typeString;
		}
		std::string describeImm(CallIndirectImm imm)
		{
			const std::string typeString = imm.type.index >= module.types.size()
				? "<invalid type index>"
				: asString(module.types[imm.type.index]);
			return " " + typeString;
		}
		template<Uptr naturalAlignmentLog2>
		std::string describeImm(LoadOrStoreImm<naturalAlignmentLog2> imm)
		{
			return " align=" + std::to_string(1<<imm.alignmentLog2) + " offset=" + std::to_string(imm.offset);
		}
		std::string describeImm(MemoryImm) { return ""; }

		#if ENABLE_SIMD_PROTOTYPE
		template<Uptr numLanes>
		std::string describeImm(LaneIndexImm<numLanes> imm) { return " " + std::to_string(imm.laneIndex); }
		template<Uptr numLanes>
		std::string describeImm(ShuffleImm<numLanes> imm)
		{
			std::string result = " [";
			const char* prefix = "";
			for(Uptr laneIndex = 0;laneIndex < numLanes;++laneIndex)
			{
				result += prefix
					+ (imm.laneIndices[laneIndex] < numLanes ? 'a' : 'b')
					+ std::to_string(imm.laneIndices[laneIndex]);
				prefix = ",";
			}
			return result;
		}
		#endif

		#if ENABLE_THREADING_PROTOTYPE
		std::string describeImm(LaunchThreadImm) { return ""; }
		
		template<Uptr naturalAlignmentLog2>
		std::string describeImm(AtomicLoadOrStoreImm<naturalAlignmentLog2> imm)
		{
			return " align=" + std::to_string(1<<imm.alignmentLog2) + " offset=" + std::to_string(imm.offset);
		}
		#endif
	};
}