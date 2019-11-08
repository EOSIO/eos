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

if $INSTALL_MONGO; then

	echo "${COLOR_CYAN}[Ensuring MongoDB installation]${COLOR_NC}"
	if [[ ! -d $MONGODB_ROOT ]]; then
		execute bash -c "cd $SRC_DIR && \
		curl -OL https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-amazon-$MONGODB_VERSION.tgz \
		&& tar -xzf mongodb-linux-x86_64-amazon-$MONGODB_VERSION.tgz \
		&& mv $SRC_DIR/mongodb-linux-x86_64-amazon-$MONGODB_VERSION $MONGODB_ROOT \
		&& touch $MONGODB_LOG_DIR/mongod.log \
		&& rm -f mongodb-linux-x86_64-amazon-$MONGODB_VERSION.tgz \
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
		&& $CMAKE -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX='$EOSIO_INSTALL_DIR' -DCMAKE_PREFIX_PATH='$EOSIO_INSTALL_DIR' $PINNED_TOOLCHAIN .. \
		&& make -j${JOBS} VERBOSE=1 \
		&& make install \
		&& cd ../.. \
		&& rm -f mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz"
		echo " - MongoDB C++ driver successfully installed @ ${MONGO_CXX_DRIVER_ROOT}."
	else
		echo " - MongoDB C++ driver found with correct version @ ${MONGO_CXX_DRIVER_ROOT}."
	fi
fi
echo ""