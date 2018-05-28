#include "Inline/BasicTypes.h"
#include "Inline/Serialization.h"
#include "Inline/UTF8.h"
#include "WASM.h"
#include "IR/Module.h"
#include "IR/Operators.h"
#include "IR/Types.h"
#include "IR/Validate.h"

using namespace Serialization;

static void throwIfNotValidUTF8(const std::string& string)
{
	const U8* endChar = (const U8*)string.data() + string.size();
	if(UTF8::validateString((const U8*)string.data(),endChar) != endChar)
	{
		throw FatalSerializationException("invalid UTF-8 encoding");
	}
}
	
// These serialization functions need to be declared in the IR namespace for the array serializer in the Serialization namespace to find them.
namespace IR
{
	template<typename Stream>
	void serialize(Stream& stream,ValueType& type)
	{
		I8 encodedValueType = -(I8)type;
		serializeVarInt7(stream,encodedValueType);
		if(Stream::isInput) { type = (ValueType)-encodedValueType; }
	}

	FORCEINLINE static void serialize(InputStream& stream,ResultType& resultType)
	{
		Uptr arity;
		serializeVarUInt1(stream,arity);
		if(arity == 0) { resultType = ResultType::none; }
		else
		{
			I8 encodedValueType = 0;
			serializeVarInt7(stream,encodedValueType);
			resultType = (ResultType)-encodedValueType;
		}
	}
	static void serialize(OutputStream& stream,ResultType& returnType)
	{
		Uptr arity = returnType == ResultType::none ? 0 : 1;
		serializeVarUInt1(stream,arity);
		if(arity)
		{
			I8 encodedValueType = -(I8)returnType;
			serializeVarInt7(stream,encodedValueType);
		}
	}
	
	template<typename Stream>
	void serialize(Stream& stream,SizeConstraints& sizeConstraints,bool hasMax)
	{
		serializeVarUInt32(stream,sizeConstraints.min);
		if(hasMax) { serializeVarUInt32(stream,sizeConstraints.max); }
		else if(Stream::isInput) { sizeConstraints.max = UINT64_MAX; }
	}
	
	template<typename Stream>
	void serialize(Stream& stream,TableElementType& elementType)
	{
		serializeNativeValue(stream,elementType);
	}

	template<typename Stream>
	void serialize(Stream& stream,TableType& tableType)
	{
		serialize(stream,tableType.elementType);
		
		Uptr flags = 0;
		if(!Stream::isInput && tableType.size.max != UINT64_MAX) { flags |= 0x01; }
		#if ENABLE_THREADING_PROTOTYPE
		if(!Stream::isInput && tableType.isShared) { flags |= 0x10; }
		serializeVarUInt32(stream,flags);
		if(Stream::isInput) { tableType.isShared = (flags & 0x10) != 0; }
		#else
		serializeVarUInt32(stream,flags);
		#endif
		serialize(stream,tableType.size,flags & 0x01);
	}

	template<typename Stream>
	void serialize(Stream& stream,MemoryType& memoryType)
	{
		Uptr flags = 0;
		if(!Stream::isInput && memoryType.size.max != UINT64_MAX) { flags |= 0x01; }
		#if ENABLE_THREADING_PROTOTYPE
		if(!Stream::isInput && memoryType.isShared) { flags |= 0x10; }
		serializeVarUInt32(stream,flags);
		if(Stream::isInput) { memoryType.isShared = (flags & 0x10) != 0; }
		#else
		serializeVarUInt32(stream,flags);
		#endif
		serialize(stream,memoryType.size,flags & 0x01);
	}

	template<typename Stream>
	void serialize(Stream& stream,GlobalType& globalType)
	{
		serialize(stream,globalType.valueType);
		U8 isMutable = globalType.isMutable ? 1 : 0;
		serializeVarUInt1(stream,isMutable);
		if(Stream::isInput) { globalType.isMutable = isMutable != 0; }
	}
	
	template<typename Stream>
	void serialize(Stream& stream,ObjectKind& kind)
	{
		serializeNativeValue(stream,*(U8*)&kind);
	}

	template<typename Stream>
	void serialize(Stream& stream,Export& e)
	{
		serialize(stream,e.name);
		throwIfNotValidUTF8(e.name);
		serialize(stream,e.kind);
		serializeVarUInt32(stream,e.index);
	}

