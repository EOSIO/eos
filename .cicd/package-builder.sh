#!/usr/bin/env bash
set -eo pipefail

if [[ $(uname) == 'Darwin' ]]; then
    echo 'Darwin family detected, building for brew.'
    [[ -z $ARTIFACT ]] && ARTIFACT='*.rb;*.tar.gz'
    PACKAGE_TYPE='brew'
else
    . /etc/os-release
    if [[ "$ID_LIKE" == 'debian' || "$ID" == 'debian' ]]; then
        echo 'Debian family detected, building for dpkg.'
        [[ -z $ARTIFACT ]] && ARTIFACT='*.deb'
        PACKAGE_TYPE='deb'
    elif [[ "$ID_LIKE" == 'rhel fedora' || "$ID" == 'fedora' ]]; then
        echo 'Fedora family detected, building for RPM.'
        [[ -z $ARTIFACT ]] && ARTIFACT='*.rpm'
        PACKAGE_TYPE='rpm'
        mkdir -p ~/rpmbuild/BUILD
        mkdir -p ~/rpmbuild/BUILDROOT
        mkdir -p ~/rpmbuild/RPMS
        mkdir -p ~/rpmbuild/SOURCES
        mkdir -p ~/rpmbuild/SPECS
        mkdir -p ~/rpmbuild/SRPMS
        yum install -y rpm-build
    elif [[ $ID == 'amzn' ]]; then
        echo "SKIPPED: We do not generate $NAME packages since they use rpms created from Centos."
        exit 0
    else
        echo 'ERROR: Could not determine which operating system this script is running on!'
        uname
        echo "ID_LIKE=\"$ID_LIKE\""
        cat /etc/os-release
        exit 1
    fi
fi
BASE_COMMIT=$(cat build/programs/nodeos/config.hpp | grep 'version' | awk '{print $5}' | tr -d ';')
BASE_COMMIT="${BASE_COMMIT:2:42}"
echo "Found build against $BASE_COMMIT."
cd build/packages
chmod 755 ./*.sh
./generate_package.sh $PACKAGE_TYPE
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