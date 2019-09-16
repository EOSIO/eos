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

	const std::vector<uint8_t>& getPICCode(ModuleInstance* moduleInstance) {
      return moduleInstance->jitModule->final_pic_code;
   }
	const std::map<unsigned, uintptr_t>& getFunctionOffsets(ModuleInstance* moduleInstance) {
      return moduleInstance->jitModule->function_to_offsets;
   }

}
