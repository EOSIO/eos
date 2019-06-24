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
  if [[ $ARCH == "Linux" ]]; then
    uninstall-package which WETRUN
    export SUDO_FORCE_REMOVE=yes
    uninstall-package sudo WETRUN
    uninstall-package devtoolset-8* WETRUN
    uninstall-package centos-release-scl
    uninstall-package gcc-c++ WETRUN
    if [[ $NAME == 'Ubuntu' ]]; then
      [[ ! $( dpkg -s build-essential 2>/dev/null ) ]] && apt autoremove build-essential -y &>/dev/null
    fi
  fi
}
trap teardown EXIT