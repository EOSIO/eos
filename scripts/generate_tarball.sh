#!/bin/bash
set -eo pipefail

NAME="$1"
EOS_PREFIX="${PREFIX}/${SUBPREFIX}"
mkdir -p "${PREFIX}/bin/"
mkdir -p "${EOS_PREFIX}/bin"
mkdir -p "${EOS_PREFIX}/licenses/eosio"
mkdir -p "${EOS_PREFIX}/etc/eosio/contracts"

# install binaries 
cp -R "${BUILD_DIR}"/bin/* ${EOS_PREFIX}/bin

# install licenses
cp -R "${BUILD_DIR}"/licenses/eosio/* ${EOS_PREFIX}/licenses

# install bios and boot contracts
cp -R "${BUILD_DIR}"/contracts/contracts/eosio.bios/eosio.bios.* ${EOS_PREFIX}/etc/eosio/contracts
cp -R "${BUILD_DIR}"/contracts/contracts/eosio.boot/eosio.boot.* ${EOS_PREFIX}/etc/eosio/contracts

for f in $(ls "${BUILD_DIR}/bin/"); do
    bn=$(basename "$f")
    ln -sf ../"${SUBPREFIX}/bin/$bn" "${PREFIX}/bin/$bn"
done
echo "Compressing '$NAME.tar.gz'..."
tar -cvzf "$NAME.tar.gz" ./"${PREFIX}"/*
rm -r "${PREFIX}"
