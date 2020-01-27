#!/bin/bash
set -eo pipefail
[[ $BUILDKITE_LABEL == ':pipeline: Pipeline Upload' ]] && exit 0 # Don't do anything if the step is Pipeline Upload
echo "[Preparing Submodules]"
if [[ $BUILDKITE_PIPELINE_SLUG =~ 'eosio-security' ]]; then
    SUBMODULES=$(git submodule)
    CWD=$(pwd)
    oIFS=$IFS
    IFS=$'\n'
    echo "$SUBMODULES"
    for SUBMOD in $SUBMODULES; do
        SUBMOD_COMMIT=$(echo $SUBMOD | awk '{print $1}' | sed 's/+//g' | sed 's/-//g')
        SUBMOD_PATH=$(echo $SUBMOD | awk '{print $2}')
        SUBMOD_NAME=$(basename $SUBMOD_PATH)
        SECURITY_REMOTE_URL="git@github.com:EOSIO/$SUBMOD_NAME-security.git"
        echo "Preparing $SUBMOD_NAME [$CWD/$SUBMOD_PATH] with security remote $SECURITY_REMOTE_URL"
        cd $CWD/$SUBMOD_PATH
        git remote add security $SECURITY_REMOTE_URL
        git pull security || true
    done
    IFS=$oIFS
    cd $CWD
else
    echo "Skipped security remote addition for $BUILDKITE_PIPELINE_SLUG..."
fi
git submodule update --init --recursive --force # ensure we obtain submodule submodules