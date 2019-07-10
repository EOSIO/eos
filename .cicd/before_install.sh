#!/bin/bash

echo "os: $TRAVIS_OS_NAME"
echo "docker image: $DOCKER_IMAGE"

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
  #todo: native build
  echo "building for mac"
else
  echo "building in docker image $DOCKER_IMAGE"
  docker pull $DOCKER_IMAGE
  #todo: mount source volume 
  docker run -itd --name builder $DOCKER_IMAGE
  docker exec builder 'echo "ps aux"'
fi

docker pull $DOCKER_IMAGE