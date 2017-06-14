#include "Inline/BasicTypes.h"
#include "Inline/Serialization.h"
#include "Logging/Logging.h"
#include "IR.h"
#include "Module.h"

using namespace Serialization;

namespace IR
{
	void getDisassemblyNames(const Module& module,DisassemblyNames& outNames)
	{
		// Fill in the output with the correct number of blank names.
		for(const auto& functionImport : module.functions.imports)
		{
			DisassemblyNames::Function functionNames;
			functionNames.locals.resize(module.types[functionImport.type.index]->parameters.size());
			outNames.functions.push_back(std::move(functionNames));
		}
		for(Uptr functionDefIndex = 0;functionDefIndex < module.functions.defs.size();++functionDefIndex)
		{
			const FunctionDef& functionDef = module.functions.defs[functionDefIndex];
			DisassemblyNames::Function functionNames;
			functionNames.locals.insert(functionNames.locals.begin(),module.types[functionDef.type.index]->parameters.size() + functionDef.nonParameterLocalTypes.size(),"");
			outNames.functions.push_back(std::move(functionNames));
		}
		
		outNames.types.insert(outNames.types.end(),module.types.size(),"");
		outNames.tables.insert(outNames.tables.end(),module.tables.defs.size() + module.tables.imports.size(),"");
		outNames.memories.insert(outNames.memories.end(),module.memories.defs.size() + module.memories.imports.size(),"");
		outNames.globals.insert(outNames.globals.end(),module.globals.defs.size() + module.globals.imports.size(),"");

		// Deserialize the name section, if it is present.
		Uptr userSectionIndex = 0;
		if(findUserSection(module,"name",userSectionIndex))
		{
			try
			{
				const UserSection& nameSection = module.userSections[userSectionIndex];
				MemoryInputStream stream(nameSection.data.data(),nameSection.data.size());
			
				#if 0
				while(stream.capacity())
				{
					U8 substreamType = 0;
					serializeVarUInt7(stream,substreamType);

					U32 numSubstreamBytes = 0;
					serializeVarUInt32(stream,numSubstreamBytes);
					
					MemoryInputStream substream(stream.advance(numSubstreamBytes),numSubstreamBytes);
					switch(substreamType)
					{
					case 0: // function names
					{
						U32 numFunctionNames = 0;
						serializeVarUInt32(substream,numFunctionNames);
						for(Uptr functionNameIndex = 0;functionNameIndex < numFunctionNames;++functionNameIndex)
						{
							U32 functionIndex = 0;
							serializeVarUInt32(substream,functionIndex);

							std::string functionName;
							serialize(substream,functionName);

							if(functionIndex < outNames.functions.size()) { outNames.functions[functionIndex].name = std::move(functionName); }
						}
						break;
					}
					case 1: // local names
					{
						U32 numFunctionLocalNameMaps = 0;
						serializeVarUInt32(substream,numFunctionLocalNameMaps);
						for(Uptr functionNameIndex = 0;functionNameIndex < numFunctionLocalNameMaps;++functionNameIndex)
						{
							U32 functionIndex = 0;
							serializeVarUInt32(substream,functionIndex);

							U32 numLocalNames = 0;
							serializeVarUInt32(substream,numLocalNames);
							
							for(Uptr localNameIndex =  0;localNameIndex < numLocalNames;++numLocalNames)
							{
								U32 localIndex = 0;
								serializeVarUInt32(substream,localIndex);

								std::string localName;
								serialize(substream,localName);

								if(functionIndex < outNames.functions.size() && localIndex < outNames.functions[functionIndex].locals.size())
								{
									outNames.functions[functionIndex].locals[localIndex] = std::move(localName);
								}
							}
						}

						break;
					}
					};
				};
				#endif

				Uptr numFunctionNames = 0;
				serializeVarUInt32(stream,numFunctionNames);
				numFunctionNames = std::min(numFunctionNames,(Uptr)outNames.functions.size());

				for(Uptr functionIndex = 0;functionIndex < numFunctionNames;++functionIndex)
				{
					DisassemblyNames::Function& functionNames = outNames.functions[functionIndex];

					serialize(stream,outNames.functions[functionIndex].name);

					Uptr numLocalNames = 0;
					serializeVarUInt32(stream,numLocalNames);

					for(Uptr localIndex = 0;localIndex < numLocalNames;++localIndex)
					{
						std::string localName;
						serialize(stream,localName);
						if(localIndex < functionNames.locals.size()) { functionNames.locals[localIndex] = std::move(localName); }
					}
				}
			}
			catch(FatalSerializationException exception)
			{
				Log::printf(Log::Category::debug,"FatalSerializationException while deserializing WASM user name section: %s\n",exception.message.c_str());
			}
		}
	}

	void setDisassemblyNames(Module& module,const DisassemblyNames& names)
	{
		// Replace an existing name section if one is present, or create a new section.
		Uptr userSectionIndex = 0;
		if(!findUserSection(module,"name",userSectionIndex))
		{
			userSectionIndex = module.userSections.size();
			module.userSections.push_back({"name",{}});
		}

		ArrayOutputStream stream;
		
		Uptr numFunctionNames = names.functions.size();
		serializeVarUInt32(stream,numFunctionNames);

		for(Uptr functionIndex = 0;functionIndex < names.functions.size();++functionIndex)
		{
			std::string functionName = names.functions[functionIndex].name;
			serialize(stream,functionName);

			Uptr numLocalNames = names.functions[functionIndex].locals.size();
			serializeVarUInt32(stream,numLocalNames);
			for(Uptr localIndex = 0;localIndex < numLocalNames;++localIndex)
			{
				std::string localName = names.functions[functionIndex].locals[localIndex];
				serialize(stream,localName);
			}
		}

		module.userSections[userSectionIndex].data = stream.getBytes();
	}
}