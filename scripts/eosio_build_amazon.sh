OS_VER=$( grep VERSION_ID /etc/os-release | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' | cut -d'.' -f1 )

MEM_MEG=$( free -m | sed -n 2p | tr -s ' ' | cut -d\  -f2 )
CPU_SPEED=$( lscpu | grep "MHz" | tr -s ' ' | cut -d\  -f3 | cut -d'.' -f1 )
CPU_CORE=$( nproc )
MEM_GIG=$(( ((MEM_MEG / 1000) / 2) ))
export JOBS=$(( MEM_GIG > CPU_CORE ? CPU_CORE : MEM_GIG ))

DISK_INSTALL=$( df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 )
DISK_TOTAL_KB=$( df . | tail -1 | awk '{print $2}' )
DISK_AVAIL_KB=$( df . | tail -1 | awk '{print $4}' )
DISK_TOTAL=$(( DISK_TOTAL_KB / 1048576 ))
DISK_AVAIL=$(( DISK_AVAIL_KB / 1048576 ))

DEP_ARRAY=( 
	sudo procps which gcc72 gcc72-c++ autoconf automake libtool make \
    bzip2 bzip2-devel openssl-devel gmp gmp-devel libstdc++72 python27 python27-devel python34-devel \
    libedit-devel ncurses-devel swig wget file
)
COUNT=1
DISPLAY=""
DEP=""

printf "\\nOS name: %s\\n" "${OS_NAME}"
printf "OS Version: %s\\n" "${OS_VER}"
printf "CPU speed: %sMhz\\n" "${CPU_SPEED}"
printf "CPU cores: %s\\n" "${CPU_CORE}"
printf "Physical Memory: %sMgb\\n" "${MEM_MEG}"
printf "Disk space total: %sGb\\n" "${DISK_TOTAL}"
printf "Disk space available: %sG\\n" "${DISK_AVAIL}"

if [ "${MEM_MEG}" -lt 7000 ]; then
	printf "Your system must have 7 or more Gigabytes of physical memory installed.\\n"
	printf "exiting now.\\n"
	exit 1
fi

if [[ "${OS_NAME}" == "Amazon Linux AMI" && "${OS_VER}" -lt 2017 ]]; then
	printf "You must be running Amazon Linux 2017.09 or higher to install EOSIO.\\n"
	printf "exiting now.\\n"
	exit 1
fi

if [ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]; then
	printf "You must have at least %sGB of available storage to install EOSIO.\\n" "${DISK_MIN}"
	printf "exiting now.\\n"
	exit 1
fi

printf "\\nChecking Yum installation.\\n"
if ! YUM=$( command -v yum 2>/dev/null )
then
	printf "\\nYum must be installed to compile EOS.IO.\\n"
	printf "\\nExiting now.\\n"
	exit 1
fi
printf "Yum installation found at ${YUM}.\\n"

printf "\\nDo you wish to update YUM repositories?\\n\\n"
select yn in "Yes" "No"; do
	case $yn in
		[Yy]* ) 
			printf "\\n\\nUpdating...\\n\\n"
			if ! sudo "${YUM}" -y update; then
				printf "\\nYUM update failed.\\n"
				printf "\\nExiting now.\\n\\n"
				exit 1;
			else
				printf "\\nYUM update complete.\\n"
			fi
		break;;
		[Nn]* ) echo "Proceeding without update!";;
		* ) echo "Please type 1 for yes or 2 for no.";;
	esac
done

