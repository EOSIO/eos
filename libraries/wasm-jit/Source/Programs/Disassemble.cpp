#include "Inline/BasicTypes.h"
#include "CLI.h"
#include "WAST/WAST.h"
#include "WASM/WASM.h"

int commandMain(int argc,char** argv)
{
	if(argc != 3)
	{
		std::cerr <<  "Usage: Disassemble in.wasm out.wast" << std::endl;
		return EXIT_FAILURE;
	}
	const char* inputFilename = argv[1];
	const char* outputFilename = argv[2];
	
	// Load the WASM file.
	IR::Module module;
	if(!loadBinaryModule(inputFilename,module)) { return EXIT_FAILURE; }

	// Print the module to WAST.
	const std::string wastString = WAST::print(module);
	
	// Write the serialized data to the output file.
	std::ofstream outputStream(outputFilename);
	outputStream.write(wastString.data(),wastString.size());
	outputStream.close();

	return EXIT_SUCCESS;
}
