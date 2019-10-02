#!/bin/bash
set -eo pipefail
VERSION=1
brew update
brew install git cmake python llvm@7 automake autoconf libtool jq python@2 curl
pip3 install conan