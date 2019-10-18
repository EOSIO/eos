#!/bin/bash
set -eo pipefail
# variables
. ./.cicd/helpers/general.sh
# tests
if [[ $(uname) == 'Darwin' ]]; then # macOS
    [[ "$USE_CONAN" == 'true' ]] && cp -r $BUILD_DIR/conan ~/.conan
    export PATH=$PATH:~/mongodb/bin
    set +e # defer error handling to end
    ./"$@"
    EXIT_STATUS=$?
else # Linux
    COMMANDS="$MOUNTED_DIR/$@"
    if [[ $USE_CONAN == 'true' ]]; then
        sed -n '/## Environment/,/## Build/p' $CONAN_DIR/$IMAGE_TAG.md | grep -v -e '```' -e '\#\#' -e '^$' -e 'export' | sed -e 's/^/RUN /' >> $CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile
        COMMANDS="ln -s $MOUNTED_DIR/build/conan ~/.conan && $COMMANDS"    
    fi
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
    echo 'Exporting xUnit XML'
    mv -f ./Testing/$(ls ./Testing/ | grep '2' | tail -n 1)/Test.xml test-results.xml
    echo 'Uploading artifacts'
    [[ -f config.ini ]] && buildkite-agent artifact upload config.ini
    [[ -f core.tar.gz ]] && buildkite-agent artifact upload core.tar.gz
    [[ -f genesis.json ]] && buildkite-agent artifact upload genesis.json
    [[ -f mongod.log ]] && buildkite-agent artifact upload mongod.log
    buildkite-agent artifact upload test-results.xml
    echo 'Done uploading artifacts.'
fi
# re-throw
if [[ "$EXIT_STATUS" != 0 ]]; then
    echo "Failing due to non-zero exit status from ctest: $EXIT_STATUS"
    exit $EXIT_STATUS
fi