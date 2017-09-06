file(GLOB HEADERS "include/eos/types/*.hpp")

add_library( eos_types
             Asset.cpp
             PublicKey.cpp
             TypeParser.cpp
             AbiSerializer.cpp
             native.cpp
             ${HEADERS}
             "${CMAKE_CURRENT_SOURCE_DIR}/include/eos/types/generated.hpp"
             types.eos
             )
target_include_directories( eos_types PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_link_libraries( eos_types eos_utilities fc )

add_executable( type_generator type_generator.cpp TypeParser.cpp native.cpp )
target_include_directories( type_generator PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_link_libraries( type_generator eos_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}  )

add_custom_command( OUTPUT  "${CMAKE_CURRENT_SOURCE_DIR}/include/eos/types/generated.hpp"
                    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/type_generator ${CMAKE_CURRENT_SOURCE_DIR}/types.eos ${CMAKE_CURRENT_SOURCE_DIR}/include/eos/types/generated.hpp
                    DEPENDS types.eos type_generator )
INSTALL( TARGETS
   eos_types

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/eos/types" )
