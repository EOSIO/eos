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

function teardown() { # teardown is run once after each test, even if it fails
  echo -e "\n-- CLEANUP --" >&3
  [[ -d "$HOME" ]] && rm -rf "$HOME"
  echo -e "-- END CLEANUP --\n" >&3
}

function install-package() {
  if [[ $ARCH == "Linux" ]]; then
    ( [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]] ) && yum install -y $1
    [[ $NAME =~ "Ubuntu" ]] && apt-get update && apt-get install -y $1
  fi
}

function uninstall-package() {
  if [[ $ARCH == "Linux" ]]; then
    ( [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]] ) && yum remove -y $1
    [[ $NAME =~ "Ubuntu" ]] && apt-get remove -y $1
  fi
}

trap teardown EXIT
setup-bats-dirs
