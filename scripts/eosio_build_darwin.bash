echo "OS name: ${NAME}"
echo "OS Version: ${OS_VER}"
echo "CPU cores: ${CPU_CORES}"
echo "Physical Memory: ${MEM_GIG}G"
echo "Disk install: ${DISK_INSTALL}"
echo "Disk space total: ${DISK_TOTAL}G"
echo "Disk space available: ${DISK_AVAIL}G"

[[ "${OS_MIN}" -lt 12 ]] && echo "You must be running Mac OS 10.12.x or higher to install EOSIO." && exit 1

[[ $MEM_GIG -lt 7 ]] && echo "Your system must have 7 or more Gigabytes of physical memory installed." && exit 1
[[ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]] && echo " - You must have at least ${DISK_MIN}GB of available storage to install EOSIO." && exit 1

echo ""

echo "${COLOR_CYAN}[Ensuring xcode-select installation]${COLOR_NC}"
if ! XCODESELECT=$( command -v xcode-select ); then echo " - xcode-select must be installed in order to proceed!" && exit 1;
else echo " - xcode-select installation found @ ${XCODESELECT}"; fi

echo "${COLOR_CYAN}[Ensuring Ruby installation]${COLOR_NC}"
if ! RUBY=$( command -v ruby ); then echo " - Ruby must be installed in order to proceed!" && exit 1;
else echo " - Ruby installation found @ ${RUBY}"; fi

echo "${COLOR_CYAN}[Ensuring HomeBrew installation]${COLOR_NC}"
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
# CLANG Installation
build-clang
# LLVM Installation
ensure-llvm
# BOOST Installation
ensure-boost
# MONGO Installation
if $INSTALL_MONGO; then
	echo "${COLOR_CYAN}[Ensuring MongoDB installation]${COLOR_NC}"
	if [[ ! -d $MONGODB_ROOT ]]; then
		execute bash -c "cd $SRC_DIR && \
		curl -OL https://fastdl.mongodb.org/osx/mongodb-osx-ssl-x86_64-$MONGODB_VERSION.tgz \
		&& tar -xzf mongodb-osx-ssl-x86_64-$MONGODB_VERSION.tgz \
		&& mv $SRC_DIR/mongodb-osx-x86_64-$MONGODB_VERSION $MONGODB_ROOT \
		&& touch $MONGODB_LOG_DIR/mongod.log \
		&& rm -f mongodb-osx-ssl-x86_64-$MONGODB_VERSION.tgz \
		&& cp -f $REPO_ROOT/scripts/mongod.conf $MONGODB_CONF \
		&& mkdir -p $MONGODB_DATA_DIR \
		&& rm -rf $MONGODB_LINK_DIR \
		&& rm -rf $BIN_DIR/mongod \
		&& ln -s $MONGODB_ROOT $MONGODB_LINK_DIR \
		&& ln -s $MONGODB_LINK_DIR/bin/mongod $BIN_DIR/mongod"
		echo " - MongoDB successfully installed @ ${MONGODB_ROOT}"
	else
		echo " - MongoDB found with correct version @ ${MONGODB_ROOT}."
	fi
	echo "${COLOR_CYAN}[Ensuring MongoDB C driver installation]${COLOR_NC}"
	if [[ ! -d $MONGO_C_DRIVER_ROOT ]]; then
		execute bash -c "cd $SRC_DIR && \
		curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/$MONGO_C_DRIVER_VERSION/mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
		&& tar -xzf mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
		&& cd mongo-c-driver-$MONGO_C_DRIVER_VERSION \
		&& mkdir -p cmake-build \
		&& cd cmake-build \
		&& $CMAKE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_HOME -DENABLE_BSON=ON -DENABLE_SSL=DARWIN -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF -DENABLE_SASL=OFF -DENABLE_SNAPPY=OFF .. \
		&& make -j${JOBS} \
		&& make install \
		&& cd ../.. \
		&& rm mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz"
		echo " - MongoDB C driver successfully installed @ ${MONGO_C_DRIVER_ROOT}."
	else
		echo " - MongoDB C driver found with correct version @ ${MONGO_C_DRIVER_ROOT}."
	fi
	echo "${COLOR_CYAN}[Ensuring MongoDB C++ driver installation]${COLOR_NC}"
	if [[ "$(grep "Version:" $EOSIO_HOME/lib/pkgconfig/libmongocxx-static.pc 2>/dev/null | tr -s ' ' | awk '{print $2}' || true)" != $MONGO_CXX_DRIVER_VERSION ]]; then
		execute bash -c "cd $SRC_DIR && \
		curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r${MONGO_CXX_DRIVER_VERSION}.tar.gz -o mongo-cxx-driver-r${MONGO_CXX_DRIVER_VERSION}.tar.gz \
		&& tar -xzf mongo-cxx-driver-r${MONGO_CXX_DRIVER_VERSION}.tar.gz \
		&& cd mongo-cxx-driver-r${MONGO_CXX_DRIVER_VERSION}/build \
		&& $CMAKE -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_HOME -DCMAKE_PREFIX_PATH=$EOSIO_HOME .. \
		&& make -j${JOBS} VERBOSE=1 \
		&& make install \
		&& cd ../.. \
		&& rm -f mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz"
		echo " - MongoDB C++ driver successfully installed @ ${MONGO_CXX_DRIVER_ROOT}."
	else
		echo " - MongoDB C++ driver found with correct version @ ${MONGO_CXX_DRIVER_ROOT}."
	fi
fi