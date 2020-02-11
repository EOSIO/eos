# Checks for Arch and OS + Support for tests setting them manually
## Necessary for linux exclusion while running bats tests/bash-bats/*.sh
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

# Setup yum and apt variables
if [[ $NAME =~ "Amazon Linux" ]] || [[ $NAME == "CentOS Linux" ]]; then
    if ! YUM=$( command -v yum 2>/dev/null ); then echo "${COLOR_RED}YUM must be installed to compile EOSIO${COLOR_NC}" && exit 1; fi
elif [[ $NAME == "Ubuntu" ]]; then
    if ! APTGET=$( command -v apt-get 2>/dev/null ); then echo "${COLOR_RED}APT-GET must be installed to compile EOSIO${COLOR_NC}" && exit 1; fi
fi

# Obtain dependency versions; Must come first in the script
. ./scripts/.environment
# Load general helpers
. ./scripts/helpers/general.sh

function setup() {
    if $VERBOSE; then
        echo "VERBOSE: ${VERBOSE}"
        echo "DRYRUN: ${DRYRUN}"
        echo "TEMP_DIR: ${TEMP_DIR}"
        echo "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}"
        echo "CORE_SYMBOL_NAME: ${CORE_SYMBOL_NAME}"
        echo "BOOST_LOCATION: ${BOOST_LOCATION}"
        echo "INSTALL_LOCATION: ${INSTALL_LOCATION}"
        echo "BUILD_DIR: ${BUILD_DIR}"
        echo "EOSIO_INSTALL_DIR: ${EOSIO_INSTALL_DIR}"
        echo "NONINTERACTIVE: ${NONINTERACTIVE}"
        echo "PROCEED: ${PROCEED}"
        echo "ENABLE_COVERAGE_TESTING: ${ENABLE_COVERAGE_TESTING}"
        echo "ENABLE_DOXYGEN: ${ENABLE_DOXYGEN}"
        echo "ENABLE_MONGO: ${ENABLE_MONGO}"
        echo "INSTALL_MONGO: ${INSTALL_MONGO}"
        echo "SUDO_LOCATION: ${SUDO_LOCATION}"
        echo "PIN_COMPILER: ${PIN_COMPILER}"
    fi
    ( [[ -d $BUILD_DIR ]] && [[ -z $BUILD_DIR_CLEANUP_SKIP ]] ) && execute rm -rf $BUILD_DIR # cleanup old build directory; support disabling it (Zach requested)
    execute-always mkdir -p $TEMP_DIR
    execute mkdir -p $BUILD_DIR
    execute mkdir -p $SRC_DIR
    execute mkdir -p $OPT_DIR
    execute mkdir -p $VAR_DIR
    execute mkdir -p $BIN_DIR
    execute mkdir -p $VAR_DIR/log
    execute mkdir -p $ETC_DIR
    execute mkdir -p $LIB_DIR
    if $ENABLE_MONGO; then
        execute mkdir -p $MONGODB_LOG_DIR
        execute mkdir -p $MONGODB_DATA_DIR
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

# Prompt user for installation directory.
function install-directory-prompt() {
    if [[ -z $INSTALL_LOCATION ]]; then
        echo "No installation location was specified. Please provide the location where EOSIO is installed."
        while true; do
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to use the default location? ${EOSIO_INSTALL_DIR}? (y/n)${COLOR_NC}" && read -p " " PROCEED
            echo ""
            case $PROCEED in
                "" )
                echo "What would you like to do?";;
                0 | true | [Yy]* )
                break;;
                1 | false | [Nn]* )
                printf "Enter the desired installation location." && read -p " " EOSIO_INSTALL_DIR;
                export EOSIO_INSTALL_DIR;
                break;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    else
        # Support relative paths : https://github.com/EOSIO/eos/issues/7560
        [[ ! $INSTALL_LOCATION =~ ^\/ ]] && export INSTALL_LOCATION="${CURRENT_WORKING_DIR}/$INSTALL_LOCATION"
        export EOSIO_INSTALL_DIR="$INSTALL_LOCATION"
    fi
    . ./scripts/.build_vars
    echo "EOSIO will be installed to: ${EOSIO_INSTALL_DIR}"
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
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}You have chosen to include MongoDB support. Do you want for us to install MongoDB as well? (y/n)?${COLOR_NC}" && read -p " " PROCEED
            echo ""
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* ) export INSTALL_MONGO=true; break;;
                1 | false | [Nn]* ) echo "${COLOR_RED} - Existing MongoDB will be used.${COLOR_NC}"; break;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    fi
}

