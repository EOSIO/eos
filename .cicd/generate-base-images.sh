#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
. "$HELPERS_DIR/file-hash.sh" "$CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile"
# search for base image in docker registries
echo '--- :docker: Build or Pull Base Image :minidisc:'
echo "Looking for '$HASHED_IMAGE_TAG' container in our registries."
export EXISTS_DOCKER_HUB='false'
export EXISTS_MIRROR='false'
MANIFEST_COMMAND="docker manifest inspect '${MIRROR_REGISTRY:-$DOCKERHUB_CI_REGISTRY}:$HASHED_IMAGE_TAG'"
echo "$ $MANIFEST_COMMAND"
set +e
eval $MANIFEST_COMMAND
MANIFEST_INSPECT_EXIT_STATUS="$?"
set -eo pipefail
if [[ "$MANIFEST_INSPECT_EXIT_STATUS" == '0' ]]; then
    if [[ "$(echo "$REGISTRY" | grep -icP 'docker[.]io/')" != '0' ]]; then
        export EXISTS_DOCKER_HUB='true'
    else
        export EXISTS_MIRROR='true'
    fi
fi
# pull and copy as-necessary
if [[ "$EXISTS_MIRROR" == 'true' && ! -z "$MIRROR_REGISTRY" ]]; then
    DOCKER_PULL_COMMAND="docker pull '$MIRROR_REGISTRY:$HASHED_IMAGE_TAG'"
    echo "$ $DOCKER_PULL_COMMAND"
    eval $DOCKER_PULL_COMMAND
    # copy, if necessary
    if [[ "$EXISTS_DOCKER_HUB" == 'false' ]]; then
        # tag
        DOCKER_TAG_COMMAND="docker tag '$MIRROR_REGISTRY:$HASHED_IMAGE_TAG' '$DOCKERHUB_CI_REGISTRY:$HASHED_IMAGE_TAG'"
        echo "$ $DOCKER_TAG_COMMAND"
        eval $DOCKER_TAG_COMMAND
        # push
        DOCKER_PUSH_COMMAND="docker push '$DOCKERHUB_CI_REGISTRY:$HASHED_IMAGE_TAG'"
        echo "$ $DOCKER_PUSH_COMMAND"
        eval $DOCKER_PUSH_COMMAND
        export EXISTS_DOCKER_HUB='true'
    fi
elif [[ "$EXISTS_DOCKER_HUB" == 'true' ]]; then
    DOCKER_PULL_COMMAND="docker pull '$DOCKERHUB_CI_REGISTRY:$HASHED_IMAGE_TAG'"
    echo "$ $DOCKER_PULL_COMMAND"
    eval $DOCKER_PULL_COMMAND
    # copy, if necessary
    if [[ "$EXISTS_MIRROR" == 'false' && ! -z "$MIRROR_REGISTRY" ]]; then
        # tag
        DOCKER_TAG_COMMAND="docker tag '$DOCKERHUB_CI_REGISTRY:$HASHED_IMAGE_TAG' '$MIRROR_REGISTRY:$HASHED_IMAGE_TAG'"
        echo "$ $DOCKER_TAG_COMMAND"
        eval $DOCKER_TAG_COMMAND
        # push
        DOCKER_PUSH_COMMAND="docker push '$MIRROR_REGISTRY:$HASHED_IMAGE_TAG'"
        echo "$ $DOCKER_PUSH_COMMAND"
        eval $DOCKER_PUSH_COMMAND
        export EXISTS_MIRROR='true'
    fi
fi
# esplain yerself
if [[ "$EXISTS_DOCKER_HUB" == 'false' && "$EXISTS_MIRROR" == 'false' ]]; then
    echo 'Building base image from scratch.'
elif [[ "$OVERWRITE_BASE_IMAGE" == 'true' ]]; then
    echo "OVERWRITE_BASE_IMAGE is set to 'true', building from scratch and pushing to docker registries."
elif [[ "$FORCE_BASE_IMAGE" == 'true' ]]; then
    echo "FORCE_BASE_IMAGE is set to 'true', building from scratch and NOT pushing to docker registries."
fi
# build, if neccessary
if [[ ("$EXISTS_DOCKER_HUB" == 'false' && "$EXISTS_MIRROR" == 'false') || "$FORCE_BASE_IMAGE" == 'true' || "$OVERWRITE_BASE_IMAGE" == 'true' ]]; then # if we cannot pull the image, we build and push it first
    export DOCKER_BUILD_COMMAND="docker build --no-cache -t 'ci:$HASHED_IMAGE_TAG' -f '$CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile' ."
    echo "$ $DOCKER_BUILD_COMMAND"
    eval $DOCKER_BUILD_COMMAND
    if [[ "$FORCE_BASE_IMAGE" != 'true' || "$OVERWRITE_BASE_IMAGE" == 'true' ]]; then
        for REGISTRY in ${CI_REGISTRIES[*]}; do
            if [[ ! -z "$REGISTRY" ]]; then
                # tag
                DOCKER_TAG_COMMAND="docker tag 'ci:$HASHED_IMAGE_TAG' '$REGISTRY:$HASHED_IMAGE_TAG'"
                echo "$ $DOCKER_TAG_COMMAND"
                eval $DOCKER_TAG_COMMAND
                # push
                DOCKER_PUSH_COMMAND="docker push '$REGISTRY:$HASHED_IMAGE_TAG'"
                echo "$ $DOCKER_PUSH_COMMAND"
                eval $DOCKER_PUSH_COMMAND
                # clean up
                if  [[ "$FULL_TAG" != "$REGISTRY:$HASHED_IMAGE_TAG" ]]; then
                    DOCKER_RMI_COMMAND="docker rmi '$REGISTRY:$HASHED_IMAGE_TAG'"
                    echo "$ $DOCKER_RMI_COMMAND"
                    eval $DOCKER_RMI_COMMAND
                fi
            fi
        done
        DOCKER_RMI_COMMAND="docker rmi 'ci:$HASHED_IMAGE_TAG'"
        echo "$ $DOCKER_RMI_COMMAND"
        eval $DOCKER_RMI_COMMAND
    else
        echo "Base image creation successful. Not pushing...".
        exit 0
    fi
else
    echo "$FULL_TAG already exists."
fi
