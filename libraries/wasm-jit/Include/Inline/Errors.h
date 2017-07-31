#pragma once

#include <stdarg.h>
#include <assert.h>
#include <cstdio>
#include <cstdlib>

namespace Errors
{
	// Fatal error handling.
	[[noreturn]] inline void fatalf(const char* messageFormat,...)
	{
		va_list varArgs;
		va_start(varArgs,messageFormat);
		std::vfprintf(stderr,messageFormat,varArgs);
		std::fflush(stderr);
		va_end(varArgs);
		std::abort();
	}
	[[noreturn]] inline void fatal(const char* message) { fatalf("%s\n",message); }
	[[noreturn]] inline void unreachable() { fatalf("reached unreachable code\n"); }
	[[noreturn]] inline void unimplemented(const char* context) { fatalf("unimplemented: %s\n",context); }
}

// Like assert, but is never removed in any build configuration.
#define errorUnless(condition) if(!(condition)) { Errors::fatalf("errorUnless(%s) failed\n",#condition); }
