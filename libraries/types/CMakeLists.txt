file(GLOB HEADERS "include/eos/types/*.hpp")

add_library( eos_types
             Asset.cpp
             PublicKey.cpp
             TypeParser.cpp
             native.cpp
             ${HEADERS}
             )
target_include_directories( eos_types PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_link_libraries( eos_types fc )

add_executable( type_generator type_generator.cpp )
target_link_libraries( type_generator fc eos_types ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}  )

add_custom_command( OUTPUT  include/eos/types/generated.hpp
                    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/type_generator ${CMAKE_CURRENT_SOURCE_DIR}/types.eos ${CMAKE_CURRENT_SOURCE_DIR}/include/eos/types/generated.hpp
                    DEPENDS types.eos type_generator )

add_executable( types_test test.cpp include/eos/types/generated.hpp )
target_link_libraries( types_test eos_types fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}  )

