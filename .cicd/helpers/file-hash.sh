#!/bin/bash
set -eo pipefail
[[ -z $1 ]] && echo "Please provide the file to be hashed" && exit 1
FILE_NAME=$(basename $1 | awk '{split($0,a,/\.(d|s)/); print a[1] }')
export DETERMINED_HASH=$(sha1sum $1 | awk '{ print $1 }')
( [[ $FILE_NAME =~ 'macos' ]] && [[ $PINNED == false || $UNPINNED == true ]] ) && FILE_NAME="${FILE_NAME}-unpinned"
export HASHED_IMAGE_TAG="eos-${FILE_NAME}-${DETERMINED_HASH}"
export FULL_TAG="eosio/producer:$HASHED_IMAGE_TAG"