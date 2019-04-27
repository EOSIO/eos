[[ -z "${VERBOSE}" ]] && export VERBOSE=false # Support tests + Disable execution messages in STDOUT
[[ -z "${DRYRUN}" ]] && export DRYRUN=false # Support tests + Disable execution, just STDOUT
# Arrays should return with newlines so we can do something like "${output##*$'\n'}" to get the last line
IFS=$'\n'

if [[ $- == *i* ]]; then # Disable if the shell isn't interactive (avoids: tput: No value for $TERM and no -T specified)
  export COLOR_NC=$(tput sgr0) # No Color
  export COLOR_RED=$(tput setaf 1)
  export COLOR_GREEN=$(tput setaf 2)
  export COLOR_YELLOW=$(tput setaf 3)
  export COLOR_BLUE=$(tput setaf 4)
  export COLOR_MAGENTA=$(tput setaf 5)
  export COLOR_CYAN=$(tput setaf 6)
  export COLOR_WHITE=$(tput setaf 7)
fi

function execute() {
  ( [[ ! -z "${VERBOSE}" ]] && $VERBOSE ) && echo " - Executing: $@"
  ( [[ ! -z "${DRYRUN}" ]] && $DRYRUN ) || "$@"
}

function setup-tmp() {
  # Use current directory's tmp directory if noexec is enabled for /tmp
  if (mount | grep "/tmp " | grep --quiet noexec) 2>/dev/null; then
    [[ -z "${REPO_ROOT}" ]] && echo "\$REPO_ROOT not set" && exit 1
    mkdir -p $REPO_ROOT/tmp
    TEMP_DIR="${REPO_ROOT}/tmp"
    rm -rf $REPO_ROOT/tmp/*
  else # noexec wasn't found
    TEMP_DIR="/tmp"
  fi
}
function ensure-git-clone() {
  if [ ! -d "${REPO_ROOT}/.git" ]; then
    echo "This build script only works with sources cloned from git"
    echo "For example, you can clone a new eos directory with: git clone https://github.com/EOSIO/eos"
    exit 1
  fi
}

function ensure-submodules-up-to-date() {
  if [[ $DRYRUN == false ]] && [[ $(execute git submodule status --recursive | grep -c "^[+\-]") -gt 0 ]]; then
    echo "git submodules are not up to date."
    echo "Please run the command 'git submodule update --init --recursive'."
    exit 1
  fi
}

function ensure-sudo() {
  if [[ $DRYRUN == false ]] && [[ -z $( command -v sudo ) ]]; then echo "You must have sudo installed to run the build scripts!" && exit 1; fi
}

function previous-install-prompt() {
  if [[ -d $EOSIO_HOME ]]; then
    echo "EOSIO has already been installed into ${EOSIO_HOME}... It's suggested that you eosio_uninstall.bash before re-running this script."
    while true; do
      [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to proceed anyway? (y/n)${COLOR_NC} " PROCEED
      case $PROCEED in
        "" ) echo "What would you like to do?";;
        0 | true | [Yy]* ) break;;
        1 | false | [Nn]* ) exit;;
        * ) echo "Please type 'y' for yes or 'n' for no.";;
      esac
	  done
  fi
}