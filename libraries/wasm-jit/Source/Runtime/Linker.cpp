#include "Inline/BasicTypes.h"
#include "Runtime.h"
#include "Platform/Platform.h"
#include "RuntimePrivate.h"
#include "Linker.h"
#include "Intrinsics.h"
#include "IR/Module.h"

#include <string.h>

namespace Runtime
{
	RUNTIME_API IntrinsicResolver IntrinsicResolver::singleton;

	bool IntrinsicResolver::resolve(const std::string& moduleName,const std::string& exportName,ObjectType type,ObjectInstance*& outObject)
	{
		// Make sure the wavmIntrinsics module can't be directly imported.
		if(moduleName == "wavmIntrinsics") { return false; }

		outObject = Intrinsics::find(moduleName + "." + exportName,type);
		return outObject != nullptr;
	}

	const FunctionType* resolveImportType(const IR::Module& module,IndexedFunctionType type)
	{
		return module.types[type.index];
	}
	TableType resolveImportType(const IR::Module& module,TableType type) { return type; }
	MemoryType resolveImportType(const IR::Module& module,MemoryType type) { return type; }
	GlobalType resolveImportType(const IR::Module& module,GlobalType type) { return type; }

	template<typename Instance,typename Type>
	void linkImport(const IR::Module& module,const Import<Type>& import,Resolver& resolver,LinkResult& linkResult,std::vector<Instance*>& resolvedImports)
	{
		// Ask the resolver for a value for this import.
		ObjectInstance* importValue;
		if(resolver.resolve(import.moduleName,import.exportName,resolveImportType(module,import.type),importValue))
		{
			// Sanity check that the resolver returned an object of the right type.
			assert(isA(importValue,resolveImportType(module,import.type)));
			resolvedImports.push_back(as<Instance>(importValue));
		}
		else { linkResult.missingImports.push_back({import.moduleName,import.exportName,resolveImportType(module,import.type)}); }
	}

	LinkResult linkModule(const IR::Module& module,Resolver& resolver)
	{
		LinkResult linkResult;
		for(const auto& import : module.functions.imports)
		{
			linkImport(module,import,resolver,linkResult,linkResult.resolvedImports.functions);
		}
		for(const auto& import : module.tables.imports)
		{
			linkImport(module,import,resolver,linkResult,linkResult.resolvedImports.tables);
		}
		for(const auto& import : module.memories.imports)
		{
			linkImport(module,import,resolver,linkResult,linkResult.resolvedImports.memories);
		}
		for(const auto& import : module.globals.imports)
		{
			linkImport(module,import,resolver,linkResult,linkResult.resolvedImports.globals);
		}

		linkResult.success = linkResult.missingImports.size() == 0;
		return linkResult;
	}
}