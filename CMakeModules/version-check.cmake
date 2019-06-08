file(WRITE ${CMAKE_BINARY_DIR}/compilerid.cpp "\
#include <boost/version.hpp>\n\
#include <iostream>\n\
\
int main() {\
	std::cout << BOOST_VERSION << \"   \" << __VERSION__;\
	return 0;\
}")


try_run(run_result_var compile_result_var ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/compilerid.cpp LINK_LIBRARIES Boost::boost RUN_OUTPUT_VARIABLE testoutput)

if(testoutput AND DEFINED LAST_COMPILE AND NOT "${LAST_COMPILE}" STREQUAL "${testoutput}")
   message(FATAL_ERROR "\
The compiler and boost versions have changed since last compile. A new nodeos built now will \
not be compatible with state files from the old nodeos. You would have to do a snapshot or replay.
  Old versions: ${LAST_COMPILE}
  New versions: ${testoutput}
To continue,
  If using cmake directly, add to the cmake command line:
    -ULAST_COMPILE
  
  If using the eosio_build.sh build script:
    pass the argument '-u' to the script
  ")
endif()

set(LAST_COMPILE "${testoutput}" CACHE STRING "Last compiler version definition" FORCE)
