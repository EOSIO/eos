( [[ "${NAME}" == "Ubuntu" ]] && ( [[ "$(echo ${VERSION_ID})" == "16.04" ]] || [[ "$(echo ${VERSION_ID})" == "18.04" ]] )  ) || ( echo " - You must be running 16.04.x or 18.04.x to install EOSIO." && exit 1 )

DISK_INSTALL=$( df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 )
DISK_TOTAL_KB=$( df . | tail -1 | awk '{print $2}' )
DISK_AVAIL_KB=$( df . | tail -1 | awk '{print $4}' )
DISK_TOTAL=$(( DISK_TOTAL_KB / 1048576 ))
DISK_AVAIL=$(( DISK_AVAIL_KB / 1048576 ))
[[ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]] && echo " - You must have at least ${DISK_MIN}GB of available storage to install EOSIO." && exit 1
MEM_GIG=$(( ( ( $(cat /proc/meminfo | grep MemTotal | awk '{print $2}') / 1000 ) / 1000 ) ))
export JOBS=$(( MEM_GIG > CPU_CORES ? CPU_CORES : MEM_GIG ))

echo "OS name: ${NAME}"
echo "OS Version: ${VERSION_ID}"
echo "CPU cores: ${CPU_CORES}"
echo "Physical Memory: ${MEM_GIG}Gb"
echo "Disk space total: ${DISK_TOTAL}Gb"
echo "Disk space available: ${DISK_AVAIL}G"

[[ $MEM_GIG -lt 7 ]] && echo "Your system must have 7 or more Gigabytes of physical memory installed." && exit 1

echo "${COLOR_CYAN}[Checking APT-GET installation]${COLOR_NC}"
if ! APTGET=$( command -v apt-get 2>/dev/null ); then echo " - APT-GET must be installed to compile EOS.IO." && exit 1
else echo " - APT-GET installation found at ${APTGET}."; fi

while true; do
	[[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to update APT-GET repositories? (y/n)?${COLOR_NC} " PROCEED
	case $PROCEED in
		"" ) echo "What would you like to do?";;
		0 | true | [Yy]* )
			if ! execute $( [[ $CURRENT_USER == "root" ]] || echo sudo ) $APTGET update; then
				echo " - ${COLOR_RED}APT-GET update failed.${COLOR_NC}"
				exit 1;
			else
				echo " - ${COLOR_GREEN}APT-GET update complete.${COLOR_NC}"
			fi
		break;;
		1 | false | [Nn]* ) echo " - Proceeding without update!"; break;;
		* ) echo "Please type 'y' for yes or 'n' for no.";;
	esac
done

# Ensure packages exist
[[ $PIN_COMPILER == false ]] && EXTRA_DEPS=(llvm-4.0,dpkg\ -s libclang-4.0-dev,dpkg\ -s)
$ENABLE_COVERAGE_TESTING && EXTRA_DEPS+=(lcov,dpkg\ -s)
ensure-apt-packages "${REPO_ROOT}/scripts/eosio_build_ubuntu_deps" "$(echo ${EXTRA_DEPS[@]})"
echo ""
# CMAKE Installation
ensure-cmake
echo ""
# CLANG Installation
build-clang
echo ""
# LLVM Installation
ensure-llvm
echo ""
# ZLIB Installation
ensure-zlib
echo ""
# BOOST Installation
ensure-boost
echo ""

