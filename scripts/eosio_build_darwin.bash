OS_VER=$(sw_vers -productVersion)
OS_MAJ=$(echo "${OS_VER}" | cut -d'.' -f1)
OS_MIN=$(echo "${OS_VER}" | cut -d'.' -f2)
OS_PATCH=$(echo "${OS_VER}" | cut -d'.' -f3)
MEM_GIG=$(bc <<< "($(sysctl -in hw.memsize) / 1024000000)")
export JOBS=$(( MEM_GIG > CPU_CORES ? CPU_CORES : MEM_GIG ))

DISK_INSTALL=$(df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 || cut -d' ' -f1)
blksize=$(df . | head -1 | awk '{print $2}' | cut -d- -f1)
gbfactor=$(( 1073741824 / blksize ))
total_blks=$(df . | tail -1 | awk '{print $2}')
avail_blks=$(df . | tail -1 | awk '{print $4}')
DISK_TOTAL=$((total_blks / gbfactor ))
DISK_AVAIL=$((avail_blks / gbfactor ))

echo "OS name: ${NAME}"
echo "OS Version: ${OS_VER}"
echo "CPU cores: ${CPU_CORES}"
echo "Physical Memory: ${MEM_GIG} Gbytes"
echo "Disk install: ${DISK_INSTALL}"
echo "Disk space total: ${DISK_TOTAL}G"
echo "Disk space available: ${DISK_AVAIL}G"

if [ "${MEM_GIG}" -lt 7 ]; then
	echo "Your system must have 7 or more Gigabytes of physical memory installed."
	echo "Exiting now."
	exit 1
fi

if [ "${OS_MIN}" -lt 12 ]; then
	echo "You must be running Mac OS 10.12.x or higher to install EOSIO."
	echo "Exiting now."
	exit 1
fi

if [ "${DISK_AVAIL}" -lt "$DISK_MIN" ]; then
	echo "You must have at least ${DISK_MIN}GB of available storage to install EOSIO."
	echo "Exiting now."
	exit 1
fi

echo ""

echo "${COLOR_CYAN}[Checking xcode-select installation]${COLOR_NC}"
if ! XCODESELECT=$( command -v xcode-select ); then echo " - XCode must be installed in order to proceed!" && exit 1;
else echo " - XCode installation found @ ${XCODESELECT}"; fi

echo "${COLOR_CYAN}[Checking Ruby installation]${COLOR_NC}"
if ! RUBY=$( command -v ruby ); then echo " - Ruby must be installed in order to proceed!" && exit 1;
else echo " - Ruby installation found @ ${RUBY}"; fi

echo "${COLOR_CYAN}[Checking HomeBrew installation]${COLOR_NC}"
if ! BREW=$( command -v brew ); then
	while true; do
		[[ $NONINTERACTIVE == false ]] && read -p "${COLOR_YELLOW}Do you wish to install HomeBrew? (y/n)?${COLOR_NC} " PROCEED
		case $PROCEED in
			"" ) echo "What would you like to do?";;
			0 | true | [Yy]* )
				execute "${XCODESELECT}" --install
				if ! execute "${RUBY}" -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"; then
					echo "Unable to install HomeBrew at this time." && exit 1;
				else BREW=$( command -v brew ); fi
			break;;
			1 | false | [Nn]* ) echo "${COLOR_RED} - User aborted required HomeBrew installation${COLOR_NC}"; exit 1;;
			* ) echo "Please type 'y' for yes or 'n' for no.";;
		esac
	done
else
	echo " - HomeBrew installation found @ ${BREW}"
fi

if [ ! -d /usr/local/Frameworks ]; then
	echo "${COLOR_YELLOW}/usr/local/Frameworks is necessary to brew install python@3. Run the following commands as sudo and try again:${COLOR_NC}"
	echo "sudo mkdir /usr/local/Frameworks && sudo chown $(whoami):admin /usr/local/Frameworks"
	exit 1;
fi

