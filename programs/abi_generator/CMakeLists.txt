set(SOURCES main.cpp)

# Find an installed build of LLVM
find_package(LLVM 4.0 REQUIRED CONFIG)

# Include the LLVM headers
include_directories(${LLVM_INCLUDE_DIRS})

add_executable(abi_generator ${SOURCES})

set( CMAKE_CXX_STANDARD 14 )

if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling steemd with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries(abi_generator fc eos_types
  clangRewrite
  clangTooling
  clangToolingCore
  clangFrontend
  clangDriver
  clangSerialization
  clangParse
  clangSema
  clangAnalysis
  clangAST
  clangBasic
  clangEdit
  clangLex
)

target_link_libraries(abi_generator
  LLVMX86AsmParser # MC, MCParser, Support, X86CodeGen, X86Desc, X86Info
  LLVMX86CodeGen # Analysis, AsmPrinter, CodeGen, Core, MC, Support, Target, 
                 # X86AsmPrinter, X86Desc, X86Info, X86Utils
  LLVMX86Desc # MC, MCDisassembler, Object, Support, X86AsmPrinter, X86Info
  LLVMX86AsmPrinter # MC, Support, X86Utils
  LLVMX86Info # Support
  LLVMX86Utils # Core, Support
  LLVMCodeGen # Analysis, Core, MC, Scalar, Support, Target, TransformUtils
  LLVMipo
  LLVMScalarOpts
  LLVMInstCombine
  LLVMTransformUtils
  LLVMTarget # Analysis, MC, Core, Support
  LLVMAnalysis # Core, Support
  LLVMOption # Support
  LLVMMCDisassembler # MC, Support
  LLVMMCParser # MC, Support
  LLVMMC # Object, Support
  LLVMProfileData # Core, Support, Object
  LLVMObject # BitReader, Core, Support
  LLVMBitReader # Core, Support
  LLVMCore # BinaryFormat, Support
  #LLVMBinaryFormat # Support
  LLVMSupport # Demangle
  LLVMDemangle
)

install( TARGETS
   abi_generator
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
