set(Sources
	NFA.cpp
	NFA.h
	Lexer.cpp
	Lexer.h
	Parse.cpp
	Parse.h
	ParseFunction.cpp
	ParseNumbers.cpp
	ParseModule.cpp
	ParseTests.cpp
	Print.cpp
	Regexp.cpp
	Regexp.h)
set(PublicHeaders
	${WAVM_INCLUDE_DIR}/WAST/WAST.h
	${WAVM_INCLUDE_DIR}/WAST/TestScript.h)
include_directories(${WAVM_INCLUDE_DIR}/WAST)

add_library(WAST STATIC ${Sources} ${PublicHeaders})
add_definitions(-DWAST_API=DLL_EXPORT)
target_link_libraries(WAST Logging IR WASM)
install(TARGETS WAST 
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})