	template<typename Stream>
	void serialize(Stream& stream,InitializerExpression& initializer)
	{
		serializeNativeValue(stream,*(U8*)&initializer.type);
		switch(initializer.type)
		{
		case InitializerExpression::Type::i32_const: serializeVarInt32(stream,initializer.i32); break;
		case InitializerExpression::Type::i64_const: serializeVarInt64(stream,initializer.i64); break;
		case InitializerExpression::Type::f32_const: serialize(stream,initializer.f32); break;
		case InitializerExpression::Type::f64_const: serialize(stream,initializer.f64); break;
		case InitializerExpression::Type::get_global: serializeVarUInt32(stream,initializer.globalIndex); break;
		default: throw FatalSerializationException("invalid initializer expression opcode");
		}
		serializeConstant(stream,"expected end opcode",(U8)Opcode::end);
	}
	
	template<typename Stream>
	void serialize(Stream& stream,TableDef& tableDef)
	{
		serialize(stream,tableDef.type);
	}

	template<typename Stream>
	void serialize(Stream& stream,MemoryDef& memoryDef)
	{
		serialize(stream,memoryDef.type);
	}

	template<typename Stream>
	void serialize(Stream& stream,GlobalDef& globalDef)
	{
		serialize(stream,globalDef.type);
		serialize(stream,globalDef.initializer);
	}

	template<typename Stream>
	void serialize(Stream& stream,DataSegment& dataSegment)
	{
		serializeVarUInt32(stream,dataSegment.memoryIndex);
		serialize(stream,dataSegment.baseOffset);
		serialize(stream,dataSegment.data);
	}

	template<typename Stream>
	void serialize(Stream& stream,TableSegment& tableSegment)
	{
		serializeVarUInt32(stream,tableSegment.tableIndex);
		serialize(stream,tableSegment.baseOffset);
		serializeArray(stream,tableSegment.indices,[](Stream& stream,Uptr& functionIndex){serializeVarUInt32(stream,functionIndex);});
	}
}

namespace WASM
{
	using namespace IR;
	using namespace Serialization;
	
	enum
	{
		magicNumber=0x6d736100, // "\0asm"
		currentVersion=1
	};

	enum class SectionType : U8
	{
		unknown = 0,
		user = 0,
		type = 1,
		import = 2,
		functionDeclarations = 3,
		table = 4,
		memory = 5,
		global = 6,
		export_ = 7,
		start = 8,
		elem = 9,
		functionDefinitions = 10,
		data = 11
	};
	
	FORCEINLINE void serialize(InputStream& stream,Opcode& opcode)
	{
		opcode = (Opcode)0;
		Serialization::serializeNativeValue(stream,*(U8*)&opcode);
		if(opcode > Opcode::maxSingleByteOpcode)
		{
			opcode = (Opcode)(U16(opcode) << 8);
			Serialization::serializeNativeValue(stream,*(U8*)&opcode);
		}
	}
	FORCEINLINE void serialize(OutputStream& stream,Opcode opcode)
	{
		if(opcode <= Opcode::maxSingleByteOpcode) { Serialization::serializeNativeValue(stream,*(U8*)&opcode); }
		else
		{
			serializeNativeValue(stream,*(((U8*)&opcode) + 1));
			serializeNativeValue(stream,*(((U8*)&opcode) + 0));
		}
	}
	
	template<typename Stream>
	void serialize(Stream& stream,NoImm&,const FunctionDef&) {}
	
	template<typename Stream>
	void serialize(Stream& stream,ControlStructureImm& imm,const FunctionDef&)
	{
		I8 encodedResultType = imm.resultType == ResultType::none ? -64 : -(I8)imm.resultType;
		serializeVarInt7(stream,encodedResultType);
		if(Stream::isInput) { imm.resultType = encodedResultType == -64 ? ResultType::none : (ResultType)-encodedResultType; }
	}

	template<typename Stream>
	void serialize(Stream& stream,BranchImm& imm,const FunctionDef&)
	{
		serializeVarUInt32(stream,imm.targetDepth);
	}

