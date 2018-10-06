#list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/libraries/fc/GitVersionGen" )
#include( GetGitRevisionDescription )
#get_git_head_revision(GIT_REFSPEC RSN_GIT_REVISION_SHA)
#get_git_unix_timestamp(RSN_GIT_REVISION_UNIX_TIMESTAMP)
#git_describe(RSN_GIT_REVISION_DESCRIPTION --tags)
if(NOT RSN_GIT_REVISION_DESCRIPTION)
    set(RSN_GIT_REVISION_DESCRIPTION "unknown")
endif(NOT RSN_GIT_REVISION_DESCRIPTION)

file(GLOB HEADERS "include/arisen/utilities/*.hpp")

set(sources
   key_conversion.cpp
   string_escape.cpp
   tempdir.cpp
   words.cpp
   ${HEADERS})

#configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)
#list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp")

add_library( rsn_utilities
             ${sources}
             ${HEADERS} )
target_link_libraries( rsn_utilities fc WAST WASM )
target_include_directories( rsn_utilities
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include" )
if (USE_PCH)
  set_target_properties(rsn_utilities PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(rsn_utilities)
endif(USE_PCH)
