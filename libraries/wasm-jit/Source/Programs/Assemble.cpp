#include "Inline/BasicTypes.h"
#include "CLI.h"
#include "WAST/WAST.h"
#include "WASM/WASM.h"

int commandMain(int argc,char** argv)
{
	if(argc < 3)
	{
		std::cerr << "Usage: Assemble in.wast out.wasm [switches]" << std::endl;
		std::cerr << "  -n|--omit-names\t\tOmits WAST function and local names from the output" << std::endl;
		return EXIT_FAILURE;
	}
	const char* inputFilename = argv[1];
	const char* outputFilename = argv[2];
	bool omitNames = false;
	if(argc > 3)
	{
		for(Iptr argumentIndex = 3;argumentIndex < argc;++argumentIndex)
		{
			if(!strcmp(argv[argumentIndex],"-n") || !strcmp(argv[argumentIndex],"--omit-names"))
			{
				omitNames = true;
			}
			else
			{
				std::cerr << "Unrecognized argument: " << argv[argumentIndex] << std::endl;
				return EXIT_FAILURE;
			}
		}
	}
	
	// Load the WAST module.
	IR::Module module;
	if(!loadTextModule(inputFilename,module)) { return EXIT_FAILURE; }

	// If the command-line switch to omit names was specified, strip the name section.
	if(omitNames)
	{
		for(auto sectionIt = module.userSections.begin();sectionIt != module.userSections.end();++sectionIt)
		{
			if(sectionIt->name == "name") { module.userSections.erase(sectionIt); break; }
		}
	}

	// Write the binary module.
	if(!saveBinaryModule(outputFilename,module)) { return EXIT_FAILURE; }

	return EXIT_SUCCESS;
}
