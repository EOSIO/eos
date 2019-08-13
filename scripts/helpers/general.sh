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
    exit 1
  fi
}

function ensure-submodules-up-to-date() {
  if [[ $DRYRUN == false ]] && [[ ! -z $(execute-without-verbose git submodule status --recursive | grep "^[+\-]") ]]; then
    echo "${COLOR_RED}git submodules are not up to date: Please run the command 'git submodule update --init --recursive'.${COLOR_NC}"
    exit 1
  fi
}

function ensure-sudo() {
  ( [[ $CURRENT_USER != "root" ]] && [[ -z $SUDO_LOCATION ]] ) && echo "${COLOR_RED}Please install the 'sudo' command before proceeding!${COLOR_NC}" && exit 1
  true 1>/dev/null # Needed
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
    export JOBS=${JOBS:-$(( MEM_GIG > CPU_CORES ? CPU_CORES : MEM_GIG ))}
}

function install-package() {
  if [[ $ARCH == "Linux" ]]; then
    EXECUTION_FUNCTION="execute"
    [[ $2 == "WETRUN" ]] && EXECUTION_FUNCTION="execute-always"
    ( [[ $2 =~ "--" ]] || [[ $3 =~ "--" ]] ) && OPTIONS="${2}${3}"
    # Can't use $SUDO_COMMAND: https://askubuntu.com/questions/953485/where-do-i-find-the-sudo-command-environment-variable
    [[ $CURRENT_USER != "root" ]] && [[ ! -z $SUDO_LOCATION ]] && NEW_SUDO_COMMAND="$SUDO_LOCATION -E"
    ( [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]] ) && eval $EXECUTION_FUNCTION $NEW_SUDO_COMMAND $YUM $OPTIONS install -y $1
    ( [[ $NAME =~ "Ubuntu" ]] ) && eval $EXECUTION_FUNCTION $NEW_SUDO_COMMAND $APTGET $OPTIONS install -y $1
  fi
  true # Required; Weird behavior without it
}

function uninstall-package() {
  if [[ $ARCH == "Linux" ]]; then
    EXECUTION_FUNCTION="execute"
    REMOVE="remove"
    [[ $2 == "WETRUN" ]] && EXECUTION_FUNCTION="execute-always"
    ( [[ $2 == "autoremove" ]] || [[ $3 == "autoremove" ]] ) && REMOVE="autoremove"
    ( [[ $2 =~ "--" ]] || [[ $3 =~ "--" ]] ) && OPTIONS="${2}${3}"
    [[ $CURRENT_USER != "root" ]] && [[ ! -z $SUDO_LOCATION ]] && NEW_SUDO_COMMAND="$SUDO_LOCATION -E"
    # Check if the packages exist before uninstalling them. This speeds things up for tests.
    ( ( [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]] ) && [[ ! -z $(rpm -qa $1) ]] ) && eval $EXECUTION_FUNCTION $NEW_SUDO_COMMAND $YUM $OPTIONS $REMOVE -y $1
    ( [[ $NAME =~ "Ubuntu" ]] && $(dpkg -s $1 &>/dev/null) ) && eval $EXECUTION_FUNCTION $NEW_SUDO_COMMAND $APTGET $OPTIONS $REMOVE -y $1
  fi
  true
}

function ensure-homebrew() {
    echo "${COLOR_CYAN}[Ensuring HomeBrew installation]${COLOR_NC}"
    if ! BREW=$( command -v brew ); then
        while true; do
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to install HomeBrew? (y/n)?${COLOR_NC}" &&  read -p " " PROCEED
            echo ""
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* )
                    execute "${XCODESELECT}" --install 2>/dev/null || true
                    if ! execute "${RUBY}" -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"; then
                        echo "${COLOR_RED}Unable to install HomeBrew!${COLOR_NC}" && exit 1;
                    else BREW=$( command -v brew ); fi
                break;;
                1 | false | [Nn]* ) echo "${COLOR_RED} - User aborted required HomeBrew installation.${COLOR_NC}"; exit 1;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    else
        echo " - HomeBrew installation found @ ${BREW}"
    fi
}

function ensure-scl() {
    echo "${COLOR_CYAN}[Ensuring installation of Centos Software Collections Repository]${COLOR_NC}" # Needed for rh-python36
    SCL=$( rpm -qa | grep -E 'centos-release-scl-[0-9].*' || true )
    if [[ -z "${SCL}" ]]; then
        while true; do
            [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to install and enable the Centos Software Collections Repository? (y/n)?${COLOR_NC} " PROCEED
            echo ""
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* ) install-package centos-release-scl "--enablerepo=extras"; break;;
                1 | false | [Nn]* ) echo " - User aborted installation of required Centos Software Collections Repository."; exit 1;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    else
        echo " - ${SCL} found."
    fi
}

