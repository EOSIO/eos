#!/bin/bash
set -eo pipefail
# variables
. ./.cicd/helpers/general.sh
# tests
if [[ $(uname) == 'Darwin' ]]; then # macOS
    set +e # defer error handling to end
    source ~/.bash_profile && ./"$@"
    EXIT_STATUS=$?
else # Linux
    COMMANDS="$MOUNTED_DIR/$@"
    . $HELPERS_DIR/file-hash.sh $CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile
    echo "$ docker run --rm --init -v $(pwd):$MOUNTED_DIR $(buildkite-intrinsics) -e JOBS -e BUILDKITE_API_KEY $FULL_TAG bash -c \"$COMMANDS\""
    set +e # defer error handling to end
    eval docker run --rm --init -v $(pwd):$MOUNTED_DIR $(buildkite-intrinsics) -e JOBS -e BUILDKITE_API_KEY $FULL_TAG bash -c \"$COMMANDS\"
    EXIT_STATUS=$?
fi
# buildkite
if [[ "$BUILDKITE" == 'true' ]]; then
    cd build
    # upload artifacts
    echo '+++ :arrow_up: Uploading Artifacts'
    echo 'Compressing configuration'
    [[ -d etc ]] && tar czf etc.tar.gz etc
    echo 'Compressing logs'
    [[ -d var ]] && tar czf var.tar.gz var
    [[ -d eosio-ignition-wd ]] && tar czf eosio-ignition-wd.tar.gz eosio-ignition-wd
    echo 'Compressing core dumps...'
    [[ $((`ls -1 core.* 2>/dev/null | wc -l`)) != 0 ]] && tar czf core.tar.gz core.* || : # collect core dumps
    echo 'Exporting xUnit XML'
    mv -f ./Testing/$(ls ./Testing/ | grep '2' | tail -n 1)/Test.xml test-results.xml
    echo 'Uploading artifacts'
    [[ -f config.ini ]] && buildkite-agent artifact upload config.ini
    [[ -f core.tar.gz ]] && buildkite-agent artifact upload core.tar.gz
    [[ -f genesis.json ]] && buildkite-agent artifact upload genesis.json
    [[ -f etc.tar.gz ]] && buildkite-agent artifact upload etc.tar.gz
    [[ -f ctest-output.log ]] && buildkite-agent artifact upload ctest-output.log
    [[ -f var.tar.gz ]] && buildkite-agent artifact upload var.tar.gz
    [[ -f eosio-ignition-wd.tar.gz ]] && buildkite-agent artifact upload eosio-ignition-wd.tar.gz
    [[ -f bios_boot.sh ]] && buildkite-agent artifact upload bios_boot.sh
    buildkite-agent artifact upload test-results.xml
    echo 'Done uploading artifacts.'
fi
# re-throw
if [[ "$EXIT_STATUS" != 0 ]]; then
    echo "Failing due to non-zero exit status from ctest: $EXIT_STATUS"
    exit $EXIT_STATUS
fi