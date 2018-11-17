#! /bin/bash

NAME="${PROJECT}_${VERSION}-1_amd64"
PREFIX="usr"
SPREFIX=${PREFIX}
SUBPREFIX="opt/${PROJECT}/${VERSION}"
SSUBPREFIX="opt\/${PROJECT}\/${VERSION}"

DEPS_STR=""
for dep in "${DEPS[@]}"; do
   DEPS_STR="${DEPS_STR} Depends: ${dep}"
done
mkdir -p ${PROJECT}/DEBIAN
echo "Package: ${PROJECT} 
Version: ${VERSION}
Section: devel
Priority: optional
Depends: libbz2-dev (>= 1.0), libssl-dev (>= 1.0), libgmp3-dev, build-essential, libicu-dev, zlib1g-dev
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
mv ${PROJECT}.deb ${NAME}.deb
rm -r ${PROJECT}
