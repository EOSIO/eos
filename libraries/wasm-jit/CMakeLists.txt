#cmake_minimum_required(VERSION 2.7.0)
#project (WAVM)
#
#enable_testing()
#
#set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#
## Put executables in the bin dir, and libraries in the lib dir.
#SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
#SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
#
## Allow includes relative to the source path.
set(WAVM_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/Include)
include_directories(${WAVM_INCLUDE_DIR})
#
	# Use C++11
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

	# Compile with all warnings and fatal warnings
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

	# Disable RTTI
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

	# Ensure that even static libraries are relocatable so they can be linked into a .so
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

	# Enable asan in debug builds.
#	if(CMAKE_BUILD_TYPE MATCHES "Debug")
#		option(ENABLE_ASAN "enable ASAN" ON)
#		add_definitions("-D_DEBUG")
#	else()
#		option(ENABLE_ASAN "enable ASAN" OFF)
#	endif()
#	if(ENABLE_ASAN)
#		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#	endif()

set(CMAKE_CXX_STANDARD 11)
option(WAVM_METRICS_OUTPUT "controls printing the timings of some operations to stdout" OFF)
if(WAVM_METRICS_OUTPUT)
	add_definitions("-DWAVM_METRICS_OUTPUT=1")
else()
	add_definitions("-DWAVM_METRICS_OUTPUT=0")
endif()

option(PRETEND_32BIT_ADDRESS_SPACE "forces 64-bit WAVM to reserve address-space as it would in a 32-bit process" OFF)
if(PRETEND_32BIT_ADDRESS_SPACE)
	add_definitions("-DPRETEND_32BIT_ADDRESS_SPACE=1")
else()
	add_definitions("-DPRETEND_32BIT_ADDRESS_SPACE=0")
endif()

option(ENABLE_SIMD_PROTOTYPE "enables the prototype implementation of the proposed WebAssembly SIMD extension" OFF)
if(ENABLE_SIMD_PROTOTYPE)
	add_definitions("-DENABLE_SIMD_PROTOTYPE=1")
else()
	add_definitions("-DENABLE_SIMD_PROTOTYPE=0")
endif()

option(ENABLE_THREADING_PROTOTYPE "enables the prototype implementation of the proposed WebAssembly threading extension" OFF)
if(ENABLE_THREADING_PROTOTYPE)
	add_definitions("-DENABLE_THREADING_PROTOTYPE=1")
else()
	add_definitions("-DENABLE_THREADING_PROTOTYPE=0")
endif()

add_subdirectory(Include/Inline)

add_subdirectory(Source/IR)
add_subdirectory(Source/Logging)
add_subdirectory(Source/Runtime)
add_subdirectory(Source/WASM)
add_subdirectory(Source/WAST)

add_library(wasm-jit STATIC Source/dummy.cpp)
target_include_directories(wasm-jit PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_link_libraries(wasm-jit PUBLIC
   IR
   Logging
   Runtime
   WASM
   WAST
)

#add_subdirectory(Test/spec)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Include/ DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/wasm-jit)
