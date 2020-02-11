#!/bin/bash
set -eo pipefail
[[ $BUILDKITE_LABEL == ':pipeline: Pipeline Upload' ]] && exit 0 # Don't do anything if the step is Pipeline Upload
echo "[Preparing Submodules]"
git submodule update --init --recursive --force 1>/dev/null || true # We need to get the .git folder in the submodule dir so we can set the remote for security and the || true avoids it failing the script
if [[ $BUILDKITE_PIPELINE_SLUG =~ '-security' ]] || [[ ${FORCE_SUBMODULE_REMOTE_PREP:-false} == true ]]; then
    SUBMODULES=$(git submodule)
    CWD=$(pwd)
    oIFS=$IFS
    IFS=$'\n'
    echo "$SUBMODULES"
    for SUBMOD in $SUBMODULES; do
        SUBMOD_COMMIT=$(echo $SUBMOD | awk '{print $1}' | sed 's/+//g' | sed 's/-//g')
        SUBMOD_PATH=$(echo $SUBMOD | awk '{print $2}')
        SUBMOD_NAME=$(basename $SUBMOD_PATH)
        REMOTE_URL="git@github.com:EOSIO/$SUBMOD_NAME-security.git"
        echo "Preparing $SUBMOD_NAME [$CWD/$SUBMOD_PATH] with security remote $REMOTE_URL"
        cd $CWD/$SUBMOD_PATH
        git remote add remote $REMOTE_URL || true
        git pull remote || true
    done
    IFS=$oIFS
    cd $CWD
else
    echo "Skipped remote url addition for $BUILDKITE_PIPELINE_SLUG..."
fi
git submodule update --init --recursive --force # ensure we obtain submodule submodules