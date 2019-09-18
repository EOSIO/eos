#!/bin/bash
set -eo pipefail
[[ -z $1 ]] && echo "Please provide at least one file to be hashed!" && exit 1
HASHES=()
for file in "$@"; do
    HASHES+=$(sha1sum $file | awk '{ print $1 }')
done
# Collect platforms file you passed in to use for FILE_NAME
for file in "$@"; do
    [[ $file =~ 'platforms' ]] && export FILE_NAME=$(basename $file | awk '{split($0,a,/\.(d|s)/); print a[1] }')
done
export DETERMINED_HASH=$(echo -n ${HASHES[@]} | sha1sum | awk '{ print $1 }')
( [[ $FILE_NAME =~ 'macos' ]] && [[ $PINNED == false || $UNPINNED == true ]] ) && FILE_NAME="${FILE_NAME}-unpinned"
export HASHED_IMAGE_TAG="eos-${FILE_NAME}-${DETERMINED_HASH}"
export FULL_TAG="eosio/producer:$HASHED_IMAGE_TAG"