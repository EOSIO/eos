include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(eosioclient
    eosioclient.hpp
    eosioclient.cpp
    functions.hpp
    )
target_link_libraries(eosioclient chain_plugin)

add_subdirectory(test)
