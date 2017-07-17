set(WASM_TOOLCHAIN FALSE)

if( NOT "$ENV{WASM_LLVM_CONFIG}" STREQUAL "" )

  execute_process(
    COMMAND $ENV{WASM_LLVM_CONFIG} --bindir
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

# TODO: Check if compiler is able to generate wasm32
if( NOT ("${WASM_CLANG}" STREQUAL "" OR "${WASM_LLC}" STREQUAL "" OR "${WASM_LLVM_LINK}" STREQUAL "") )
  set(WASM_TOOLCHAIN TRUE)
endif()

macro(add_wast_target target SOURCE_FILES INCLUDE_FOLDERS DESTINATION_FOLDER)

  # add_definitions( -DDebug )
  # get_directory_property( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )
  
  set(outfiles "")
  foreach(srcfile ${SOURCE_FILES})
    
    get_filename_component(outfile ${srcfile} NAME)
    get_filename_component(infile ${srcfile} ABSOLUTE)

    add_custom_command(OUTPUT ${outfile}.bc
      DEPENDS ${infile}
      COMMAND ${WASM_CLANG} -emit-llvm -O3 --std=c++14 --target=wasm32 -I ${INCLUDE_FOLDERS} -fno-threadsafe-statics -fno-rtti -fno-exceptions -c ${infile} -o ${outfile}.bc
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
    COMMAND s2wasm -o ${DESTINATION_FOLDER}/${target}.wast -s 1024 ${target}.s
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

  add_custom_target(${target} ALL DEPENDS ${DESTINATION_FOLDER}/${target}.wast.hpp)

endmacro(add_wast_target)
