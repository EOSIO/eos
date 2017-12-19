set(WASM_TOOLCHAIN FALSE)

if(NOT DEFINED WASM_LLVM_CONFIG)
  if(NOT "$ENV{WASM_LLVM_CONFIG}" STREQUAL "")
    set(WASM_LLVM_CONFIG "$ENV{WASM_LLVM_CONFIG}" CACHE FILEPATH "Location of llvm-config compiled with WASM support.")
  endif()
endif()

if(WASM_LLVM_CONFIG)
  execute_process(
    COMMAND ${WASM_LLVM_CONFIG} --bindir
    RESULT_VARIABLE WASM_LLVM_CONFIG_OK
    OUTPUT_VARIABLE WASM_LLVM_BIN
  )

  if("${WASM_LLVM_CONFIG_OK}" STREQUAL "0")
    string(STRIP "${WASM_LLVM_BIN}" WASM_LLVM_BIN)
    set(WASM_CLANG ${WASM_LLVM_BIN}/clang)
    set(WASM_LLC ${WASM_LLVM_BIN}/llc)
    set(WASM_LLVM_LINK ${WASM_LLVM_BIN}/llvm-link)
  endif()

else()
  set(WASM_CLANG $ENV{WASM_CLANG})
  set(WASM_LLC $ENV{WASM_LLC})
  set(WASM_LLVM_LINK $ENV{WASM_LLVM_LINK})
endif()

if( NOT ("${WASM_CLANG}" STREQUAL "" OR "${WASM_LLC}" STREQUAL "" OR "${WASM_LLVM_LINK}" STREQUAL "") )
  if( NOT "${BINARYEN_ROOT}" STREQUAL "" )

    if(EXISTS "${BINARYEN_ROOT}/bin/s2wasm")

      set(BINARYEN_BIN ${BINARYEN_ROOT}/bin)

    endif()

  else()

    message(STATUS "BINARYEN_BIN not defined looking in PATH")
    find_path(BINARYEN_BIN
              NAMES s2wasm
              ENV PATH )
    if (BINARYEN_BIN AND NOT EXISTS ${BINARYEN_ROOT}/s2wasm)

      unset(BINARYEN_BIN)

    endif()

  endif()

  message(STATUS "BINARYEN_BIN => " ${BINARYEN_BIN})

endif()

# TODO: Check if compiler is able to generate wasm32
if( NOT ("${WASM_CLANG}" STREQUAL "" OR "${WASM_LLC}" STREQUAL "" OR "${WASM_LLVM_LINK}" STREQUAL "" OR NOT BINARYEN_BIN) )
  set(WASM_TOOLCHAIN TRUE)
endif()

