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

if [[ -f /etc/upstream-release/lsb-release ]]; then
  source /etc/upstream-release/lsb-release
elif [[ -f /etc/lsb-release ]]; then
  source /etc/lsb-release
else
  echo "Unrecognized Debian derivative.  Not generating .deb file."
  exit 1
fi

if [ ${DISTRIB_RELEASE} = "16.04" ]; then
  LIBSSL="libssl1.0.0"
elif [ ${DISTRIB_RELEASE} = "18.04" ]; then
  LIBSSL="libssl1.1"
else
  echo "Unrecognized Ubuntu version.  Update generate_deb.sh.  Not generating .deb file."
  exit 1
fi

mkdir -p ${PROJECT}/DEBIAN
chmod 0755 ${PROJECT}/DEBIAN
echo "Package: ${PROJECT}
Version: ${VERSION_NO_SUFFIX}-${RELEASE}
Section: devel
Priority: optional
Depends: libc6, libgcc1, ${LIBSSL}, libstdc++6, libtinfo5, zlib1g
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
