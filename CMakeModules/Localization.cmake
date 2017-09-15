# Builds on top of FindGetText from standard cmake to allow running the
# localized binaries before installation.
#
# this requires placing the files in the proper tree with a proper
# 

find_package(Gettext)
if(GETTEXT_FOUND)
   set(LOCALIZATION_ENABLED TRUE)
else()
   set(LOCALIZATION_ENABLED FALSE)
endif()

find_package(ICU)
if(ICU_FOUND)
   set(Localization_LIBRARIES ICU_LIBRARIES)
else()
   set(Localization_LIBRARIES "")
endif()

function(LOCALIZATION_PROCESS_PO_FILES _lang)
   set(_options)
   set(_oneValueArgs)
   set(_multiValueArgs PO_FILES)
   set(_moFiles)

   CMAKE_PARSE_ARGUMENTS(_parsedArguments "${_options}" "${_oneValueArgs}" "${_multiValueArgs}" ${ARGN})

   set(_moFileDest ${CMAKE_CURRENT_BINARY_DIR}/LC_MESSAGES )

   foreach(_current_PO_FILE ${_parsedArguments_PO_FILES})
      get_filename_component(_name ${_current_PO_FILE} NAME)
      string(REGEX REPLACE "^(.+)(\\.[^.]+)$" "\\1" _basename ${_name})
      set(_moFile ${_moFileDest}/${_basename}.mo)
      add_custom_command(OUTPUT ${_moFile}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${_moFileDest}
            COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${_moFile} ${_current_PO_FILE}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            DEPENDS ${_current_PO_FILE}
         )

      list(APPEND _moFiles ${_moFile})
   endforeach()


  if(NOT TARGET localization)
     add_custom_target(localization)
  endif()

  _GETTEXT_GET_UNIQUE_TARGET_NAME( localization uniqueTargetName)
  add_custom_target(${uniqueTargetName} ALL DEPENDS ${_moFiles})
  add_dependencies(localization ${uniqueTargetName})

endfunction()