function ensure-devtoolset() {
    echo "${COLOR_CYAN}[Ensuring installation of devtoolset-8]${COLOR_NC}"
    DEVTOOLSET=$( rpm -qa | grep -E 'devtoolset-8-[0-9].*' || true )
    if [[ -z "${DEVTOOLSET}" ]]; then
        while true; do
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Not Found: Do you wish to install it? (y/n)?${COLOR_NC}" && read -p " " PROCEED
            echo ""
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* ) install-package devtoolset-8; break;;
                1 | false | [Nn]* ) echo " - User aborted installation of devtoolset-8."; break;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    else
        echo " - ${DEVTOOLSET} found."
    fi
}

function ensure-build-essential() {
    if [[ ! $(dpkg -s clang 2>/dev/null) ]]; then # Clang already exists, so no need for build essentials
        echo "${COLOR_CYAN}[Ensuring installation of build-essential]${COLOR_NC}"
        BUILD_ESSENTIAL=$( dpkg -s build-essential | grep 'Package: build-essential' || true )
        if [[ -z $BUILD_ESSENTIAL ]]; then
            while true; do
                [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to install it? (y/n)?${COLOR_NC}" && read -p " " PROCEED
                echo ""
                case $PROCEED in
                    "" ) echo "What would you like to do?";;
                    0 | true | [Yy]* )
                        install-package build-essential
                        echo " - ${COLOR_GREEN}Installed build-essential${COLOR_NC}"
                    break;;
                    1 | false | [Nn]* ) echo " - User aborted installation of build-essential."; break;;
                    * ) echo "Please type 'y' for yes or 'n' for no.";;
                esac
            done
        else
            echo " - ${BUILD_ESSENTIAL} found."
        fi
    fi
}

function ensure-yum-packages() {
    ( [[ -z "${1}" ]] || [[ ! -f "${1}" ]] ) && echo "\$1 must be the location of your dependency file!" && exit 1
    DEPS_FILE="${TEMP_DIR}/$(basename ${1})"
    # Create temp file so we can add to it
    cat $1 > $DEPS_FILE
    if [[ ! -z "${2}" ]]; then # Handle EXTRA_DEPS passed in and add them to temp DEPS_FILE
        printf "\n" >> $DEPS_FILE # Avoid needing a new line at the end of deps files
        OLDIFS="$IFS"; IFS=$''
        _2=("$(echo $2 | sed 's/-qa /-qa\n/g')")
        for ((i = 0; i < ${#_2[@]}; i++)); do echo "${_2[$i]}\n" | sed 's/-qa\\n/-qa/g' >> $DEPS_FILE; done
    fi
    while true; do
        [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to update YUM repositories? (y/n)?${COLOR_NC}" && read -p " " PROCEED
        echo ""
        case $PROCEED in
            "" ) echo "What would you like to do?";;
            0 | true | [Yy]* ) execute eval $( [[ $CURRENT_USER == "root" ]] || echo $SUDO_LOCATION -E ) $YUM -y update; break;;
            1 | false | [Nn]* ) echo " - Proceeding without update!"; break;;
            * ) echo "Please type 'y' for yes or 'n' for no.";;
        esac
    done
    echo "${COLOR_CYAN}[Ensuring package dependencies]${COLOR_NC}"
    OLDIFS="$IFS"; IFS=$','
    # || [[ -n "$testee" ]]; needed to see last line of deps file (https://stackoverflow.com/questions/12916352/shell-script-read-missing-last-line)
    while read -r testee tester || [[ -n "$testee" ]]; do
        if [[ ! -z $(eval $tester $testee) ]]; then
            echo " - ${testee} ${COLOR_GREEN}ok${COLOR_NC}"
        else
            DEPS=$DEPS"${testee} "
            echo " - ${testee} ${COLOR_RED}NOT${COLOR_NC} found!"
            (( COUNT+=1 ))
        fi
    done < $DEPS_FILE
    IFS=$OLDIFS
    OLDIFS="$IFS"; IFS=$' '
    echo ""
    if [[ $COUNT > 0 ]]; then
        while true; do
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to install missing dependencies? (y/n)?${COLOR_NC}" && read -p " " PROCEED
            echo ""
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* )
                    for DEP in $DEPS; do
                        install-package $DEP
                    done
                break;;
                1 | false | [Nn]* ) echo " ${COLOR_RED}- User aborted installation of required dependencies.${COLOR_NC}"; exit;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
        echo ""
    else
        echo "${COLOR_GREEN} - No required package dependencies to install.${COLOR_NC}"
        echo ""
    fi
    IFS=$OLDIFS
}