	void serialize(InputStream& stream,BranchTableImm& imm,FunctionDef& functionDef)
	{
		std::vector<U32> branchTable;
		serializeArray(stream,branchTable,[](InputStream& stream,U32& targetDepth){serializeVarUInt32(stream,targetDepth);});
		imm.branchTableIndex = functionDef.branchTables.size();
		functionDef.branchTables.push_back(std::move(branchTable));
		serializeVarUInt32(stream,imm.defaultTargetDepth);
	}
	void serialize(OutputStream& stream,BranchTableImm& imm,FunctionDef& functionDef)
	{
		WAVM_ASSERT_THROW(imm.branchTableIndex < functionDef.branchTables.size());
		std::vector<U32>& branchTable = functionDef.branchTables[imm.branchTableIndex];
		serializeArray(stream,branchTable,[](OutputStream& stream,U32& targetDepth){serializeVarUInt32(stream,targetDepth);});
		serializeVarUInt32(stream,imm.defaultTargetDepth);
	}

	template<typename Stream>
	void serialize(Stream& stream,LiteralImm<I32>& imm,const FunctionDef&)
	{ serializeVarInt32(stream,imm.value); }
	
	template<typename Stream>
	void serialize(Stream& stream,LiteralImm<I64>& imm,const FunctionDef&)
	{ serializeVarInt64(stream,imm.value); }

	template<typename Stream,bool isGlobal>
	void serialize(Stream& stream,GetOrSetVariableImm<isGlobal>& imm,const FunctionDef&)
	{ serializeVarUInt32(stream,imm.variableIndex); }

	template<typename Stream>
	void serialize(Stream& stream,CallImm& imm,const FunctionDef&)
	{
		serializeVarUInt32(stream,imm.functionIndex);
	}

	template<typename Stream>
	void serialize(Stream& stream,CallIndirectImm& imm,const FunctionDef&)
	{
		serializeVarUInt32(stream,imm.type.index);
		serializeConstant(stream,"call_indirect immediate reserved field must be 0",U8(0));
	}

	template<typename Stream,Uptr naturalAlignmentLog2>
	void serialize(Stream& stream,LoadOrStoreImm<naturalAlignmentLog2>& imm,const FunctionDef&)
	{
		serializeVarUInt7(stream,imm.alignmentLog2);
		serializeVarUInt32(stream,imm.offset);
	}
	template<typename Stream>
	void serialize(Stream& stream,MemoryImm& imm,const FunctionDef&)
	{
		serializeConstant(stream,"grow_memory/current_memory immediate reserved field must be 0",U8(0));
	}

	#if ENABLE_SIMD_PROTOTYPE
		template<typename Stream>
		void serialize(Stream& stream,V128& v128)
		{
			serializeNativeValue(stream,v128);
		}
	
		template<Uptr numLanes>
		void serialize(InputStream& stream,BoolVector<numLanes>& boolVector)
		{
			U64 mask = 0;
			serializeBytes(stream,(U8*)&mask,(numLanes + 7) / 8);
			for(Uptr index = 0;index < numLanes;++index)
			{
				boolVector.b[index] = (mask & (U64(1) << index)) != 0;
			}
		}
	
		template<Uptr numLanes>
		void serialize(OutputStream& stream,BoolVector<numLanes>& boolVector)
		{
			U64 mask = 0;
			for(Uptr index = 0;index < numLanes;++index)
			{
				if(boolVector.b[index]) { mask |= U64(1) << index; }
			}
			serializeBytes(stream,(U8*)&mask,(numLanes + 7) / 8);
		}

		template<typename Stream,Uptr numLanes>
		void serialize(Stream& stream,LaneIndexImm<numLanes>& imm,const FunctionDef&)
		{
			serializeVarUInt7(stream,imm.laneIndex);
		}
		template<typename Stream,Uptr numLanes>
		void serialize(Stream& stream,ShuffleImm<numLanes>& imm,const FunctionDef&)
		{
			for(Uptr laneIndex = 0;laneIndex < numLanes;++laneIndex)
			{
				serializeVarUInt7(stream,imm.laneIndices[laneIndex]);
			}
		}
	#endif

	#if ENABLE_THREADING_PROTOTYPE
		template<typename Stream>
		void serialize(Stream& stream,LaunchThreadImm& imm,const FunctionDef&) {}
		
		template<typename Stream,Uptr naturalAlignmentLog2>
		void serialize(Stream& stream,AtomicLoadOrStoreImm<naturalAlignmentLog2>& imm,const FunctionDef&)
		{
			serializeVarUInt7(stream,imm.alignmentLog2);
			serializeVarUInt32(stream,imm.offset);
		}
	#endif
		
