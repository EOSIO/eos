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

macro(compile_wast)
  #read arguments include ones that we don't since arguments get forwared "as is" and we don't want to threat unknown argument names as values
  cmake_parse_arguments(ARG "NOWARNINGS" "TARGET;DESTINATION_FOLDER" "SOURCE_FILES;INCLUDE_FOLDERS;SYSTEM_INCLUDE_FOLDERS;LIBRARIES" ${ARGN})
  set(target ${ARG_TARGET})

  # NOTE: Setting SOURCE_FILE and looping over it to avoid cmake issue with compilation ${target}.bc's rule colliding with
  # linking ${target}.bc's rule
  if ("${ARG_SOURCE_FILES}" STREQUAL "")
    set(SOURCE_FILES ${target}.cpp)
  else()
    set(SOURCE_FILES ${ARG_SOURCE_FILES})
  endif()
  set(outfiles "")
  foreach(srcfile ${SOURCE_FILES})
    
    get_filename_component(outfile ${srcfile} NAME)
    get_filename_component(extension ${srcfile} EXT)
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
    if ("${extension}" STREQUAL ".c")
      set(STDFLAG -D_XOPEN_SOURCE=700)
    else()
      set(STDFLAG "--std=c++14")
    endif()

    set(WASM_COMMAND ${WASM_CLANG} -emit-llvm -O3 ${STDFLAG} --target=wasm32 -ffreestanding
              -nostdlib -nostdlibinc -fno-threadsafe-statics -fno-rtti -fno-exceptions
              -c ${infile} -o ${outfile}.bc
    )
    if (${ARG_NOWARNINGS})
      list(APPEND WASM_COMMAND -Wno-everything)
    else()
      list(APPEND WASM_COMMAND -Weverything -Wno-c++98-compat -Wno-old-style-cast -Wno-vla -Wno-vla-extension -Wno-c++98-compat-pedantic
                  -Wno-missing-prototypes -Wno-missing-variable-declarations -Wno-packed)
    endif()

    foreach(folder ${ARG_INCLUDE_FOLDERS})
       list(APPEND WASM_COMMAND -I ${folder})
    endforeach()

    if ("${ARG_SYSTEM_INCLUDE_FOLDERS}" STREQUAL "")
       set (ARG_SYSTEM_INCLUDE_FOLDERS ${DEFAULT_SYSTEM_INCLUDE_FOLDERS})
    endif()
    foreach(folder ${ARG_SYSTEM_INCLUDE_FOLDERS})
       list(APPEND WASM_COMMAND -isystem ${folder})
    endforeach()

    foreach(folder ${ARG_SYSTEM_INCLUDE_FOLDERS})
       list(APPEND WASM_COMMAND -isystem ${folder})
    endforeach()

    add_custom_command(OUTPUT ${outfile}.bc
      DEPENDS ${infile}
      COMMAND ${WASM_COMMAND}
      IMPLICIT_DEPENDS CXX ${infile}
      COMMENT "Building LLVM bitcode ${outfile}.bc"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      VERBATIM
    )
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${outfile}.bc)
    list(APPEND outfiles ${outfile}.bc)

  endforeach(srcfile)

  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${target}.bc)

endmacro(compile_wast)

macro(add_wast_library)
  cmake_parse_arguments(ARG "NOWARNINGS" "TARGET;DESTINATION_FOLDER" "SOURCE_FILES;INCLUDE_FOLDERS;SYSTEM_INCLUDE_FOLDERS" ${ARGN})
  set(target ${ARG_TARGET})
  compile_wast(${ARGV})

  get_filename_component("${ARG_TARGET}_BC_FILENAME" "${ARG_DESTINATION_FOLDER}/${ARG_TARGET}.bc" ABSOLUTE CACHE)
  add_custom_target(${target} ALL DEPENDS ${${ARG_TARGET}_BC_FILENAME})

  add_custom_command(OUTPUT ${${ARG_TARGET}_BC_FILENAME}
    DEPENDS ${outfiles}
    COMMAND ${WASM_LLVM_LINK} -o ${${ARG_TARGET}_BC_FILENAME} ${outfiles}
    COMMENT "Linking LLVM bitcode library ${target}.bc"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
  )

