#!/bin/bash

set -euo pipefail

# buildkite-agent artifact download someversion.deb .

buildkite-agent artifact download '*.deb' --step ':ubuntu: Ubuntu 18.04 - Package Builder' .
echo ":done: download successfull"

SANITIZED_BRANCH=$(echo "$BUILDKITE_BRANCH" | sed 's.^/..' | sed 's/[:/]/_/g')
SANITIZED_TAG=$(echo "$BUILDKITE_TAG" | sed 's.^/..' | tr '/' '_')
echo "$SANITIZED_BRANCH"
echo "$SANITIZED_TAG"

# do docker build
echo ":docker::build: Building image..."
DOCKERHUB_EOS_REGISTRY="docker.io/eosio/eos"

if [[ ! -z "$SANITIZED_TAG" ]]; then
    IMAGE_ECR_C="$EOSIO_REGISTRY:$BUILDKITE_COMMIT"
    IMAGE_ECR_B="$EOSIO_REGISTRY:$SANITIZED_BRANCH"
    IMAGE_DOCKER_C="$DOCKERHUB_EOS_REGISTRY:$BUILDKITE_COMMIT"
    IMAGE_DOCKER_B="$DOCKERHUB_EOS_REGISTRY:$SANITIZED_BRANCH"
    DOCKER_BUILD_COMMAND="docker build -t $IMAGE_ECR_C -t $IMAGE_DOCKER_C -t $IMAGE_ECR_B -t $IMAGE_DOCKER_B -f ./docker/dockerfile ."
    echo "Building Image...."
    echo "$ $DOCKER_BUILD_COMMAND"
    eval $DOCKER_BUILD_COMMAND
    echo "Build Done successfully!!"
        if [[ ! -z "$BUILDKITE_TAG" && "$SANITIZED_BRANCH" != "$SANITIZED_TAG" ]]; then
            IMAGE_ECR_T="$EOSIO_REGISTRY:$SANITIZED_TAG"
            IMAGE_DOCKER_T="$DOCKERHUB_EOS_REGISTRY:$SANITIZED_TAG"
            DOCKER_TAG_ECR="docker tag '$IMAGE_ECR_C' '$IMAGE_ECR_T"
            DOCKER_TAG_HUB="docker tag '$IMAGE_DOCKER_C '$IMAGE_DOCKER_T"
fi

#do docker push to ECR
DOCKER_PUSH_ECR_C="docker push $IMAGE_ECR_C"
echo "Pushing Image to ECR..."
echo "$ $DOCKER_PUSH_ECR_C"
eval $DOCKER_PUSH_ECR_C
echo "Done!"

DOCKER_PUSH_ECR_B="docker push $IMAGE_ECR_B"
echo "Pushing Image to ECR..."
echo "$ $DOCKER_PUSH_ECR_B"
eval $DOCKER_PUSH_ECR_B
echo "Done!"

# do docker push Dockerhub
DOCKER_PUSH_HUB_C="docker push $IMAGE_DOCKER_C"
echo "Pushing Image DockerHub EOSIO/EOS ..."
echo "$ $DOCKER_PUSH_HUB_C"
eval $DOCKER_PUSH_HUB_C
echo "Done!"

DOCKER_PUSH_HUB_B="docker push $IMAGE_DOCKER_B"
echo "Pushing Image DockerHub EOSIO/EOS ..."
echo "$ $DOCKER_PUSH_HUB_B"
eval $DOCKER_PUSH_HUB_B
echo "Done!"

if [[ ! -z "$BUILDKITE_TAG" && "$SANITIZED_BRANCH" != "$SANITIZED_TAG" ]]; then
    DOCKER_PUSH_ECR_T="docker push $IMAGE_ECR_T"
    echo "Pushing Image to ECR..."
    echo "$ $DOCKER_PUSH_ECR_T"
    eval $DOCKER_PUSH_ECR_T
    echo "Done!"
    DOCKER_PUSH_HUB_T="docker push $IMAGE_DOCKER_T"
    echo "Pushing Image DockerHub EOSIO/EOS ..."
    echo "$ $DOCKER_PUSH_HUB_T"
    eval $DOCKER_PUSH_HUB_T
    echo "Done!"
fi

#do clean up image
DOCKER_RMI="docker rmi ${IMAGE_ECR_C} ${IMAGE_ECR_B} ${IMAGE_ECR_T} ${IMAGE_DOCKER_C} ${IMAGE_DOCKER_B} ${IMAGE_DOCKER_T}"
echo "Cleaning up \n${IMAGE_ECR_C} \n${IMAGE_ECR_B} \n${IMAGE_ECR_T} \n${IMAGE_DOCKER_C} \n${IMAGE_DOCKER_B} \n${IMAGE_DOCKER_T}"
echo "$ $DOCKER_RMI"
eval $DOCKER_RMI
echo "Cleaned"