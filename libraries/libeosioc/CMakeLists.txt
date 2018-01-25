add_library(eosioc
    endpoints.hpp
    remote.hpp
    remote.cpp
    wallet.hpp
    wallet.cpp
    peer.hpp
    peer.cpp
    )
target_link_libraries(eosioc fc eosio_chain)
target_include_directories(eosioc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(test)

install(TARGETS eosioc DESTINATION lib)
install(FILES endpoints.hpp peer.hpp  wallet.hpp remote.hpp DESTINATION "include/eosio/eosioclient")
