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

# Execution helpers; necessary for BATS testing and log output in buildkite

function execute() {
  $VERBOSE && echo "--- Executing: $@"
  $DRYRUN || "$@"
}

function execute-quiet() {
  $VERBOSE && echo "--- Executing: $@ &>/dev/null"
  $DRYRUN || "$@" &>/dev/null
}

function execute-always() {
  ORIGINAL_DRYRUN=$DRYRUN
  DRYRUN=false
  execute "$@"
  DRYRUN=$ORIGINAL_DRYRUN
}

function execute-without-verbose() {
  ORIGINAL_VERBOSE=$VERBOSE
  VERBOSE=false
  execute "$@"
  VERBOSE=$ORIGINAL_VERBOSE
}

function ensure-git-clone() {
  if [[ ! -e "${REPO_ROOT}/.git" ]]; then
    echo "This build script only works with sources cloned from git"
    echo "For example, you can clone a new eos directory with: git clone https://github.com/EOSIO/eos"
    exit 1
  fi
}

function ensure-submodules-up-to-date() {
  if [[ $DRYRUN == false ]] && [[ ! -z $(execute-without-verbose git submodule status --recursive | grep "^[+\-]") ]]; then
    echo "${COLOR_RED}git submodules are not up to date: Please run the command 'git submodule update --init --recursive'.${COLOR_NC}"
    exit 1
  fi
}

function ensure-which() {
  if ! which ls &>/dev/null; then
    while true; do
      [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}EOSIO compiler checks require the 'which' package: Would you like for us to install it? (y/n)?${COLOR_NC}" && read -p " " PROCEED
      echo ""
      case $PROCEED in
          "" ) echo "What would you like to do?";;
          0 | true | [Yy]* ) install-package which WETRUN; break;;
          1 | false | [Nn]* ) echo "${COLOR_RED}Please install the 'which' command before proceeding!${COLOR_NC}"; exit 1;;
          * ) echo "Please type 'y' for yes or 'n' for no.";;
      esac
    done
  fi
}

function ensure-sudo() {
  ( [[ $CURRENT_USER != "root" ]] && [[ -z $SUDO_LOCATION ]] ) && echo "${COLOR_RED}Please install the 'sudo' command before proceeding!${COLOR_NC}" && exit 1
  true 1>/dev/null # Needed
}

function previous-install-prompt() {
  if [[ -d $EOSIO_INSTALL_DIR ]]; then
    echo "EOSIO has already been installed into ${EOSIO_INSTALL_DIR}... It's suggested that you eosio_uninstall.sh before re-running this script."
    while true; do
      [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to proceed anyway? (y/n)${COLOR_NC}" && read -p " " PROCEED
      echo ""
      case $PROCEED in
        "" ) echo "What would you like to do?";;
        0 | true | [Yy]* ) break;;
        1 | false | [Nn]* ) exit;;
        * ) echo "Please type 'y' for yes or 'n' for no.";;
      esac
	  done
  fi
}

function set_system_vars() {
    if [[ $ARCH == "Darwin" ]]; then
        export OS_VER=$(sw_vers -productVersion)
        export OS_MAJ=$(echo "${OS_VER}" | cut -d'.' -f1)
        export OS_MIN=$(echo "${OS_VER}" | cut -d'.' -f2)
        export OS_PATCH=$(echo "${OS_VER}" | cut -d'.' -f3)
        export MEM_GIG=$(bc <<< "($(sysctl -in hw.memsize) / 1024000000)")
        export DISK_INSTALL=$(df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 || cut -d' ' -f1)
        export blksize=$(df . | head -1 | awk '{print $2}' | cut -d- -f1)
        export gbfactor=$(( 1073741824 / blksize ))
        export total_blks=$(df . | tail -1 | awk '{print $2}')
        export avail_blks=$(df . | tail -1 | awk '{print $4}')
        export DISK_TOTAL=$((total_blks / gbfactor ))
        export DISK_AVAIL=$((avail_blks / gbfactor ))
    else
        export DISK_INSTALL=$( df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 )
        export DISK_TOTAL_KB=$( df . | tail -1 | awk '{print $2}' )
        export DISK_AVAIL_KB=$( df . | tail -1 | awk '{print $4}' )
        export MEM_GIG=$(( ( ( $(cat /proc/meminfo | grep MemTotal | awk '{print $2}') / 1000 ) / 1000 ) ))
        export DISK_TOTAL=$(( DISK_TOTAL_KB / 1048576 ))
        export DISK_AVAIL=$(( DISK_AVAIL_KB / 1048576 ))
    fi
    export JOBS=$(( MEM_GIG > CPU_CORES ? CPU_CORES : MEM_GIG ))
}

function install-package() {
  if [[ $ARCH == "Linux" ]]; then
    EXECUTION_FUNCTION="execute"
    [[ $2 == "WETRUN" ]] && EXECUTION_FUNCTION="execute-always"
    ( [[ $2 =~ "--" ]] || [[ $3 =~ "--" ]] ) && OPTIONS="${2}${3}"
    [[ $CURRENT_USER != "root" ]] && [[ ! -z $SUDO_LOCATION ]] && SUDO_COMMAND="$SUDO_LOCATION -E"
    ( [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]] ) && eval $EXECUTION_FUNCTION $SUDO_COMMAND $YUM $OPTIONS install -y $1
    ( [[ $NAME =~ "Ubuntu" ]] ) && eval $EXECUTION_FUNCTION $SUDO_COMMAND $APTGET $OPTIONS install -y $1
  fi
  true # Required; Weird behavior without it
}

function uninstall-package() {
  if [[ $ARCH == "Linux" ]]; then
    EXECUTION_FUNCTION="execute"
    [[ $2 == "WETRUN" ]] && EXECUTION_FUNCTION="execute-always"
    ( [[ $2 =~ "--" ]] || [[ $3 =~ "--" ]] ) && OPTIONS="${2}${3}"
    [[ $CURRENT_USER != "root" ]] && [[ ! -z $SUDO_LOCATION ]] && SUDO_COMMAND="$SUDO_LOCATION -E"
    # Check if the packages exist before uninstalling them. This speeds things up for tests.
    ( ( [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]] ) && [[ ! -z $(rpm -qa $1) ]] ) && eval $EXECUTION_FUNCTION $SUDO_COMMAND $YUM $OPTIONS remove -y $1
    ( [[ $NAME =~ "Ubuntu" ]] && $(dpkg -s $1 &>/dev/null) ) && eval $EXECUTION_FUNCTION $SUDO_COMMAND $APTGET $OPTIONS remove -y $1
  fi
  true
}