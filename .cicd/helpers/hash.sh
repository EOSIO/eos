#!/bin/bash
set -eo pipefail
# determine the sha1 hash of all dockerfiles in the .cicd directory
[[ -z $1 ]] && echo "Please provide the files to be hashed (wildcards supported)" && exit 1
FILE_NAME=$(basename $1 | awk '{split($0,a,/\.(d|s)/); print a[1] }')
# collect all files, hash each, then get a hash of hashes
HASHES=()
for FILE in $(find $1 -type f); do
    HASH=$(sha1sum $FILE | sha1sum | awk '{ print $1 }')
    HASHES=($HASH "${HASHES[*]}")
    #echo "$FILE - $HASH"
done
export DETERMINED_HASH=$(echo ${HASHES[*]} | sha1sum | awk '{ print $1 }')
export HASHED_IMAGE_TAG="eos-${FILE_NAME}-${DETERMINED_HASH}"
export FULL_TAG="eosio/producer:$HASHED_IMAGE_TAG"
