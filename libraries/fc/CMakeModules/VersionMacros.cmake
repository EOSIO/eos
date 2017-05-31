# This module defines several macros that are useful for handling version
# information. These macros work for version strings of format "#.#.#"
# representing major, minor, and patch integer components.


INCLUDE( ArgumentParser )


# PARSE_VERSION_STR Macro
# This macro parses the version string information from a string. The macro
# parses the string for the given definitions followed by whitespace (or by ':'
# or '"' characters) and then version information. For example, passing
# "MyVersion" as a definition would properly retrieve the version from a string
# "containing the line "def MyVersion:    1.2.3".
#
# Usage:
#     PARSE_VERSION_STR( version string definition [definition2...] )
#
# Parameters:
#     version       The variable to store the version string in.
#     string        The string to parse.
#     definition    The definition(s) that may preceed the version string
#                   information.
MACRO( PARSE_VERSION_STR version string )

    # Parse the arguments into variables.
    ARGUMENT_PARSER( definitions "" "" ${ARGN} )

    # For each of the given definitions...
    FOREACH( def_itr ${definitions} )
        # If the version has not been found, then attempt to parse it.
        IF( NOT ${version} )
            # Parse the version string.
            STRING( REGEX MATCH "${def_itr}[ \t\":]+[0-9]+(.[0-9]+)?(.[0-9]+)?"
                    ${version} ${string} )

            STRING( REGEX MATCH "[0-9]+(.[0-9]+)?(.[0-9]+)?" ${version}
                    "${${version}}" )

            CORRECT_VERSION_STR( ${version} "${${version}}" )
        ENDIF( NOT ${version} )
    ENDFOREACH( def_itr )

ENDMACRO( PARSE_VERSION_STR )


# PARSE_VERSION_INT Macro
# This macro parses the version integer component information from a string. The
# macro parses the string for the given definitions followed by whitespace (or
# by ':' or '"' characters) and then version information. For example, passing
# "MyVersionMajor" as a definition would properly retrieve the version from a
# string "containing the line "def MyVersionMajor:    1".
#
# Usage:
#     PARSE_VERSION_INT( version string definition [definition2...] )
#
# Parameters:
#     version       The variable to store the version integer component in.
#     string        The string to parse.
#     definition    The definition(s) that may preceed the version integer
#                   component information.
MACRO( PARSE_VERSION_INT version string )

    # Parse the arguments into variables.
    ARGUMENT_PARSER( definitions "" "" ${ARGN} )

    # For each of the given definitions...
    FOREACH( def_itr ${definitions} )
        # If the version has not been found, then attempt to parse it.
        IF( NOT ${version} )
            # Parse the version string.
            STRING( REGEX MATCH "${def_itr}[ \t\":]+[0-9]+" ${version}
                    ${string} )

            STRING( REGEX MATCH "[0-9]+" ${version} "${${version}}" )
        ENDIF( NOT ${version} )
    ENDFOREACH( def_itr )

ENDMACRO( PARSE_VERSION_INT )


# VERSION_STR_TO_INTS Macro
# This macro converts a version string into its three integer components.
#
# Usage:
#     VERSION_STR_TO_INTS( major minor patch version )
#
# Parameters:
#     major      The variable to store the major integer component in.
#     minor      The variable to store the minor integer component in.
#     patch      The variable to store the patch integer component in.
#     version    The version string to convert ("#.#.#" format).
MACRO( VERSION_STR_TO_INTS major minor patch version )

    STRING( REGEX REPLACE "([0-9]+).[0-9]+.[0-9]+" "\\1" ${major} ${version} )
    STRING( REGEX REPLACE "[0-9]+.([0-9]+).[0-9]+" "\\1" ${minor} ${version} )
    STRING( REGEX REPLACE "[0-9]+.[0-9]+.([0-9]+)" "\\1" ${patch} ${version} )

ENDMACRO( VERSION_STR_TO_INTS )


# VERSION_INTS_TO_STR Macro
# This macro converts three version integer components into a version string.
#
# Usage:
#     VERSION_INTS_TO_STR( version major minor patch )
#
# Parameters:
#     version    The variable to store the version string in.
#     major      The major version integer.
#     minor      The minor version integer.
#     patch      The patch version integer.
MACRO( VERSION_INTS_TO_STR version major minor patch )

    SET( ${version} "${major}.${minor}.${patch}" )
    CORRECT_VERSION_STR( ${version} ${${version}} )

ENDMACRO( VERSION_INTS_TO_STR version major minor patch )


# COMPARE_VERSION_STR Macro
# This macro compares two version strings to each other. The macro sets the
# result variable to -1 if lhs < rhs, 0 if lhs == rhs, and 1 if lhs > rhs.
#
# Usage:
#     COMPARE_VERSION_STR( result lhs rhs )
#
# Parameters:
#     result    The variable to store the result of the comparison in.
#     lhs       The version of the left hand side ("#.#.#" format).
#     rhs       The version of the right hand side ("#.#.#" format).
MACRO( COMPARE_VERSION_STR result lhs rhs )

    VERSION_STR_TO_INTS( lhs_major lhs_minor lhs_patch ${lhs} )
    VERSION_STR_TO_INTS( rhs_major rhs_minor rhs_patch ${rhs} )

    COMPARE_VERSION_INTS( ${result}
                          ${lhs_major} ${lhs_minor} ${lhs_patch}
                          ${rhs_major} ${rhs_minor} ${rhs_patch} )

