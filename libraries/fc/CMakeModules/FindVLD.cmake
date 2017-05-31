# Module for locating Visual Leak Detector.
#
# Customizable variables:
#   VLD_ROOT_DIR
#     This variable points to the Visual Leak Detector root directory. By
#     default, the module looks for the installation directory by examining the
#     Program Files/Program Files (x86) folders and the VLDROOT environment
#     variable.
#
# Read-only variables:
#   VLD_FOUND
#     Indicates that the library has been found.
#
#   VLD_INCLUDE_DIRS
#     Points to the Visual Leak Detector include directory.
#
#   VLD_LIBRARY_DIRS
#     Points to the Visual Leak Detector directory that contains the libraries.
#     The content of this variable can be passed to link_directories.
#
#   VLD_LIBRARIES
#     Points to the Visual Leak Detector libraries that can be passed to
#     target_link_libararies.
#
#
# Copyright (c) 2012 Sergiu Dotenco
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

INCLUDE (FindPackageHandleStandardArgs)

SET (_VLD_POSSIBLE_LIB_SUFFIXES lib)

# Version 2.0 uses vld_x86 and vld_x64 instead of simply vld as library names
IF (CMAKE_SIZEOF_VOID_P EQUAL 4)
  LIST (APPEND _VLD_POSSIBLE_LIB_SUFFIXES lib/Win32)
ELSEIF (CMAKE_SIZEOF_VOID_P EQUAL 8)
  LIST (APPEND _VLD_POSSIBLE_LIB_SUFFIXES lib/Win64)
ENDIF (CMAKE_SIZEOF_VOID_P EQUAL 4)

FIND_PATH (VLD_ROOT_DIR
  NAMES include/vld.h
  PATHS ENV VLDROOT
        "$ENV{PROGRAMFILES}/Visual Leak Detector"
        "$ENV{PROGRAMFILES(X86)}/Visual Leak Detector"
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Visual Leak Detector;InstallLocation]"
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Visual Leak Detector;InstallLocation]"
  DOC "VLD root directory")

FIND_PATH (VLD_INCLUDE_DIR
  NAMES vld.h
  HINTS ${VLD_ROOT_DIR}
  PATH_SUFFIXES include
  DOC "VLD include directory")

FIND_LIBRARY (VLD_LIBRARY_DEBUG
  NAMES vld
  HINTS ${VLD_ROOT_DIR}
  PATH_SUFFIXES ${_VLD_POSSIBLE_LIB_SUFFIXES}
  DOC "VLD debug library")

IF (VLD_ROOT_DIR)
  SET (_VLD_VERSION_FILE ${VLD_ROOT_DIR}/CHANGES.txt)

  IF (EXISTS ${_VLD_VERSION_FILE})
    SET (_VLD_VERSION_REGEX
      "Visual Leak Detector \\(VLD\\) Version (([0-9]+)\\.([0-9]+)([a-z]|(.([0-9]+)))?)")
    FILE (STRINGS ${_VLD_VERSION_FILE} _VLD_VERSION_TMP REGEX
      ${_VLD_VERSION_REGEX})

    STRING (REGEX REPLACE ${_VLD_VERSION_REGEX} "\\1" _VLD_VERSION_TMP
      "${_VLD_VERSION_TMP}")

    STRING (REGEX REPLACE "([0-9]+).([0-9]+).*" "\\1" VLD_VERSION_MAJOR
      "${_VLD_VERSION_TMP}")
    STRING (REGEX REPLACE "([0-9]+).([0-9]+).*" "\\2" VLD_VERSION_MINOR
      "${_VLD_VERSION_TMP}")

    SET (VLD_VERSION ${VLD_VERSION_MAJOR}.${VLD_VERSION_MINOR})

    IF ("${_VLD_VERSION_TMP}" MATCHES "^([0-9]+).([0-9]+).([0-9]+)$")
      # major.minor.patch version numbering scheme
      STRING (REGEX REPLACE "([0-9]+).([0-9]+).([0-9]+)" "\\3"
        VLD_VERSION_PATCH "${_VLD_VERSION_TMP}")
      SET (VLD_VERSION "${VLD_VERSION}.${VLD_VERSION_PATCH}")
      SET (VLD_VERSION_COUNT 3)
    ELSE ("${_VLD_VERSION_TMP}" MATCHES "^([0-9]+).([0-9]+).([0-9]+)$")
      # major.minor version numbering scheme. The trailing letter is ignored.
      SET (VLD_VERSION_COUNT 2)
    ENDIF ("${_VLD_VERSION_TMP}" MATCHES "^([0-9]+).([0-9]+).([0-9]+)$")
  ENDIF (EXISTS ${_VLD_VERSION_FILE})
ENDIF (VLD_ROOT_DIR)

IF (VLD_LIBRARY_DEBUG)
  SET (VLD_LIBRARY debug ${VLD_LIBRARY_DEBUG} CACHE DOC "VLD library")
  GET_FILENAME_COMPONENT (_VLD_LIBRARY_DIR ${VLD_LIBRARY_DEBUG} PATH)
  SET (VLD_LIBRARY_DIR ${_VLD_LIBRARY_DIR} CACHE PATH "VLD library directory")
ENDIF (VLD_LIBRARY_DEBUG)

SET (VLD_INCLUDE_DIRS ${VLD_INCLUDE_DIR})
SET (VLD_LIBRARY_DIRS ${VLD_LIBRARY_DIR})
SET (VLD_LIBRARIES ${VLD_LIBRARY})

MARK_AS_ADVANCED (VLD_INCLUDE_DIR VLD_LIBRARY_DIR VLD_LIBRARY_DEBUG VLD_LIBRARY)

FIND_PACKAGE_HANDLE_STANDARD_ARGS (VLD REQUIRED_VARS VLD_ROOT_DIR
  VLD_INCLUDE_DIR VLD_LIBRARY VERSION_VAR VLD_VERSION)
