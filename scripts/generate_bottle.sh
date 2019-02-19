#! /bin/bash

VERS=`sw_vers -productVersion | awk '/10\.13\..*/{print $0}'`
if [[ -z "$VERS" ]];
then
   VERS=`sw_vers -productVersion | awk '/10\.14.*/{print $0}'`
   if [[ -z "$VERS" ]];
   then
      echo "Error, unsupported OS X version"
      exit -1
   fi
   MAC_VERSION="mojave"
else
   MAC_VERSION="high_sierra"
fi

NAME="${PROJECT}-${VERSION}.${MAC_VERSION}.bottle"

mkdir -p ${PROJECT}/${VERSION}/opt/eosio/lib/cmake

PREFIX="${PROJECT}/${VERSION}"
SPREFIX="\/usr\/local"
SUBPREFIX="opt/${PROJECT}"
SSUBPREFIX="opt\/${PROJECT}"

export PREFIX
export SPREFIX
export SUBPREFIX
export SSUBPREFIX

. ./generate_tarball.sh ${NAME}

hash=`openssl dgst -sha256 ${NAME}.tar.gz | awk 'NF>1{print $NF}'`

echo "class Eosio < Formula

   homepage \"${URL}\"
   revision 0
   url \"https://github.com/eosio/eos/archive/v${VERSION}.tar.gz\"
   version \"${VERSION}\"

   option :universal

   depends_on \"gmp\"
   depends_on \"gettext\"
   depends_on \"openssl\"
   depends_on \"libusb\"
   depends_on :macos => :high_sierra
   depends_on :arch =>  :intel

   bottle do
      root_url \"https://github.com/eosio/eos/releases/download/v${VERSION}\"
      sha256 \"${hash}\" => :${MAC_VERSION}
   end
   def install
      raise \"Error, only supporting binary packages at this time\"
   end
end
__END__" &> eosio.rb

rm -r ${PROJECT} || exit 1
