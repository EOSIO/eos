#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
echo '+++ Build Script Started'
export DOCKERIZATION=false
[[ $ENABLE_INSTALL == true ]] && . ./.cicd/helpers/populate-template-and-hash.sh '<!-- DAC ENV' '<!-- DAC CLONE' '<!-- DAC BUILD' '<!-- DAC INSTALL' || . ./.cicd/helpers/populate-template-and-hash.sh '<!-- DAC ENV' '<!-- DAC CLONE' '<!-- DAC BUILD'
sed -i -e 's/git clone https:\/\/github.com\/EOSIO\/eos\.git.*/cp -rfp $(pwd) \$EOS_LOCATION \&\& cd \$EOS_LOCATION/g' /tmp/$POPULATED_FILE_NAME # We don't need to clone twice
if [[ "$(uname)" == 'Darwin' ]]; then
    # You can't use chained commands in execute
    if [[ $TRAVIS == true ]]; then
        ccache -s
        brew link --overwrite md5sha1sum
        brew link --overwrite python
        brew reinstall openssl@1.1 # Fixes issue where builds in Travis cannot find libcrypto.
        sed -i -e 's/^cmake /cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache /g' /tmp/$POPULATED_FILE_NAME
        sed -i -e 's/ -DCMAKE_TOOLCHAIN_FILE=$EOS_LOCATION\/scripts\/pinned_toolchain_dac.cmake//g' /tmp/$POPULATED_FILE_NAME # We can't use pinned for mac cause building clang8 would take too long
        sed -i -e 's/ -DBUILD_MONGO_DB_PLUGIN=true -DENABLE_MULTIVERSION_PROTOCOL_TEST=true//g' /tmp/$POPULATED_FILE_NAME # We can't use pinned for mac cause building clang8 would take too long
        # Support ship_test
        export NVM_DIR="$HOME/.nvm"
        . "/usr/local/opt/nvm/nvm.sh"
        nvm install --lts=dubnium
    else
        source ~/.bash_profile # Make sure node is available for ship_test
    fi
    . $HELPERS_DIR/populate-template-and-hash.sh -h # obtain $FULL_TAG (and don't overwrite existing file)
    cat /tmp/$POPULATED_FILE_NAME
    . /tmp/$POPULATED_FILE_NAME # This file is populated from the platform's build documentation code block
else # Linux
    ARGS=${ARGS:-"--rm --init -v $(pwd):$(pwd) $(buildkite-intrinsics) -e JOBS"} # We must mount $(pwd) in as itself to avoid https://stackoverflow.com/questions/31381322/docker-in-docker-cannot-mount-volume
    if [[ $TRAVIS == true ]]; then
        ARGS="$ARGS -v /usr/lib/ccache -v $HOME/.ccache:/opt/.ccache -e TRAVIS -e CCACHE_DIR=/opt/.ccache"
        [[ ! $IMAGE_TAG =~ 'unpinned' ]] && sed -i -e 's/^cmake /cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache /g' /tmp/$POPULATED_FILE_NAME
        if [[ $IMAGE_TAG == 'amazon_linux-2-pinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'centos-7.7-pinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'ubuntu-16.04-pinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'ubuntu-18.04-pinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'amazon_linux-2-unpinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'centos-7.7-unpinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'ubuntu-18.04-unpinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib/ccache:\\\$PATH"
        fi
        BUILD_COMMANDS="ccache -s && $PRE_COMMANDS &&"
    fi
    echo "cp -rfp \$EOS_LOCATION/build $(pwd)" >> /tmp/$POPULATED_FILE_NAME
    BUILD_COMMANDS="cd $(pwd) && ./$POPULATED_FILE_NAME"
    . $HELPERS_DIR/populate-template-and-hash.sh -h # obtain $FULL_TAG (and don't overwrite existing file)
    cat /tmp/$POPULATED_FILE_NAME
    mv /tmp/$POPULATED_FILE_NAME ./$POPULATED_FILE_NAME
    echo "$ docker run $ARGS $FULL_TAG bash -c \"$BUILD_COMMANDS\""
    eval docker run $ARGS $FULL_TAG bash -c \"$BUILD_COMMANDS\"
fi
if [[ $TRAVIS != true ]]; then
    [[ $(uname) == 'Darwin' ]] && cd $EOS_LOCATION
    tar -pczf build.tar.gz build && buildkite-agent artifact upload build.tar.gz
fi
echo '+++ Build Script Finished'
