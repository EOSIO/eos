#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
echo '+++ Build Script Started'
export DOCKERIZATION=false
[[ "$(uname)" == 'Darwin' ]] && brew install md5sha1sum
if [[ $BUILDKITE == true ]]; then # Buildkite uses tags with deps already on them
    [[ $ENABLE_INSTALL == true ]] && . ./.cicd/helpers/populate-template-and-hash.sh '<!-- DAC ENV' '<!-- DAC CLONE' '<!-- DAC BUILD' '<!-- DAC INSTALL' || . ./.cicd/helpers/populate-template-and-hash.sh '<!-- DAC ENV' '<!-- DAC CLONE' '<!-- DAC BUILD'
    sed -i -e 's/git clone https:\/\/github.com\/EOSIO\/eos\.git.*/cp -rf \$(pwd) \$EOS_LOCATION \&\& cd \$EOS_LOCATION/g' /tmp/$POPULATED_FILE_NAME # We don't need to clone twice
else
    [[ $ENABLE_INSTALL == true ]] && . ./.cicd/helpers/populate-template-and-hash.sh '<!-- DAC ENV' '<!-- DAC CLONE' '<!-- DAC DEPS' '<!-- DAC BUILD' '<!-- DAC INSTALL' || . ./.cicd/helpers/populate-template-and-hash.sh '<!-- DAC ENV' '<!-- DAC CLONE' '<!-- DAC DEPS' '<!-- DAC BUILD'
    sed -i -e '/git clone https:\/\/github.com\/EOSIO\/eos\.git.*/d' /tmp/$POPULATED_FILE_NAME # We don't need to clone twice
fi
if [[ "$(uname)" == 'Darwin' ]]; then
    # You can't use chained commands in execute
    if [[ $BUILDKITE == true ]]; then
        source ~/.bash_profile # Make sure node is available for ship_test
    else
        export PINNED=false
    fi
    . $HELPERS_DIR/populate-template-and-hash.sh -h # obtain $FULL_TAG (and don't overwrite existing file)
    cat /tmp/$POPULATED_FILE_NAME
    . /tmp/$POPULATED_FILE_NAME # This file is populated from the platform's build documentation code block
else # Linux
    ARGS=${ARGS:-"--rm --init -v $(pwd):$(pwd) $(buildkite-intrinsics) -e GITHUB_WORKSPACE -e JOBS"} # We must mount $(pwd) in as itself to avoid https://stackoverflow.com/questions/31381322/docker-in-docker-cannot-mount-volume
    echo "cp -rf \$EOS_LOCATION/build $(pwd)" >> /tmp/$POPULATED_FILE_NAME
    BUILD_COMMANDS="cd $(pwd) && ./$POPULATED_FILE_NAME"
    . $HELPERS_DIR/populate-template-and-hash.sh -h # obtain $FULL_TAG (and don't overwrite existing file)
    cat /tmp/$POPULATED_FILE_NAME
    mv /tmp/$POPULATED_FILE_NAME ./$POPULATED_FILE_NAME
    echo "$ docker run $ARGS $FULL_TAG bash -c \"$BUILD_COMMANDS\""
    eval docker run $ARGS $FULL_TAG bash -c \"$BUILD_COMMANDS\"
fi
if [[ $GITHUB_ACTIONS != true ]]; then
    [[ $(uname) == 'Darwin' ]] && cd $EOS_LOCATION
    tar -pczf build.tar.gz build && buildkite-agent artifact upload build.tar.gz
fi
echo '+++ Build Script Finished'
