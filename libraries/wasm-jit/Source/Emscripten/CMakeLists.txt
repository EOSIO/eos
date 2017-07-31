set(Sources Emscripten.cpp)
set(PublicHeaders ${WAVM_INCLUDE_DIR}/Emscripten/Emscripten.h)
include_directories(${WAVM_INCLUDE_DIR}/Emscripten)

add_library(Emscripten STATIC ${Sources} ${PublicHeaders})

add_definitions(-DEMSCRIPTEN_API=DLL_EXPORT)

target_link_libraries(Emscripten Logging Platform Runtime)
