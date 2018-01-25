add_library(libeosioc
    functions.hpp
    remote.hpp
    remote.cpp
    wallet.hpp
    wallet.cpp
    peer.hpp
    peer.cpp
    )
target_link_libraries(libeosioc fc eosio_chain)
target_include_directories(libeosioc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(test)

install(TARGETS libeosioc DESTINATION lib)
install(FILES functions.hpp peer.hpp  wallet.hpp remote.hpp DESTINATION "include/eosio/eosioclient")