# Ensure packages exist
ensure-brew-packages "${REPO_ROOT}/scripts/eosio_build_darwin_deps"
[[ -z "${CMAKE}" ]] && export CMAKE="/usr/local/bin/cmake"
echo ""
# CLANG Installation
build-clang
echo ""
# LLVM Installation
ensure-llvm
echo ""
# BOOST Installation
ensure-boost
echo ""
# MONGO Installation
if $INSTALL_MONGO; then
   printf "Checking MongoDB installation...\\n"
   if [[ ! -d $MONGODB_ROOT ]] then
      printf "Installing MongoDB into ${MONGODB_ROOT}...\\n"
      curl -OL http://downloads.mongodb.org/linux/mongodb-linux-x86_64-ubuntu$OS_MAJ$OS_MIN-$MONGODB_VERSION.tgz \
      && tar -xzf mongodb-linux-x86_64-ubuntu$OS_MAJ$OS_MIN-$MONGODB_VERSION.tgz \
      && mv $SRC_LOCATION/mongodb-linux-x86_64-ubuntu$OS_MAJ$OS_MIN-$MONGODB_VERSION $MONGODB_ROOT \
      && touch $MONGODB_LOG_LOCATION/mongod.log \
      && rm -f mongodb-linux-x86_64-ubuntu$OS_MAJ$OS_MIN-$MONGODB_VERSION.tgz \
      && cp -f $REPO_ROOT/scripts/mongod.conf $MONGODB_CONF \
      && mkdir -p $MONGODB_DATA_LOCATION \
      && rm -rf $MONGODB_LINK_LOCATION \
      && rm -rf $BIN_LOCATION/mongod \
      && ln -s $MONGODB_ROOT $MONGODB_LINK_LOCATION \
      && ln -s $MONGODB_LINK_LOCATION/bin/mongod $BIN_LOCATION/mongod
      printf " - MongoDB successfully installed @ ${MONGODB_ROOT} (Symlinked to ${MONGODB_LINK_LOCATION}).\\n"
   else
      printf " - MongoDB found with correct version @ ${MONGODB_ROOT} (Symlinked to ${MONGODB_LINK_LOCATION}).\\n"
   fi
   printf "Checking MongoDB C driver installation...\\n"
   if [[ ! -d $MONGO_C_DRIVER_ROOT ]]; then
      printf "Installing MongoDB C driver...\\n"
      curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/$MONGO_C_DRIVER_VERSION/mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
      && tar -xzf mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
      && cd mongo-c-driver-$MONGO_C_DRIVER_VERSION \
      && mkdir -p cmake-build \
      && cd cmake-build \
      && $CMAKE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PREFIX -DENABLE_BSON=ON -DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF -DENABLE_SNAPPY=OFF $PINNED_TOOLCHAIN .. \
      && make -j"${JOBS}" \
      && make install \
      && cd ../.. \
      && rm mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz
      printf " - MongoDB C driver successfully installed @ ${MONGO_C_DRIVER_ROOT}.\\n"
   else
      printf " - MongoDB C driver found with correct version @ ${MONGO_C_DRIVER_ROOT}.\\n"
   fi
   printf "Checking MongoDB C++ driver installation...\\n"
   if [[ ! -d $MONGO_CXX_DRIVER_ROOT ]]; then
      printf "Installing MongoDB C++ driver...\\n"
      curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r$MONGO_CXX_DRIVER_VERSION.tar.gz -o mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz \
      && tar -xzf mongo-cxx-driver-r${MONGO_CXX_DRIVER_VERSION}.tar.gz \
      && cd mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION \
      && sed -i 's/"maxAwaitTimeMS", count/"maxAwaitTimeMS", static_cast<int64_t>(count)/' src/mongocxx/options/change_stream.cpp \
      && sed -i 's/add_subdirectory(test)//' src/mongocxx/CMakeLists.txt src/bsoncxx/CMakeLists.txt \
      && cd build \
      && $CMAKE -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_PREFIX_PATH=$PREFIX $PINNED_TOOLCHAIN .. \
      && make -j"${JOBS}" VERBOSE=1 \
      && make install \
      && cd ../.. \
      && rm -f mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz
      printf " - MongoDB C++ driver successfully installed @ ${MONGO_CXX_DRIVER_ROOT}.\\n"
   else
      printf " - MongoDB C++ driver found with correct version @ ${MONGO_CXX_DRIVER_ROOT}.\\n"
   fi
fi