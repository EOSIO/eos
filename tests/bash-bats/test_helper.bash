# You can add `load test_helper` to the .bats files you create to include anything in this file.
# DO NOT REMOVE
export DRYRUN=true
export VERBOSE=true
export CURRENT_USER=$(whoami)

export HOME="$BATS_TMPDIR/bats-eosio-user-home" # Ensure $HOME is available for all scripts
mkdir -p $HOME

# Obtain dependency versions and paths
. ./scripts/helpers/eosio.bash

# Ensure we're in the root directory to execute
if [[ ! -d "tests" ]] && [[ ! -f "README.md" ]]; then
  echo "You must navigate into the root directory to execute tests..." >&3
  exit 1
fi

function debug() {
  printf " ---------\\n STATUS: ${status}\\n${output}\\n ---------\\n\\n" >&3
}

trap teardown EXIT
function teardown() { # teardown is run once after each test, even if it fails
  # echo -e "\n-- CLEANUP --" >&3
  [[ -d "$HOME" ]] && rm -rf "$HOME"
  # echo -e "-- END CLEANUP --\n" >&3
}

function install-package() {
  if [[ $ARCH == "Linux" ]]; then
    ( [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]] ) && yum install $1 -y
  fi
}

function uninstall-package() {
  if [[ $ARCH == "Linux" ]]; then
    ( [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]] ) && yum remove $1 -y
  fi
}

install-package which 1>/dev/null