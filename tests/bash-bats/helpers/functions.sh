. ./scripts/helpers/eosio.sh # Obtain dependency versions and paths

function debug() {
  printf " ---------\\n STATUS: ${status}\\n${output}\\n ---------\\n\\n" >&3
}

function setup-bats-dirs () {
    if [[ ! $HOME =~ "/$(whoami)" ]]; then 
        mkdir -p $HOME
    fi
    if [[ $TEMP_DIR =~ "${HOME}" ]]; then # Protection
        mkdir -p $TEMP_DIR
        rm -rf $TEMP_DIR/*
    fi
}
setup-bats-dirs

function teardown() { # teardown is run once after each test, even if it fails
  [[ -d "$HOME" ]] && rm -rf "$HOME"
  uninstall-package which WETRUN
  uninstall-package sudo WETRUN
  uninstall-package devtoolset-8* WETRUN
  uninstall-package centos-release-scl
  uninstall-package gcc-c++ WETRUN
  uninstall-package build-essential WETRUN
}
trap teardown EXIT