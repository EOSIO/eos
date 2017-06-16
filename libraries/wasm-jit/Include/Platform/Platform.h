#pragma once

#include <assert.h>
#include <vector>
#include <functional>

#include "Inline/BasicTypes.h"

#ifdef _WIN32
	#define THREAD_LOCAL thread_local
	#define DLL_EXPORT __declspec(dllexport)
	#define DLL_IMPORT __declspec(dllimport)
	#define FORCEINLINE __forceinline
	#define SUPPRESS_UNUSED(variable) (void)(variable);
	#include <intrin.h>
	#define PACKED_STRUCT(definition) __pragma(pack(push, 1)) definition; __pragma(pack(pop))
#else
	// Use __thread instead of the C++11 thread_local because Apple's clang doesn't support thread_local yet.
	#define THREAD_LOCAL __thread
	#define DLL_EXPORT
	#define DLL_IMPORT
	#define FORCEINLINE inline __attribute__((always_inline))
	#define SUPPRESS_UNUSED(variable) (void)(variable);
	#define PACKED_STRUCT(definition) definition __attribute__((packed));
#endif

#ifndef PLATFORM_API
	#define PLATFORM_API DLL_IMPORT
#endif

namespace Platform
{
	// countLeadingZeroes/countTrailingZeroes returns the number of leading/trailing zeroes, or the bit width of the input if no bits are set.
	#ifdef _WIN32
		// BitScanReverse/BitScanForward return 0 if the input is 0.
		inline U64 countLeadingZeroes(U64 value) { unsigned long result; return _BitScanReverse64(&result,value) ? (63 - result) : 64; }
		inline U32 countLeadingZeroes(U32 value) { unsigned long result; return _BitScanReverse(&result,value) ? (31 - result) : 32; }
		inline U64 countTrailingZeroes(U64 value) { unsigned long result; return _BitScanForward64(&result,value) ? result : 64; }
		inline U32 countTrailingZeroes(U32 value) { unsigned long result; return _BitScanForward(&result,value) ? result : 32; }
	#else
		// __builtin_clz/__builtin_ctz are undefined if the input is 0. 
		inline U64 countLeadingZeroes(U64 value) { return value == 0 ? 64 : __builtin_clzll(value); }
		inline U32 countLeadingZeroes(U32 value) { return value == 0 ? 32 : __builtin_clz(value); }
		inline U64 countTrailingZeroes(U64 value) { return value == 0 ? 64 : __builtin_ctzll(value); }
		inline U32 countTrailingZeroes(U32 value) { return value == 0 ? 32 : __builtin_ctz(value); }
	#endif
	inline U64 floorLogTwo(U64 value) { return value <= 1 ? 0 : 63 - countLeadingZeroes(value); }
	inline U32 floorLogTwo(U32 value) { return value <= 1 ? 0 : 31 - countLeadingZeroes(value); }
	inline U64 ceilLogTwo(U64 value) { return floorLogTwo(value * 2 - 1); }
	inline U32 ceilLogTwo(U32 value) { return floorLogTwo(value * 2 - 1); }

	//
	// Memory
	//

	// Describes allowed memory accesses.
	enum class MemoryAccess
	{
		None,
		ReadOnly,
		ReadWrite,
		Execute,
		ReadWriteExecute
	};

	// Returns the base 2 logarithm of the smallest virtual page size.
	PLATFORM_API Uptr getPageSizeLog2();

	// Allocates virtual addresses without commiting physical pages to them.
	// Returns the base virtual address of the allocated addresses, or nullptr if the virtual address space has been exhausted.
	PLATFORM_API U8* allocateVirtualPages(Uptr numPages);

	// Commits physical memory to the specified virtual pages.
	// baseVirtualAddress must be a multiple of the preferred page size.
	// Return true if successful, or false if physical memory has been exhausted.
	PLATFORM_API bool commitVirtualPages(U8* baseVirtualAddress,Uptr numPages,MemoryAccess access = MemoryAccess::ReadWrite);

	// Changes the allowed access to the specified virtual pages.
	// baseVirtualAddress must be a multiple of the preferred page size.
	// Return true if successful, or false if the access-level could not be set.
	PLATFORM_API bool setVirtualPageAccess(U8* baseVirtualAddress,Uptr numPages,MemoryAccess access);

	// Decommits the physical memory that was committed to the specified virtual pages.
	// baseVirtualAddress must be a multiple of the preferred page size.
	PLATFORM_API void decommitVirtualPages(U8* baseVirtualAddress,Uptr numPages);

	// Frees virtual addresses. Any physical memory committed to the addresses must have already been decommitted.
	// baseVirtualAddress must be a multiple of the preferred page size.
	PLATFORM_API void freeVirtualPages(U8* baseVirtualAddress,Uptr numPages);

	//
	// Call stack and exceptions
	//

	// Describes a call stack.
	struct CallStack
	{
		struct Frame
		{
			Uptr ip;
		};
		std::vector<Frame> stackFrames;
	};

	// Captures the execution context of the caller.
	PLATFORM_API CallStack captureCallStack(Uptr numOmittedFramesFromTop = 0);
	
	// Describes an instruction pointer.
	PLATFORM_API bool describeInstructionPointer(Uptr ip,std::string& outDescription);

	#ifdef _WIN32
		// Registers/deregisters the data used by Windows SEH to unwind stack frames.
		PLATFORM_API void registerSEHUnwindInfo(Uptr imageLoadAddress,Uptr pdataAddress,Uptr pdataNumBytes);
		PLATFORM_API void deregisterSEHUnwindInfo(Uptr pdataAddress);
	#endif

	// Calls a thunk, and if it causes any of some specific hardware traps, returns true.
	// If a trap was caught, the outCause, outContext, and outOperand parameters are set to describe the trap.
	enum HardwareTrapType
	{
		none,
		accessViolation,
		stackOverflow,
		intDivideByZeroOrOverflow
	};
	PLATFORM_API HardwareTrapType catchHardwareTraps(
		CallStack& outTrapCallStack,
		Uptr& outTrapOperand,
		const std::function<void()>& thunk
		);

	//
	// Threading
	//

	// Returns the current value of a clock that may be used as an absolute time for wait timeouts.
	// The resolution is microseconds, and the origin is arbitrary.
	PLATFORM_API U64 getMonotonicClock();

	// Platform-independent mutexes.
	struct Mutex;
	PLATFORM_API Mutex* createMutex();
	PLATFORM_API void destroyMutex(Mutex* mutex);
	PLATFORM_API void lockMutex(Mutex* mutex);
	PLATFORM_API void unlockMutex(Mutex* mutex);

	// RAII-style lock for Mutex.
	struct Lock
	{
		Lock(Mutex* inMutex) : mutex(inMutex) { lockMutex(mutex); }
		~Lock() { unlockMutex(mutex); }

		void release()
		{
			if(mutex)
			{
				unlockMutex(mutex);
			}
			mutex = nullptr;
		}

		void detach()
		{
			assert(mutex);
			mutex = nullptr;
		}

	private:
		Mutex* mutex;
	};

	// Platform-independent events.
	struct Event;
	PLATFORM_API Event* createEvent();
	PLATFORM_API void destroyEvent(Event* event);
	PLATFORM_API bool waitForEvent(Event* event,U64 untilClock);
	PLATFORM_API void signalEvent(Event* event);
}
