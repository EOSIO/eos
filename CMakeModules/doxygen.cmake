include(FindDoxygen)

if(NOT DOXYGEN_FOUND)
  message(STATUS "Doxygen not found.  Contract documentation will not be generated.")
else()
  message(STATUS "Doxygen found.  Contract documentation will be generated.")
  set(DOXY_EOS_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" CACHE INTERNAL "Version string used in PROJECT_NUMBER.")
  # CMake strips trailing path separators off of variables it knows are paths,
  # so the trailing '/' Doxygen expects is embedded in the doxyfile.
  set(DOXY_DOC_DEST_DIR "${CMAKE_BINARY_DIR}/docs" CACHE PATH "Path to the doxygen output")
  set(DOXY_DOC_INPUT_ROOT_DIR "contracts" CACHE PATH "Path to the doxygen input")
  if(DOXYGEN_DOT_FOUND)
    set(DOXY_HAVE_DOT "YES" CACHE STRING "Doxygen to use dot for diagrams.")
  else(DOXYGEN_DOT_FOUND)
    set(DOXY_HAVE_DOT "NO" CACHE STRING "Doxygen to use dot for diagrams.")
  endif(DOXYGEN_DOT_FOUND)
  configure_file("eos.doxygen.in" "${CMAKE_BINARY_DIR}/eos.doxygen")
  if(BUILD_DOXYGEN)
     # !! Core developers who don't want to see the doxygen output on every build
     # !! should remove the 'ALL' option from both commands below.
     # Doxygen has issues making destination directories more than one level deep, so do it for it.
     add_custom_target(make_doc_dir ALL COMMAND ${CMAKE_COMMAND} -E make_directory "${DOXY_DOC_DEST_DIR}")
     add_custom_target(contract_documentation ALL
         COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/eos.doxygen"
         DEPENDS make_doc_dir
         WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
         COMMENT "Building doxygen documentation into ${DOXY_DOC_DEST_DIR}..."
         VERBATIM)
  endif(BUILD_DOXYGEN)
endif()
