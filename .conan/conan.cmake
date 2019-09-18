# The MIT License (MIT)

# Copyright (c) 2018 JFrog

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# This file comes from: https://github.com/conan-io/cmake-conan. Please refer
# to this repository for issues and documentation.

# Its purpose is to wrap and launch Conan C/C++ Package Manager when cmake is called.
# It will take CMake current settings (os, compiler, compiler version, architecture)
# and translate them to conan settings for installing and retrieving dependencies.

# It is intended to facilitate developers building projects that have conan dependencies,
# but it is only necessary on the end-user side. It is not necessary to create conan
# packages, in fact it shouldn't be use for that. Check the project documentation.


include(CMakeParseArguments)

function(_get_msvc_ide_version result)
    set(${result} "" PARENT_SCOPE)
    if(NOT MSVC_VERSION VERSION_LESS 1400 AND MSVC_VERSION VERSION_LESS 1500)
        set(${result} 8 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1500 AND MSVC_VERSION VERSION_LESS 1600)
        set(${result} 9 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1600 AND MSVC_VERSION VERSION_LESS 1700)
        set(${result} 10 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1700 AND MSVC_VERSION VERSION_LESS 1800)
        set(${result} 11 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1800 AND MSVC_VERSION VERSION_LESS 1900)
        set(${result} 12 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1900 AND MSVC_VERSION VERSION_LESS 1910)
        set(${result} 14 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1910 AND MSVC_VERSION VERSION_LESS 1920)
        set(${result} 15 PARENT_SCOPE)
    elseif(NOT MSVC_VERSION VERSION_LESS 1920 AND MSVC_VERSION VERSION_LESS 1930)
        set(${result} 16 PARENT_SCOPE)
    else()
        message(FATAL_ERROR "Conan: Unknown MSVC compiler version [${MSVC_VERSION}]")
    endif()
endfunction()

