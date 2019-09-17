#include "Inline/BasicTypes.h"
#include "Runtime.h"
#include "RuntimePrivate.h"
#include "IR/Module.h"

#include <string.h>

namespace Runtime
{
	


	ModuleInstance* instantiateModule(const IR::Module& module)
	{
		ModuleInstance* moduleInstance = new ModuleInstance(

			);

		// Generate machine code for the module.
		LLVMJIT::instantiateModule(module,moduleInstance);

		return moduleInstance;
	}

	ModuleInstance::~ModuleInstance()
	{
		delete jitModule;
	}

}
