# Obtain dependency versions; Must come first in the script
. ./scripts/.environment
# Load general helpers
. ./scripts/helpers/general.bash
# Load helper for functions that allow us to obtain cores, memory, disk space, etc
. ./scripts/helpers/system.bash

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

[[ $ARCH == "Darwin" ]] && export CTEST_BIN=ctest || export CTEST_BIN=$BIN_DIR/ctest

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
        echo "BUILD_DIR: ${BUILD_DIR}"
        echo "EOSIO_INSTALL_DIR: ${EOSIO_INSTALL_DIR}"
        echo "NONINTERACTIVE: ${NONINTERACTIVE}"
        echo "PROCEED: ${PROCEED}"
        echo "ENABLE_COVERAGE_TESTING: ${ENABLE_COVERAGE_TESTING}"
        echo "ENABLE_DOXYGEN: ${ENABLE_DOXYGEN}"
        echo "ENABLE_MONGO: ${ENABLE_MONGO}"
        echo "INSTALL_MONGO: ${INSTALL_MONGO}"
        echo "PIN_COMPILER: ${PIN_COMPILER}"
        echo "BUILD_CLANG: ${BUILD_CLANG}"
    fi
    [[ -d $BUILD_DIR ]] && execute rm -rf $BUILD_DIR # cleanup old build directory
    # Be absolutely sure TEMP_DIR is set (we could do damage here)
    ## IF user set, they can be responsible for creating and then removing stale data
    if [[ $TEMP_DIR =~ "/tmp" ]]; then
        if [[ ! -d $TEMP_DIR ]]; then
            DRYRUN_ORIGINAL=$DRYRUN
            DRYRUN=false
            execute mkdir -p $TEMP_DIR
            execute rm -rf $TEMP_DIR/*
            DRYRUN=$DRYRUN_ORIGINAL
        fi
    fi
    execute mkdir -p $BUILD_DIR
    execute mkdir -p $SRC_DIR
    execute mkdir -p $OPT_DIR
    execute mkdir -p $VAR_DIR
    execute mkdir -p $BIN_DIR
    execute mkdir -p $VAR_DIR/log
    execute mkdir -p $ETC_DIR
    execute mkdir -p $LIB_DIR
    execute mkdir -p $MONGODB_LOG_DIR
    execute mkdir -p $MONGODB_DATA_DIR
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
                1 | false | [Nn]* ) echo "${COLOR_RED} - Existing MongoDB will be used.${COLOR_NC}"; break;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    fi
}

function ensure-compiler() {
    export CXX=${CXX:-c++}
    export CC=${CC:-cc}
    if [[ $PIN_COMPILER == false ]]; then
        which $CXX &>/dev/null || ( echo "${COLOR_RED} - Unable to find compiler \"${CXX}\"! Pass in the -P option if you wish for us to install it OR set \$CXX to the proper binary. ${COLOR_NC}"; exit 1 )
        # readlink on mac differs from linux readlink (mac doesn't have -f)
        [[ $ARCH == "Linux" ]] && READLINK_COMMAND="readlink -f" || READLINK_COMMAND="readlink"
        COMPILER_TYPE=$( eval $READLINK_COMMAND $(which $CXX) )
        [[ -z "${COMPILER_TYPE}" ]] && echo "${COLOR_RED}COMPILER_TYPE not set!${COLOR_NC}" && exit 1
        if [[ $COMPILER_TYPE == "clang++" ]]; then
            if [[ $ARCH == "Darwin" ]]; then
                ### Check for apple clang version 10 or higher
                [[ $( $(which $CXX) --version | cut -d ' ' -f 4 | cut -d '.' -f 1 | head -n 1 ) -lt 10 ]] && export NO_CPP17=true
            else
                ### Check for clang version 5 or higher
                [[ $( $(which $CXX) --version | cut -d ' ' -f 4 | cut -d '.' -f 1 | head -n 1 ) -lt 5 ]] && export NO_CPP17=true
            fi
        else
            ## Check for c++ version 7 or higher
            [[ $( $(which $CXX) -dumpversion | cut -d '.' -f 1 ) -lt 7 ]] && export NO_CPP17=true
        fi
    elif $PIN_COMPILER; then
        export BUILD_CLANG=true
        export CPP_COMP=$CLANG_ROOT/bin/clang++
        export CC_COMP=$CLANG_ROOT/bin/clang
        export PATH=$CLANG_ROOT/bin:$PATH
    fi
    if $NO_CPP17; then
        while true; do
            echo "${COLOR_YELLOW}Unable to find C++17 support!${COLOR_NC}"
            echo "If you already have a C++17 compiler installed or would like to install your own, export CXX to point to the compiler of your choosing."
            [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to download and build C++17? (y/n)?${COLOR_NC} " PROCEED
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
        if [[ $ARCH == "Linux" ]] && $PIN_COMPILER; then
            B2_FLAGS="toolset=clang cxxflags='-stdlib=libc++ -D__STRICT_ANSI__ -nostdinc++ -I${CLANG_ROOT}/include/c++/v1' linkflags='-stdlib=libc++' link=static threading=multi --with-iostreams --with-date_time --with-filesystem --with-system --with-program_options --with-chrono --with-test -q -j${JOBS} install"
        fi
		execute bash -c "cd $SRC_DIR && \
        curl -LO https://dl.bintray.com/boostorg/release/$BOOST_VERSION_MAJOR.$BOOST_VERSION_MINOR.$BOOST_VERSION_PATCH/source/boost_$BOOST_VERSION.tar.bz2 \
        && tar -xjf boost_$BOOST_VERSION.tar.bz2 \
        && cd $BOOST_ROOT \
        && ./bootstrap.sh --prefix=$BOOST_ROOT \
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
        if [[ $ARCH == "Darwin" ]]; then # Handle brew installed llvm@4
	        execute ln -s /usr/local/opt/llvm@4 $LLVM_ROOT
	        echo " - LLVM successfully linked from /usr/local/opt/llvm@4 to ${LLVM_ROOT}"
        else
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
        fi
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
            && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/clang.git clang && cd clang/tools \
            && git checkout $PINNED_COMPILER_CLANG_VERSION \
            && mkdir extra && cd extra \
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
            && rm -rf ${TEMP_DIR}/*"
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

function ensure-yum-packages() {
    ( [[ -z "${1}" ]] || [[ ! -f "${1}" ]] ) && echo "\$1 must be the location of your dependency file!" && exit 1
    DEPS_FILE="${TEMP_DIR}/$(basename ${1})"
    # Create temp file so we can add to it
    cat $1 > $DEPS_FILE
    if [[ -n "${2}" ]]; then # Handle EXTRA_DEPS passed in and add them to temp DEPS_FILE
        OLDIFS="$IFS"; IFS=$''
        _2=("$(echo $2 | sed 's/-s /-s\n/g')")
        for ((i = 0; i < ${#_2[@]}; i++)); do echo "${_2[$i]}\n" | sed 's/-s\\n/-s/g' >> $DEPS_FILE; done
    fi
    echo "${COLOR_CYAN}[Ensuring YUM installation]${COLOR_NC}"
    if ! YUM=$( command -v yum 2>/dev/null ); then echo " - YUM must be installed to compile EOS.IO." && exit 1
    else echo "Yum installation found at ${YUM}."; fi
    while true; do
        [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to update YUM repositories? (y/n)?${COLOR_NC} " PROCEED
        case $PROCEED in
            "" ) echo "What would you like to do?";;
            0 | true | [Yy]* )
                if ! execute $( [[ $CURRENT_USER == "root" ]] || echo /usr/bin/sudo -E ) $YUM -y update; then
                    echo " - ${COLOR_RED}YUM update failed.${COLOR_NC}"
                    exit 1;
                else
                    echo " - ${COLOR_GREEN}YUM update complete.${COLOR_NC}"
                fi
            break;;
            1 | false | [Nn]* ) echo " - Proceeding without update!"; break;;
            * ) echo "Please type 'y' for yes or 'n' for no.";;
        esac
    done
    if [[ $NAME == "CentOS Linux" ]]; then
        echo "${COLOR_CYAN}[Ensuring installation of Centos Software Collections Repository]${COLOR_NC}" # Needed for rh-python36
        SCL=$( rpm -qa | grep -E 'centos-release-scl-[0-9].*' || true )
        if [[ -z "${SCL}" ]]; then
            while true; do
                [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to install and enable the Centos Software Collections Repository? (y/n)?${COLOR_NC} " PROCEED
                case $PROCEED in
                    "" ) echo "What would you like to do?";;
                    0 | true | [Yy]* )
                        echo "Installing Centos Software Collections Repository..."
                        if ! execute $( [[ $CURRENT_USER == "root" ]] || echo /usr/bin/sudo -E ) "${YUM}" -y --enablerepo=extras install centos-release-scl; then
                            echo " - Centos Software Collections Repository installation failed." && exit 1;
                        else
                            echo " - Centos Software Collections Repository installed successfully."
                        fi
                    break;;
                    1 | false | [Nn]* ) echo " - User aborted installation of required Centos Software Collections Repository."; exit;;
                    * ) echo "Please type 'y' for yes or 'n' for no.";;
                esac
            done
        else
            echo " - ${SCL} found."
        fi
    fi
    echo "${COLOR_CYAN}[Ensuring for installed package dependencies]${COLOR_NC}"
    OLDIFS="$IFS"; IFS=$','
    while read -r testee tester; do
        if [[ ! -z $(eval $tester $testee) ]]; then
            echo " - ${testee} ${COLOR_GREEN}found!${COLOR_NC}"
        else
            DEPS=$DEPS"${testee} "
            echo " - ${testee} ${COLOR_RED}NOT${COLOR_NC} found."
            (( COUNT++ ))
        fi
    done < $DEPS_FILE
    IFS=$OLDIFS
    echo ""
    if [ "${COUNT}" -gt 1 ]; then
        while true; do
            [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to install missing dependencies? (y/n)?${COLOR_NC} " PROCEED
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* )
                    execute eval $( [[ $CURRENT_USER == "root" ]] || echo /usr/bin/sudo -E ) $YUM -y install $DEPS
                break;;
                1 | false | [Nn]* ) echo " ${COLOR_RED}- User aborted installation of required dependencies.${COLOR_NC}"; exit;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    else
        echo " - No required package dependencies to install."
        echo ""
    fi
}

function ensure-brew-packages() {
    ( [[ -z "${1}" ]] || [[ ! -f "${1}" ]] ) && echo "\$1 must be the location of your dependency file!" && exit 1
    DEPS_FILE="${TEMP_DIR}/$(basename ${1})"
    # Create temp file so we can add to it
    cat $1 > $DEPS_FILE
    if [[ -n "${2}" ]]; then # Handle EXTRA_DEPS passed in and add them to temp DEPS_FILE
        OLDIFS="$IFS"; IFS=$''
        _2=("$(echo $2 | sed 's/-s /-s\n/g')")
        for ((i = 0; i < ${#_2[@]}; i++)); do echo "${_2[$i]}\n" | sed 's/-s\\n/-s/g' >> $DEPS_FILE; done
    fi
    echo "${COLOR_CYAN}[Ensuring HomeBrew dependencies]${COLOR_NC}"
    OLDIFS="$IFS"
    IFS=$','
    while read -r name path; do
        if [[ -f $path ]] || [[ -d $path ]]; then
            echo " - ${name} ${COLOR_GREEN}found!${COLOR_NC}"
            continue
        fi
        # resolve conflict with homebrew glibtool and apple/gnu installs of libtool
        if [[ "${testee}" == "/usr/local/bin/glibtool" ]]; then
            if [ "${tester}" "/usr/local/bin/libtool" ]; then
                echo " - ${name} ${COLOR_GREEN}found!${COLOR_NC}"
                continue
            fi
        fi
        DEPS=$DEPS"${name} "
        echo " - ${name} ${COLOR_RED}NOT${COLOR_NC} found."
        (( COUNT++ ))
    done < $DEPS_FILE
    if [ $COUNT -gt 1 ]; then
        echo ""
        while true; do
            [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to install missing dependencies? (y/n)${COLOR_NC} " PROCEED
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* )
                    execute "${XCODESELECT}" --install 2>/dev/null || true
                    while true; do
                        [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to update HomeBrew packages first? (y/n)${COLOR_NC} " PROCEED
                        case $PROCEED in
                            "" ) echo "What would you like to do?";;
                            0 | true | [Yy]* ) echo "[Updating HomeBrew]" && execute brew update; break;;
                            1 | false | [Nn]* ) echo " - Proceeding without update!"; break;;
                            * ) echo "Please type 'y' for yes or 'n' for no.";;
                        esac
                    done
                    execute brew tap eosio/eosio
                    echo "${COLOR_GREEN}[Installing HomeBrew Dependencies]${COLOR_NC}"
                    execute eval $BREW install $DEPS
                    IFS="$OIFS"
                break;;
                1 | false | [Nn]* ) echo " ${COLOR_RED}- User aborted installation of required dependencies.${COLOR_NC}"; exit;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    else
        echo " - No required package dependencies to install."
        echo ""
    fi
}

function ensure-apt-packages() {
    ( [[ -z "${1}" ]] || [[ ! -f "${1}" ]] ) && echo "\$1 must be the location of your dependency file!" && exit 1
    DEPS_FILE="${TEMP_DIR}/$(basename ${1})"
    # Create temp file so we can add to it
    cat $1 > $DEPS_FILE
    if [[ -n "${2}" ]]; then # Handle EXTRA_DEPS passed in and add them to temp DEPS_FILE
        OLDIFS="$IFS"; IFS=$''
        _2=("$(echo $2 | sed 's/-s /-s\n/g')")
        for ((i = 0; i < ${#_2[@]}; i++)); do echo "${_2[$i]}" >> $DEPS_FILE; done
    fi
    echo "${COLOR_CYAN}[Ensuring for installed package dependencies]${COLOR_NC}"
    OLDIFS="$IFS"; IFS=$','
    while read -r testee tester; do
        if [[ ! -z $(eval $tester $testee 2>/dev/null) ]]; then
            echo " - ${testee} ${COLOR_GREEN}found!${COLOR_NC}"
        else
            DEPS=$DEPS"${testee} "
            echo " - ${testee} ${COLOR_RED}NOT${COLOR_NC} found."
            (( COUNT++ ))
        fi
    done < $DEPS_FILE
    IFS=$OLDIFS
    if [[ $COUNT > 0 ]]; then
        echo ""
        while true; do
            [[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to install missing dependencies? (y/n)?${COLOR_NC} " PROCEED
            case $PROCEED in
                "" ) echo "What would you like to do?";;
                0 | true | [Yy]* )
                    execute eval $( [[ $CURRENT_USER == "root" ]] || echo /usr/bin/sudo -E ) $APTGET -y install $DEPS
                break;;
                1 | false | [Nn]* ) echo " ${COLOR_RED}- User aborted installation of required dependencies.${COLOR_NC}"; exit;;
                * ) echo "Please type 'y' for yes or 'n' for no.";;
            esac
        done
    else
        echo " - No required package dependencies to install."
        echo ""
    fi
}