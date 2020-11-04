#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
. $HELPERS_DIR/file-hash.sh $CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile

echo "+++ :mag_right: Looking for $HASHED_IMAGE_TAG"
for REGISTRY in "${CI_REGISTRIES[@]}"; do
  if [[ ! -z $REGISTRY ]]; then
    if [[ ! $(docker manifest inspect "$REGISTRY:$HASHED_IMAGE_TAG") ]]; then
      EXISTS='false'
    fi
  fi
done

# build, if neccessary
if [[ "$EXISTS" == 'false' || "$FORCE_BASE_IMAGE" == 'true' ]]; then # if we cannot pull the image, we build and push it first
  export DOCKER_BUILD_COMMAND="docker build --no-cache -t 'eosio/ci:$HASHED_IMAGE_TAG' -f '$CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile' ."
  echo "$ $DOCKER_BUILD_COMMAND"
  eval $DOCKER_BUILD_COMMAND
  if [[ $FORCE_BASE_IMAGE != true ]]; then
    for REGISTRY in "${CI_REGISTRIES[@]}"; do
      if [[ ! -z $REGISTRY ]]; then
        docker tag eosio/ci:$HASHED_IMAGE_TAG $REGISTRY:$HASHED_IMAGE_TAG
        docker push $REGISTRY:$HASHED_IMAGE_TAG
        docker rmi $REGISTRY:$HASHED_IMAGE_TAG
      fi
    done
  else
    echo "Base image creation successful. Not pushing...".
    exit 0
  fi
else
  echo "$FULL_TAG already exists."
fi