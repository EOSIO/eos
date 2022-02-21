#!/bin/bash
set -eo pipefail

export PREFIX='usr'
export SPREFIX="${PREFIX}"
export SUBPREFIX="opt/${PROJECT}/${VERSION}"
export SSUBPREFIX="opt\/${PROJECT}\/${VERSION}"
RELEASE="${VERSION_SUFFIX}"

# default release to "1" if there is no suffix
if [[ -z "$RELEASE" ]]; then
    RELEASE='1'
fi

NAME="${PROJECT}_${VERSION_NO_SUFFIX}-${RELEASE}_amd64"

if [[ -f '/etc/upstream-release/lsb-release' ]]; then
    source '/etc/upstream-release/lsb-release'
elif [[ -f '/etc/lsb-release' ]]; then
    source '/etc/lsb-release'
else
    echo 'Operating system not recognized, exiting...'
    exit 1
fi

DISTRIB_MAJOR_VERSION="$(echo "$DISTRIB_RELEASE" | cut -d '.' -f '1')"
if (( "$DISTRIB_MAJOR_VERSION" >= 18 )); then
    RELEASE_SPECIFIC_DEPS='libssl1.1'
elif (( "$DISTRIB_MAJOR_VERSION" >= 16 )); then
    RELEASE_SPECIFIC_DEPS='libssl1.0.0'
else
    echo "Found Ubuntu $DISTRIB_MAJOR_VERSION, but not sure which version of libssl to use. Exiting..."
    exit 2
fi

echo "Creating './${PROJECT}/DEBIAN/'."
mkdir -p "${PROJECT}/DEBIAN"
chmod 0755 "${PROJECT}/DEBIAN"
echo "Writing control file to '${PROJECT}/DEBIAN/control'."
echo "Package: ${PROJECT}
Version: ${VERSION_NO_SUFFIX}-${RELEASE}
Section: devel
Priority: optional
Depends: libc6, libgcc1, ${RELEASE_SPECIFIC_DEPS}, libstdc++6, libtinfo5, zlib1g, libusb-1.0-0, libcurl3-gnutls, libpq5
Architecture: amd64
Homepage: ${URL}
Maintainer: ${EMAIL}
Description: ${DESC}" | tee &> "${PROJECT}/DEBIAN/control"

GENERATE_TARBALL=". ./generate_tarball.sh '${NAME}'"
echo "$ $GENERATE_TARBALL"
eval $GENERATE_TARBALL
echo "Unpacking '${NAME}.tar.gz'..."
tar -xzvf "${NAME}.tar.gz" -C "${PROJECT}"
DPKG_BUILD="\$(type -P fakeroot) dpkg-deb --build '${PROJECT}'"
echo "$ $DPKG_BUILD"
eval $DPKG_BUILD
echo "Deleting './${PROJECT}/DEBIAN/'."
mv "${PROJECT}.deb" "${NAME}.deb"
rm -r "${PROJECT}"
echo "Done, package is '${NAME}.deb'."
