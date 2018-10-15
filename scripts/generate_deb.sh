#! /bin/bash

NAME="${PROJECT}-${VERSION}.x86_64"
PREFIX="usr"
SPREFIX=${PREFIX}
SUBPREFIX="opt/${PROJECT}/${VERSION}"
SSUBPREFIX="opt\/${PROJECT}\/${VERSION}"

mkdir -p ${PROJECT}/DEBIAN
echo "Package: ${PROJECT} 
Version: ${VERSION}
Section: devel
Priority: optional
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
