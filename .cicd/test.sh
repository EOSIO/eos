#!/bin/bash
set -eo pipefail
# variables
. ./.cicd/helpers/general.sh
# tests
if [[ $(uname) == 'Darwin' ]]; then # macOS
    export PATH=$PATH:~/mongodb/bin
    set +e # defer error handling to end
    source ~/.bash_profile && ./"$@"
    EXIT_STATUS=$?
else # Linux
    COMMANDS="$MOUNTED_DIR/$@"
    . $HELPERS_DIR/file-hash.sh $CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile
    echo "$ docker run --rm --init -v $(pwd):$MOUNTED_DIR $(buildkite-intrinsics) -e JOBS $FULL_TAG bash -c \"$COMMANDS\""
    set +e # defer error handling to end
    eval docker run --rm --init -v $(pwd):$MOUNTED_DIR $(buildkite-intrinsics) -e JOBS $FULL_TAG bash -c \"$COMMANDS\"
    EXIT_STATUS=$?
fi
# buildkite
if [[ "$BUILDKITE" == 'true' ]]; then
    cd build
    # upload artifacts
    echo '+++ :arrow_up: Uploading Artifacts'
    echo 'Compressing core dumps...'
    [[ $((`ls -1 core.* 2>/dev/null | wc -l`)) != 0 ]] && tar czf core.tar.gz core.* || : # collect core dumps
    echo 'Compressing ls-tests logs...'
    [[ $((`ls -1 data-dir/cluster*/node*/std*.txt 2>/dev/null | wc -l`)) != 0 ]] && tar czf ls_tests_logs.tar.gz data-dir/cluster*/node*/std*.txt data-dir/cluster*/node*/config.ini
    [[ $((`ls -1 *debug*.log 2>/dev/null | wc -l`)) != 0 ]] && tar czf debug_logs.tar.gz *debug*.log
    echo 'Exporting xUnit XML'
    mv -f ./Testing/$(ls ./Testing/ | grep '2' | tail -n 1)/Test.xml test-results.xml
    echo 'Uploading artifacts'
    [[ -f config.ini ]] && buildkite-agent artifact upload config.ini
    [[ -f core.tar.gz ]] && buildkite-agent artifact upload core.tar.gz
    [[ -f genesis.json ]] && buildkite-agent artifact upload genesis.json
    [[ -f mongod.log ]] && buildkite-agent artifact upload mongod.log
    [[ -f launcher_service.log ]] && buildkite-agent artifact upload launcher_service.log
    [[ -f ls_tests_logs.tar.gz ]] && buildkite-agent artifact upload ls_tests_logs.tar.gz
    [[ -f debug_logs.tar.gz ]] && buildkite-agent artifact upload debug_logs.tar.gz
    buildkite-agent artifact upload test-results.xml
    echo 'Done uploading artifacts.'
fi
# re-throw
if [[ "$EXIT_STATUS" != 0 ]]; then
    echo "Failing due to non-zero exit status from ctest: $EXIT_STATUS"
    exit $EXIT_STATUS
fi
exit $EXIT_STATUS
