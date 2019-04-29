find_package(PkgConfig REQUIRED)
pkg_search_module(HIDAPI REQUIRED IMPORTED_TARGET hidapi)
add_subdirectory(tinycbor)

add_library(microfido microfido.cpp)
target_include_directories(microfido PUBLIC include)
target_link_libraries(microfido ${HIDAPI_LIBRARIES} tinycbor fc PkgConfig::HIDAPI)