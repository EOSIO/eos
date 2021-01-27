echo "OS name: ${NAME}"
echo "OS Version: ${VERSION_ID}"
echo "CPU cores: ${CPU_CORES}"
echo "Physical Memory: ${MEM_GIG}G"
echo "Disk space total: ${DISK_TOTAL}G"
echo "Disk space available: ${DISK_AVAIL}G"

if [[ "${NAME}" == "Amazon Linux" ]] && [[ $VERSION == 2 ]]; then
	DEPS_FILE="${REPO_ROOT}/scripts/eosio_build_amazonlinux2_deps"
else
	echo " - You must be running Amazon Linux 2017.09 or higher to install EOSIO." && exit 1
fi

[[ $MEM_GIG -lt 7 ]] && echo "Your system must have 7 or more Gigabytes of physical memory installed." && exit 1
[[ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]] && echo " - You must have at least ${DISK_MIN}GB of available storage to install EOSIO." && exit 1

# Ensure packages exist
($PIN_COMPILER && $BUILD_CLANG) && EXTRA_DEPS=(gcc-c++,rpm\ -qa)
ensure-yum-packages $DEPS_FILE $(echo ${EXTRA_DEPS[@]})
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

echo ""
