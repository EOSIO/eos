#!/bin/bash
set -eo pipefail
echo '--- :evergreen_tree: Configuring Environment'
. ./.cicd/helpers/general.sh
mkdir -p "$BUILD_DIR"
if [[ $(uname) == 'Darwin' && $FORCE_LINUX != true ]]; then
    echo '+++ :package: Packaging EOSIO'
    PACKAGE_COMMANDS="bash -c 'cd build/packages && chmod 755 ./*.sh && ./generate_package.sh brew'"
    echo "$ $PACKAGE_COMMANDS"
    eval $PACKAGE_COMMANDS
    ARTIFACT='*.rb;*.tar.gz'
else # Linux
    echo '--- :docker: Selecting Container'
    ARGS="${ARGS:-"--rm --init -v \"\$(pwd):$MOUNTED_DIR\""}"
    . "$HELPERS_DIR/file-hash.sh" "$CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile"
    PRE_COMMANDS="cd \"$MOUNTED_DIR/build/packages\" && chmod 755 ./*.sh"
    if [[ "$IMAGE_TAG" =~ "ubuntu" ]]; then
        ARTIFACT='*.deb'
        PACKAGE_TYPE='deb'
        PACKAGE_COMMANDS="./generate_package.sh \"$PACKAGE_TYPE\""
    elif [[ "$IMAGE_TAG" =~ "centos" ]]; then
        ARTIFACT='*.rpm'
        PACKAGE_TYPE='rpm'
        PACKAGE_COMMANDS="mkdir -p ~/rpmbuild/BUILD && mkdir -p ~/rpmbuild/BUILDROOT && mkdir -p ~/rpmbuild/RPMS && mkdir -p ~/rpmbuild/SOURCES && mkdir -p ~/rpmbuild/SPECS && mkdir -p ~/rpmbuild/SRPMS && yum install -y rpm-build && ./generate_package.sh \"$PACKAGE_TYPE\""
    fi
    COMMANDS="echo \"+++ :package: Packaging EOSIO\" && $PRE_COMMANDS && $PACKAGE_COMMANDS"
    DOCKER_RUN_COMMAND="docker run $ARGS $(buildkite-intrinsics) '$FULL_TAG' bash -c '$COMMANDS'"
    echo "$ $DOCKER_RUN_COMMAND"
    eval $DOCKER_RUN_COMMAND
fi
cd build/packages
[[ -d x86_64 ]] && cd 'x86_64' # backwards-compatibility with release/1.6.x
if [[ "$BUILDKITE" == 'true' ]]; then
    echo '--- :arrow_up: Uploading Artifacts'
    buildkite-agent artifact upload "./$ARTIFACT" --agent-access-token $BUILDKITE_AGENT_ACCESS_TOKEN
fi
for A in $(echo $ARTIFACT | tr ';' ' '); do
    if [[ $(ls "$A" | grep -c '') == 0 ]]; then
        echo "+++ :no_entry: ERROR: Expected artifact \"$A\" not found!"
        pwd
        ls -la
        exit 1
    fi
done
echo '--- :white_check_mark: Done!'