function(conan_cmake_settings result)
    #message(STATUS "COMPILER " ${CMAKE_CXX_COMPILER})
    #message(STATUS "COMPILER " ${CMAKE_CXX_COMPILER_ID})
    #message(STATUS "VERSION " ${CMAKE_CXX_COMPILER_VERSION})
    #message(STATUS "FLAGS " ${CMAKE_LANG_FLAGS})
    #message(STATUS "LIB ARCH " ${CMAKE_CXX_LIBRARY_ARCHITECTURE})
    #message(STATUS "BUILD TYPE " ${CMAKE_BUILD_TYPE})
    #message(STATUS "GENERATOR " ${CMAKE_GENERATOR})
    #message(STATUS "GENERATOR WIN64 " ${CMAKE_CL_64})

    message(STATUS "Conan: Automatic detection of conan settings from cmake")

    parse_arguments(${ARGV})

    if(ARGUMENTS_BUILD_TYPE)
        set(_CONAN_SETTING_BUILD_TYPE ${ARGUMENTS_BUILD_TYPE})
    elseif(CMAKE_BUILD_TYPE)
        set(_CONAN_SETTING_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    else()
        message(FATAL_ERROR "Please specify in command line CMAKE_BUILD_TYPE (-DCMAKE_BUILD_TYPE=Release)")
    endif()

    string(TOUPPER ${_CONAN_SETTING_BUILD_TYPE} _CONAN_SETTING_BUILD_TYPE_UPPER)
    if (_CONAN_SETTING_BUILD_TYPE_UPPER STREQUAL "DEBUG")
        set(_CONAN_SETTING_BUILD_TYPE "Debug")
    elseif(_CONAN_SETTING_BUILD_TYPE_UPPER STREQUAL "RELEASE")
        set(_CONAN_SETTING_BUILD_TYPE "Release")
    elseif(_CONAN_SETTING_BUILD_TYPE_UPPER STREQUAL "RELWITHDEBINFO")
        set(_CONAN_SETTING_BUILD_TYPE "RelWithDebInfo")
    elseif(_CONAN_SETTING_BUILD_TYPE_UPPER STREQUAL "MINSIZEREL")
        set(_CONAN_SETTING_BUILD_TYPE "MinSizeRel")
    endif()

    if(ARGUMENTS_ARCH)
        set(_CONAN_SETTING_ARCH ${ARGUMENTS_ARCH})
    endif()
    #handle -s os setting
    if(CMAKE_SYSTEM_NAME)
        #use default conan os setting if CMAKE_SYSTEM_NAME is not defined
        set(CONAN_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
        if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
            set(CONAN_SYSTEM_NAME Macos)
        endif()
        set(CONAN_SUPPORTED_PLATFORMS Windows Linux Macos Android iOS FreeBSD WindowsStore)
        list (FIND CONAN_SUPPORTED_PLATFORMS "${CONAN_SYSTEM_NAME}" _index)
        if (${_index} GREATER -1)
            #check if the cmake system is a conan supported one
            set(_CONAN_SETTING_OS ${CONAN_SYSTEM_NAME})
        else()
            message(FATAL_ERROR "cmake system ${CONAN_SYSTEM_NAME} is not supported by conan. Use one of ${CONAN_SUPPORTED_PLATFORMS}")
        endif()
    endif()

    get_property(_languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    if (";${_languages};" MATCHES ";CXX;")
        set(LANGUAGE CXX)
        set(USING_CXX 1)
    elseif (";${_languages};" MATCHES ";C;")
        set(LANGUAGE C)
        set(USING_CXX 0)
    else ()
        message(FATAL_ERROR "Conan: Neither C or C++ was detected as a language for the project. Unabled to detect compiler version.")
    endif()

    if (${CMAKE_${LANGUAGE}_COMPILER_ID} STREQUAL GNU)
        # using GCC
        # TODO: Handle other params
        string(REPLACE "." ";" VERSION_LIST ${CMAKE_${LANGUAGE}_COMPILER_VERSION})
        list(GET VERSION_LIST 0 MAJOR)
        list(GET VERSION_LIST 1 MINOR)
        set(COMPILER_VERSION ${MAJOR}.${MINOR})
        if(${MAJOR} GREATER 4)
            set(COMPILER_VERSION ${MAJOR})
        endif()
        set(_CONAN_SETTING_COMPILER gcc)
        set(_CONAN_SETTING_COMPILER_VERSION ${COMPILER_VERSION})
        if (USING_CXX)
            conan_cmake_detect_unix_libcxx(_LIBCXX)
            set(_CONAN_SETTING_COMPILER_LIBCXX ${_LIBCXX})
        endif ()
    elseif (${CMAKE_${LANGUAGE}_COMPILER_ID} STREQUAL AppleClang)
        # using AppleClang
        string(REPLACE "." ";" VERSION_LIST ${CMAKE_${LANGUAGE}_COMPILER_VERSION})
        list(GET VERSION_LIST 0 MAJOR)
        list(GET VERSION_LIST 1 MINOR)
        set(_CONAN_SETTING_COMPILER apple-clang)
        set(_CONAN_SETTING_COMPILER_VERSION ${MAJOR}.${MINOR})
        if (USING_CXX)
            conan_cmake_detect_unix_libcxx(_LIBCXX)
            set(_CONAN_SETTING_COMPILER_LIBCXX ${_LIBCXX})
        endif ()
    elseif (${CMAKE_${LANGUAGE}_COMPILER_ID} STREQUAL Clang)
        string(REPLACE "." ";" VERSION_LIST ${CMAKE_${LANGUAGE}_COMPILER_VERSION})
        list(GET VERSION_LIST 0 MAJOR)
        list(GET VERSION_LIST 1 MINOR)
        set(_CONAN_SETTING_COMPILER clang)
        set(_CONAN_SETTING_COMPILER_VERSION ${MAJOR}.${MINOR})
        if(APPLE)
            cmake_policy(GET CMP0025 APPLE_CLANG_POLICY_ENABLED)
            if(NOT APPLE_CLANG_POLICY_ENABLED)
                message(STATUS "Conan: APPLE and Clang detected. Assuming apple-clang compiler. Set CMP0025 to avoid it")
                set(_CONAN_SETTING_COMPILER apple-clang)
            endif()
        endif()
        if(${_CONAN_SETTING_COMPILER} STREQUAL clang AND ${MAJOR} GREATER 7)
            set(_CONAN_SETTING_COMPILER_VERSION ${MAJOR})
        endif()
        if (USING_CXX)
            conan_cmake_detect_unix_libcxx(_LIBCXX)
            set(_CONAN_SETTING_COMPILER_LIBCXX ${_LIBCXX})
        endif ()
    elseif(${CMAKE_${LANGUAGE}_COMPILER_ID} STREQUAL MSVC)
        set(_VISUAL "Visual Studio")
        _get_msvc_ide_version(_VISUAL_VERSION)
        if("${_VISUAL_VERSION}" STREQUAL "")
            message(FATAL_ERROR "Conan: Visual Studio not recognized")
        else()
            set(_CONAN_SETTING_COMPILER ${_VISUAL})
            set(_CONAN_SETTING_COMPILER_VERSION ${_VISUAL_VERSION})
        endif()

        if(NOT _CONAN_SETTING_ARCH)
            if (MSVC_${LANGUAGE}_ARCHITECTURE_ID MATCHES "64")
                set(_CONAN_SETTING_ARCH x86_64)
            elseif (MSVC_${LANGUAGE}_ARCHITECTURE_ID MATCHES "^ARM")
                message(STATUS "Conan: Using default ARM architecture from MSVC")
                set(_CONAN_SETTING_ARCH armv6)
            elseif (MSVC_${LANGUAGE}_ARCHITECTURE_ID MATCHES "86")
                set(_CONAN_SETTING_ARCH x86)
            else ()
                message(FATAL_ERROR "Conan: Unknown MSVC architecture [${MSVC_${LANGUAGE}_ARCHITECTURE_ID}]")
            endif()
        endif()

        conan_cmake_detect_vs_runtime(_vs_runtime)
        message(STATUS "Conan: Detected VS runtime: ${_vs_runtime}")
        set(_CONAN_SETTING_COMPILER_RUNTIME ${_vs_runtime})

        if (CMAKE_GENERATOR_TOOLSET)
            set(_CONAN_SETTING_COMPILER_TOOLSET ${CMAKE_VS_PLATFORM_TOOLSET})
        elseif(CMAKE_VS_PLATFORM_TOOLSET AND (CMAKE_GENERATOR STREQUAL "Ninja"))
            set(_CONAN_SETTING_COMPILER_TOOLSET ${CMAKE_VS_PLATFORM_TOOLSET})
        endif()
    else()
        message(FATAL_ERROR "Conan: compiler setup not recognized")
    endif()

    # If profile is defined it is used
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND ARGUMENTS_DEBUG_PROFILE)
        set(_APPLIED_PROFILES ${ARGUMENTS_DEBUG_PROFILE})
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release" AND ARGUMENTS_RELEASE_PROFILE)
        set(_APPLIED_PROFILES ${ARGUMENTS_RELEASE_PROFILE})
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" AND ARGUMENTS_RELWITHDEBINFO_PROFILE)
        set(_APPLIED_PROFILES ${ARGUMENTS_RELWITHDEBINFO_PROFILE})
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" AND ARGUMENTS_MINSIZEREL_PROFILE)
        set(_APPLIED_PROFILES ${ARGUMENTS_MINSIZEREL_PROFILE})
    elseif(ARGUMENTS_PROFILE)
        set(_APPLIED_PROFILES ${ARGUMENTS_PROFILE})
    endif()

    foreach(ARG ${_APPLIED_PROFILES})
        set(_SETTINGS ${_SETTINGS} -pr ${ARG})
    endforeach()

    if(NOT _SETTINGS OR ARGUMENTS_PROFILE_AUTO STREQUAL "ALL")
        set(ARGUMENTS_PROFILE_AUTO arch build_type compiler compiler.version
                                   compiler.runtime compiler.libcxx compiler.toolset)
    endif()

    # Automatic from CMake
    foreach(ARG ${ARGUMENTS_PROFILE_AUTO})
        string(TOUPPER ${ARG} _arg_name)
        string(REPLACE "." "_" _arg_name ${_arg_name})
        if(_CONAN_SETTING_${_arg_name})
            set(_SETTINGS ${_SETTINGS} -s ${ARG}=${_CONAN_SETTING_${_arg_name}})
        endif()
    endforeach()

    foreach(ARG ${ARGUMENTS_SETTINGS})
        set(_SETTINGS ${_SETTINGS} -s ${ARG})
    endforeach()

    message(STATUS "Conan: Settings= ${_SETTINGS}")

    set(${result} ${_SETTINGS} PARENT_SCOPE)
endfunction()


function(conan_cmake_detect_unix_libcxx result)
    # Take into account any -stdlib in compile options
    get_directory_property(compile_options DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_OPTIONS)

    # Take into account any _GLIBCXX_USE_CXX11_ABI in compile definitions
    get_directory_property(defines DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS)
    foreach(define ${defines})
        if(define MATCHES "_GLIBCXX_USE_CXX11_ABI")
            if(define MATCHES "^-D")
                set(compile_options ${compile_options} "${define}")
            else()
                set(compile_options ${compile_options} "-D${define}")
            endif()
        endif()
    endforeach()

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E echo "#include <string>"
        COMMAND ${CMAKE_CXX_COMPILER} -x c++ ${compile_options} -E -dM -
        OUTPUT_VARIABLE string_defines
    )

    if(string_defines MATCHES "#define __GLIBCXX__")
        # Allow -D_GLIBCXX_USE_CXX11_ABI=ON/OFF as argument to cmake
        if(DEFINED _GLIBCXX_USE_CXX11_ABI)
            if(_GLIBCXX_USE_CXX11_ABI)
                set(${result} libstdc++11 PARENT_SCOPE)
                return()
            else()
                set(${result} libstdc++ PARENT_SCOPE)
                return()
            endif()
        endif()

        if(string_defines MATCHES "#define _GLIBCXX_USE_CXX11_ABI 1\n")
            set(${result} libstdc++11 PARENT_SCOPE)
        else()
            # Either the compiler is missing the define because it is old, and so
            # it can't use the new abi, or the compiler was configured to use the
            # old abi by the user or distro (e.g. devtoolset on RHEL/CentOS)
            set(${result} libstdc++ PARENT_SCOPE)
        endif()
    else()
        set(${result} libc++ PARENT_SCOPE)
    endif()
endfunction()

function(conan_cmake_detect_vs_runtime result)
    string(TOUPPER ${CMAKE_BUILD_TYPE} build_type)
    set(variables CMAKE_CXX_FLAGS_${build_type} CMAKE_C_FLAGS_${build_type} CMAKE_CXX_FLAGS CMAKE_C_FLAGS)
    foreach(variable ${variables})
        string(REPLACE " " ";" flags ${${variable}})
        foreach (flag ${flags})
            if(${flag} STREQUAL "/MD" OR ${flag} STREQUAL "/MDd" OR ${flag} STREQUAL "/MT" OR ${flag} STREQUAL "/MTd")
                string(SUBSTRING ${flag} 1 -1 runtime)
                set(${result} ${runtime} PARENT_SCOPE)
                return()
            endif()
        endforeach()
    endforeach()
    if(${build_type} STREQUAL "DEBUG")
        set(${result} "MDd" PARENT_SCOPE)
    else()
        set(${result} "MD" PARENT_SCOPE)
    endif()
endfunction()


macro(parse_arguments)
  set(options BASIC_SETUP CMAKE_TARGETS UPDATE KEEP_RPATHS NO_LOAD NO_OUTPUT_DIRS OUTPUT_QUIET NO_IMPORTS)
  set(oneValueArgs CONANFILE  ARCH BUILD_TYPE INSTALL_FOLDER CONAN_COMMAND)
  set(multiValueArgs DEBUG_PROFILE RELEASE_PROFILE RELWITHDEBINFO_PROFILE MINSIZEREL_PROFILE
                     PROFILE REQUIRES OPTIONS IMPORTS SETTINGS BUILD ENV GENERATORS PROFILE_AUTO
                     INSTALL_ARGS)
  cmake_parse_arguments(ARGUMENTS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
endmacro()

function(conan_cmake_install)
    # Calls "conan install"
    # Argument BUILD is equivalant to --build={missing, PkgName,...} or
    # --build when argument is 'BUILD all' (which builds all packages from source)
    # Argument CONAN_COMMAND, to specify the conan path, e.g. in case of running from source
    # cmake does not identify conan as command, even if it is +x and it is in the path
    parse_arguments(${ARGV})

    if(CONAN_CMAKE_MULTI)
        set(ARGUMENTS_GENERATORS ${ARGUMENTS_GENERATORS} cmake_multi)
    else()
        set(ARGUMENTS_GENERATORS ${ARGUMENTS_GENERATORS} cmake)
    endif()

    set(CONAN_BUILD_POLICY "")
    foreach(ARG ${ARGUMENTS_BUILD})
        if(${ARG} STREQUAL "all")
            set(CONAN_BUILD_POLICY ${CONAN_BUILD_POLICY} --build)
            break()
        else()
            set(CONAN_BUILD_POLICY ${CONAN_BUILD_POLICY} --build=${ARG})
        endif()
    endforeach()
    if(ARGUMENTS_CONAN_COMMAND)
       set(conan_command ${ARGUMENTS_CONAN_COMMAND})
    else()
      set(conan_command conan)
    endif()
    set(CONAN_OPTIONS "")
    if(ARGUMENTS_CONANFILE)
      set(CONANFILE ${CMAKE_CURRENT_SOURCE_DIR}/${ARGUMENTS_CONANFILE})
      # A conan file has been specified - apply specified options as well if provided
      foreach(ARG ${ARGUMENTS_OPTIONS})
          set(CONAN_OPTIONS ${CONAN_OPTIONS} -o=${ARG})
      endforeach()
    else()
      set(CONANFILE ".")
    endif()
    if(ARGUMENTS_UPDATE)
      set(CONAN_INSTALL_UPDATE --update)
    endif()
    if(ARGUMENTS_NO_IMPORTS)
      set(CONAN_INSTALL_NO_IMPORTS --no-imports)
    endif()
    set(CONAN_INSTALL_FOLDER "")
    if(ARGUMENTS_INSTALL_FOLDER)
      set(CONAN_INSTALL_FOLDER -if=${ARGUMENTS_INSTALL_FOLDER})
    endif()
    foreach(ARG ${ARGUMENTS_GENERATORS})
        set(CONAN_GENERATORS ${CONAN_GENERATORS} -g=${ARG})
    endforeach()
    foreach(ARG ${ARGUMENTS_ENV})
        set(CONAN_ENV_VARS ${CONAN_ENV_VARS} -e=${ARG})
    endforeach()
    set(conan_args install ${CONANFILE} ${settings} ${CONAN_ENV_VARS} ${CONAN_GENERATORS} ${CONAN_BUILD_POLICY} ${CONAN_INSTALL_UPDATE} ${CONAN_INSTALL_NO_IMPORTS} ${CONAN_OPTIONS} ${CONAN_INSTALL_FOLDER} ${ARGUMENTS_INSTALL_ARGS})

    string (REPLACE ";" " " _conan_args "${conan_args}")
    message(STATUS "Conan executing: ${conan_command} ${_conan_args}")

    if(ARGUMENTS_OUTPUT_QUIET)
        execute_process(COMMAND ${conan_command} ${conan_args}
                        RESULT_VARIABLE return_code
                        OUTPUT_VARIABLE conan_output
                        ERROR_VARIABLE conan_output
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    else()
        execute_process(COMMAND ${conan_command} ${conan_args}
                        RESULT_VARIABLE return_code
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    if(NOT "${return_code}" STREQUAL "0")
      message(FATAL_ERROR "Conan install failed='${return_code}'")
    endif()

endfunction()


function(conan_cmake_setup_conanfile)
  parse_arguments(${ARGV})
  if(ARGUMENTS_CONANFILE)
    get_filename_component(_CONANFILE_NAME ${ARGUMENTS_CONANFILE} NAME)
    # configure_file will make sure cmake re-runs when conanfile is updated
    configure_file(${ARGUMENTS_CONANFILE} ${CMAKE_CURRENT_BINARY_DIR}/${_CONANFILE_NAME}.junk)
    file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/${_CONANFILE_NAME}.junk)
  else()
    conan_cmake_generate_conanfile(${ARGV})
  endif()
endfunction()

function(conan_cmake_generate_conanfile)
  # Generate, writing in disk a conanfile.txt with the requires, options, and imports
  # specified as arguments
  # This will be considered as temporary file, generated in CMAKE_CURRENT_BINARY_DIR)
  parse_arguments(${ARGV})
  set(_FN "${CMAKE_CURRENT_BINARY_DIR}/conanfile.txt")

  file(WRITE ${_FN} "[generators]\ncmake\n\n[requires]\n")
  foreach(ARG ${ARGUMENTS_REQUIRES})
    file(APPEND ${_FN} ${ARG} "\n")
  endforeach()

  file(APPEND ${_FN} ${ARG} "\n[options]\n")
  foreach(ARG ${ARGUMENTS_OPTIONS})
    file(APPEND ${_FN} ${ARG} "\n")
  endforeach()

  file(APPEND ${_FN} ${ARG} "\n[imports]\n")
  foreach(ARG ${ARGUMENTS_IMPORTS})
    file(APPEND ${_FN} ${ARG} "\n")
  endforeach()
endfunction()


macro(conan_load_buildinfo)
    if(CONAN_CMAKE_MULTI)
      set(_CONANBUILDINFO conanbuildinfo_multi.cmake)
    else()
      set(_CONANBUILDINFO conanbuildinfo.cmake)
    endif()
    if(ARGUMENTS_INSTALL_FOLDER)
        set(_CONANBUILDINFOFOLDER ${ARGUMENTS_INSTALL_FOLDER})
    else()
        set(_CONANBUILDINFOFOLDER ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    # Checks for the existence of conanbuildinfo.cmake, and loads it
    # important that it is macro, so variables defined at parent scope
    if(EXISTS "${_CONANBUILDINFOFOLDER}/${_CONANBUILDINFO}")
      message(STATUS "Conan: Loading ${_CONANBUILDINFO}")
      include(${_CONANBUILDINFOFOLDER}/${_CONANBUILDINFO})
    else()
      message(FATAL_ERROR "${_CONANBUILDINFO} doesn't exist in ${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endmacro()


macro(conan_cmake_run)
    parse_arguments(${ARGV})

    if(CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE AND NOT CONAN_EXPORTED
            AND NOT ARGUMENTS_BUILD_TYPE)
        set(CONAN_CMAKE_MULTI ON)
        message(STATUS "Conan: Using cmake-multi generator")
    else()
        set(CONAN_CMAKE_MULTI OFF)
    endif()

    if(NOT CONAN_EXPORTED)
        conan_cmake_setup_conanfile(${ARGV})
        if(CONAN_CMAKE_MULTI)
            foreach(CMAKE_BUILD_TYPE "Release" "Debug")
                set(ENV{CONAN_IMPORT_PATH} ${CMAKE_BUILD_TYPE})
                conan_cmake_settings(settings ${ARGV})
                conan_cmake_install(SETTINGS ${settings} ${ARGV})
            endforeach()
            set(CMAKE_BUILD_TYPE)
        else()
            conan_cmake_settings(settings ${ARGV})
            conan_cmake_install(SETTINGS ${settings} ${ARGV})
        endif()
    endif()

    if (NOT ARGUMENTS_NO_LOAD)
    	conan_load_buildinfo()
    endif()

    if(ARGUMENTS_BASIC_SETUP)
        foreach(_option CMAKE_TARGETS KEEP_RPATHS NO_OUTPUT_DIRS)
            if(ARGUMENTS_${_option})
                if(${_option} STREQUAL "CMAKE_TARGETS")
                    list(APPEND _setup_options "TARGETS")
                else()
                    list(APPEND _setup_options ${_option})
                endif()
            endif()
        endforeach()
        conan_basic_setup(${_setup_options})
    endif()
endmacro()

macro(conan_check)
    # Checks conan availability in PATH
    # Arguments REQUIRED and VERSION are optional
    # Example usage:
    #    conan_check(VERSION 1.0.0 REQUIRED)
    message(STATUS "Conan: checking conan executable in path")
    set(options REQUIRED)
    set(oneValueArgs VERSION)
    cmake_parse_arguments(CONAN "${options}" "${oneValueArgs}" "" ${ARGN})

    find_program(CONAN_CMD conan)
    if(NOT CONAN_CMD AND CONAN_REQUIRED)
        message(FATAL_ERROR "Conan executable not found!")
    endif()
    message(STATUS "Conan: Found program ${CONAN_CMD}")
    execute_process(COMMAND ${CONAN_CMD} --version
                    OUTPUT_VARIABLE CONAN_VERSION_OUTPUT
                    ERROR_VARIABLE CONAN_VERSION_OUTPUT)
    message(STATUS "Conan: Version found ${CONAN_VERSION_OUTPUT}")

    if(DEFINED CONAN_VERSION)
        string(REGEX MATCH ".*Conan version ([0-9]+\.[0-9]+\.[0-9]+)" FOO
            "${CONAN_VERSION_OUTPUT}")
        if(${CMAKE_MATCH_1} VERSION_LESS ${CONAN_VERSION})
            message(FATAL_ERROR "Conan outdated. Installed: ${CMAKE_MATCH_1}, \
                required: ${CONAN_VERSION}. Consider updating via 'pip \
                install conan==${CONAN_VERSION}'.")
        endif()
    endif()
endmacro()

macro(conan_add_remote)
    # Adds a remote
    # Arguments URL and NAME are required, INDEX is optional
    # Example usage:
    #    conan_add_remote(NAME bincrafters INDEX 1
    #       URL https://api.bintray.com/conan/bincrafters/public-conan)
    set(oneValueArgs URL NAME INDEX)
    cmake_parse_arguments(CONAN "" "${oneValueArgs}" "" ${ARGN})

    if(DEFINED CONAN_INDEX)
        set(CONAN_INDEX_ARG "-i ${CONAN_INDEX}")
    endif()

    message(STATUS "Conan: Adding ${CONAN_NAME} remote repository (${CONAN_URL})")
    execute_process(COMMAND ${CONAN_CMD} remote add ${CONAN_NAME} ${CONAN_URL}
      ${CONAN_INDEX_ARG} -f)
endmacro()