function ensure-brew-packages() {
    ( [[ -z "${1}" ]] || [[ ! -f "${1}" ]] ) && echo "\$1 must be the location of your dependency file!" && exit 1
    DEPS_FILE="${TEMP_DIR}/$(basename ${1})"
    # Create temp file so we can add to it
    cat $1 > $DEPS_FILE
    if [[ ! -z "${2}" ]]; then # Handle EXTRA_DEPS passed in and add them to temp DEPS_FILE
        printf "\n" >> $DEPS_FILE # Avoid needing a new line at the end of deps files
        OLDIFS="$IFS"; IFS=$''
        _2=("$(echo $2 | sed 's/-s /-s\n/g')")
        for ((i = 0; i < ${#_2[@]}; i++)); do echo "${_2[$i]}\n" | sed 's/-s\\n/-s/g' >> $DEPS_FILE; done
    fi
    echo "${COLOR_CYAN}[Ensuring HomeBrew dependencies]${COLOR_NC}"
    OLDIFS="$IFS"; IFS=$','
    # || [[ -n "$nmae" ]]; needed to see last line of deps file (https://stackoverflow.com/questions/12916352/shell-script-read-missing-last-line)
    while read -r name path || [[ -n "$name" ]]; do
        if [[ -f $path ]] || [[ -d $path ]]; then
            echo " - ${name} ${COLOR_GREEN}ok${COLOR_NC}"
            continue
        fi
        # resolve conflict with homebrew glibtool and apple/gnu installs of libtool
        if [[ "${testee}" == "/usr/local/bin/glibtool" ]]; then
            if [ "${tester}" "/usr/local/bin/libtool" ]; then
                echo " - ${name} ${COLOR_GREEN}ok${COLOR_NC}"
                continue
            fi
        fi
        DEPS=$DEPS"${name} "
        echo " - ${name} ${COLOR_RED}NOT${COLOR_NC} found!"
        (( COUNT+=1 ))
    done < $DEPS_FILE
    if [[ $COUNT > 0 ]]; then
        echo ""
        while true; do
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to install missing dependencies? (y/n)${COLOR_NC}" && read -p " " PROCEED
            echo ""
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* )
                    execute "${XCODESELECT}" --install 2>/dev/null || true
                    while true; do
                        [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to update HomeBrew packages first? (y/n)${COLOR_NC}" && read -p " " PROCEED
                        case $PROCEED in
                            "" ) echo "What would you like to do?";;
                            0 | true | [Yy]* ) echo "${COLOR_CYAN}[Updating HomeBrew]${COLOR_NC}" && execute brew update; break;;
                            1 | false | [Nn]* ) echo " - Proceeding without update!"; break;;
                            * ) echo "Please type 'y' for yes or 'n' for no.";;
                        esac
                    done
                    execute brew tap eosio/eosio
                    echo "${COLOR_CYAN}[Installing HomeBrew Dependencies]${COLOR_NC}"
                    execute eval $BREW install $DEPS
                    IFS="$OIFS"
                break;;
                1 | false | [Nn]* ) echo " ${COLOR_RED}- User aborted installation of required dependencies.${COLOR_NC}"; exit;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    else
        echo "${COLOR_GREEN} - No required package dependencies to install.${COLOR_NC}"
        echo ""
    fi
}

function apt-update-prompt() {
    if [[ $NAME == "Ubuntu" ]]; then
        while true; do # APT
            [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to update APT-GET repositories before proceeding? (y/n)?${COLOR_NC} " PROCEED
            echo ""
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* ) execute-always $APTGET update; break;;
                1 | false | [Nn]* ) echo " - Proceeding without update!"; break;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    fi
    true
}

function ensure-apt-packages() {
    ( [[ -z "${1}" ]] || [[ ! -f "${1}" ]] ) && echo "\$1 must be the location of your dependency file!" && exit 1
    DEPS_FILE="${TEMP_DIR}/$(basename ${1})"
    # Create temp file so we can add to it
    cat $1 > $DEPS_FILE
    if [[ ! -z "${2}" ]]; then # Handle EXTRA_DEPS passed in and add them to temp DEPS_FILE
        printf "\n" >> $DEPS_FILE # Avoid needing a new line at the end of deps files
        OLDIFS="$IFS"; IFS=$''
        _2=("$(echo $2 | sed 's/-s /-s\n/g')")
        for ((i = 0; i < ${#_2[@]}; i++)); do echo "${_2[$i]}" >> $DEPS_FILE; done
    fi
    echo "${COLOR_CYAN}[Ensuring package dependencies]${COLOR_NC}"
    OLDIFS="$IFS"; IFS=$','
    # || [[ -n "$testee" ]]; needed to see last line of deps file (https://stackoverflow.com/questions/12916352/shell-script-read-missing-last-line)
    while read -r testee tester || [[ -n "$testee" ]]; do
        if [[ ! -z $(eval $tester $testee 2>/dev/null) ]]; then
            echo " - ${testee} ${COLOR_GREEN}ok${COLOR_NC}"
        else
            DEPS=$DEPS"${testee} "
            echo " - ${testee} ${COLOR_RED}NOT${COLOR_NC} found!"
            (( COUNT+=1 ))
        fi
    done < $DEPS_FILE
    IFS=$OLDIFS
    OLDIFS="$IFS"; IFS=$' '
    if [[ $COUNT > 0 ]]; then
        echo ""
        while true; do
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to install missing dependencies? (y/n)?${COLOR_NC}" && read -p " " PROCEED
            echo ""
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* )
                    for DEP in $DEPS; do
                        install-package $DEP
                    done
                break;;
                1 | false | [Nn]* ) echo " ${COLOR_RED}- User aborted installation of required dependencies.${COLOR_NC}"; exit;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    else
        echo "${COLOR_GREEN} - No required package dependencies to install.${COLOR_NC}"
        echo ""
    fi
    IFS=$OLDIFS
}