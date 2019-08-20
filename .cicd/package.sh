#!/usr/bin/env bash
set -eo pipefail
. ./.cicd/helpers/general.sh

mkdir -p $BUILD_DIR

PRE_COMMANDS="cd $MOUNTED_DIR"
PACKAGE_COMMANDS="./.cicd/package-builder.sh"
COMMANDS="$PRE_COMMANDS && $PACKAGE_COMMNADS"

if [[ $(uname) == 'Darwin' ]]; then

    # You can't use chained commands in execute
    bash -c "$PACKAGE_COMMANDS"
    
else # Linux

    ARGS=${ARGS:-"--rm --init -v $(pwd):$MOUNTED_DIR"}

    . $HELPERS_DIR/docker-hash.sh

    # Load BUILDKITE Environment Variables for use in docker run
    if [[ -f $BUILDKITE_ENV_FILE ]]; then
        evars=""
        while read -r var; do
            evars="$evars --env ${var%%=*}"
        done < "$BUILDKITE_ENV_FILE"
    fi
    echo "docker run $ARGS $evars $FULL_TAG bash -c \\\"$COMMANDS\\\""
    eval docker run $ARGS $evars $FULL_TAG bash -c \"$COMMANDS\"

fi