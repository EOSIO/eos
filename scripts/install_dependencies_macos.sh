#!/usr/bin/env bash
#
# Install dependencies for ubuntu

exists() {
    command -v "$1" >/dev/null 2>&1
}

get_deps() {
  deps="git automake libtool boost openssl llvm gmp wget cmake gettext"
  missing=""
  echo "==> detecting the dependencies"
  for dep in $deps; do
    if brew ls --versions $dep > /dev/null; then
      # The package is installed
      echo "[OK] $dep"
    else
      # The package is not installed
      missing="$missing $dep"
    fi
  done
  if [[ ! -z $missing ]]; then
    echo "==> missing: $missing"
    echo "trying to install the missing packages:"
    brew install $missing
  fi
  # See https://stackoverflow.com/questions/11370684/what-is-libintl-h-and-where-can-i-get-it
  brew reinstall gettext
  brew unlink gettext && brew link gettext --force
}

# install secp256k1-zkp (Cryptonomex branch)
secp256k1_zkp() {
  cd ${TEMP_DIR}
  git clone https://github.com/cryptonomex/secp256k1-zkp.git
  cd secp256k1-zkp
  ./autogen.sh
  ./configure
  make
  sudo make install
  sudo rm -rf ${TEMP_DIR}/secp256k1-zkp
}

# Install binaryen v1.37.14:
binaryen() {
  cd ${TEMP_DIR}
  git clone https://github.com/WebAssembly/binaryen
  cd binaryen
  git checkout tags/1.37.14
  cmake . && make
  sudo mkdir /usr/local/binaryen
  sudo mv ${TEMP_DIR}/binaryen/bin /usr/local/binaryen
  sudo ln -s /usr/local/binaryen/bin/* /usr/local
  sudo rm -rf ${TEMP_DIR}/binaryen
  export BINARYEN_BIN=/usr/local/binaryen/bin/
}

# Build LLVM and clang for WASM:
llvm_clang() {
  cd ${TEMP_DIR}
  mkdir wasm-compiler
  cd wasm-compiler
  git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
  cd llvm/tools
  git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
  cd ..
  mkdir build
  cd build
  sudo cmake \
    -G "Unix Makefiles" \
    -DCMAKE_INSTALL_PREFIX=/usr/local/wasm \
    -DLLVM_TARGETS_TO_BUILD= \
    -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly \
    -DCMAKE_BUILD_TYPE=Release ../
  sudo make -j4 install
  sudo rm -rf ${TEMP_DIR}/wasm-compiler
  export WASM_LLVM_CONFIG=/usr/local/wasm/bin/llvm-config
}

#### Main
get_deps
secp256k1_zkp
binaryen
llvm_clang
