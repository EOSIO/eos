SET(SRC_FILENAMES algorithm.cpp any.cpp bind.cpp condition_variable.cpp exception.cpp functional.cpp
                  future.cpp ios.cpp iostream.cpp locale.cpp memory.cpp mutex.cpp new.cpp optional.cpp
                  regex.cpp shared_mutex.cpp stdexcept.cpp string.cpp strstream.cpp system_error.cpp
                  thread.cpp typeinfo.cpp utility.cpp valarray.cpp variant.cpp vector.cpp)

#SET(SRC_FILENAMES exception.cpp)

SET(SRC_FILES "")
FOREACH(FN ${SRC_FILENAMES})
   LIST(APPEND SRC_FILES "upstream/src/${FN}")
ENDFOREACH(FN)

add_wast_library(TARGET libc++
  NOWARNINGS
  SOURCE_FILES "${SRC_FILES}"
  INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}"
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)