function ensure-compiler() {
    # Support build-essentials on ubuntu
    if [[ $NAME == "CentOS Linux" ]] || [[ $VERSION_ID == "16.04" ]] || ( $PIN_COMPILER && [[ $VERSION_ID == "18.04" ]] ); then
        export CXX=${CXX:-'g++'}
        export CC=${CC:-'gcc'}
    fi
    export CXX=${CXX:-'clang++'}
    export CC=${CC:-'clang'}
    if $PIN_COMPILER || [[ -f $CLANG_ROOT/bin/clang++ ]]; then
        export PIN_COMPILER=true
        export BUILD_CLANG=true
        export CPP_COMP=$CLANG_ROOT/bin/clang++
        export CC_COMP=$CLANG_ROOT/bin/clang
        export PATH=$CLANG_ROOT/bin:$PATH
    elif [[ $PIN_COMPILER == false ]]; then
        which $CXX &>/dev/null || ( echo "${COLOR_RED}Unable to find $CXX compiler: Pass in the -P option if you wish for us to install it or install a C++17 compiler and set \$CXX and \$CC to the proper binary locations. ${COLOR_NC}"; exit 1 )
        # readlink on mac differs from linux readlink (mac doesn't have -f)
        [[ $ARCH == "Linux" ]] && READLINK_COMMAND="readlink -f" || READLINK_COMMAND="readlink"
        COMPILER_TYPE=$( eval $READLINK_COMMAND $(which $CXX) || true )
        if [[ $CXX =~ "clang" ]] || [[ $COMPILER_TYPE =~ "clang" ]]; then
            if [[ $ARCH == "Darwin" ]]; then
                ### Check for apple clang version 10 or higher
                [[ $( $(which $CXX) --version | cut -d ' ' -f 4 | cut -d '.' -f 1 | head -n 1 ) -lt 10 ]] && export NO_CPP17=true
            else
                if [[ $( $(which $CXX) --version | cut -d ' ' -f 3 | head -n 1 | cut -d '.' -f1) =~ ^[0-9]+$ ]]; then # Check if the version message cut returns an integer
                    [[ $( $(which $CXX) --version | cut -d ' ' -f 3 | head -n 1 | cut -d '.' -f1) < 6 ]] && export NO_CPP17=true
                elif [[ $(clang --version | cut -d ' ' -f 4 | head -n 1 | cut -d '.' -f1) =~ ^[0-9]+$ ]]; then # Check if the version message cut returns an integer
                    [[ $( $(which $CXX) --version | cut -d ' ' -f 4 | cut -d '.' -f 1 | head -n 1 ) < 6 ]] && export NO_CPP17=true
                fi
            fi
        else
            ## Check for c++ version 7 or higher
            [[ $( $(which $CXX) -dumpversion | cut -d '.' -f 1 ) -lt 7 ]] && export NO_CPP17=true
            if [[ $NO_CPP17 == false ]]; then # https://github.com/EOSIO/eos/issues/7402
                while true; do
                    echo "${COLOR_YELLOW}WARNING: Your GCC compiler ($CXX) is less performant than clang (https://github.com/EOSIO/eos/issues/7402). We suggest running the build script with -P or install your own clang and try again.${COLOR_NC}"
                    [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to proceed anyway? (y/n)?${COLOR_NC}" && read -p " " PROCEED
                    case $PROCEED in
                        "" ) echo "What would you like to do?";;
                        0 | true | [Yy]* ) break;;
                        1 | false | [Nn]* ) exit 1;;
                        * ) echo "Please type 'y' for yes or 'n' for no.";;
                    esac
                done
            fi
        fi
    fi
    if $NO_CPP17; then
        while true; do
            echo "${COLOR_YELLOW}Unable to find C++17 support in ${CXX}!${COLOR_NC}"
            echo "If you already have a C++17 compiler installed or would like to install your own, export CXX to point to the compiler of your choosing."
            [[ $NONINTERACTIVE == false ]] && printf "${COLOR_YELLOW}Do you wish to download and build C++17? (y/n)?${COLOR_NC}" && read -p " " PROCEED
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* )
                    export PIN_COMPILER=true
                    export BUILD_CLANG=true
                    export CPP_COMP=$CLANG_ROOT/bin/clang++
                    export CC_COMP=$CLANG_ROOT/bin/clang
                    export PATH=$CLANG_ROOT/bin:$PATH
                break;;
                1 | false | [Nn]* ) echo "${COLOR_RED} - User aborted C++17 installation!${COLOR_NC}"; exit 1;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    fi
    $BUILD_CLANG && export PINNED_TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE='${BUILD_DIR}/pinned_toolchain.cmake'"
    echo ""
}

