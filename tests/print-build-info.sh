#!/bin/bash
# The purpose of this test is to ensure that the output of the "nodeos --print-build-info" command matches the version string defined by our CMake files
echo '##### Nodeos Version Label Test #####'
# orient ourselves
[[ "$EOSIO_ROOT" == '' ]] && EOSIO_ROOT=$(echo $(pwd)/ | grep -ioe '.*/eos/')
[[ "$EOSIO_ROOT" == '' ]] && EOSIO_ROOT=$(echo $(pwd)/ | grep -ioe '.*/EOSIO/eosio/')
[[ "$EOSIO_ROOT" == '' ]] && EOSIO_ROOT=$(echo $(pwd)/ | grep -ioe '.*/build/' | sed 's,/build/,,')
echo "Using EOSIO_ROOT=\"$EOSIO_ROOT\"."
# determine expected value
BUILD_VARS="$EOSIO_ROOT/scripts/.build_vars"
if [[ -f "$BUILD_VARS" ]]; then
    echo "Parsing \"$BUILD_VARS\"..."
    $(cat $BUILD_VARS | grep -ie 'export BOOST_VERSION_MAJOR' | cut -d '(' -f 2 | cut -d ')' -f 1 | awk '{print $1" "$2}')
    $(cat $BUILD_VARS | grep -ie 'export BOOST_VERSION_MINOR' | cut -d '(' -f 2 | cut -d ')' -f 1 | awk '{print $1" "$2}')
    $(cat $BUILD_VARS | grep -ie 'export BOOST_VERSION_PATCH' | cut -d '(' -f 2 | cut -d ')' -f 1 | awk '{print $1" "$2}')
    EXPECTED_BOOST_VERSION=$(printf "%d%03d%02d" $BOOST_VERSION_MAJOR $BOOST_VERSION_MINOR $BOOST_VERSION_PATCH)
else
    echo 'No build vars available.'
    exit 1
fi
# fail if no expected value was found
if [[ -z "$BOOST_VERSION_MAJOR" || -z "$BOOST_VERSION_MINOR" || -z "$BOOST_VERSION_PATCH" ]]; then
    echo 'ERROR: Could not determine expected value for version label!'
    set +e
    echo "EOSIO_ROOT=\"$EOSIO_ROOT\""
    echo "BUILD_VARS=\"$BUILD_VARS\""
    echo ''
    echo "BOOST_VERSION_MAJOR=\"$BOOST_VERSION_MAJOR\""
    echo "BOOST_VERSION_MINOR=\"$BOOST_VERSION_MINOR\""
    echo "BOOST_VERSION_PATCH=\"$BOOST_VERSION_PATCH\""
    echo "EXPECTED_BOOST_VERSION=\"$EXPECTED_BOOST_VERSION\""
    echo ''
    echo '$ pwd'
    pwd
    echo '$ ls -la "$EOSIO_ROOT"'
    ls -la "$EOSIO_ROOT"
    echo '$ ls -la "$EOSIO_ROOT/build"'
    ls -la "$EOSIO_ROOT/build"
    exit 1
fi

OUTPUT=$($EOSIO_ROOT/build/bin/nodeos --print-build-info 2>&1)
if [[ $? -eq 0 ]]; then
    echo 'Expected non-zero nodeos exit code.'
    exit 1
fi

JSON=$(echo $OUTPUT | tr -d '\r\n' | sed 's/^.\+ JSON: \({ .\+ }\).\+$/\1/')
ACTUAL_BOOST_VERSION=$(echo $JSON | sed 's/^.\+"boost_version": \([0-9]\+\).\+$/\1/')

echo "Expecting boost version \"$EXPECTED_BOOST_VERSION\"..."
if [[ "$EXPECTED_BOOST_VERSION" == "$ACTUAL_BOOST_VERSION" ]]; then
    echo "Passed with \"$ACTUAL_BOOST_VERSION\"."
    exit 0;
fi

echo 'Failed!'
echo "\"$EXPECTED_BOOST_VERSION\" != \"$ACTUAL_BOOST_VERSION\""
exit 1
