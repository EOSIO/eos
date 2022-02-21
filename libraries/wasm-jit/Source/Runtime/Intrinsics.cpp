#include "Inline/BasicTypes.h"
#include "Intrinsics.h"
#include "Platform/Platform.h"
#include "Runtime.h"
#include "RuntimePrivate.h"
#include <iostream>
#include <string>

namespace Intrinsics
{
	struct Singleton
	{
		std::map<std::string,Intrinsics::Function*> functionMap;

		Singleton() {}
		Singleton(const Singleton&) = delete;

		static Singleton& get()
		{
			static Singleton result;
			return result;
		}
	};
	
	std::string getDecoratedName(const std::string& name,const IR::ObjectType& type)
	{
		std::string decoratedName = name;
		decoratedName += " : ";
		decoratedName += IR::asString(type);
		return decoratedName;
	}

	Function::Function(const char* inName,const IR::FunctionType* type,void* nativeFunction)
	:	name(inName)
	{
		function = new Runtime::FunctionInstance(nullptr,type,nativeFunction);
		Singleton::get().functionMap[getDecoratedName(inName,type)] = this;
	}

	Function::~Function()
	{
      {
         Singleton::get().functionMap.erase(Singleton::get().functionMap.find(getDecoratedName(name,function->type)));
      }
      delete function;
	}

	Runtime::ObjectInstance* find(const std::string& name,const IR::ObjectType& type)
	{
		std::string decoratedName = getDecoratedName(name,type);
		Runtime::ObjectInstance* result = nullptr;
		switch(type.kind)
		{
		case IR::ObjectKind::function:
		{
			auto keyValue = Singleton::get().functionMap.find(decoratedName);
			result = keyValue == Singleton::get().functionMap.end() ? nullptr : asObject(keyValue->second->function);
			break;
		}
		case IR::ObjectKind::table:
		{
			result = nullptr;
			break;
		}
		case IR::ObjectKind::memory:
		{
			result = nullptr;
			break;
		}
		case IR::ObjectKind::global:
		{
			result = nullptr;
			break;
		}
		default: Errors::unreachable();
		};
		if(result && !isA(result,type)) { result = nullptr; }
		return result;
	}
	
	std::vector<Runtime::ObjectInstance*> getAllIntrinsicObjects()
	{
		std::vector<Runtime::ObjectInstance*> result;
		for(auto mapIt : Singleton::get().functionMap) { result.push_back(mapIt.second->function); }
		return result;
	}
}
