set(Sources
	POSIX.cpp
	Windows.cpp)
set(PublicHeaders
	${WAVM_INCLUDE_DIR}/Platform/Platform.h)
include_directories(${WAVM_INCLUDE_DIR}/Platform)

add_definitions(-DPLATFORM_API=DLL_EXPORT)

add_library(Platform STATIC ${Sources} ${PublicHeaders})

# Link with dl on Linux for dladdr.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(Platform dl pthread rt)
endif()
