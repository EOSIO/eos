add_library(libeosioclient
    eosioclient.hpp
    eosioclient.cpp
    functions.hpp
    )
target_link_libraries(libeosioclient chain_plugin)

add_subdirectory(test)
