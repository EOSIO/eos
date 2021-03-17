#!/bin/bash
set -eo pipefail
# The purpose of this test is to ensure that the output of the "nodeos --print-build-info" command.
# This includes verifying valid output in JSON shape and checking parameters (only boost for now).
echo '##### Nodeos Print Build Info Test #####'
# orient ourselves
[[ "$EOSIO_ROOT" == '' ]] && EOSIO_ROOT=$(echo $(pwd)/ | grep -ioe '.*/eos/')
[[ "$EOSIO_ROOT" == '' ]] && EOSIO_ROOT=$(echo $(pwd)/ | grep -ioe '.*/EOSIO/eosio/')
[[ "$EOSIO_ROOT" == '' ]] && EOSIO_ROOT=$(echo $(pwd)/ | grep -ioe '.*/build/' | sed 's,/build/,,')
echo "Using EOSIO_ROOT=\"$EOSIO_ROOT\"."

exec 9>&1 # enable tee to write to STDOUT as a file
PRINT_BUILD_INFO="$EOSIO_ROOT/build/bin/nodeos --print-build-info 2>&1 | tee >(cat - >&9) || :"
echo "$ $PRINT_BUILD_INFO"
OUTPUT="$(eval $PRINT_BUILD_INFO)"

OUTPUT=$(echo "$OUTPUT" | tr -d '\r\n')
OUTPUT=$(echo "$OUTPUT" | sed -E 's/^.+JSON://')
OUTPUT=$(echo "$OUTPUT" | sed -E 's/}.+$/}/')

JQ_OUTPUT=$(echo "$OUTPUT" | jq type)
EXIT_CODE=$?
if [[ "$EXIT_CODE" -ne 0 ]]; then
    echo "Not valid JSON type."
    exit $EXIT_CODE
fi

V_ARCH=$(echo "$OUTPUT" | jq '.arch')
echo "ARCH: $V_ARCH"
V_BOOST=$(echo "$OUTPUT" | jq '.boost_version')
echo "BOOST_VERSION: $V_BOOST"
V_COMPILER=$(echo "$OUTPUT" | jq '.compiler')
echo "COMPILER: $V_COMPILER"
V_DEBUG=$(echo "$OUTPUT" | jq '.debug')
echo "DEBUG: $V_DEBUG"
V_OS=$(echo "$OUTPUT" | jq '.os')
echo "OS: $V_OS"

if [[ -z "$V_ARCH" || -z "$V_BOOST" || -z "$V_COMPILER" || -z "$V_DEBUG" || -z "$V_OS" ]]; then
    echo "Missing expected build info key(s)."
    exit 1
fi

if [[ "$PLATFORM_TYPE" == "pinned" ]]; then
    if [[ -z "$IMAGE_TAG" ]]; then
        echo "Missing IMAGE_TAG variable."
        exit 1
    fi
    FILE=$(ls $EOSIO_ROOT/.cicd/platforms/pinned/$IMAGE_TAG* | head)
    BOOST=$(cat $FILE | grep boost | tr -d '\r\n' | sed -E 's/^.+boost_([0-9]+_[0-9]+_[0-9]+).+$/\1/' | head)
    BOOST_MAJOR=$(echo $BOOST | sed -E 's/^([0-9])+_[0-9]+_[0-9]+$/\1/')
    BOOST_MINOR=$(echo $BOOST | sed -E 's/^[0-9]+_([0-9]+)_[0-9]+$/\1/')
    BOOST_PATCH=$(echo $BOOST | sed -E 's/^[0-9]+_[0-9]+_([0-9])+$/\1/')
    E_BOOST=$(printf "%d%03d%02d" $BOOST_MAJOR $BOOST_MINOR $BOOST_PATCH)

    echo "Verifying boost version: \"$E_BOOST\" == \"$V_BOOST\"."
    if [[ "$E_BOOST" != "$V_BOOST" ]]; then
        echo "Expected boost version \"$E_BOOST\" does not match actual \"$V_BOOST\"."
        exit 1
    fi
fi

echo "Validation of build info complete."
exit 0
