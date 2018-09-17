set(Sources WASMSerialization.cpp)
set(PublicHeaders ${WAVM_INCLUDE_DIR}/WASM/WASM.h)
include_directories(${WAVM_INCLUDE_DIR}/WASM)

add_library(WASM STATIC ${Sources} ${PublicHeaders})
add_definitions(-DWEBASSEMBLY_API=DLL_EXPORT)
target_link_libraries(WASM Logging IR)

install(TARGETS WASM
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR})
