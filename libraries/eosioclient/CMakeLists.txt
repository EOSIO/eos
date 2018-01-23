add_library(eosioclient
    remote.hpp
    remote.cpp
    wallet.hpp
    wallet.cpp
    peer.hpp
    peer.cpp
    )
target_link_libraries(eosioclient fc eosio_chain)
target_include_directories(eosioclient INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(test)

install(TARGETS eosioclient DESTINATION lib)
install(FILES functions.hpp eosioclient.hpp DESTINATION "include/eosio/eosioclient")
