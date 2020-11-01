#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
. $HELPERS_DIR/file-hash.sh $CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile
# look for Docker image
echo "+++ :mag_right: Looking for $FULL_TAG"
DOCKER_CLI_EXPERIMENTAL='enabled'
ORG_REPO=$(echo $FULL_TAG | cut -d: -f1)
TAG=$(echo $FULL_TAG | cut -d: -f2)
MANIFEST_COMMAND="docker manifest inspect '$ORG_REPO:$TAG'"
echo "$ $MANIFEST_COMMAND"
eval $MANIFEST_COMMAND
EXISTS="$?"
# build, if neccessary
if [[ "$EXISTS" != '0' || "$FORCE_BASE_IMAGE" == 'true' ]]; then # if we cannot pull the image, we build and push it first
    DOCKER_BUILD_COMMAND="docker build --no-cache -t '$FULL_TAG' -f '$CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile' ."
    echo "$ $DOCKER_BUILD_COMMAND"
    eval $DOCKER_BUILD_COMMAND
    if [[ $FORCE_BASE_IMAGE != true ]]; then
        DOCKER_PUSH_COMMAND="docker push '$FULL_TAG'"
        echo "$ $DOCKER_PUSH_COMMAND"
        eval $DOCKER_PUSH_COMMAND
    else
        echo "Base image creation successful. Not pushing...".
        exit 0
    fi
else
    echo "$FULL_TAG already exists."
fi