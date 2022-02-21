#pragma once

#ifndef WEBASSEMBLY_API
	#define WEBASSEMBLY_API DLL_IMPORT
#endif

#include "Inline/BasicTypes.h"

namespace IR { struct Module; struct DisassemblyNames; }
namespace Serialization { struct InputStream; struct OutputStream; }

namespace WASM
{
	extern bool check_limits;
	struct scoped_skip_checks {
		scoped_skip_checks() { check_limits = false; }
		~scoped_skip_checks() { check_limits = true; }
	};
	WEBASSEMBLY_API void serialize(Serialization::InputStream& stream,IR::Module& module);
	WEBASSEMBLY_API void serialize(Serialization::OutputStream& stream,const IR::Module& module);
}
