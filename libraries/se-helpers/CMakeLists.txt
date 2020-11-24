if(NOT APPLE)
   return()
endif()

add_library(se-helpers se-helpers.cpp)
target_include_directories(se-helpers PUBLIC include)
target_link_libraries(se-helpers fc)