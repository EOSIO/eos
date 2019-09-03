#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
. $HELPERS_DIR/file-hash.sh $CICD_DIR/platforms/${IMAGE_TAG}.dockerfile
# look for Docker image
echo "+++ :mag_right: Looking for $FULL_TAG"
ORG_REPO=$(echo $FULL_TAG | cut -d: -f1)
TAG=$(echo $FULL_TAG | cut -d: -f2)
EXISTS=$(curl -s -H "Authorization: Bearer $(curl -sSL "https://auth.docker.io/token?service=registry.docker.io&scope=repository:${ORG_REPO}:pull" | jq --raw-output .token)" "https://registry.hub.docker.com/v2/${ORG_REPO}/manifests/$TAG")
# build, if neccessary
if [[ $EXISTS =~ '404 page not found' || $EXISTS =~ 'manifest unknown' ]]; then # if we cannot pull the image, we build and push it first
    docker build -t $FULL_TAG -f $CICD_DIR/platforms/${IMAGE_TAG}.dockerfile .
    docker push $FULL_TAG
else
    echo "$FULL_TAG already exists."
fi
