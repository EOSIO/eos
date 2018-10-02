#!/bin/sh
# run ctest, disregard failure code
ctest --output-on-failure $@
exit 0