	template<typename Stream,typename Value>
	void serialize(Stream& stream,LiteralImm<Value>& imm,const FunctionDef&)
	{ serialize(stream,imm.value); }
		
	template<typename SerializeSection>
	void serializeSection(OutputStream& stream,SectionType type,SerializeSection serializeSectionBody)
	{
		serializeNativeValue(stream,type);
		ArrayOutputStream sectionStream;
		serializeSectionBody(sectionStream);
		std::vector<U8> sectionBytes = sectionStream.getBytes();
		Uptr sectionNumBytes = sectionBytes.size();
		serializeVarUInt32(stream,sectionNumBytes);
		serializeBytes(stream,sectionBytes.data(),sectionBytes.size());
	}
	template<typename SerializeSection>
	void serializeSection(InputStream& stream,SectionType expectedType,SerializeSection serializeSectionBody)
	{
		WAVM_ASSERT_THROW((SectionType)*stream.peek(sizeof(SectionType)) == expectedType);
		stream.advance(sizeof(SectionType));
		Uptr numSectionBytes = 0;
		serializeVarUInt32(stream,numSectionBytes);
		MemoryInputStream sectionStream(stream.advance(numSectionBytes),numSectionBytes);
		serializeSectionBody(sectionStream);
		if(sectionStream.capacity()) { throw FatalSerializationException("section contained more data than expected"); }
	}

	void serialize(OutputStream& stream,UserSection& userSection)
	{
		serializeConstant(stream,"expected user section (section ID 0)",(U8)SectionType::user);
		ArrayOutputStream sectionStream;
		serialize(sectionStream,userSection.name);
		userSection.data.resize( sectionStream.capacity() ? sectionStream.capacity() : 1 );
		serializeBytes(sectionStream,userSection.data.data(),userSection.data.size());
		std::vector<U8> sectionBytes = sectionStream.getBytes();
		serialize(stream,sectionBytes);
		if( !sectionStream.capacity() ) throw FatalSerializationException( "empty section" );
	}
	
	void serialize(InputStream& stream,UserSection& userSection)
	{
		serializeConstant(stream,"expected user section (section ID 0)",(U8)SectionType::user);
		Uptr numSectionBytes = 0;
		serializeVarUInt32(stream,numSectionBytes);
		
		MemoryInputStream sectionStream(stream.advance(numSectionBytes),numSectionBytes);
		serialize(sectionStream,userSection.name);
		throwIfNotValidUTF8(userSection.name);
		userSection.data.resize(sectionStream.capacity());
		serializeBytes(sectionStream,userSection.data.data(),userSection.data.size());
		WAVM_ASSERT_THROW(!sectionStream.capacity());
	}

	struct LocalSet
	{
		Uptr num;
		ValueType type;
	};
	
	template<typename Stream>
	void serialize(Stream& stream,LocalSet& localSet)
	{
		serializeVarUInt32(stream,localSet.num);
		serialize(stream,localSet.type);
	}

	struct OperatorSerializerStream
	{
		typedef void Result;

		OperatorSerializerStream(Serialization::OutputStream& inByteStream,FunctionDef& inFunctionDef)
		: byteStream(inByteStream), functionDef(inFunctionDef) {}

		#define VISIT_OPCODE(_,name,nameString,Imm,...) \
			void name(Imm imm) const \
			{ \
				Opcode opcode = Opcode::name; \
				serialize(byteStream,opcode); \
				serialize(byteStream,imm,functionDef); \
			}
		ENUM_OPERATORS(VISIT_OPCODE)
		#undef VISIT_OPCODE

		void unknown(Opcode opcode)
		{
			throw FatalSerializationException("unknown opcode");
		}
	private:
		Serialization::OutputStream& byteStream;
		FunctionDef& functionDef;
	};

