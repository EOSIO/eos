#!/usr/bin/env bash
set -eo pipefail
. ./.cicd/helpers/general.sh

mkdir -p $BUILD_DIR

PRE_COMMANDS="cd $MOUNTED_DIR"
PACKAGE_COMMANDS="./.cicd/package-builder.sh"
COMMANDS="$PRE_COMMANDS && $PACKAGE_COMMANDS"

if [[ $(uname) == 'Darwin' ]]; then

    # You can't use chained commands in execute
    bash -c "$PACKAGE_COMMANDS"

    ARTIFACT='*.rb;*.tar.gz'
    cd build/packages
    [[ -d x86_64 ]] && cd 'x86_64' # backwards-compatibility with release/1.6.x
    buildkite-agent artifact upload "./$ARTIFACT" --agent-access-token $BUILDKITE_AGENT_ACCESS_TOKEN
    for A in $(echo $ARTIFACT | tr ';' ' '); do
        if [[ $(ls $A | grep -c '') == 0 ]]; then
            echo "+++ :no_entry: ERROR: Expected artifact \"$A\" not found!"
            pwd
            ls -la
            exit 1
        fi
    done

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

    eval docker run $ARGS $evars $FULL_TAG bash -c \"$COMMANDS\"

    if [[ "$IMAGE_TAG" =~ "ubuntu" ]]; then
        echo 'Uploading DEB.'
        ARTIFACT='*.deb'
    elif [[ "$IMAGE_TAG" =~ "centos" ]]; then
        echo 'Uploading RPM'
        ARTIFACT='*.rpm'
    fi
    cd build/packages
    [[ -d x86_64 ]] && cd 'x86_64' # backwards-compatibility with release/1.6.x
    buildkite-agent artifact upload "./$ARTIFACT" --agent-access-token $BUILDKITE_AGENT_ACCESS_TOKEN
    for A in $(echo $ARTIFACT | tr ';' ' '); do
        if [[ $(ls $A | grep -c '') == 0 ]]; then
            echo "+++ :no_entry: ERROR: Expected artifact \"$A\" not found!"
            pwd
            ls -la
            exit 1
        fi
    done

fi