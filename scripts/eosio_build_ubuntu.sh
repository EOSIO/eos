echo "OS name: ${NAME}"
echo "OS Version: ${VERSION_ID}"
echo "CPU cores: ${CPU_CORES}"
echo "Physical Memory: ${MEM_GIG}G"
echo "Disk space total: ${DISK_TOTAL}G"
echo "Disk space available: ${DISK_AVAIL}G"

( [[ $NAME == "Ubuntu" ]] && ( [[ "$(echo ${VERSION_ID})" == "16.04" ]] || [[ "$(echo ${VERSION_ID})" == "18.04" ]] || [[ "$(echo ${VERSION_ID})" == "20.04" ]])  ) || ( echo " - You must be running 16.04.x or 18.04.x or 20.04.x to install EOSIO." && exit 1 )

[[ $MEM_GIG -lt 7 ]] && echo "Your system must have 7 or more Gigabytes of physical memory installed." && exit 1
[[ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]] && echo " - You must have at least ${DISK_MIN}GB of available storage to install EOSIO." && exit 1

# system clang and build essential for Ubuntu 18 (16 too old)
( [[ $PIN_COMPILER == false ]] && ( [[ $VERSION_ID == "18.04" ]] ||  [[ $VERSION_ID == "20.04" ]] ) ) && EXTRA_DEPS=(clang,dpkg\ -s llvm-7-dev,dpkg\ -s)
# We install clang8 for Ubuntu 16, but we still need something to compile cmake, boost, etc + pinned 18 still needs something to build source
( [[ $VERSION_ID == "16.04" ]] || ( $PIN_COMPILER && ( [[ $VERSION_ID == "18.04" ]] || [[ $VERSION_ID == "20.04" ]]) ) ) && ensure-build-essential
$ENABLE_COVERAGE_TESTING && EXTRA_DEPS+=(lcov,dpkg\ -s)
ensure-apt-packages "${REPO_ROOT}/scripts/eosio_build_ubuntu_deps" $(echo ${EXTRA_DEPS[@]})
echo ""
# Handle clang/compiler
ensure-compiler
# CMAKE Installation
ensure-cmake
# CLANG Installation
build-clang
# LLVM Installation
ensure-llvm
# BOOST Installation
ensure-boost
# `libpq` and `libpqxx` Installation
ensure-libpq-and-libpqxx
VERSION_MAJ=$(echo "${VERSION_ID}" | cut -d'.' -f1)
VERSION_MIN=$(echo "${VERSION_ID}" | cut -d'.' -f2)
