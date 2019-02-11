#! /bin/bash

PREFIX="usr"
SPREFIX=${PREFIX}
SUBPREFIX="opt/${PROJECT}/${VERSION}"
SSUBPREFIX="opt\/${PROJECT}\/${VERSION}"
RELEASE="${VERSION_SUFFIX}"

# default release to "1" if there is no suffix
if [[ -z $RELEASE ]]; then
  RELEASE="1"
fi

NAME="${PROJECT}_${VERSION_NO_SUFFIX}-${RELEASE}_amd64"

mkdir -p ${PROJECT}/DEBIAN
chmod 0755 ${PROJECT}/DEBIAN
echo "Package: ${PROJECT}
Version: ${VERSION_NO_SUFFIX}-${RELEASE}
Section: devel
Priority: optional
Depends: libbz2-dev (>= 1.0), libssl-dev (>= 1.0), libgmp3-dev, build-essential, libicu-dev, zlib1g-dev, libtinfo5
Architecture: amd64
Homepage: ${URL}
Maintainer: ${EMAIL}
Description: ${DESC}" &> ${PROJECT}/DEBIAN/control

export PREFIX
export SUBPREFIX
export SPREFIX
export SSUBPREFIX

bash generate_tarball.sh ${NAME}.tar.gz

tar -xvzf ${NAME}.tar.gz -C ${PROJECT}
dpkg-deb --build ${PROJECT}
BUILDSTATUS=$?
mv ${PROJECT}.deb ${NAME}.deb
rm -r ${PROJECT}

exit $BUILDSTATUS
