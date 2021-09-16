echo "OS name: ${NAME}"
echo "OS Version: ${VERSION_ID}"
echo "CPU cores: ${CPU_CORES}"
echo "Physical Memory: ${MEM_GIG}G"
echo "Disk space total: ${DISK_TOTAL}G"
echo "Disk space available: ${DISK_AVAIL}G"

( [[ $NAME == "Oracle Linux Server" ]] && [[ "$(echo ${VERSION} | sed 's/ .*//g')" < 8 ]] ) && echo " - You must be running Oracle Linux 8 or higher to install EOSIO." && exit 1
[[ $MEM_GIG -lt 7 ]] && echo "Your system must have 7 or more Gigabytes of physical memory installed." && exit 1
[[ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]] && echo " - You must have at least ${DISK_MIN}GB of available storage to install EOSIO." && exit 1

echo ""

# Ensure packages exist
ensure-yum-packages "${REPO_ROOT}/scripts/eosio_build_oraclelinux8_deps"

dnf update && dnf install -y cmake
if [[  $PIN_COMPILER == "true" ]]; then
	yum install -y gcc-c++;
else
	# for unpinned builds, install clang as well
	dnf install -y clang llvm llvm-devel llvm-toolset;
fi

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
# Build doxygen 
build-doxygen