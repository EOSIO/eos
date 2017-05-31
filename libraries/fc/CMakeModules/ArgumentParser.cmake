# This module defines the ARGUMENT_PARSER macro for parsing macro arguments.

# ARGUMENT_PARSER Macro
# This macro parses a mixed list of arguments and headers into lists and boolean
# variables. The output lists and boolean variables are stored using
# tolower( header ) variable names. All non-header arguments will be added to
# the output list that corresponds to the header that they follow (or to the
# default list if no header has been parsed yet). If a boolean header is passed,
# then its corresponding output variable is set to YES.
#
# Usage:
#      ARGUMENT_PARSER( default_list lists bools ARGN )
#
# Parameters:
#      default_list    The name of the variable that list values should be added
#                      to before any list headers have been reached. You may
#                      pass "" to disregard premature list values.
#      lists           The list headers (semicolon-separated string).
#      bools           The boolean headers (semicolon-separated string).
#      ARGN            The arguments to parse.
MACRO( ARGUMENT_PARSER default_list lists bools )

    # Start using the default list.
    SET( dest "${default_list}" )
    IF( NOT dest )
        SET( dest tmp )
    ENDIF( NOT dest )

    # Clear all of the lists.
    FOREACH( list_itr ${lists} )
        STRING( TOLOWER ${list_itr} lower )
        SET( ${lower} "" )
    ENDFOREACH( list_itr )

    # Set all boolean variables to NO.
    FOREACH( bool_itr ${bools} )
        STRING( TOLOWER ${bool_itr} lower )
        SET( ${lower} NO )
    ENDFOREACH( bool_itr )

    # For all arguments.
    FOREACH( arg_itr ${ARGN} )

        SET( done NO )

        # For each of the list headers, if the current argument matches a list
        # header, then set the destination to the header.
        FOREACH( list_itr ${lists} )
            IF( ${arg_itr} STREQUAL ${list_itr} )
                STRING( TOLOWER ${arg_itr} lower )
                SET( dest ${lower} )
                SET( done YES )
            ENDIF( ${arg_itr} STREQUAL ${list_itr} )
        ENDFOREACH( list_itr )

        # For each of the boolean headers, if the current argument matches a
        # boolean header, then set the boolean variable to true.
        FOREACH( bool_itr ${bools} )
            IF( ${arg_itr} STREQUAL ${bool_itr} )
                STRING( TOLOWER ${arg_itr} lower )
                SET( ${lower} YES )
                SET( done YES )
            ENDIF( ${arg_itr} STREQUAL ${bool_itr} )
        ENDFOREACH( bool_itr )

        # If the current argument is not a header, then add it to the current
        # destination list.
        IF( NOT done )
            SET( ${dest} ${${dest}} ${arg_itr} )
        ENDIF( NOT done )

    ENDFOREACH( arg_itr )

ENDMACRO( ARGUMENT_PARSER )
