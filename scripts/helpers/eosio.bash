# Obtain dependency versions; Must come first in the script
. ./scripts/.environment
# Load general helpers
. ./scripts/helpers/general.bash

# Checks for Arch and OS + Support for tests setting them manually
## Necessary for linux exclusion while running bats tests/bash-bats/*.bash
[[ -z "${ARCH}" ]] && export ARCH=$( uname )
if [[ -z "${NAME}" ]]; then
    if [[ $ARCH == "Linux" ]]; then 
        [[ ! -e /etc/os-release ]] && echo "${COLOR_RED} - /etc/os-release not found! It seems you're attempting to use an unsupported Linux distribution.${COLOR_NC}" && exit 1
        # Obtain OS NAME, and VERSION
        . /etc/os-release
    elif [[ $ARCH == "Darwin" ]]; then export NAME=$(sw_vers -productName)
    else echo " ${COLOR_RED}- EOSIO is not supported for your Architecture!${COLOR_NC}" && exit 1
    fi
fi

# Test that which is on the system before proceeding
which ls &>/dev/null || ( echo "${COLOR_RED}Please install the 'which' command before proceeding!${COLOR_NC}"; $DRYRUN || exit 1 )

function setup() {
    if $VERBOSE; then
        echo "DRYRUN: ${DRYRUN}"
        echo "TEMP_DIR: ${TEMP_DIR}"
        echo "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}"
        echo "CORE_SYMBOL_NAME: ${CORE_SYMBOL_NAME}"
        echo "BOOST_LOCATION: ${BOOST_LOCATION}"
        echo "INSTALL_LOCATION: ${INSTALL_LOCATION}"
        echo "EOSIO_HOME: ${EOSIO_HOME}"
        echo "NONINTERACTIVE: ${NONINTERACTIVE}"
        echo "PROCEED: ${PROCEED}"
        echo "ENABLE_COVERAGE_TESTING: ${ENABLE_COVERAGE_TESTING}"
        echo "ENABLE_DOXYGEN: ${ENABLE_DOXYGEN}"
        echo "ENABLE_MONGO: ${ENABLE_MONGO}"
        echo "INSTALL_MONGO: ${INSTALL_MONGO}"
        echo "PIN_COMPILER: ${PIN_COMPILER}"
        echo "BUILD_CLANG: ${BUILD_CLANG}"
    fi
    [[ -d ./build ]] && execute rm -rf ./build # cleanup old build directory
    # Be absolutely sure TEMP_DIR is set (we could do damage here)
    ## IF user set, they can be responsible for creating and then removing stale data
    if [[ $TEMP_DIR =~ "/tmp" ]]; then 
        execute mkdir -p $TEMP_DIR
        execute rm -rf $TEMP_DIR/*
    fi
    execute mkdir -p $SRC_LOCATION
    execute mkdir -p $OPT_LOCATION
    execute mkdir -p $VAR_LOCATION
    execute mkdir -p $BIN_LOCATION
    execute mkdir -p $VAR_LOCATION/log
    execute mkdir -p $ETC_LOCATION
    execute mkdir -p $LIB_LOCATION
    execute mkdir -p $MONGODB_LOG_LOCATION
    execute mkdir -p $MONGODB_DATA_LOCATION
}

function resources() {
    echo "${COLOR_CYAN}EOSIO website:${COLOR_NC} https://eos.io"
    echo "${COLOR_CYAN}EOSIO Telegram channel:${COLOR_NC} https://t.me/EOSProject"
    echo "${COLOR_CYAN}EOSIO resources:${COLOR_NC} https://eos.io/resources/"
    echo "${COLOR_CYAN}EOSIO Stack Exchange:${COLOR_NC} https://eosio.stackexchange.com"
}

function print_supported_linux_distros_and_exit() {
   echo "On Linux the EOSIO build script only supports Amazon, Centos, and Ubuntu."
   echo "Please install on a supported version of one of these Linux distributions."
   echo "https://aws.amazon.com/amazon-linux-ami/"
   echo "https://www.centos.org/"
   echo "https://www.ubuntu.com/"
   echo "Exiting now."
   exit 1
}

function prompt-mongo-install() {
    if $ENABLE_MONGO; then
        while true; do
            [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}You have chosen to include MongoDB support. Do you want for us to install MongoDB as well? (y/n)?${COLOR_NC} " PROCEED
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* ) export INSTALL_MONGO=true; break;;
                1 | false | [Nn]* ) echo "${COLOR_RED} - Existing MongoDB will be used.${COLOR_NC}"; exit 1;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    fi
}

function ensure-compiler() {
    CPP_COMP=${CXX:-c++}
    CC_COMP=${CC:-cc}
    echo $(which $CPP_COMP || echo "${COLOR_RED} - Unable to find compiler ${CPP_COMP}! ${COLOR_NC}") # which won't show "not found", so we force it
    COMPILER_TYPE=$(readlink $(which $CPP_COMP))
    [[ -z "${COMPILER_TYPE}" ]] && echo "COMPILER_TYPE not set!" && exit 1
    if [[ $COMPILER_TYPE == "clang++" ]]; then
        if [[ $(c++ --version | cut -d ' ' -f 1 | head -n 1) == "Apple" ]]; then
            ### Apple clang version 10
            [[ $(c++ --version | cut -d ' ' -f 4 | cut -d '.' -f 1 | head -n 1) -lt 10 ]] && export NO_CPP17=true
        else
            ### clang version 5
            [[ $(c++ --version | cut -d ' ' -f 4 | cut -d '.' -f 1 | head -n 1) -lt 5 ]] && export NO_CPP17=true
        fi
    else
        ### gcc version 7
        [[ $(c++ -dumpversion | cut -d '.' -f 1) -lt 7 ]] && export NO_CPP17=true
    fi
    if $PIN_COMPILER; then
        export BUILD_CLANG=true
        export CPP_COMP=$OPT_LOCATION/clang8/bin/clang++
        export CC_COMP=$OPT_LOCATION/clang8/bin/clang
    elif $NO_CPP17; then
        while true; do
            echo "${COLOR_YELLOW}Unable to find C++17 support!${COLOR_NC}"
            echo "If you already have a C++17 compiler installed or would like to install your own, export CXX to point to the compiler of your choosing."
            [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to download and build C++17? (y/n)?${COLOR_NC} " PROCEED
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* )
                    export BUILD_CLANG=true
                    export CPP_COMP=${OPT_LOCATION}/clang8/bin/clang++
                    export CC_COMP=${OPT_LOCATION}/clang8/bin/clang
                break;;
                1 | false | [Nn]* ) echo "${COLOR_RED} - User aborted C++17 installation!${COLOR_NC}"; exit 1;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    else
        echo "${COLOR_RED} - Unable to find CPP17 and user did not pin compiler (-P)!${COLOR_NC}"
        exit 1
    fi
    export CXX=$CPP_COMP
    export CC=$CC_COMP
}

