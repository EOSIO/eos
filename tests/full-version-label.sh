#!/bin/bash
set -eo pipefail
# The purpose of this test is to ensure that the output of the "nodeos --full-version" command matches the version string defined by our CMake files
echo '##### Nodeos Full Version Label Test #####'
# orient ourselves
[[ -z "$BUILD_ROOT" ]] && export BUILD_ROOT="$(pwd)"
echo "Using BUILD_ROOT=\"$BUILD_ROOT\"."
[[ -z "$CMAKE_SOURCE_DIR" ]] && export CMAKE_SOURCE_DIR="$2"
# test expectations
if [[ -z "$EXPECTED" ]]; then
    [[ -z "$BUILDKITE_COMMIT" ]] && export BUILDKITE_COMMIT="$(pushd "$CMAKE_SOURCE_DIR" &>/dev/null && git rev-parse HEAD 2>/dev/null ; popd &>/dev/null)"
    [[ -z "$BUILDKITE_TAG" ]] && export BUILDKITE_TAG="${GIT_TAG:-$1}"
    export EXPECTED="$BUILDKITE_TAG-$BUILDKITE_COMMIT"
fi
if [[ -z "$EXPECTED" ]]; then
    echo "Missing version input."
    exit 1
fi
echo "Expecting \"$EXPECTED\"..."
# get nodeos version
ACTUAL=$($BUILD_ROOT/bin/nodeos --full-version)
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