function ensure-cmake() {
    echo "${COLOR_CYAN}[Ensuring CMAKE installation]${COLOR_NC}"
    if [[ ! -e "${CMAKE}" ]]; then
		execute bash -c "cd $SRC_DIR && \
        curl -LO https://cmake.org/files/v${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR}/cmake-${CMAKE_VERSION}.tar.gz \
        && tar -xzf cmake-${CMAKE_VERSION}.tar.gz \
        && cd cmake-${CMAKE_VERSION} \
        && ./bootstrap --prefix=${EOSIO_INSTALL_DIR} \
        && make -j${JOBS} \
        && make install \
        && cd .. \
        && rm -f cmake-${CMAKE_VERSION}.tar.gz"
        [[ -z "${CMAKE}" ]] && export CMAKE="${BIN_DIR}/cmake"
        echo " - CMAKE successfully installed @ ${CMAKE}"
        echo ""
    else
        echo " - CMAKE found @ ${CMAKE}."
        echo ""
    fi
}

function ensure-boost() {
    [[ $ARCH == "Darwin" ]] && export CPATH="$(python-config --includes | awk '{print $1}' | cut -dI -f2):$CPATH" # Boost has trouble finding pyconfig.h
    echo "${COLOR_CYAN}[Ensuring Boost $( echo $BOOST_VERSION | sed 's/_/./g' ) library installation]${COLOR_NC}"
    BOOSTVERSION=$( grep "#define BOOST_VERSION" "$BOOST_ROOT/include/boost/version.hpp" 2>/dev/null | tail -1 | tr -s ' ' | cut -d\  -f3 || true )
    if [[ "${BOOSTVERSION}" != "${BOOST_VERSION_MAJOR}0${BOOST_VERSION_MINOR}0${BOOST_VERSION_PATCH}" ]]; then
        B2_FLAGS="-q -j${JOBS} --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test install"
        BOOTSTRAP_FLAGS=""
        if [[ $ARCH == "Linux" ]] && $PIN_COMPILER; then
            B2_FLAGS="toolset=clang cxxflags='-stdlib=libc++ -D__STRICT_ANSI__ -nostdinc++ -I${CLANG_ROOT}/include/c++/v1' linkflags='-stdlib=libc++' link=static threading=multi --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j${JOBS} install"
            BOOTSTRAP_FLAGS="--with-toolset=clang"
        fi
		execute bash -c "cd $SRC_DIR && \
        curl -LO https://dl.bintray.com/boostorg/release/$BOOST_VERSION_MAJOR.$BOOST_VERSION_MINOR.$BOOST_VERSION_PATCH/source/boost_$BOOST_VERSION.tar.bz2 \
        && tar -xjf boost_$BOOST_VERSION.tar.bz2 \
        && cd $BOOST_ROOT \
        && ./bootstrap.sh ${BOOTSTRAP_FLAGS} --prefix=$BOOST_ROOT \
        && ./b2 ${B2_FLAGS} \
        && cd .. \
        && rm -f boost_$BOOST_VERSION.tar.bz2 \
        && rm -rf $BOOST_LINK_LOCATION"        
        echo " - Boost library successfully installed @ ${BOOST_ROOT}"
        echo ""
    else
        echo " - Boost library found with correct version @ ${BOOST_ROOT}"
        echo ""
    fi
}

