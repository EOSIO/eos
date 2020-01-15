#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
export DOCKERIZATION=false
buildkite-agent artifact download build.tar.gz . --step "$PLATFORM_FULL_NAME - Build"
. ./.cicd/helpers/populate-template-and-hash.sh '<!-- DAC ENV'
if [[ $(uname) == 'Darwin' ]]; then
    cat /tmp/$POPULATED_FILE_NAME
    . /tmp/$POPULATED_FILE_NAME
    cd $EOS_LOCATION
    tar -xzf build.tar.gz
    bash -c "cd $EOS_LOCATION/build/packages && chmod 755 ./*.sh && ./generate_package.sh brew"
    ARTIFACT='*.rb;*.tar.gz'
    cd $EOS_LOCATION/build/packages
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
    ARGS=${ARGS:-"--rm --init -v $(pwd):$(pwd) $(buildkite-intrinsics)"}
    . $HELPERS_DIR/populate-template-and-hash.sh -h # Prepare the platform-template with contents from the documentation
    echo "cp -rfp $(pwd) \$EOS_LOCATION && cd \$EOS_LOCATION" >> /tmp/$POPULATED_FILE_NAME # We don't need to clone twice
    [[ $BUILDKITE == true ]]&& echo "tar -xzf build.tar.gz" >> /tmp/$POPULATED_FILE_NAME
    echo "cd \$EOS_LOCATION/build/packages && chmod 755 ./*.sh" >> /tmp/$POPULATED_FILE_NAME
    if [[ "$IMAGE_TAG" =~ "ubuntu" ]]; then
        ARTIFACT='*.deb'
        PACKAGE_TYPE='deb'
        echo "./generate_package.sh $PACKAGE_TYPE" >> /tmp/$POPULATED_FILE_NAME
    elif [[ "$IMAGE_TAG" =~ "centos" ]]; then
        ARTIFACT='*.rpm'
        PACKAGE_TYPE='rpm'
        echo "mkdir -p ~/rpmbuild/BUILD && mkdir -p ~/rpmbuild/BUILDROOT && mkdir -p ~/rpmbuild/RPMS && mkdir -p ~/rpmbuild/SOURCES && mkdir -p ~/rpmbuild/SPECS && mkdir -p ~/rpmbuild/SRPMS && yum install -y rpm-build && ./generate_package.sh $PACKAGE_TYPE" >> /tmp/$POPULATED_FILE_NAME
    fi
    echo "cp -rfp \$EOS_LOCATION/build $(pwd)" >> /tmp/$POPULATED_FILE_NAME
    PACKAGE_COMMANDS="cd $(pwd) && . ./$POPULATED_FILE_NAME"
    cat /tmp/$POPULATED_FILE_NAME
    mv /tmp/$POPULATED_FILE_NAME ./$POPULATED_FILE_NAME
    echo "docker run $ARGS $FULL_TAG bash -c \"$PACKAGE_COMMANDS\""
    set +e # defer error handling to end
    eval docker run $ARGS $FULL_TAG bash -c \"$PACKAGE_COMMANDS\"
    EXIT_STATUS=$?
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
# re-throw
if [[ $EXIT_STATUS != 0 ]]; then
    echo "Failing due to non-zero exit status: $EXIT_STATUS"
    exit $EXIT_STATUS
fi