printf "Checking RPM for installed dependencies...\\n"
for (( i=0; i<${#DEP_ARRAY[@]}; i++ )); do
	pkg=$( rpm -qi "${DEP_ARRAY[$i]}" 2>/dev/null | grep Name )
	if [[ -z $pkg ]]; then
		DEP=$DEP" ${DEP_ARRAY[$i]} "
		DISPLAY="${DISPLAY}${COUNT}. ${DEP_ARRAY[$i]}\\n"
		printf "!! Package %s ${bldred} NOT ${txtrst} found !!\\n" "${DEP_ARRAY[$i]}"
		(( COUNT++ ))
	else
		printf " - Package %s found.\\n" "${DEP_ARRAY[$i]}"
		continue
	fi
done
printf "\\n"
if [ "${COUNT}" -gt 1 ]; then
	printf "The following dependencies are required to install EOSIO.\\n"
	printf "${DISPLAY}\\n\\n"
	printf "Do you wish to install these dependencies?\\n"
	select yn in "Yes" "No"; do
		case $yn in
			[Yy]* )
				printf "Installing dependencies\\n\\n"
				if ! sudo "${YUM}" -y install ${DEP}; then
					printf "!! YUM dependency installation failed !!\\n"
					printf "Exiting now.\\n"
					exit 1;
				else
					printf "YUM dependencies installed successfully.\\n"
				fi
			break;;
			[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
			* ) echo "Please type 1 for yes or 2 for no.";;
		esac
	done
else
	printf " - No required YUM dependencies to install.\\n"
fi


printf "\\n"


printf "Checking CMAKE installation...\\n"
CMAKE=$(command -v cmake 2>/dev/null)
if [ -z $CMAKE ]; then
	printf "Installing CMAKE...\\n"
	curl -LO https://cmake.org/files/v$CMAKE_VERSION_MAJOR.$CMAKE_VERSION_MINOR/cmake-$CMAKE_VERSION.tar.gz \
	&& tar xf cmake-$CMAKE_VERSION.tar.gz \
	&& cd cmake-$CMAKE_VERSION \
	&& ./bootstrap --prefix=$HOME \
	&& make -j"${JOBS}" \
	&& make install \
	&& cd .. \
	&& rm -f cmake-$CMAKE_VERSION.tar.gz \
	|| exit 1
	printf " - CMAKE successfully installed @ ${CMAKE}.\\n"
else
	printf " - CMAKE found @ ${CMAKE}.\\n"
fi


printf "\\n"

printf "Checking Boost library (${BOOST_VERSION}) installation...\\n"
if [ ! -d $BOOST_ROOT ]; then
	printf "Installing Boost library...\\n"
	curl -LO https://dl.bintray.com/boostorg/release/${BOOST_VERSION_MAJOR}.${BOOST_VERSION_MINOR}.${BOOST_VERSION_PATCH}/source/boost_$BOOST_VERSION.tar.bz2 \
	&& tar -xvjf boost_$BOOST_VERSION.tar.bz2 \
	&& cd boost_$BOOST_VERSION/ \
	&& ./bootstrap.sh "--prefix=${BOOST_ROOT}" \
	&& ./b2 -q -j"${JOBS}" install \
	&& cd .. \
	&& rm -f boost_$BOOST_VERSION.tar.bz2 \
	&& rm -rf $BOOST_LINK_LOCATION \
	&& ln -s $BOOST_ROOT $BOOST_LINK_LOCATION \
	|| exit 1
	printf " - Boost library successfully installed @ ${BOOST_ROOT} (Symlinked to ${BOOST_LINK_LOCATION}).\\n"
else
	printf " - Boost library found with correct version @ ${BOOST_ROOT} (Symlinked to ${BOOST_LINK_LOCATION}).\\n"
fi


printf "\\n"


printf "Checking MongoDB installation...\\n"
if [ ! -d $MONGODB_ROOT ]; then
	printf "Installing MongoDB into ${MONGODB_ROOT}...\\n"
	curl -OL https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-amazon-$MONGODB_VERSION.tgz \
	&& tar -xzvf mongodb-linux-x86_64-amazon-$MONGODB_VERSION.tgz \
	&& mv $SRC_LOCATION/mongodb-linux-x86_64-amazon-$MONGODB_VERSION $MONGODB_ROOT \
	&& touch $MONGODB_LOG_LOCATION/mongod.log \
	&& rm -f mongodb-linux-x86_64-amazon-$MONGODB_VERSION.tgz \
	&& cp -f $CURRENT_DIR/scripts/mongod.conf $MONGODB_CONF \
	&& mkdir -p $MONGODB_DATA_LOCATION \
	&& rm -rf $MONGODB_LINK_LOCATION \
	&& rm -rf $BIN_LOCATION/mongod \
	&& ln -s $MONGODB_ROOT $MONGODB_LINK_LOCATION \
	&& ln -s $MONGODB_LINK_LOCATION/bin/mongod $BIN_LOCATION/mongod \
	|| exit 1
	printf " - MongoDB successfully installed @ ${MONGODB_ROOT} (Symlinked to ${MONGODB_LINK_LOCATION}).\\n"
else
	printf " - MongoDB found with correct version @ ${MONGODB_ROOT} (Symlinked to ${MONGODB_LINK_LOCATION}).\\n"
fi
printf "Checking MongoDB C driver installation...\\n"
if [ ! -d $MONGO_C_DRIVER_ROOT ]; then
	printf "Installing MongoDB C driver...\\n"
	curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/$MONGO_C_DRIVER_VERSION/mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
	&& tar -xf mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
	&& cd mongo-c-driver-$MONGO_C_DRIVER_VERSION \
	&& mkdir -p cmake-build \
	&& cd cmake-build \
	&& cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME -DENABLE_BSON=ON -DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON .. \
	&& make -j"${JOBS}" \
	&& make install \
	&& cd ../.. \
	&& rm mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
	|| exit 1
	printf " - MongoDB C driver successfully installed @ ${MONGO_C_DRIVER_ROOT}.\\n"
else
	printf " - MongoDB C driver found with correct version @ ${MONGO_C_DRIVER_ROOT}.\\n"
fi
printf "Checking MongoDB C++ driver installation...\\n"
if [ ! -d $MONGO_CXX_DRIVER_ROOT ]; then
	printf "Installing MongoDB C++ driver...\\n"
	git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/v$MONGO_CXX_DRIVER_VERSION --depth 1 mongo-cxx-driver-$MONGO_CXX_DRIVER_VERSION \
	&& cd mongo-cxx-driver-$MONGO_CXX_DRIVER_VERSION/build \
	&& cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME .. \
	&& make -j"${JOBS}" VERBOSE=1 \
	&& make install \
	&& cd ../.. \
	|| exit 1
	printf " - MongoDB C++ driver successfully installed @ ${MONGO_CXX_DRIVER_ROOT}.\\n"
else
	printf " - MongoDB C++ driver found with correct version @ ${MONGO_CXX_DRIVER_ROOT}.\\n"
fi


printf "\\n"


printf "Checking LLVM with WASM support...\\n"
if [ ! -d $LLVM_ROOT ]; then
	printf "Installing LLVM with WASM...\\n"
	cd ../opt \
	&& git clone --depth 1 --single-branch --branch $LLVM_VERSION https://github.com/llvm-mirror/llvm.git llvm && cd llvm \
	&& mkdir build \
	&& cd build \
	&& cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="${LLVM_ROOT}" -DLLVM_TARGETS_TO_BUILD="host" -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE="Release" .. \
	&& make -j"${JOBS}" \
	&& make install \
	&& cd ../.. \
	|| exit 1
	printf "WASM compiler successfully installed @ ${LLVM_ROOT}\\n"
else
	printf " - WASM found @ ${LLVM_ROOT}.\\n"
fi


cd ..
printf "\\n"

function print_instructions()
{
	printf "Please ensure the following \$PATH and \$LD_LIBRARY_PATH stucture, in the order specified, within your ~/.bash_profile/rc file:\\n"
	# HOME/bin first to load proper cmake version over the one in /usr/bin.
	# llvm/bin last to prevent llvm/bin/clang from being used over /usr/bin/clang + We don't symlink into $HOME/bin
	printf "export PATH=\$HOME/bin:\$PATH:$MONGODB_LINK_LOCATION/bin:\$HOME/opt/llvm/bin\\n"
	printf "$( command -v mongod ) --dbpath ${MONGODB_DATA_LOCATION} -f ${MONGODB_CONF} --logpath ${MONGODB_LOG_LOCATION}/mongod.log &\\n"
	printf "cd ${BUILD_DIR} && make test\\n"
	return 0
}