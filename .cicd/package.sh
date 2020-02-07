#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
[[ $BUILDKITE == true ]] && buildkite-agent artifact download build.tar.gz . --step "$PLATFORM_NAME_FULL - Build" && tar -xzf build.tar.gz
mkdir -p $BUILD_DIR
if [[ $(uname) == 'Darwin' ]]; then
    bash -c "cd build/packages && chmod 755 ./*.sh && ./generate_package.sh brew"
    ARTIFACT='*.rb;*.tar.gz'
    cd build/packages
    [[ -d x86_64 ]] && cd 'x86_64' # backwards-compatibility with release/1.6.x
    buildkite-agent artifact upload "./$ARTIFACT" --agent-access-token $BUILDKITE_AGENT_ACCESS_TOKEN
    for A in $(echo $ARTIFACT | tr ';' ' '); do
        if [[ $(ls $A | grep -c '') == 0 ]]; then
            echo "+++ :no_entry: ERROR: Expected artifact \"$A\" not found!"
            pwd
            ls -la
            exit 1
        fi
    done
else # Linux
    ARGS=${ARGS:-"--rm --init -v $(pwd):$MOUNTED_DIR"}
    . $HELPERS_DIR/file-hash.sh $CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile
    PRE_COMMANDS="cd $MOUNTED_DIR/build/packages && chmod 755 ./*.sh"
    if [[ "$IMAGE_TAG" =~ "ubuntu" ]]; then
        ARTIFACT='*.deb'
        PACKAGE_TYPE='deb'
        PACKAGE_COMMANDS="./generate_package.sh $PACKAGE_TYPE"
    elif [[ "$IMAGE_TAG" =~ "centos" ]]; then
        ARTIFACT='*.rpm'
        PACKAGE_TYPE='rpm'
        PACKAGE_COMMANDS="mkdir -p ~/rpmbuild/BUILD && mkdir -p ~/rpmbuild/BUILDROOT && mkdir -p ~/rpmbuild/RPMS && mkdir -p ~/rpmbuild/SOURCES && mkdir -p ~/rpmbuild/SPECS && mkdir -p ~/rpmbuild/SRPMS && yum install -y rpm-build && ./generate_package.sh $PACKAGE_TYPE"
    fi
    COMMANDS="$PRE_COMMANDS && $PACKAGE_COMMANDS"
    echo "docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$COMMANDS\""
    eval docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$COMMANDS\"
    cd build/packages
    [[ -d x86_64 ]] && cd 'x86_64' # backwards-compatibility with release/1.6.x
    buildkite-agent artifact upload "./$ARTIFACT" --agent-access-token $BUILDKITE_AGENT_ACCESS_TOKEN
    for A in $(echo $ARTIFACT | tr ';' ' '); do
        if [[ $(ls $A | grep -c '') == 0 ]]; then
            echo "+++ :no_entry: ERROR: Expected artifact \"$A\" not found!"
            pwd
            ls -la
            exit 1
        fi
    done
fi