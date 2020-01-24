#!/bin/bash
set -eo pipefail
[[ $BUILDKITE_LABEL == ':pipeline: Pipeline Upload' ]] && exit 0
echo "[Preparing Submodules]"
git submodule foreach --recursive "git clean -ffxdq"
if [[ $BUILDKITE_PIPELINE_SLUG =~ 'eosio-security' ]]; then
    CWD=$(pwd)
    oIFS=$IFS
    IFS=$'\n'
    echo "$SUBMODULES"
    for SUBMOD in $SUBMODULES; do
        SUBMOD_COMMIT=$(echo $SUBMOD | awk '{print $1}' | sed 's/+//g' | sed 's/-//g')
        SUBMOD_PATH=$(echo $SUBMOD | awk '{print $2}')
        SUBMOD_NAME=$(basename $SUBMOD_PATH)
        echo "Preparing $SUBMOD_NAME with commit $SUBMOD_COMMIT..."
        SECURITY_REMOTE_URL="git@github.com:EOSIO/$SUBMOD_NAME-security.git"
        cd $CWD/$SUBMOD_PATH
        git remote add security $SECURITY_REMOTE_URL
        git pull security || true
    done
    IFS=$oIFS
    cd $CWD
fi
git submodule update --init --recursive --force # ensure we obtain submodule submodules