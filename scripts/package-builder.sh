#!/bin/bash
set -e # exit on failure of any "simple" command (excludes &&, ||, or | chains)
echo '+++ :evergreen_tree: Configuring Environment'
[[ "$BUILD_STEP" == '' ]] && BUILD_STEP="$1"
[[ "$BUILD_STEP" == '' ]] && (echo '+++ :no_entry: ERROR: Build step name must be given as BUILD_STEP or argument 1!' && exit 1)
if [[ "$(uname)" == 'Darwin' ]]; then
    echo 'Darwin family detected, building for brew.'
    [[ "$ARTIFACT" == '' ]] && ARTIFACT='*.rb;*.tar.gz'
    PACKAGE_TYPE='brew'
else
    . /etc/os-release
    if [[ "$ID_LIKE" == 'rhel fedora' ]]; then
        echo 'Fedora family detected, building for RPM.'
        [[ "$ARTIFACT" == '' ]] && ARTIFACT='*.rpm'
        PACKAGE_TYPE='rpm'
        mkdir -p /root/rpmbuild/BUILD
        mkdir -p /root/rpmbuild/BUILDROOT
        mkdir -p /root/rpmbuild/RPMS
        mkdir -p /root/rpmbuild/SOURCES
        mkdir -p /root/rpmbuild/SPECS
        mkdir -p /root/rpmbuild/SRPMS
        yum install -y rpm-build
    elif [[ "$ID_LIKE" == 'debian' ]]; then
        echo 'Debian family detected, building for dpkg.'
        [[ "$ARTIFACT" == '' ]] && ARTIFACT='*.deb'
        PACKAGE_TYPE='deb'
    else
        echo '+++ :no_entry: ERROR: Could not determine which operating system this script is running on!'
        echo '$ uname'
        uname
        echo "ID_LIKE=\"$ID_LIKE\""
        echo '$ cat /etc/os-release'
        cat /etc/os-release
        echo 'Exiting...'
        exit 1
    fi
fi
echo "+++ :arrow_down: Downloading Build Directory"
buildkite-agent artifact download build.tar.gz . --step "$BUILD_STEP"
echo "+++ :compression: Extracting Build Directory"
[[ -d build ]] && rm -rf build
tar -zxf build.tar.gz
echo "+++ :package: Starting Package Build"
BASE_COMMIT=$(cat build/programs/nodeos/config.hpp | grep 'version' | awk '{print $5}' | tr -d ';')
BASE_COMMIT="${BASE_COMMIT:2:42}"
echo "Found build against $BASE_COMMIT."
cd build/packages
chmod 755 ./*.sh
./generate_package.sh "$PACKAGE_TYPE"
echo '+++ :arrow_up: Uploading Artifacts'
buildkite-agent artifact upload "./$ARTIFACT"
for A in $(echo $ARTIFACT | tr ';' ' '); do
    if [[ $(ls $A | grep -c '') == 0 ]]; then
        echo "+++ :no_entry: ERROR: Expected artifact \"$A\" not found!"
        echo '$ pwd'
        pwd
        echo '$ ls -la'
        ls -la
        exit 1
    fi
done
echo "+++ :white_check_mark: Done."