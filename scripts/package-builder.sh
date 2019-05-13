#!/bin/bash
set -e # exit on failure of any "simple" command (excludes &&, ||, or | chains)
echo '+++ :evergreen_tree: Configuring Environment'
[[ "$BUILD_STEP" == '' ]] && BUILD_STEP="$1"
[[ "$BUILD_STEP" == '' ]] && (echo '+++ :no_entry: ERROR: Build step name must be given as BUILD_STEP or argument 1!' && exit 1)
if [[ "$(uname)" == 'Darwin' ]]; then
    echo 'Darwin family detected, building for brew.'
    ARTIFACT='build/packages/*.rb;build/packages/*.tar.gz'
    PACKAGE_TYPE='brew'
else
    . /etc/os-release
    if [[ "$ID_LIKE" == 'rhel fedora' ]]; then
        echo 'Fedora family detected, building for RPM.'
        ARTIFACT='build/packages/*.rpm'
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
        ARTIFACT='build/packages/*.deb'
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
cd build/packages
chmod 755 ./*.sh
./generate_package.sh "$PACKAGE_TYPE"
echo '+++ :arrow_up: Uploading Artifacts'
cd ../..
buildkite-agent artifact upload \"$ARTIFACT\"
echo "+++ :white_check_mark: Done."