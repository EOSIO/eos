set(Sources
	Logging.cpp)
set(PublicHeaders
	${WAVM_INCLUDE_DIR}/Logging/Logging.h)
include_directories(${WAVM_INCLUDE_DIR}/Logging)

add_definitions(-DLOGGING_API=DLL_EXPORT)

add_library(Logging STATIC ${Sources} ${PublicHeaders})

# Link with dl on Linux for dladdr.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(Logging dl pthread)
endif()

