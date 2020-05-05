# Defines builtins library
project (builtins C)

# generate compile command database for external tools
set (CMAKE_EXPORT_COMPILE_COMMANDS "ON")

message ( STATUS "Configuring Builtins" )
set(C_DEFINES, "-D__wasm__ -DQUAD_PRECISION")
set( CMAKE_C_FLAGS " -Wall ${CMAKE_C_FLAGS} ${C_DEFINES}" )
set ( builtins_sources
   fixtfti.c
   fixunstfti.c
   fixsfti.c
   fixdfti.c
   fixunssfti.c
   fixunsdfti.c
   floattidf.c
   floatuntidf.c
)

file ( GLOB builtins_headers "${CMAKE_CURRENT_SOURCE_DIR}*.h" )
list( APPEND builtins_sources ${builtins_headers} )
add_library ( builtins STATIC ${builtins_sources} )
target_include_directories( builtins PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
                                            "${CMAKE_CURRENT_SOURCE_DIR}../softfloat/source/include" )
