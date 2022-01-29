#!/bin/bash
set -euo pipefail
[[ -z "$1" ]] && echo 'Please provide the file to be hashed as first argument.' && exit 1
FILE_NAME="$(basename "$1" | awk '{split($0,a,/\.(d|s)/); print a[1] }')"
export DETERMINED_HASH=$(sha1sum "$1" | awk '{ print $1 }')
export HASHED_IMAGE_TAG="eos-${FILE_NAME}-${DETERMINED_HASH}"
if [[ -n "${REGISTRY_BASE:-''}" ]] ; then
  export FULL_TAG="$REGISTRY_BASE:$HASHED_IMAGE_TAG"
fi
set +u
