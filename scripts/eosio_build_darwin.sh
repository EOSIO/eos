echo "OS name: ${NAME}"
echo "OS Version: ${OS_VER}"
echo "CPU cores: ${CPU_CORES}"
echo "Physical Memory: ${MEM_GIG}G"
echo "Disk install: ${DISK_INSTALL}"
echo "Disk space total: ${DISK_TOTAL}G"
echo "Disk space available: ${DISK_AVAIL}G"

[[ "${OS_MAX}" -eq 10 && "${OS_MIN}" -lt 14 ]] && echo "You must be running Mac OS 10.14.x or higher to install EOSIO." && exit 1

[[ $MEM_GIG -lt 7 ]] && echo "Your system must have 7 or more Gigabytes of physical memory installed." && exit 1
[[ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]] && echo " - You must have at least ${DISK_MIN}GB of available storage to install EOSIO." && exit 1

echo ""

echo "${COLOR_CYAN}[Ensuring xcode-select installation]${COLOR_NC}"
if ! XCODESELECT=$( command -v xcode-select ); then echo " - xcode-select must be installed in order to proceed!" && exit 1;
else echo " - xcode-select installation found @ ${XCODESELECT}"; fi

echo "${COLOR_CYAN}[Ensuring Ruby installation]${COLOR_NC}"
if ! RUBY=$( command -v ruby ); then echo " - Ruby must be installed in order to proceed!" && exit 1;
else echo " - Ruby installation found @ ${RUBY}"; fi

ensure-homebrew

if [ ! -d /usr/local/Frameworks ]; then
	echo "${COLOR_YELLOW}/usr/local/Frameworks is necessary to brew install python@3. Run the following commands as sudo and try again:${COLOR_NC}"
	echo "sudo mkdir /usr/local/Frameworks && sudo chown $(whoami):admin /usr/local/Frameworks"
	exit 1;
fi

# Handle clang/compiler
ensure-compiler
# Ensure packages exist
ensure-brew-packages "${REPO_ROOT}/scripts/eosio_build_darwin_deps"
[[ -z "${CMAKE}" ]] && export CMAKE="/usr/local/bin/cmake"
# CLANG Installation
build-clang
# BOOST Installation
ensure-boost