ENDMACRO( COMPARE_VERSION_STR result lhs rhs )


# COMPARE_VERSION_INTS Macro
# This macro compares two versions to each other using their integer components.
# The macro sets the result variable to -1 if lhs < rhs, 0 if lhs == rhs, and 1
# if lhs > rhs.
#
# Usage:
#     COMPARE_VERSION_INTS( result
#                           lhs_major lhs_minor lhs_patch
#                           rhs_major rhs_minor rhs_patch )
#
# Parameters:
#     result       The variable to store the result of the comparison in.
#     lhs_major    The major integer component for the left hand side.
#     lhs_minor    The minor integer component for the left hand side.
#     lhs_patch    The patch integer component for the left hand side.
#     rhs_major    The major integer component for the right hand side.
#     rhs_minor    The minor integer component for the right hand side.
#     rhs_patch    The patch integer component for the right hand side.
MACRO( COMPARE_VERSION_INTS result lhs_major lhs_minor lhs_patch
       rhs_major rhs_minor rhs_patch )

    SET( ${result} 0 )
    IF( NOT ${result} AND ${lhs_major} LESS ${rhs_major} )
        SET( ${result} -1 )
    ENDIF( NOT ${result} AND ${lhs_major} LESS ${rhs_major} )
    IF( NOT ${result} AND ${lhs_major} GREATER ${rhs_major} )
        SET( ${result} 1 )
    ENDIF( NOT ${result} AND ${lhs_major} GREATER ${rhs_major} )

    IF( NOT ${result} AND ${lhs_minor} LESS ${rhs_minor} )
        SET( ${result} -1 )
    ENDIF( NOT ${result} AND ${lhs_minor} LESS ${rhs_minor} )
    IF( NOT ${result} AND ${lhs_minor} GREATER ${rhs_minor} )
        SET( ${result} 1 )
    ENDIF( NOT ${result} AND ${lhs_minor} GREATER ${rhs_minor} )

    IF( NOT ${result} AND ${lhs_patch} LESS ${rhs_patch} )
        SET( ${result} -1 )
    ENDIF( NOT ${result} AND ${lhs_patch} LESS ${rhs_patch} )
    IF( NOT ${result} AND ${lhs_patch} GREATER ${rhs_patch} )
        SET( ${result} 1 )
    ENDIF( NOT ${result} AND ${lhs_patch} GREATER ${rhs_patch} )

ENDMACRO( COMPARE_VERSION_INTS result lhs_major lhs_minor lhs_patch
          rhs_major rhs_minor rhs_patch )


# CORRECT_VERSION_STR Macro
# This macro corrects the version_str and stores the result in the version
# variable. If the version_str contains a version string in "#" or "#.#" format,
# then ".0" will be appended to the string to convert it to "#.#.#" format. If
# the version_str is invalid, then version will be set to "".
#
# Usage:
#     CORRECT_VERSION_STR( version version_str )
#
# Parameters:
#     version        The variable to store the corrected version string in.
#     version_str    The version string to correct.
MACRO( CORRECT_VERSION_STR version version_str )

    SET( ${version} "${version_str}" )

    # Add ".0" to the end of the version string in case a full "#.#.#" string
    # was not given.
    FOREACH( itr RANGE 2 )
        IF( NOT ${version} MATCHES "[0-9]+.[0-9]+.[0-9]+" )
            SET( ${version} "${${version}}.0" )
        ENDIF( NOT ${version} MATCHES "[0-9]+.[0-9]+.[0-9]+" )
    ENDFOREACH( itr )

    # If the version string is not correct, then set it to "".
    IF( NOT ${version} MATCHES "^[0-9]+.[0-9]+.[0-9]+$" )
        SET( ${version} "" )
    ENDIF( NOT ${version} MATCHES "^[0-9]+.[0-9]+.[0-9]+$" )

ENDMACRO( CORRECT_VERSION_STR )



# CORRECT_VERSION_Int Macro
# This macro corrects the version_int and stores the result in the version
# variable. If the version_int is invalid, then version will be set to "".
#
# Usage:
#     CORRECT_VERSION_Int( version version_int )
#
# Parameters:
#     version        The variable to store the corrected version integer
#                    component in.
#     version_INT    The version integer component to correct.
MACRO( CORRECT_VERSION_INT version version_int )

    SET( ${version} "${version_int}" )

    # If the version is not an integer, then set it to "".
    IF( NOT ${version} MATCHES "^[0-9]+$" )
        SET( ${version} "" )
    ENDIF( NOT ${version} MATCHES "^[0-9]+$" )

ENDMACRO( CORRECT_VERSION_INT )
