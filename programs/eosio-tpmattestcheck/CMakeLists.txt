if(NOT TARGET tpm-helpers)
   return()
endif()

add_executable(eosio-tpmattestcheck main.cpp)

target_link_libraries(eosio-tpmattestcheck PRIVATE tpm-helpers Boost::program_options)

copy_bin(eosio-tpmattestcheck)
install(TARGETS
   eosio-tpmattestcheck

   COMPONENT base

   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)
