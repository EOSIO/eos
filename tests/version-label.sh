#!/bin/bash
set -eo pipefail
# The purpose of this test is to ensure that the output of the "nodeos --version" command matches the version string defined by our CMake files
echo '##### Nodeos Version Label Test #####'
# orient ourselves
[[ "$BUILD_ROOT" == '' ]] && BUILD_ROOT="$(pwd)"
echo "Using BUILD_ROOT=\"$BUILD_ROOT\"."
EXPECTED=v$1
if [[ -z "$EXPECTED" ]]; then
    echo "Missing version input."
    exit 1
fi
echo "Expecting \"$EXPECTED\"..."
# get nodeos version
ACTUAL=$($BUILD_ROOT/bin/nodeos --version)
EXIT_CODE=$?
# verify 0 exit code explicitly
if [[ $EXIT_CODE -ne 0 ]]; then
    echo "Nodeos produced non-zero exit code \"$EXIT_CODE\"."
    exit $EXIT_CODE
fi
# test version
if [[ "$EXPECTED" == "$ACTUAL" ]]; then
    echo "Passed with \"$ACTUAL\"."
    exit 0
fi
echo 'Failed!'
echo "\"$EXPECTED\" != \"$ACTUAL\""
exit 1
