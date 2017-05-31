# -*- mode: cmake -*-

#
# Shamelessly stolen from MSTK who shamelessly stole from Amanzi open source code https://software.lanl.gov/ascem/trac)
#      
#      PARSE_LIBRARY_LIST( <lib_list>
#                         DEBUG   <out_debug_list>
#                         OPT     <out_opt_list>
#                         GENERAL <out_gen_list> )

# CMake module
include(CMakeParseArguments)

function(PARSE_LIBRARY_LIST)

    # Macro: _print_usage
    macro(_print_usage)
        message("PARSE_LIBRARY_LIST <lib_list>\n"
                "         FOUND   <out_flag>\n"
                "         DEBUG   <out_debug_list>\n"
                "         OPT     <out_opt_list>\n"
                "         GENERAL <out_gen_list>\n" 
                "lib_list string to parse\n"
                "FOUND    flag to indicate if keywords were found\n"
                "DEBUG    variable containing debug libraries\n"
                "OPT      variable containing optimized libraries\n" 
                "GENERAL  variable containing debug libraries\n")

    endmacro()     

    # Read in args
    cmake_parse_arguments(PARSE_ARGS "" "FOUND;DEBUG;OPT;GENERAL" "" ${ARGN}) 
    set(_parse_list "${PARSE_ARGS_UNPARSED_ARGUMENTS}")
    if ( (NOT PARSE_ARGS_FOUND) OR
         (NOT PARSE_ARGS_DEBUG)  OR
         (NOT PARSE_ARGS_OPT)  OR
         (NOT PARSE_ARGS_GENERAL) OR
         (NOT _parse_list )
       )  
        _print_usage()
        message(FATAL_ERROR "Invalid arguments")
    endif() 

    # Now split the list
    set(_debug_libs "") 
    set(_opt_libs "") 
    set(_gen_libs "") 
    foreach( item ${_parse_list} )
        if( ${item} MATCHES debug     OR 
            ${item} MATCHES optimized OR 
            ${item} MATCHES general )

            if( ${item} STREQUAL "debug" )
                set( mylist "_debug_libs" )
            elseif( ${item} STREQUAL "optimized" )
                set( mylist "_opt_libs" )
            elseif( ${item} STREQUAL "general" )
                set( mylist "_gen_libs" )
            endif()
        else()
            list( APPEND ${mylist} ${item} )
        endif()
    endforeach()


    # Now set output vairables
    set(${PARSE_ARGS_DEBUG}     "${_debug_libs}" PARENT_SCOPE)
    set(${PARSE_ARGS_OPT}       "${_opt_libs}"   PARENT_SCOPE)
    set(${PARSE_ARGS_GENERAL}   "${_gen_libs}"   PARENT_SCOPE)

    # If any of the lib lists are defined set flag to TRUE
    if ( (_debug_libs) OR (_opt_libs) OR (_gen_libs) )
        set(${PARSE_ARGS_FOUND} TRUE PARENT_SCOPE)
    else()
        set(${PARSE_ARGS_FOUND} FALSE PARENT_SCOPE)
    endif()    

endfunction(PARSE_LIBRARY_LIST)

