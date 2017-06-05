file(GLOB HEADERS "include/wrenbase/*.hpp")

add_library( wrenbase
             Int.cpp 
             ${HEADERS} )

target_include_directories( wrenbase PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_link_libraries( wrenbase wren )
set_property(TARGET wrenbase PROPERTY CXX_STANDARD 14)

add_executable( test test.cpp )
target_link_libraries( test fc wren wrenbase ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}  )
