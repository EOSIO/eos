add_library(eosioclient
    eosioclient.hpp
    eosioclient.cpp
    functions.hpp
    )
target_link_libraries(eosioclient chain_plugin)
target_include_directories(eosioclient INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(test)
