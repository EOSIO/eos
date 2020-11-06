#!/bin/bash
set -eo pipefail # exit on failure of any "simple" command (excludes &&, ||, or | chains)
# variables
GIT_ROOT="$(dirname $BASH_SOURCE[0])/.."
cd "$GIT_ROOT"
echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':evergreen_tree: ')Configuring Environment"
[[ "$PIPELINE_CONFIG" == '' ]] && export PIPELINE_CONFIG='pipeline.json'
[[ "$RAW_PIPELINE_CONFIG" == '' ]] && export RAW_PIPELINE_CONFIG='pipeline.jsonc'
[[ ! -d "$GIT_ROOT/eos_multiversion_builder" ]] && mkdir "$GIT_ROOT/eos_multiversion_builder"
# pipeline config
echo 'Reading pipeline configuration file...'
[[ -f "$RAW_PIPELINE_CONFIG" ]] && cat "$RAW_PIPELINE_CONFIG" | grep -Po '^[^"/]*("((?<=\\).|[^"])*"[^"/]*)*' | jq -c .\"eos-multiversion-tests\" > "$PIPELINE_CONFIG"
if [[ -f "$PIPELINE_CONFIG" ]]; then
    [[ "$DEBUG" == 'true' ]] && cat "$PIPELINE_CONFIG" | jq .
    # export environment
    if [[ "$(cat "$PIPELINE_CONFIG" | jq -r '.environment')" != 'null' ]]; then
        for OBJECT in $(cat "$PIPELINE_CONFIG" | jq -r '.environment | to_entries | .[] | @base64'); do
            KEY="$(echo $OBJECT | base64 --decode | jq -r .key)"
            VALUE="$(echo $OBJECT | base64 --decode | jq -r .value)"
            [[ ! -v $KEY ]] && export $KEY="$VALUE"
        done
    fi
    # export multiversion.conf
    echo '[eosio]' > multiversion.conf
    for OBJECT in $(cat "$PIPELINE_CONFIG" | jq -r '.configuration | .[] | @base64'); do
        echo "$(echo $OBJECT | base64 --decode)" >> multiversion.conf # outer echo adds '\n'
    done
    mv -f "$GIT_ROOT/multiversion.conf" "$GIT_ROOT/tests"
elif [[ "$DEBUG" == 'true' ]]; then
    echo 'Pipeline configuration file not found!'
    echo "PIPELINE_CONFIG = \"$PIPELINE_CONFIG\""
    echo "RAW_PIPELINE_CONFIG = \"$RAW_PIPELINE_CONFIG\""
    echo '$ pwd'
    pwd
    echo '$ ls'
    ls
    echo 'Skipping that step...'
fi
# multiversion
cd "$GIT_ROOT/eos_multiversion_builder"
echo 'Downloading other versions of nodeos...'
DOWNLOAD_COMMAND="python2.7 '$GIT_ROOT/.cicd/helpers/multi_eos_docker.py'"
echo "$ $DOWNLOAD_COMMAND"
eval $DOWNLOAD_COMMAND
cd "$GIT_ROOT"
cp "$GIT_ROOT/tests/multiversion_paths.conf" "$GIT_ROOT/build/tests"
cd "$GIT_ROOT/build"
# count tests
echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':microscope: ')Running Multiversion Test"
TEST_COUNT=$(ctest -N -L mixed_version_tests | grep -i 'Total Tests: ' | cut -d ':' -f 2 | awk '{print $1}')
if [[ $TEST_COUNT > 0 ]]; then
    echo "$TEST_COUNT tests found."
else
    echo "+++ $([[ "$BUILDKITE" == 'true' ]] && echo ':no_entry: ')ERROR: No tests registered with ctest! Exiting..."
    exit 1
fi
# run tests
set +e # defer ctest error handling to end
TEST_COMMAND='ctest -L mixed_version_tests --output-on-failure -T Test'
echo "$ $TEST_COMMAND"
eval $TEST_COMMAND
EXIT_STATUS=$?
echo 'Done running multiversion test.'
exit $EXIT_STATUS
