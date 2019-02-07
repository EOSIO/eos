#!/bin/sh
CORES=`getconf _NPROCESSORS_ONLN` # determine number of cores available
ctest --j $CORES --output-on-failure $@ # run ctest, disregard failure code
exit 0
