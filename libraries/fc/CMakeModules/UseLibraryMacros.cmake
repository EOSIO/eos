# This module defines macros that are useful for using libraries in a build. The
# macros in this module are typically used along with the FindDependencyMacros.

# ADD_LIBRARY_TO_LIST Macro
# Adds a library to a list of libraries if it is found. Otherwise, reports an
# error.
#
# Usage:
#     ADD_LIBRARY_TO_LIST( libraries found lib lib_name )
#
# Parameters:
#     libraries    The list of libraries to add the library to.
#     found        Whether or not the library to add was found.
#     lib          The library to add to the list.
#     lib_name     The name of the library to add to the list.
MACRO( ADD_LIBRARY_TO_LIST libraries found lib lib_name )

    # Setting found to itself is necessary for the conditional to work.
    SET( found ${found} )

    # IF found, then add the library to the list, else report an error.
    IF( found )
        LIST( REMOVE_ITEM ${libraries} ${lib} )
        SET( ${libraries} ${${libraries}} ${lib} )
    ENDIF( found )
    IF( NOT found )
        MESSAGE( "Using ${lib_name} failed." )
    ENDIF( NOT found )

ENDMACRO( ADD_LIBRARY_TO_LIST )


# USE_LIBRARY_GLOBALS Macro
# If ${prefix}_USE_${LIB} is true, then ${prefix}_${LIB}_LIBRARY will be added
# to ${prefix}_LIBRARIES (assuming the library was correctly found). All of the
# dependencies will also be added to ${prefix}_LIBRARIES.
#
# Usage:
#     USE_LIBRARY_GLOBALS( prefix lib
#                          DEPS dependency1 [dependency2...] )
#
# Parameters:
#     prefix    The prefix for the global variables.
#     lib       The library to try to use.
#     DEPS      Follow with the list of dependencies that should be added with
#               the given library.
MACRO( USE_LIBRARY_GLOBALS prefix lib )

    STRING( TOUPPER ${lib} upper )

    # If the library should be used...
    IF( ${prefix}_USE_${upper} )

        # Parse the arguments into variables.
        ARGUMENT_PARSER( "" "DEPS" "" ${ARGN} )

        # Add the library to the list.
        ADD_LIBRARY_TO_LIST( ${prefix}_LIBRARIES "${${prefix}_${upper}_FOUND}"
                             "${${prefix}_${upper}_LIBRARY}" ${lib} )

        # For each of the library's dependencies.
        FOREACH( dep_itr ${deps} )
            STRING( TOUPPER ${dep_itr} upper )

            # Add the dependency to the list.
            ADD_LIBRARY_TO_LIST( ${prefix}_LIBRARIES
                                 "${${prefix}_${upper}_FOUND}"
                                 "${${prefix}_${upper}_LIBRARY}" ${dep_itr} )
        ENDFOREACH( dep_itr )
    ENDIF( ${prefix}_USE_${upper} )

ENDMACRO( USE_LIBRARY_GLOBALS )
