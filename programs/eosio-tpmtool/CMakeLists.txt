if(NOT TARGET tpm-helpers)
   return()
endif()

add_executable(eosio-tpmtool main.cpp)

target_link_libraries(eosio-tpmtool PRIVATE tpm-helpers Boost::program_options)

copy_bin(eosio-tpmtool)
install(TARGETS
   eosio-tpmtool

   COMPONENT base

   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)