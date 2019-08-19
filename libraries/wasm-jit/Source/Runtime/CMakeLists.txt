if(LLVM_VERSION VERSION_LESS 7.1 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	enable_language(ASM-LLVMWAR)
	set(LLVM_SHIM_FILE llvmWARshim.llvmwar)
else()
	set(LLVM_SHIM_FILE llvmWARshim.cpp)
endif()

set(Sources
	Intrinsics.cpp
	Linker.cpp
	LLVMEmitIR.cpp
	LLVMJIT.cpp
	LLVMJIT.h
	Memory.cpp
	ModuleInstance.cpp
	ObjectGC.cpp
	Runtime.cpp
	RuntimePrivate.h
	Table.cpp
	Threads.cpp
	WAVMIntrinsics.cpp
	${LLVM_SHIM_FILE}
  )
set(PublicHeaders
	${WAVM_INCLUDE_DIR}/Runtime/Intrinsics.h
	${WAVM_INCLUDE_DIR}/Runtime/Linker.h
	${WAVM_INCLUDE_DIR}/Runtime/Runtime.h
	${WAVM_INCLUDE_DIR}/Runtime/TaggedValue.h)
include_directories(${WAVM_INCLUDE_DIR}/Runtime)

add_library(Runtime STATIC ${Sources} ${PublicHeaders})

# Include the LLVM headers
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_definitions(-DRUNTIME_API=DLL_EXPORT)

target_include_directories( Runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../chain/include )

# Link against the LLVM libraries
llvm_map_components_to_libnames(LLVM_LIBS support core passes mcjit native DebugInfoDWARF orcjit)
target_link_libraries(Runtime Platform Logging IR ${LLVM_LIBS})

install(TARGETS Runtime 
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})

