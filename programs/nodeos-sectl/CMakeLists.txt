if(NOT MAS_ENABLED OR MAS_LEGACY)
   return()
endif()

add_executable(${NODE_EXECUTABLE_NAME}-sectl main.cpp)

configure_file(config.hpp.in config.hpp ESCAPE_QUOTES)

target_include_directories(${NODE_EXECUTABLE_NAME}-sectl PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${NODE_EXECUTABLE_NAME}-sectl PRIVATE se-helpers Boost::program_options)

copy_bin(${NODE_EXECUTABLE_NAME}-sectl)
install(TARGETS
   ${NODE_EXECUTABLE_NAME}-sectl

   COMPONENT base

   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
)

mas_sign(${NODE_EXECUTABLE_NAME}-sectl ${NODE_EXECUTABLE_NAME})