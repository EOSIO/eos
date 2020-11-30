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
cp -R "${BUILD_DIR}"/contracts/eosio.bios/eosio.abi ${EOS_PREFIX}/etc/eosio/contracts/eosio.bios.abi
cp -R "${BUILD_DIR}"/contracts/eosio.bios/eosio.bios ${EOS_PREFIX}/etc/eosio/contracts/eosio.bios.wasm
cp -R "${BUILD_DIR}"/contracts/eosio.boot/eosio.abi ${EOS_PREFIX}/etc/eosio/contracts/eosio.boot.abi
cp -R "${BUILD_DIR}"/contracts/eosio.boot/eosio.boot ${EOS_PREFIX}/etc/eosio/contracts/eosio.boot.wasm

for f in $(ls "${BUILD_DIR}/bin/"); do
    bn=$(basename "$f")
    ln -sf ../"${SUBPREFIX}/bin/$bn" "${PREFIX}/bin/$bn"
done
echo "Compressing '$NAME.tar.gz'..."
tar -cvzf "$NAME.tar.gz" ./"${PREFIX}"/*
rm -r "${PREFIX}"