	void serializeFunctionBody(OutputStream& sectionStream,Module& module,FunctionDef& functionDef)
	{
		ArrayOutputStream bodyStream;

		// Convert the function's local types into LocalSets: runs of locals of the same type.
		LocalSet* localSets = (LocalSet*)alloca(sizeof(LocalSet)*functionDef.nonParameterLocalTypes.size());
		Uptr numLocalSets = 0;
		if(functionDef.nonParameterLocalTypes.size())
		{
			localSets[0].type = ValueType::any;
			localSets[0].num = 0;
			for(auto localType : functionDef.nonParameterLocalTypes)
			{
				if(localSets[numLocalSets].type != localType)
				{
					if(localSets[numLocalSets].type != ValueType::any) { ++numLocalSets; }
					localSets[numLocalSets].type = localType;
					localSets[numLocalSets].num = 0;
				}
				++localSets[numLocalSets].num;
			}
			if(localSets[numLocalSets].type != ValueType::any) { ++numLocalSets; }
		}

		// Serialize the local sets.
		serializeVarUInt32(bodyStream,numLocalSets);
		for(Uptr setIndex = 0;setIndex < numLocalSets;++setIndex) { serialize(bodyStream,localSets[setIndex]); }

		// Serialize the function code.
		OperatorDecoderStream irDecoderStream(functionDef.code);
		OperatorSerializerStream wasmOpEncoderStream(bodyStream,functionDef);
		while(irDecoderStream) { irDecoderStream.decodeOp(wasmOpEncoderStream); };

		std::vector<U8> bodyBytes = bodyStream.getBytes();
		serialize(sectionStream,bodyBytes);
	}
	
	void serializeFunctionBody(InputStream& sectionStream,Module& module,FunctionDef& functionDef)
	{
		Uptr numBodyBytes = 0;
		serializeVarUInt32(sectionStream,numBodyBytes);

		MemoryInputStream bodyStream(sectionStream.advance(numBodyBytes),numBodyBytes);
		
		// Deserialize local sets and unpack them into a linear array of local types.
		Uptr numLocalSets = 0;
		serializeVarUInt32(bodyStream,numLocalSets);
		for(Uptr setIndex = 0;setIndex < numLocalSets;++setIndex)
		{
			LocalSet localSet;
			serialize(bodyStream,localSet);

			if( localSet.num > 1024*1024 )
				throw FatalSerializationException( "localSet.num too large" );

			for(Uptr index = 0;index < localSet.num;++index) { functionDef.nonParameterLocalTypes.push_back(localSet.type); }
		}

		// Deserialize the function code, validate it, and re-encode it in the IR format.
		ArrayOutputStream irCodeByteStream;
		OperatorEncoderStream irEncoderStream(irCodeByteStream);
		CodeValidationStream codeValidationStream(module,functionDef);
		while(bodyStream.capacity())
		{
			Opcode opcode;
			serialize(bodyStream,opcode);
			switch(opcode)
			{
			#define VISIT_OPCODE(_,name,nameString,Imm,...) \
				case Opcode::name: \
				{ \
					Imm imm; \
					serialize(bodyStream,imm,functionDef); \
					codeValidationStream.name(imm); \
					irEncoderStream.name(imm); \
					break; \
				}
			ENUM_OPERATORS(VISIT_OPCODE)
			#undef VISIT_OPCODE
			default: throw FatalSerializationException("unknown opcode");
			};
		};
		codeValidationStream.finish();

		functionDef.code = std::move(irCodeByteStream.getBytes());
	}
	