macro(add_wast_target target INCLUDE_FOLDERS DESTINATION_FOLDER)

  # NOTE: Setting SOURCE_FILE and looping over it to avoid cmake issue with compilation ${target}.bc's rule colliding with
  # linking ${target}.bc's rule 
  set(SOURCE_FILE ${target}.cpp)
  foreach(srcfile ${SOURCE_FILE})
    
    get_filename_component(outfile ${srcfile} NAME)
    get_filename_component(infile ${srcfile} ABSOLUTE)

    # -ffreestanding
    #   Assert that compilation targets a freestanding environment.
    #   This implies -fno-builtin. A freestanding environment is one in which the standard library may not exist, and program startup may not necessarily be at main.
    #   The most obvious example is an OS kernel.

    # -nostdlib
    #   Do not use the standard system startup files or libraries when linking.
    #   No startup files and only the libraries you specify are passed to the linker, and options specifying linkage of the system libraries, such as -static-libgcc or -shared-libgcc, are ignored.
    #   The compiler may generate calls to memcmp, memset, memcpy and memmove.
    #   These entries are usually resolved by entries in libc. These entry points should be supplied through some other mechanism when this option is specified.

    # -fno-threadsafe-statics
    #   Do not emit the extra code to use the routines specified in the C++ ABI for thread-safe initialization of local statics.
    #   You can use this option to reduce code size slightly in code that doesn’t need to be thread-safe.

    # -fno-rtti
    #   Disable generation of information about every class with virtual functions for use by the C++ run-time type identification features (dynamic_cast and typeid).

    # -fno-exceptions
    #   Disable the generation of extra code needed to propagate exceptions
  
    add_custom_command(OUTPUT ${outfile}.bc
      DEPENDS ${infile}
      COMMAND ${WASM_CLANG} -emit-llvm -O3 --std=c++14 --target=wasm32 -ffreestanding
              -nostdlib -fno-threadsafe-statics -fno-rtti -fno-exceptions -I ${INCLUDE_FOLDERS}
              -c ${infile} -o ${outfile}.bc 
      IMPLICIT_DEPENDS CXX ${infile}
      COMMENT "Building LLVM bitcode ${outfile}.bc"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      VERBATIM
    )
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${outfile}.bc)
    list(APPEND outfiles ${outfile}.bc)

  endforeach(srcfile)

  add_custom_command(OUTPUT ${target}.bc
    DEPENDS ${outfiles}
    COMMAND ${WASM_LLVM_LINK} -o ${target}.bc ${outfiles}
    COMMENT "Linking LLVM bitcode ${target}.bc"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
  )
  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${target}.bc)

  add_custom_command(OUTPUT ${target}.s
    DEPENDS ${target}.bc
    COMMAND ${WASM_LLC} -asm-verbose=false -o ${target}.s ${target}.bc
    COMMENT "Generating textual assembly ${target}.s"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
  )
  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${target}.s)

  add_custom_command(OUTPUT ${DESTINATION_FOLDER}/${target}.wast
    DEPENDS ${target}.s
    COMMAND ${BINARYEN_BIN}/s2wasm -o ${DESTINATION_FOLDER}/${target}.wast -s 1024 ${target}.s

    COMMENT "Generating WAST ${target}.wast"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
  )
  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${target}.wast)
  
  add_custom_command(OUTPUT ${DESTINATION_FOLDER}/${target}.wast.hpp
    DEPENDS ${DESTINATION_FOLDER}/${target}.wast
    COMMAND echo "const char* ${target}_wast = R\"=====("  > ${DESTINATION_FOLDER}/${target}.wast.hpp
    COMMAND cat ${DESTINATION_FOLDER}/${target}.wast >> ${DESTINATION_FOLDER}/${target}.wast.hpp
    COMMAND echo ")=====\";"  >> ${DESTINATION_FOLDER}/${target}.wast.hpp
    COMMENT "Generating ${target}.wast.hpp"
    VERBATIM
  )
  
  if (EXISTS ${DESTINATION_FOLDER}/${target}.abi )
    add_custom_command(OUTPUT ${DESTINATION_FOLDER}/${target}.abi.hpp
      DEPENDS ${DESTINATION_FOLDER}/${target}.abi
      COMMAND echo "const char* ${target}_abi = R\"=====("  > ${DESTINATION_FOLDER}/${target}.abi.hpp
      COMMAND cat ${DESTINATION_FOLDER}/${target}.abi >> ${DESTINATION_FOLDER}/${target}.abi.hpp
      COMMAND echo ")=====\";"  >> ${DESTINATION_FOLDER}/${target}.abi.hpp
      COMMENT "Generating ${target}.abi.hpp"
      VERBATIM
    )
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${target}.abi.hpp)
  else()
  endif()
  
  
  add_custom_target(${target} ALL DEPENDS ${DESTINATION_FOLDER}/${target}.wast.hpp ${extra_target_dependency})
  
  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${DESTINATION_FOLDER}/${target}.wast.hpp)

  set_property(TARGET ${target} PROPERTY INCLUDE_DIRECTORIES ${INCLUDE_FOLDERS})

  set(extra_target_dependency)

endmacro(add_wast_target)

function(add_wast_abi_target target INCLUDE_FOLDERS SOURCE_FOLDER DESTINATION_FOLDER)
  add_custom_command(OUTPUT ${DESTINATION_FOLDER}/${target}.abi.hpp
    DEPENDS ${SOURCE_FOLDER}/${target}.abi
    COMMAND echo "const char* ${target}_abi = R\"=====("  > ${DESTINATION_FOLDER}/${target}.abi.hpp
    COMMAND cat ${SOURCE_FOLDER}/${target}.abi >> ${DESTINATION_FOLDER}/${target}.abi.hpp
    COMMAND echo ")=====\";"  >> ${DESTINATION_FOLDER}/${target}.abi.hpp
    COMMENT "Generating ${target}.abi.hpp"
    VERBATIM
  )
  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${target}.abi.hpp)
  set(extra_target_dependency   ${DESTINATION_FOLDER}/${target}.abi.hpp)
  add_wast_target(${target} "${INCLUDE_FOLDERS}" ${CMAKE_CURRENT_BINARY_DIR})
endfunction(add_wast_abi_target)
