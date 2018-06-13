#!/bin/bash

if (( $# < 3 )); then
   echo "$0 SOURCE_DIR BUILD_DIR ARGS..."
   exit 0
fi

function get_absolute_dir_path() {
  pushd $1 > /dev/null
  echo $PWD
  popd > /dev/null
}

SOURCE_DIR=$(get_absolute_dir_path $1)

BUILD_DIR=$2

shift; shift

#echo "Source directory: ${SOURCE_DIR}"

eval "${BUILD_DIR}/programs/eosio-abigen/eosio-abigen -extra-arg=--std=c++14 -extra-arg=--target=wasm32 -extra-arg=\"-I${SOURCE_DIR}/contracts/libc++/upstream/include\" -extra-arg=\"-I${SOURCE_DIR}/contracts/musl/upstream/include\" -extra-arg=\"-I${SOURCE_DIR}/externals/magic_get/include\" -extra-arg=\"-I${SOURCE_DIR}/contracts\" $@"