	template<typename Stream>
	void serializeTypeSection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::type,[&module](Stream& sectionStream)
		{
			serializeArray(sectionStream,module.types,[](Stream& stream,const FunctionType*& functionType)
			{
				serializeConstant(stream,"function type tag",U8(0x60));
				if(Stream::isInput)
				{
					std::vector<ValueType> parameterTypes;
					ResultType returnType;
					serialize(stream,parameterTypes);
					serialize(stream,returnType);
					functionType = FunctionType::get(returnType,parameterTypes);
				}
				else
				{
					serialize(stream,const_cast<std::vector<ValueType>&>(functionType->parameters));
					serialize(stream,const_cast<ResultType&>(functionType->ret));
				}
			});
		});
	}
	template<typename Stream>
	void serializeImportSection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::import,[&module](Stream& sectionStream)
		{
			Uptr size = module.functions.imports.size()
				+ module.tables.imports.size()
				+ module.memories.imports.size()
				+ module.globals.imports.size();
			serializeVarUInt32(sectionStream,size);
			if(Stream::isInput)
			{
				for(Uptr index = 0;index < size;++index)
				{
					std::string moduleName;
					std::string exportName;
					ObjectKind kind = ObjectKind::invalid;
					serialize(sectionStream,moduleName);
					serialize(sectionStream,exportName);
					throwIfNotValidUTF8(moduleName);
					throwIfNotValidUTF8(exportName);
					serialize(sectionStream,kind);
					switch(kind)
					{
					case ObjectKind::function:
					{
						U32 functionTypeIndex = 0;
						serializeVarUInt32(sectionStream,functionTypeIndex);
						if(functionTypeIndex >= module.types.size())
						{
							throw FatalSerializationException("invalid import function type index");
						}
						module.functions.imports.push_back({{functionTypeIndex},std::move(moduleName),std::move(exportName)});
						break;
					}
					case ObjectKind::table:
					{
						TableType tableType;
						serialize(sectionStream,tableType);
						module.tables.imports.push_back({tableType,std::move(moduleName),std::move(exportName)});
						break;
					}
					case ObjectKind::memory:
					{
						MemoryType memoryType;
						serialize(sectionStream,memoryType);
						module.memories.imports.push_back({memoryType,std::move(moduleName),std::move(exportName)});
						break;
					}
					case ObjectKind::global:
					{
						GlobalType globalType;
						serialize(sectionStream,globalType);
						module.globals.imports.push_back({globalType,std::move(moduleName),std::move(exportName)});
						break;
					}
					default: throw FatalSerializationException("invalid ObjectKind");
					}
				}
			}
			else
			{
				for(auto& functionImport : module.functions.imports)
				{
					serialize(sectionStream,functionImport.moduleName);
					serialize(sectionStream,functionImport.exportName);
					ObjectKind kind = ObjectKind::function;
					serialize(sectionStream,kind);
					serializeVarUInt32(sectionStream,functionImport.type.index);
				}
				for(auto& tableImport : module.tables.imports)
				{
					serialize(sectionStream,tableImport.moduleName);
					serialize(sectionStream,tableImport.exportName);
					ObjectKind kind = ObjectKind::table;
					serialize(sectionStream,kind);
					serialize(sectionStream,tableImport.type);
				}
				for(auto& memoryImport : module.memories.imports)
				{
					serialize(sectionStream,memoryImport.moduleName);
					serialize(sectionStream,memoryImport.exportName);
					ObjectKind kind = ObjectKind::memory;
					serialize(sectionStream,kind);
					serialize(sectionStream,memoryImport.type);
				}
				for(auto& globalImport : module.globals.imports)
				{
					serialize(sectionStream,globalImport.moduleName);
					serialize(sectionStream,globalImport.exportName);
					ObjectKind kind = ObjectKind::global;
					serialize(sectionStream,kind);
					serialize(sectionStream,globalImport.type);
				}
			}
		});
	}

	template<typename Stream>
	void serializeFunctionSection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::functionDeclarations,[&module](Stream& sectionStream)
		{
			Uptr numFunctions = module.functions.defs.size();
			serializeVarUInt32(sectionStream,numFunctions);
			if(Stream::isInput)
			{
				// Grow the vector one element at a time:
				// try to get a serialization exception before making a huge allocation for malformed input.
				module.functions.defs.clear();
				for(Uptr functionIndex = 0;functionIndex < numFunctions;++functionIndex)
				{
					U32 functionTypeIndex = 0;
					serializeVarUInt32(sectionStream,functionTypeIndex);
					if(functionTypeIndex >= module.types.size()) { throw FatalSerializationException("invalid function type index"); }
					module.functions.defs.push_back({{functionTypeIndex},{},{}});
				}
				module.functions.defs.shrink_to_fit();
			}
			else
			{
				for(FunctionDef& function : module.functions.defs)
				{
					serializeVarUInt32(sectionStream,function.type.index);
				}
			}
		});
	}

	template<typename Stream>
	void serializeTableSection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::table,[&module](Stream& sectionStream)
		{
			serialize(sectionStream,module.tables.defs);
		});
	}

	template<typename Stream>
	void serializeMemorySection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::memory,[&module](Stream& sectionStream)
		{
			serialize(sectionStream,module.memories.defs);
		});
	}

	template<typename Stream>
	void serializeGlobalSection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::global,[&module](Stream& sectionStream)
		{
			serialize(sectionStream,module.globals.defs);
		});
	}

	template<typename Stream>
	void serializeExportSection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::export_,[&module](Stream& sectionStream)
		{
			serialize(sectionStream,module.exports);
		});
	}

	template<typename Stream>
	void serializeStartSection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::start,[&module](Stream& sectionStream)
		{
			serializeVarUInt32(sectionStream,module.startFunctionIndex);
		});
	}

	template<typename Stream>
	void serializeElementSection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::elem,[&module](Stream& sectionStream)
		{
			serialize(sectionStream,module.tableSegments);
		});
	}

	template<typename Stream>
	void serializeCodeSection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::functionDefinitions,[&module](Stream& sectionStream)
		{
			Uptr numFunctionBodies = module.functions.defs.size();
			serializeVarUInt32(sectionStream,numFunctionBodies);
			if(Stream::isInput && numFunctionBodies != module.functions.defs.size())
				{ throw FatalSerializationException("function and code sections have mismatched function counts"); }
			for(FunctionDef& functionDef : module.functions.defs) { serializeFunctionBody(sectionStream,module,functionDef); }
		});
	}

	template<typename Stream>
	void serializeDataSection(Stream& moduleStream,Module& module)
	{
		serializeSection(moduleStream,SectionType::data,[&module](Stream& sectionStream)
		{
			serialize(sectionStream,module.dataSegments);
		});
	}

	void serializeModule(OutputStream& moduleStream,Module& module)
	{
		serializeConstant(moduleStream,"magic number",U32(magicNumber));
		serializeConstant(moduleStream,"version",U32(currentVersion));

		if(module.types.size() > 0) { serializeTypeSection(moduleStream,module); }
		if(module.functions.imports.size() > 0
		|| module.tables.imports.size() > 0
		|| module.memories.imports.size() > 0
		|| module.globals.imports.size() > 0) { serializeImportSection(moduleStream,module); }
		if(module.functions.defs.size() > 0) { serializeFunctionSection(moduleStream,module); }
		if(module.tables.defs.size() > 0) { serializeTableSection(moduleStream,module); }
		if(module.memories.defs.size() > 0) { serializeMemorySection(moduleStream,module); }
		if(module.globals.defs.size() > 0) { serializeGlobalSection(moduleStream,module); }
		if(module.exports.size() > 0) { serializeExportSection(moduleStream,module); }
		if(module.startFunctionIndex != UINTPTR_MAX) { serializeStartSection(moduleStream,module); }
		if(module.tableSegments.size() > 0) { serializeElementSection(moduleStream,module); }
		if(module.functions.defs.size() > 0) { serializeCodeSection(moduleStream,module); }
		if(module.dataSegments.size() > 0) { serializeDataSection(moduleStream,module); }

		for(auto& userSection : module.userSections) { serialize(moduleStream,userSection); }
	}
	void serializeModule(InputStream& moduleStream,Module& module)
	{
		serializeConstant(moduleStream,"magic number",U32(magicNumber));
		serializeConstant(moduleStream,"version",U32(currentVersion));

		SectionType lastKnownSectionType = SectionType::unknown;
		while(moduleStream.capacity())
		{
			const SectionType sectionType = *(SectionType*)moduleStream.peek(sizeof(SectionType));
			if(sectionType != SectionType::user)
			{
				if(sectionType > lastKnownSectionType) { lastKnownSectionType = sectionType; }
				else { throw FatalSerializationException("incorrect order for known section"); }
			}
			switch(sectionType)
			{
			case SectionType::type: serializeTypeSection(moduleStream,module); break;
			case SectionType::import: serializeImportSection(moduleStream,module); break;
			case SectionType::functionDeclarations: serializeFunctionSection(moduleStream,module); break;
			case SectionType::table: serializeTableSection(moduleStream,module); break;
			case SectionType::memory: serializeMemorySection(moduleStream,module); break;
			case SectionType::global: serializeGlobalSection(moduleStream,module); break;
			case SectionType::export_: serializeExportSection(moduleStream,module); break;
			case SectionType::start: serializeStartSection(moduleStream,module); break;
			case SectionType::elem: serializeElementSection(moduleStream,module); break;
			case SectionType::functionDefinitions: serializeCodeSection(moduleStream,module); break;
			case SectionType::data: serializeDataSection(moduleStream,module); break;
			case SectionType::user:
			{
				UserSection& userSection = *module.userSections.insert(module.userSections.end(),UserSection());
				serialize(moduleStream,userSection);
				break;
			}
			default: throw FatalSerializationException("unknown section ID");
			};
		};
	}

	void serialize(Serialization::InputStream& stream,Module& module)
	{
		serializeModule(stream,module);
		IR::validateDefinitions(module);
	}
	void serialize(Serialization::OutputStream& stream,const Module& module)
	{
		serializeModule(stream,const_cast<Module&>(module));
	}
}