endmacro(add_wast_library)

macro(add_wast_executable)
  cmake_parse_arguments(ARG "NOWARNINGS" "TARGET;DESTINATION_FOLDER" "SOURCE_FILES;INCLUDE_FOLDERS;SYSTEM_INCLUDE_FOLDERS;LIBRARIES" ${ARGN})
  set(target ${ARG_TARGET})
  set(DESTINATION_FOLDER ${ARG_DESTINATION_FOLDER})

  compile_wast(${ARGV})

  foreach(lib ${ARG_LIBRARIES})
     list(APPEND LIBRARIES ${${lib}_BC_FILENAME})
  endforeach()
  add_custom_command(OUTPUT ${target}.bc
    DEPENDS ${outfiles} ${ARG_LIBRARIES} ${LIBRARIES}
    COMMAND ${WASM_LLVM_LINK} -only-needed -o ${target}.bc ${outfiles} ${LIBRARIES}
    COMMENT "Linking LLVM bitcode executable ${target}.bc"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
  )

  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${target}.bc)

  add_custom_command(OUTPUT ${target}.s
    DEPENDS ${target}.bc
    COMMAND ${WASM_LLC} -thread-model=single -asm-verbose=false -o ${target}.s ${target}.bc
    COMMENT "Generating textual assembly ${target}.s"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
  )
  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${target}.s)

  add_custom_command(OUTPUT ${DESTINATION_FOLDER}/${target}.wast
    DEPENDS ${target}.s
    COMMAND ${BINARYEN_BIN}/s2wasm -o ${DESTINATION_FOLDER}/${target}.wast -s 4096 ${target}.s
    COMMENT "Generating WAST ${target}.wast"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
  )
  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${target}.wast)
  STRING (REPLACE "." "_" TARGET_VARIABLE "${target}")

  add_custom_command(OUTPUT ${DESTINATION_FOLDER}/${target}.wast.hpp
    DEPENDS ${DESTINATION_FOLDER}/${target}.wast
    COMMAND echo "const char* const ${TARGET_VARIABLE}_wast = R\"=====("  > ${DESTINATION_FOLDER}/${target}.wast.hpp
    COMMAND cat ${DESTINATION_FOLDER}/${target}.wast >> ${DESTINATION_FOLDER}/${target}.wast.hpp
    COMMAND echo ")=====\";"  >> ${DESTINATION_FOLDER}/${target}.wast.hpp
    COMMENT "Generating ${target}.wast.hpp"
    VERBATIM
  )
  
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${target}.abi )
    add_custom_command(OUTPUT ${DESTINATION_FOLDER}/${target}.abi.hpp
      DEPENDS ${DESTINATION_FOLDER}/${target}.abi
      COMMAND echo "const char* const ${TARGET_VARIABLE}_abi = R\"=====("  > ${DESTINATION_FOLDER}/${target}.abi.hpp
      COMMAND cat ${DESTINATION_FOLDER}/${target}.abi >> ${DESTINATION_FOLDER}/${target}.abi.hpp
      COMMAND echo ")=====\";"  >> ${DESTINATION_FOLDER}/${target}.abi.hpp
      COMMENT "Generating ${target}.abi.hpp"
      VERBATIM
    )
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${target}.abi.hpp)
    set(extra_target_dependency   ${DESTINATION_FOLDER}/${target}.abi.hpp)
  else()
  endif()
  
  add_custom_target(${target} ALL DEPENDS ${DESTINATION_FOLDER}/${target}.wast.hpp ${extra_target_dependency})
  
  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${DESTINATION_FOLDER}/${target}.wast.hpp)

  set_property(TARGET ${target} PROPERTY INCLUDE_DIRECTORIES ${ARG_INCLUDE_FOLDERS})

  set(extra_target_dependency)

endmacro(add_wast_executable)
