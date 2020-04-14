echo "OS name: ${NAME}"
echo "OS Version: ${VERSION_ID}"
echo "CPU cores: ${CPU_CORES}"
echo "Physical Memory: ${MEM_GIG}G"
echo "Disk space total: ${DISK_TOTAL}G"
echo "Disk space available: ${DISK_AVAIL}G"

( [[ $NAME == "Ubuntu" ]] && ( [[ "$(echo ${VERSION_ID})" == "16.04" ]] || [[ "$(echo ${VERSION_ID})" == "18.04" ]] )  ) || ( echo " - You must be running 16.04.x or 18.04.x to install EOSIO." && exit 1 )

[[ $MEM_GIG -lt 7 ]] && echo "Your system must have 7 or more Gigabytes of physical memory installed." && exit 1
[[ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]] && echo " - You must have at least ${DISK_MIN}GB of available storage to install EOSIO." && exit 1

# system clang and build essential for Ubuntu 18 (16 too old)
( [[ $PIN_COMPILER == false ]] && [[ $VERSION_ID == "18.04" ]] ) && EXTRA_DEPS=(clang,dpkg\ -s llvm-7-dev,dpkg\ -s)
# We install clang8 for Ubuntu 16, but we still need something to compile cmake, boost, etc + pinned 18 still needs something to build source
( [[ $VERSION_ID == "16.04" ]] || ( $PIN_COMPILER && [[ $VERSION_ID == "18.04" ]] ) ) && ensure-build-essential
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
VERSION_MAJ=$(echo "${VERSION_ID}" | cut -d'.' -f1)
VERSION_MIN=$(echo "${VERSION_ID}" | cut -d'.' -f2)
if $INSTALL_MONGO; then
	if [[ $VERSION_MAJ == 18 ]]; then
		# UBUNTU 18 doesn't have MONGODB 3.6.3
		MONGODB_VERSION=4.1.1
		# We have to re-set this with the new version
		MONGODB_ROOT=${OPT_DIR}/mongodb-${MONGODB_VERSION}
	fi
	echo "${COLOR_CYAN}[Ensuring MongoDB installation]${COLOR_NC}"
	if [[ ! -d $MONGODB_ROOT ]]; then
		execute bash -c "cd $SRC_DIR && \
		curl -OL http://downloads.mongodb.org/linux/mongodb-linux-x86_64-ubuntu${VERSION_MAJ}${VERSION_MIN}-$MONGODB_VERSION.tgz \
		&& tar -xzf mongodb-linux-x86_64-ubuntu${VERSION_MAJ}${VERSION_MIN}-${MONGODB_VERSION}.tgz \
		&& mv $SRC_DIR/mongodb-linux-x86_64-ubuntu${VERSION_MAJ}${VERSION_MIN}-${MONGODB_VERSION} $MONGODB_ROOT \
		&& touch $MONGODB_LOG_DIR/mongod.log \
		&& rm -f mongodb-linux-x86_64-ubuntu${VERSION_MAJ}${VERSION_MIN}-$MONGODB_VERSION.tgz \
		&& cp -f $REPO_ROOT/scripts/mongod.conf $MONGODB_CONF \
		&& mkdir -p $MONGODB_DATA_DIR \
		&& rm -rf $MONGODB_LINK_DIR \
		&& rm -rf $BIN_DIR/mongod \
		&& ln -s $MONGODB_ROOT $MONGODB_LINK_DIR \
		&& ln -s $MONGODB_LINK_DIR/bin/mongod $BIN_DIR/mongod"
		echo " - MongoDB successfully installed @ ${MONGODB_ROOT} (Symlinked to ${MONGODB_LINK_DIR})."
	else
		echo " - MongoDB found with correct version @ ${MONGODB_ROOT} (Symlinked to ${MONGODB_LINK_DIR})."
	fi
	echo "${COLOR_CYAN}[Ensuring MongoDB C driver installation]${COLOR_NC}"
	if [[ ! -d $MONGO_C_DRIVER_ROOT ]]; then
		execute bash -c "cd $SRC_DIR && \
		curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/$MONGO_C_DRIVER_VERSION/mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
		&& tar -xzf mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
		&& cd mongo-c-driver-$MONGO_C_DRIVER_VERSION \
		&& mkdir -p cmake-build \
		&& cd cmake-build \
		&& $CMAKE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_DIR -DENABLE_BSON=ON -DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF -DENABLE_SNAPPY=OFF $PINNED_TOOLCHAIN .. \
		&& make -j${JOBS} \
		&& make install \
		&& cd ../.. \
		&& rm mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz"
		echo " - MongoDB C driver successfully installed @ ${MONGO_C_DRIVER_ROOT}."
	else
		echo " - MongoDB C driver found with correct version @ ${MONGO_C_DRIVER_ROOT}."
	fi
	echo "${COLOR_CYAN}[Ensuring MongoDB C++ driver installation]${COLOR_NC}"
	if [[ ! -d $MONGO_CXX_DRIVER_ROOT ]]; then
		execute bash -c "cd $SRC_DIR && \
		curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r$MONGO_CXX_DRIVER_VERSION.tar.gz -o mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz \
		&& tar -xzf mongo-cxx-driver-r${MONGO_CXX_DRIVER_VERSION}.tar.gz \
		&& cd mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION \
		&& sed -i 's/\"maxAwaitTimeMS\", count/\"maxAwaitTimeMS\", static_cast<int64_t>(count)/' src/mongocxx/options/change_stream.cpp \
		&& sed -i 's/add_subdirectory(test)//' src/mongocxx/CMakeLists.txt src/bsoncxx/CMakeLists.txt \
		&& cd build \
		&& $CMAKE -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$EOSIO_INSTALL_DIR -DCMAKE_PREFIX_PATH=$EOSIO_INSTALL_DIR $PINNED_TOOLCHAIN .. \
		&& make -j${JOBS} VERBOSE=1 \
		&& make install \
		&& cd ../.. \
		&& rm -f mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz"
		echo " - MongoDB C++ driver successfully installed @ ${MONGO_CXX_DRIVER_ROOT}."
	else
		echo " - MongoDB C++ driver found with correct version @ ${MONGO_CXX_DRIVER_ROOT}."
	fi
fi

if [[ $VERSION_MAJ == 18 ]]; then
    echo "${COLOR_CYAN}[Installing cppkin]${COLOR_NC}"
    wget https://github.com/EOSIO/cppKin/releases/download/v1.1.0/cppkin_1.1.0-ubuntu-18.04_amd64.deb
    apt install -y ./cppkin_1.1.0-ubuntu-18.04_amd64.deb
fi
