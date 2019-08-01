#!/usr/bin/env bash
set -eo pipefail
cd $( dirname "${BASH_SOURCE[0]}" ) # Ensure we're in the .cicd dir
. ./.helpers
( [[ -z $IMAGE_TAG ]] && [[ -z $1 ]] ) && echo "You must provide the distro IMAGE_TAG name (example: ubuntu-18.04) as argument \$1 or set it within your ENV" && exit 1
echo "Looking for $FULL_TAG"
docker_tag_exists $FULL_TAG && echo "$FULL_TAG already exists" || generate_docker_image
