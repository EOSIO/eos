###############################################################################
# CMake module to search for ODB library
#
# WARNING: This module is experimental work in progress.
#
# This module defines:
#  ODB_INCLUDE_DIRS        = include dirs to be used when using the odb library
#  ODB_LIBRARY             = full path to the odb library
#  ODB_FOUND               = true if odb was found
#
# This module respects:
#  LIB_SUFFIX         = (64|32|"") Specifies the suffix for the lib directory
#
# Copyright (c) 2011 Michael Jansen <info@michael-jansen.biz>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
###############################################################################

# Override default CMAKE_FIND_LIBRARY_SUFFIXES
if (ODB_SHARED_LIB)
    SET(ODB_SUFFIX so)
else()
    SET(ODB_SUFFIX a)
endif()
MESSAGE(STATUS "Odb finding .${ODB_SUFFIX} library")

#
### Global Configuration Section
#
SET(_ODB_REQUIRED_VARS  ODB_INCLUDE_DIR ODB_LIBRARY)

#
### FIRST STEP: Find the odb headers.
#
FIND_PATH(
    ODB_INCLUDE_DIR version.hxx
    PATH "/usr/local"
    PATH_SUFFIXES "" "odb")
MARK_AS_ADVANCED(ODB_INCLUDE_DIR)

SET(ODB_INCLUDE_DIRS ${ODB_INCLUDE_DIR})

#
### SECOND STEP: Find the odb library. Respect LIB_SUFFIX
#
FIND_LIBRARY(
    ODB_LIBRARY
    NAMES odb.${ODB_SUFFIX} libodb.${ODB_SUFFIX}
    HINTS ${ODB_INCLUDE_DIR}/..
    PATH_SUFFIXES lib${LIB_SUFFIX})
MARK_AS_ADVANCED(ODB_LIBRARY)

FIND_LIBRARY(
        ODB_MYSQL_LIBRARY
        NAMES odb-mysql.${ODB_SUFFIX} libodb-mysql.${ODB_SUFFIX}
        HINTS ${ODB_INCLUDE_DIR}/..
        PATH_SUFFIXES lib${LIB_SUFFIX})
MARK_AS_ADVANCED(ODB_MYSQL_LIBRARY)

FIND_LIBRARY(
        ODB_BOOST_LIBRARY
        NAMES odb-boost.${ODB_SUFFIX} libodb-boost.${ODB_SUFFIX}
        HINTS ${ODB_INCLUDE_DIR}/..
        PATH_SUFFIXES lib${LIB_SUFFIX})
MARK_AS_ADVANCED(ODB_BOOST_LIBRARY)

FIND_LIBRARY(ODB_MYSQL_CLIENT_LIBRARY
        NAMES mysqlclient libmysqlclient
        PATHS /usr/lib64/mysql /usr/lib/mysql /usr/lib/x86_64-linux-gnu)

MESSAGE(STATUS "Odb found ${ODB_LIBRARY} ${ODB_MYSQL_LIBRARY} ${ODB_BOOST_LIBRARY} ${ODB_MYSQL_CLIENT_LIBRARY}")
SET(ODB_LIBRARY ${ODB_MYSQL_LIBRARY} ${ODB_BOOST_LIBRARY} ${ODB_LIBRARY} ${ODB_MYSQL_CLIENT_LIBRARY})

#
### ADHERE TO STANDARDS
#
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Odb DEFAULT_MSG ${_ODB_REQUIRED_VARS})
