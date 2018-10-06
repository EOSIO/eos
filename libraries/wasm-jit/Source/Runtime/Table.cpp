#include "Inline/BasicTypes.h"
#include "Runtime.h"
#include "Platform/Platform.h"
#include "RuntimePrivate.h"
#include <eosio/chain/wasm_eosio_constraints.hpp>

namespace Runtime
{
	// Global lists of tables; used to query whether an address is reserved by one of them.
	std::vector<TableInstance*> tables;

	static Uptr getNumPlatformPages(Uptr numBytes)
	{
		return (numBytes + (Uptr(1)<<Platform::getPageSizeLog2()) - 1) >> Platform::getPageSizeLog2();
	}

	TableInstance* createTable(TableType type)
	{
		TableInstance* table = new TableInstance(type);

		const Uptr tableMaxBytes = sizeof(TableInstance::FunctionElement)*eosio::chain::wasm_constraints::maximum_table_elements;
		
		const Uptr alignmentBytes = 1U << Platform::getPageSizeLog2();
		table->baseAddress = (TableInstance::FunctionElement*)allocateVirtualPagesAligned(tableMaxBytes,alignmentBytes,table->reservedBaseAddress,table->reservedNumPlatformPages);
		table->endOffset = tableMaxBytes;
		if(!table->baseAddress) { delete table; return nullptr; }
		
		// Grow the table to the type's minimum size.
		WAVM_ASSERT_THROW(type.size.min <= UINTPTR_MAX);
		if(growTable(table,Uptr(type.size.min)) == -1) { delete table; return nullptr; }
		
		// Add the table to the global array.
		tables.push_back(table);
		return table;
	}
	
	TableInstance::~TableInstance()
	{
		// Decommit all pages.
		if(elements.size() > 0) { Platform::decommitVirtualPages((U8*)baseAddress,getNumPlatformPages(elements.size() * sizeof(TableInstance::FunctionElement))); }

		// Free the virtual address space.
		if(reservedNumPlatformPages > 0) { Platform::freeVirtualPages((U8*)reservedBaseAddress,reservedNumPlatformPages); }
		reservedBaseAddress = nullptr;
		reservedNumPlatformPages = 0;
		baseAddress = nullptr;
		
		// Remove the table from the global array.
		for(Uptr tableIndex = 0;tableIndex < tables.size();++tableIndex)
		{
			if(tables[tableIndex] == this) { tables.erase(tables.begin() + tableIndex); break; }
		}
	}

	bool isAddressOwnedByTable(U8* address)
	{
		// Iterate over all tables and check if the address is within the reserved address space for each.
		for(auto table : tables)
		{
			U8* startAddress = (U8*)table->reservedBaseAddress;
			U8* endAddress = ((U8*)table->reservedBaseAddress) + (table->reservedNumPlatformPages << Platform::getPageSizeLog2());
			if(address >= startAddress && address < endAddress) { return true; }
		}
		return false;
	}

	ObjectInstance* setTableElement(TableInstance* table,Uptr index,ObjectInstance* newValue)
	{
		// Write the new table element to both the table's elements array and its indirect function call data.
		WAVM_ASSERT_THROW(index < table->elements.size());
		FunctionInstance* functionInstance = asFunction(newValue);
		WAVM_ASSERT_THROW(functionInstance->nativeFunction);
		table->baseAddress[index].type = functionInstance->type;
		table->baseAddress[index].value = functionInstance->nativeFunction;
		auto oldValue = table->elements[index];
		table->elements[index] = newValue;
		return oldValue;
	}

	Uptr getTableNumElements(TableInstance* table)
	{
		return table->elements.size();
	}

	Iptr growTable(TableInstance* table,Uptr numNewElements)
	{
		const Uptr previousNumElements = table->elements.size();
		if(numNewElements > 0)
		{
			// If the number of elements to grow would cause the table's size to exceed its maximum, return -1.
			if(numNewElements > table->type.size.max || table->elements.size() > table->type.size.max - numNewElements) { return -1; }
			
			// Try to commit pages for the new elements, and return -1 if the commit fails.
			const Uptr previousNumPlatformPages = getNumPlatformPages(table->elements.size() * sizeof(TableInstance::FunctionElement));
			const Uptr newNumPlatformPages = getNumPlatformPages((table->elements.size()+numNewElements) * sizeof(TableInstance::FunctionElement));
			if(newNumPlatformPages != previousNumPlatformPages
			&& !Platform::commitVirtualPages(
				(U8*)table->baseAddress + (previousNumPlatformPages << Platform::getPageSizeLog2()),
				newNumPlatformPages - previousNumPlatformPages
				))
			{
				return -1;
			}

			// Also grow the table's elements array.
			table->elements.insert(table->elements.end(),numNewElements,nullptr);
		}
		return previousNumElements;
	}

	Iptr shrinkTable(TableInstance* table,Uptr numElementsToShrink)
	{
		const Uptr previousNumElements = table->elements.size();
		if(numElementsToShrink > 0)
		{
			// If the number of elements to shrink would cause the tables's size to drop below its minimum, return -1.
			if(numElementsToShrink > table->elements.size()
			|| table->elements.size() - numElementsToShrink < table->type.size.min) { return -1; }

			// Shrink the table's elements array.
			table->elements.resize(table->elements.size() - numElementsToShrink);
			
			// Decommit the pages that were shrunk off the end of the table's indirect function call data.
			const Uptr previousNumPlatformPages = getNumPlatformPages(previousNumElements * sizeof(TableInstance::FunctionElement));
			const Uptr newNumPlatformPages = getNumPlatformPages(table->elements.size() * sizeof(TableInstance::FunctionElement));
			if(newNumPlatformPages != previousNumPlatformPages)
			{
				Platform::decommitVirtualPages(
					(U8*)table->baseAddress + (newNumPlatformPages << Platform::getPageSizeLog2()),
					(previousNumPlatformPages - newNumPlatformPages) << Platform::getPageSizeLog2()
					);
			}
		}
		return previousNumElements;
	}
}
