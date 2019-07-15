#!/usr/bin/env bash
set -eo pipefail

function determine-hash() {
    # Determine the sha1 hash of all dockerfiles in the .cicd directory.
    [[ -z $1 ]] && echo "Please provide the files to be hashed (wildcards supported)" && exit 1
    echo "Obtaining Hash of files from $1..."
    # Collect all files, hash them, then hash those.
    HASHES=()
    for FILE in $(find $1 -type f); do
        HASH=$(sha1sum $FILE | sha1sum | awk '{ print $1 }')
        HASHES=($HASH "${HASHES[*]}")
        echo "$FILE - $HASH"
    done
    export DETERMINED_HASH=$(echo ${HASHES[*]} | sha1sum | awk '{ print $1 }')
    export HASHED_IMAGE_TAG="${IMAGE_TAG}-${DETERMINED_HASH}"
}

function build() {
    # Per distro additions to docker command
    [[ $IMAGE_TAG  == centos-7 ]] && PRE_COMMANDS="source /opt/rh/devtoolset-8/enable && source /opt/rh/rh-python36/enable &&"
    [[ $IMAGE_TAG == amazonlinux-2 ]] && CMAKE_EXTRAS="-DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang'" # explicitly set to clang as gcc is default
    [[ $IMAGE_TAG == ubuntu-16.04 ]] && CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_TOOLCHAIN_FILE='/tmp/pinned_toolchain.cmake' -DCMAKE_CXX_COMPILER_LAUNCHER=ccache" # pinned only
    [[ $IMAGE_TAG == amazonlinux-2 || $IMAGE_TAG == centos-7 ]] && EXPORTS="export PATH=/usr/lib64/ccache:$PATH &&" || EXPORTS="$EXPORTS export PATH=/usr/lib/ccache:$PATH &&" # ccache needs to come first in the list (devtoolset-8 overrides that if we include this in the Dockerfile)
    # DOCKER
    docker run --rm -v $(pwd):/eos -v /usr/lib/ccache -v $HOME/.ccache:/opt/.ccache -e CCACHE_DIR=/opt/.ccache eosio/producer:ci-${HASHED_IMAGE_TAG} bash -c " \
        $PRE_COMMANDS ccache -s && \
        mkdir /eos/build && cd /eos/build && $EXPORTS cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DOPENSSL_ROOT_DIR='/usr/include/openssl' -DBUILD_MONGO_DB_PLUGIN=true $CMAKE_EXTRAS /eos && make -j $(getconf _NPROCESSORS_ONLN) 
        ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test"
}

function generate_docker_image() {
    # If we cannot pull the image, we build and push it first.
    echo "$DOCKER_PASSWORD" | docker login -u "$DOCKER_USERNAME" --password-stdin
    cd ./.cicd
    docker build -t eosio/producer:ci-${HASHED_IMAGE_TAG} -f ./${IMAGE_TAG}.dockerfile .
    docker push eosio/producer:ci-${HASHED_IMAGE_TAG}
    cd -
}

determine-hash ".cicd/${IMAGE_TAG}.dockerfile"
[[ -z $DETERMINED_HASH ]] && echo "DETERMINED_HASH empty! (check determine-hash function)" && exit 1
echo "Looking for $IMAGE_TAG-$DETERMINED_HASH"
if docker pull eosio/producer:ci-${HASHED_IMAGE_TAG}; then
    echo "eosio/producer:ci-${HASHED_IMAGE_TAG} already exists"
else
    generate_docker_image
fi