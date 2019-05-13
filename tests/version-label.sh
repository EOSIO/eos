#!/bin/bash
set -e # exit on failure of any "simple" command (excludes &&, ||, or | chains)
# The purpose of this test is to ensure that the output of the "nodeos --version" command matches the 
# If the environment variable BUILDKITE_TAG is empty or unset, this test will echo success
echo '##### Nodeos Version Label Test #####'
if [[ "$BUILDKITE_TAG" == '' || "$BUILDKITE" != 'true' ]]; then
    echo 'This test is only run in Buildkite against tagged builds.'
    [[ "$BUILDKITE" != 'true' ]] && echo 'This is not Buildkite.'
    [[ "$BUILDKITE_TAG" == '' ]] && echo 'This is not a tagged build.'
    echo 'Exiting...'
    exit 0
fi
echo 'Tagged build detected, running test.'
# orient ourselves
[[ "$EOSIO_ROOT" == '' ]] && EOSIO_ROOT=$(echo $(pwd)/ | grep -ioe '.*/eos/')
[[ "$EOSIO_ROOT" == '' ]] && EOSIO_ROOT=$(echo $(pwd)/ | grep -ioe '.*/EOSIO/eosio/')
[[ "$EOSIO_ROOT" == '' ]] && EOSIO_ROOT=$(echo $(pwd)/ | grep -ioe '.*/build/' | sed 's,/build/,,')
# determine expected value
CMAKE_CACHE="$EOSIO_ROOT/build/CMakeCache.txt"
CMAKE_LISTS="$EOSIO_ROOT/CMakeLists.txt"
if [[ -f "$CMAKE_CACHE" && $(cat "$CMAKE_CACHE" | grep -c 'DOXY_EOS_VERSION') > 0 ]]; then
    EXPECTED=$(cat "$CMAKE_CACHE" | grep 'DOXY_EOS_VERSION' | cut -d '=' -f 2)
elif [[ -f "$CMAKE_LISTS" ]]; then
    export $(cat $CMAKE_LISTS | grep -ie 'set *( *VERSION_MAJOR' | cut -d '(' -f 2 | cut -d ')' -f 1 | awk '{print $1"="$2}')
    export $(cat $CMAKE_LISTS | grep -ie 'set *( *VERSION_MINOR' | cut -d '(' -f 2 | cut -d ')' -f 1 | awk '{print $1"="$2}')
    export $(cat $CMAKE_LISTS | grep -ie 'set *( *VERSION_PATCH' | cut -d '(' -f 2 | cut -d ')' -f 1 | awk '{print $1"="$2}')
    if [[ $(cat $CMAKE_LISTS | grep -ice 'set *( *VERSION_SUFFIX') > 0 ]]; then
        export $(cat $CMAKE_LISTS | grep -ie 'set *( *VERSION_SUFFIX' | cut -d '(' -f 2 | cut -d ')' -f 1 | awk '{print $1"="$2}')
        export $(echo "$(cat $CMAKE_LISTS | grep -ie 'set *( *VERSION_FULL.*VERSION_SUFFIX' | cut -d '(' -f 2 | cut -d ')' -f 1 | awk '{print $1"="$2}')" | envsubst | tr -d '"')
    else
        export $(echo "$(cat $CMAKE_LISTS | grep -ie 'set *( *VERSION_FULL' | grep -ive 'VERSION_SUFFIX' | cut -d '(' -f 2 | cut -d ')' -f 1 | awk '{print $1"="$2}')" | envsubst | tr -d '"')
    fi
    EXPECTED="v$VERSION_FULL"
fi
# fail if no expected value was found
if [[ "$EXPECTED" == '' ]]; then
    echo 'ERROR: Could not determine expected value for version label!'
    set +e
    echo "EOSIO_ROOT=\"$EOSIO_ROOT\""
    echo "CMAKE_CACHE=\"$CMAKE_CACHE\""
    echo "CMAKE_LISTS=\"$CMAKE_LISTS\""
    echo ''
    echo "VERSION_MAJOR=\"$VERSION_MAJOR\""
    echo "VERSION_MINOR=\"$VERSION_MINOR\""
    echo "VERSION_PATCH=\"$VERSION_PATCH\""
    echo "VERSION_SUFFIX=\"$VERSION_SUFFIX\""
    echo "VERSION_FULL=\"$VERSION_FULL\""
    echo ''
    echo '$ cat "$CMAKE_CACHE" | grep "DOXY_EOS_VERSION"'
    cat "$CMAKE_CACHE" | grep "DOXY_EOS_VERSION"
    echo '$ pwd'
    pwd
    echo '$ ls "$EOSIO_ROOT"'
    ls "$EOSIO_ROOT"
    echo '$ ls "$EOSIO_ROOT/build"'
    ls "$EOSIO_ROOT/build"
    exit 1
fi
# test nodeos version
ACTUAL=$($EOSIO_ROOT/build/bin/nodeos --version) || : # nodeos currently returns -1 for --version
# check
echo "Expecting \"$EXPECTED\"..."
if [[ "$EXPECTED" == "$ACTUAL" ]]; then
    echo 'Passed with \"$ACTUAL\".'
    exit 0
fi
echo 'Failed!'
echo "\"$EXPECTED\" != \"$ACTUAL\""
exit 1