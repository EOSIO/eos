#!/bin/bash
set -eo pipefail

NAME="$1"
EOS_PREFIX="${PREFIX}/${SUBPREFIX}"
mkdir -p "${PREFIX}/bin/"
mkdir -p "${EOS_PREFIX}/bin"
mkdir -p "${EOS_PREFIX}/licenses/eosio"

# install binaries 
cp -R "${BUILD_DIR}"/bin/* ${EOS_PREFIX}/bin

# install licenses
cp -R "${BUILD_DIR}"/licenses/eosio/* ${EOS_PREFIX}/licenses

for f in $(ls "${BUILD_DIR}/bin/"); do
    bn=$(basename "$f")
    ln -sf ../"${SUBPREFIX}/bin/$bn" "${PREFIX}/bin/$bn"
done
echo "Compressing '$NAME.tar.gz'..."
tar -cvzf "$NAME.tar.gz" ./"${PREFIX}"/*
rm -r "${PREFIX}"
