#!/bin/bash
set -eo pipefail
[[ -z "$1" ]] && echo 'Please provide the file to be hashed as first argument.' && exit 1
FILE_NAME="$(basename "$1" | awk '{split($0,a,/\.(d|s)/); print a[1] }')"
export DETERMINED_HASH=$(sha1sum "$1" | awk '{ print $1 }')
export HASHED_IMAGE_TAG="eos-${FILE_NAME}-${DETERMINED_HASH}"
export FULL_TAG="${MIRROR_REGISTRY:-$DOCKERHUB_CI_REGISTRY}:$HASHED_IMAGE_TAG"

[[ "$RAW_PIPELINE_CONFIG" == '' ]] && export RAW_PIPELINE_CONFIG="$1"
[[ "$RAW_PIPELINE_CONFIG" == '' ]] && export RAW_PIPELINE_CONFIG='pipeline.jsonc'
[[ "$PIPELINE_CONFIG" == '' ]] && export PIPELINE_CONFIG='pipeline.json'
# read dependency file
if [[ -f "$RAW_PIPELINE_CONFIG" ]]; then
    echo 'Reading pipeline configuration file...'
    cat "$RAW_PIPELINE_CONFIG" | grep -Po '^[^"/]*("((?<=\\).|[^"])*"[^"/]*)*' | jq -c .\"eosio\" > "$PIPELINE_CONFIG"
    CDT_VERSION=$(cat "$PIPELINE_CONFIG" | jq -r '.dependencies."eosio.cdt"')
else
    echo 'ERROR: No pipeline configuration file or dependencies file found!'
    exit 1
fi

if [[ "$BUILDKITE" == 'true' ]]; then
    CDT_COMMIT=$((curl -s https://api.github.com/repos/EOSIO/eosio.cdt/git/refs/tags/$CDT_VERSION && curl -s https://api.github.com/repos/EOSIO/eosio.cdt/git/refs/heads/$CDT_VERSION) | jq '.object.sha' | sed "s/null//g" | sed "/^$/d" | tr -d '"' | sed -n '1p')
    test -z "$CDT_COMMIT" && CDT_COMMIT=$(echo $CDT_VERSION | tr -d '"' | tr -d "''" | cut -d ' ' -f 1) # if both searches returned nothing, the version is probably specified by commit hash already
fi

echo "Using cdt ${CDT_COMMIT} from \"$CDT_VERSION\"..."
export CDT_URL="https://eos-public-oss-binaries.s3-us-west-2.amazonaws.com/${CDT_COMMIT:0:7}-eosio.cdt-ubuntu-18.04_amd64.deb"