function ensure-llvm() {
    echo "${COLOR_CYAN}[Ensuring LLVM 4 support]${COLOR_NC}"
    if [[ ! -d $LLVM_ROOT ]]; then
        if $PIN_COMPILER || $BUILD_CLANG; then
            CMAKE_FLAGS="-DCMAKE_INSTALL_PREFIX='${LLVM_ROOT}' -DLLVM_TARGETS_TO_BUILD=host -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE='${BUILD_DIR}/pinned_toolchain.cmake' .."
        else
            if [[ $NAME == "Ubuntu" ]]; then
                execute ln -s /usr/lib/llvm-4.0 $LLVM_ROOT
                echo " - LLVM successfully linked from /usr/lib/llvm-4.0 to ${LLVM_ROOT}"
                return 0
            fi
            CMAKE_FLAGS="-G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX=${LLVM_ROOT} -DLLVM_TARGETS_TO_BUILD='host' -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE=Release .."
        fi
        execute bash -c "cd ${OPT_DIR} \
        && git clone --depth 1 --single-branch --branch $LLVM_VERSION https://github.com/llvm-mirror/llvm.git llvm && cd llvm \
        && mkdir build \
        && cd build \
        && ${CMAKE} ${CMAKE_FLAGS} \
        && make -j${JOBS} \
        && make install"
        echo " - LLVM successfully installed @ ${LLVM_ROOT}"
        echo ""
    else
        echo " - LLVM found @ ${LLVM_ROOT}."
        echo ""
    fi
}


function build-clang() {
    if $BUILD_CLANG; then
        echo "${COLOR_CYAN}[Ensuring Clang support]${COLOR_NC}"
        if [[ ! -d $CLANG_ROOT ]]; then
            execute bash -c "cd ${TEMP_DIR} \
            && rm -rf clang8 \
            && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/llvm.git clang8 \
            && cd clang8 && git checkout $PINNED_COMPILER_LLVM_COMMIT \
            && cd tools \
            && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/lld.git \
            && cd lld && git checkout $PINNED_COMPILER_LLD_COMMIT && cd ../ \
            && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/polly.git \
            && cd polly && git checkout $PINNED_COMPILER_POLLY_COMMIT && cd ../ \
            && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/clang.git clang && cd clang \
            && git checkout $PINNED_COMPILER_CLANG_COMMIT \
            && patch -p2 < \"$REPO_ROOT/scripts/clang-devtoolset8-support.patch\" \
            && cd tools && mkdir extra && cd extra \
            && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/clang-tools-extra.git \
            && cd clang-tools-extra && git checkout $PINNED_COMPILER_CLANG_TOOLS_EXTRA_COMMIT && cd .. \
            && cd ../../../../projects \
            && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/libcxx.git \
            && cd libcxx && git checkout $PINNED_COMPILER_LIBCXX_COMMIT && cd ../ \
            && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/libcxxabi.git \
            && cd libcxxabi && git checkout $PINNED_COMPILER_LIBCXXABI_COMMIT && cd ../ \
            && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/libunwind.git \
            && cd libunwind && git checkout $PINNED_COMPILER_LIBUNWIND_COMMIT && cd ../ \
            && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/compiler-rt.git \
            && cd compiler-rt && git checkout $PINNED_COMPILER_COMPILER_RT_COMMIT && cd ../ \
            && cd ${TEMP_DIR}/clang8 \
            && mkdir build && cd build \
            && ${CMAKE} -G 'Unix Makefiles' -DCMAKE_INSTALL_PREFIX='${CLANG_ROOT}' -DLLVM_BUILD_EXTERNAL_COMPILER_RT=ON -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_LIBCXX=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_TARGETS_TO_BUILD=all -DCMAKE_BUILD_TYPE=Release .. \
            && make -j${JOBS} \
            && make install \
            && rm -rf ${TEMP_DIR}/clang8"
            echo " - Clang 8 successfully installed @ ${CLANG_ROOT}"
            echo ""
        else
            echo " - Clang 8 found @ ${CLANG_ROOT}"
            echo ""
        fi
        export CXX=$CPP_COMP
        export CC=$CC_COMP
    fi
}