VERSION_MAJ=$(echo "${VERSION_ID}" | cut -d'.' -f1)
VERSION_MIN=$(echo "${VERSION_ID}" | cut -d'.' -f2)
if $INSTALL_MONGO; then

	$BUILD_CLANG && PINNED_TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=$BUILD_DIR/pinned_toolchain.cmake" # if we've pinned the compiler with -P

	if [[ $VERSION_MAJ == 18 ]]; then
		# UBUNTU 18 doesn't have MONGODB 3.6.3
		MONGODB_VERSION=4.1.1
		# We have to re-set this with the new version
		MONGODB_ROOT=${OPT_LOCATION}/mongodb-${MONGODB_VERSION}
	fi
	echo "${COLOR_CYAN}[Checking MongoDB installation]${COLOR_NC}"
	if [ ! -d $MONGODB_ROOT ]; then
		echo "Installing MongoDB into ${MONGODB_ROOT}..."
		execute bash -c "curl -OL http://downloads.mongodb.org/linux/mongodb-linux-x86_64-ubuntu$VERSION_MAJ$VERSION_MIN-$MONGODB_VERSION.tgz \
		&& tar -xzf mongodb-linux-x86_64-ubuntu$VERSION_MAJ$VERSION_MIN-$MONGODB_VERSION.tgz \
		&& mv $SRC_LOCATION/mongodb-linux-x86_64-ubuntu$VERSION_MAJ$VERSION_MIN-$MONGODB_VERSION $MONGODB_ROOT \
		&& touch $MONGODB_LOG_LOCATION/mongod.log \
		&& rm -f mongodb-linux-x86_64-ubuntu$VERSION_MAJ$VERSION_MIN-$MONGODB_VERSION.tgz \
		&& cp -f $REPO_ROOT/scripts/mongod.conf $MONGODB_CONF \
		&& mkdir -p $MONGODB_DATA_LOCATION \
		&& rm -rf $MONGODB_LINK_LOCATION \
		&& rm -rf $BIN_LOCATION/mongod \
		&& ln -s $MONGODB_ROOT $MONGODB_LINK_LOCATION \
		&& ln -s $MONGODB_LINK_LOCATION/bin/mongod $BIN_LOCATION/mongod"
		echo " - MongoDB successfully installed @ ${MONGODB_ROOT} (Symlinked to ${MONGODB_LINK_LOCATION})."
	else
		echo " - MongoDB found with correct version @ ${MONGODB_ROOT} (Symlinked to ${MONGODB_LINK_LOCATION})."
	fi
	echo "${COLOR_CYAN}[Checking MongoDB C driver installation]${COLOR_NC}"
	if [[ ! -d $MONGO_C_DRIVER_ROOT ]]; then
		echo "Installing MongoDB C driver..."
		execute bash -c "curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/$MONGO_C_DRIVER_VERSION/mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
		&& tar -xzf mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
		&& cd mongo-c-driver-$MONGO_C_DRIVER_VERSION \
		&& mkdir -p cmake-build \
		&& cd cmake-build \
		&& $CMAKE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_HOME -DENABLE_BSON=ON -DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF $PINNED_TOOLCHAIN .. \
		&& make -j${JOBS} \
		&& make install \
		&& cd ../.. \
		&& rm mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz"
		echo " - MongoDB C driver successfully installed @ ${MONGO_C_DRIVER_ROOT}."
	else
		echo " - MongoDB C driver found with correct version @ ${MONGO_C_DRIVER_ROOT}."
	fi
	echo "${COLOR_CYAN}[Checking MongoDB C++ driver installation]${COLOR_NC}"
	if [[ ! -d $MONGO_CXX_DRIVER_ROOT ]]; then
		echo "Installing MongoDB C++ driver..."
		execute bash -c "curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r$MONGO_CXX_DRIVER_VERSION.tar.gz -o mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz \
		&& tar -xzf mongo-cxx-driver-r${MONGO_CXX_DRIVER_VERSION}.tar.gz \
		&& cd mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION/build \
		&& $CMAKE -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_HOME -DCMAKE_PREFIX_PATH=$EOSIO_HOME $PINNED_TOOLCHAIN .. \
		&& make -j${JOBS} VERBOSE=1 \
		&& make install \
		&& cd ../.. \
		&& rm -f mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz"
		echo " - MongoDB C++ driver successfully installed @ ${MONGO_CXX_DRIVER_ROOT}."
	else
		echo " - MongoDB C++ driver found with correct version @ ${MONGO_CXX_DRIVER_ROOT}."
	fi
fi