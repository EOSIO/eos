#include "Inline/BasicTypes.h"
#include "Runtime.h"
#include "Platform/Platform.h"
#include "RuntimePrivate.h"

namespace Runtime
{
	// Global lists of memories; used to query whether an address is reserved by one of them.
	std::vector<MemoryInstance*> memories;

	static Uptr getPlatformPagesPerWebAssemblyPageLog2()
	{
		errorUnless(Platform::getPageSizeLog2() <= IR::numBytesPerPageLog2);
		return IR::numBytesPerPageLog2 - Platform::getPageSizeLog2();
	}

	U8* allocateVirtualPagesAligned(Uptr numBytes,Uptr alignmentBytes,U8*& outUnalignedBaseAddress,Uptr& outUnalignedNumPlatformPages)
	{
		const Uptr numAllocatedVirtualPages = numBytes >> Platform::getPageSizeLog2();
		const Uptr alignmentPages = alignmentBytes >> Platform::getPageSizeLog2();
		outUnalignedNumPlatformPages = numAllocatedVirtualPages + alignmentPages;
		outUnalignedBaseAddress = Platform::allocateVirtualPages(outUnalignedNumPlatformPages);
		if(!outUnalignedBaseAddress) { outUnalignedNumPlatformPages = 0; return nullptr; }
		else { return (U8*)((Uptr)(outUnalignedBaseAddress + alignmentBytes - 1) & ~(alignmentBytes - 1)); }
	}

	MemoryInstance* createMemory(MemoryType type)
	{
		MemoryInstance* memory = new MemoryInstance(type);

		// On a 64-bit runtime, allocate 8GB of address space for the memory.
		// This allows eliding bounds checks on memory accesses, since a 32-bit index + 32-bit offset will always be within the reserved address-space.
		// On a 32-bit runtime, allocate 256MB.
		const Uptr memoryMaxBytes = HAS_64BIT_ADDRESS_SPACE ? Uptr(8ull*1024*1024*1024) : 0x10000000;
		
		// On a 64 bit runtime, align the instance memory base to a 4GB boundary, so the lower 32-bits will all be zero. Maybe it will allow better code generation?
		// Note that this reserves a full extra 4GB, but only uses (4GB-1 page) for alignment, so there will always be a guard page at the end to
		// protect against unaligned loads/stores that straddle the end of the address-space.
		const Uptr alignmentBytes = HAS_64BIT_ADDRESS_SPACE ? Uptr(4ull*1024*1024*1024) : ((Uptr)1 << Platform::getPageSizeLog2());
		memory->baseAddress = allocateVirtualPagesAligned(memoryMaxBytes,alignmentBytes,memory->reservedBaseAddress,memory->reservedNumPlatformPages);
		memory->endOffset = memoryMaxBytes;
		if(!memory->baseAddress) { delete memory; return nullptr; }

		// Grow the memory to the type's minimum size.
		assert(type.size.min <= UINTPTR_MAX);
		if(growMemory(memory,Uptr(type.size.min)) == -1) { delete memory; return nullptr; }

		// Add the memory to the global array.
		memories.push_back(memory);
		return memory;
	}

	MemoryInstance::~MemoryInstance()
	{
		// Decommit all default memory pages.
		if(numPages > 0) { Platform::decommitVirtualPages(baseAddress,numPages << getPlatformPagesPerWebAssemblyPageLog2()); }

		// Free the virtual address space.
		if(reservedNumPlatformPages > 0) { Platform::freeVirtualPages(reservedBaseAddress,reservedNumPlatformPages); }
		reservedBaseAddress = baseAddress = nullptr;
		reservedNumPlatformPages = 0;

		// Remove the memory from the global array.
		for(Uptr memoryIndex = 0;memoryIndex < memories.size();++memoryIndex)
		{
			if(memories[memoryIndex] == this) { memories.erase(memories.begin() + memoryIndex); break; }
		}
	}
	
	bool isAddressOwnedByMemory(U8* address)
	{
		// Iterate over all memories and check if the address is within the reserved address space for each.
		for(auto memory : memories)
		{
			U8* startAddress = memory->reservedBaseAddress;
			U8* endAddress = memory->reservedBaseAddress + (memory->reservedNumPlatformPages << Platform::getPageSizeLog2());
			if(address >= startAddress && address < endAddress) { return true; }
		}
		return false;
	}

	Uptr getMemoryNumPages(MemoryInstance* memory) { return memory->numPages; }
	Uptr getMemoryMaxPages(MemoryInstance* memory)
	{
		assert(memory->type.size.max <= UINTPTR_MAX);
		return Uptr(memory->type.size.max);
	}

	Iptr growMemory(MemoryInstance* memory,Uptr numNewPages)
	{
		const Uptr previousNumPages = memory->numPages;
		if(numNewPages > 0)
		{
			// If the number of pages to grow would cause the memory's size to exceed its maximum, return -1.
			if(numNewPages > memory->type.size.max || memory->numPages > memory->type.size.max - numNewPages) { return -1; }

			// Try to commit the new pages, and return -1 if the commit fails.
			if(!Platform::commitVirtualPages(
				memory->baseAddress + (memory->numPages << IR::numBytesPerPageLog2),
				numNewPages << getPlatformPagesPerWebAssemblyPageLog2()
				))
			{
				return -1;
			}
			memory->numPages += numNewPages;
		}
		return previousNumPages;
	}

	Iptr shrinkMemory(MemoryInstance* memory,Uptr numPagesToShrink)
	{
		const Uptr previousNumPages = memory->numPages;
		if(numPagesToShrink > 0)
		{
			// If the number of pages to shrink would cause the memory's size to drop below its minimum, return -1.
			if(numPagesToShrink > memory->numPages
			|| memory->numPages - numPagesToShrink < memory->type.size.min)
			{ return -1; }
			memory->numPages -= numPagesToShrink;

			// Decommit the pages that were shrunk off the end of the memory.
			Platform::decommitVirtualPages(
				memory->baseAddress + (memory->numPages << IR::numBytesPerPageLog2),
				numPagesToShrink << getPlatformPagesPerWebAssemblyPageLog2()
				);
		}
		return previousNumPages;
	}

	U8* getMemoryBaseAddress(MemoryInstance* memory)
	{
		return memory->baseAddress;
	}
	
	U8* getValidatedMemoryOffsetRange(MemoryInstance* memory,Uptr offset,Uptr numBytes)
	{
		if(!memory)
			causeException(Exception::Cause::accessViolation);
		// Validate that the range [offset..offset+numBytes) is contained by the memory's reserved pages.
		U8* address = memory->baseAddress + offset;
		if(	address < memory->reservedBaseAddress
		||	address + numBytes < address
		||	address + numBytes >= memory->reservedBaseAddress + (memory->reservedNumPlatformPages << Platform::getPageSizeLog2()))
		{
			causeException(Exception::Cause::accessViolation);
		}
		return address;
	}

}
