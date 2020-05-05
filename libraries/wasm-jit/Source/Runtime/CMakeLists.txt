set(Sources
	Intrinsics.cpp
	Linker.cpp
	LLVMJIT.h
	ObjectGC.cpp
	RuntimePrivate.h
  )

set(PublicHeaders
	${WAVM_INCLUDE_DIR}/Runtime/Intrinsics.h
	${WAVM_INCLUDE_DIR}/Runtime/Linker.h
	${WAVM_INCLUDE_DIR}/Runtime/Runtime.h
	${WAVM_INCLUDE_DIR}/Runtime/TaggedValue.h)
include_directories(${WAVM_INCLUDE_DIR}/Runtime)

add_library(Runtime STATIC ${Sources} ${PublicHeaders})

add_definitions(-DRUNTIME_API=DLL_EXPORT)

target_include_directories( Runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../chain/include )

# Link against the LLVM libraries
target_link_libraries(Runtime Logging IR)
