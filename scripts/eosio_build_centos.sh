if [ $1 == 1 ]; then answer=1; fi # NONINTERACTIVE

OS_VER=$( grep VERSION_ID /etc/os-release | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' \
| cut -d'.' -f1 )

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

printf "\\nOS name: ${OS_NAME}\\n"
printf "OS Version: ${OS_VER}\\n"
printf "CPU speed: ${CPU_SPEED}Mhz\\n"
printf "CPU cores: ${CPU_CORE}\\n"
printf "Physical Memory: ${MEM_MEG}Mgb\\n"
printf "Disk install: ${DISK_INSTALL}\\n"
printf "Disk space total: ${DISK_TOTAL%.*}G\\n" 
printf "Disk space available: ${DISK_AVAIL%.*}G\\n"
printf "Concurrent Jobs (make -j): ${JOBS}\\n"

if [ "${MEM_MEG}" -lt 7000 ]; then
	printf "\\nYour system must have 7 or more Gigabytes of physical memory installed.\\n"
	printf "Exiting now.\\n\\n"
	exit 1;
fi

if [ "${OS_VER}" -lt 7 ]; then
	printf "\\nYou must be running Centos 7 or higher to install EOSIO.\\n"
	printf "Exiting now.\\n\\n"
	exit 1;
fi

if [ "${DISK_AVAIL%.*}" -lt "${DISK_MIN}" ]; then
	printf "\\nYou must have at least %sGB of available storage to install EOSIO.\\n" "${DISK_MIN}"
	printf "Exiting now.\\n\\n"
	exit 1;
fi

printf "\\n"

printf "Checking Yum installation...\\n"
if ! YUM=$( command -v yum 2>/dev/null ); then
		printf "!! Yum must be installed to compile EOS.IO !!\\n"
		printf "Exiting now.\\n"
		exit 1;
fi
printf " - Yum installation found at %s.\\n" "${YUM}"

if [ $1 == 0 ]; then read -p "Do you wish to update YUM repositories? (y/n) " answer; fi
case ${answer} in
	1 | [Yy]* )
		if ! "${YUM}" -y update; then
			printf " - YUM update failed.\\n"
			exit 1;
		else
			printf " - YUM update complete.\\n"
		fi
	;;
	[Nn]* ) echo " - Proceeding without update!";;
	* ) echo "Please type 'y' for yes or 'n' for no.";;
esac

printf "Checking installation of Centos Software Collections Repository...\\n"
SCL=$( rpm -qa | grep -E 'centos-release-scl-[0-9].*' )
if [ -z "${SCL}" ]; then
	if [ $1 == 0 ]; then read -p "Do you wish to install and enable this repository? (y/n)? " answer; fi
	case ${answer} in
		1 | [Yy]* )
			printf "Installing SCL...\\n"
			if ! "${YUM}" -y --enablerepo=extras install centos-release-scl 2>/dev/null; then
				printf "!! Centos Software Collections Repository installation failed !!\\n"
				printf "Exiting now.\\n\\n"
				exit 1;
			else
				printf "Centos Software Collections Repository installed successfully.\\n"
			fi
		;;
		[Nn]* ) echo "User aborting installation of required Centos Software Collections Repository, Exiting now."; exit;;
		* ) echo "Please type 'y' for yes or 'n' for no.";;
	esac
else
	printf " - ${SCL} found.\\n"
fi

printf "Checking installation of devtoolset-7...\\n"
DEVTOOLSET=$( rpm -qa | grep -E 'devtoolset-7-[0-9].*' )
if [ -z "${DEVTOOLSET}" ]; then
	if [ $1 == 0 ]; then read -p "Do you wish to install devtoolset-7? (y/n)? " answer; fi
	case ${answer} in
		1 | [Yy]* )
			printf "Installing devtoolset-7...\\n"
			if ! "${YUM}" install -y devtoolset-7; then
					printf "!! Centos devtoolset-7 installation failed !!\\n"
					printf "Exiting now.\\n"
					exit 1;
			else
					printf " - Centos devtoolset installed successfully!\\n"
			fi
		;;
		[Nn]* ) echo "User aborting installation of devtoolset-7. Exiting now."; exit;;
		* ) echo "Please type 'y' for yes or 'n' for no.";;
	esac
else
	printf " - ${DEVTOOLSET} found.\\n"
fi
if [ -d /opt/rh/devtoolset-7 ]; then
	printf "Enabling Centos devtoolset-7 so we can use GCC 7...\\n"
	source /opt/rh/devtoolset-7/enable || exit 1
	printf " - Centos devtoolset-7 successfully enabled!\\n"
fi

printf "\\n"

DEP_ARRAY=( 
	git autoconf automake libtool make bzip2 doxygen graphviz \
				bzip2-devel openssl-devel gmp-devel \
				ocaml libicu-devel python python-devel python33 python33-devel \
				gettext-devel file sudo
 )
COUNT=1
DISPLAY=""
DEP=""
printf "Checking RPM for installed dependencies...\\n"
for (( i=0; i<${#DEP_ARRAY[@]}; i++ )); do
	pkg=$( rpm -qi "${DEP_ARRAY[$i]}" 2>/dev/null | grep Name )
	if [[ -z $pkg ]]; then
		DEP=$DEP" ${DEP_ARRAY[$i]} "
		DISPLAY="${DISPLAY}${COUNT}. ${DEP_ARRAY[$i]}\\n"
		printf " - Package %s ${bldred} NOT ${txtrst} found!\\n" "${DEP_ARRAY[$i]}"
		(( COUNT++ ))
	else
		printf " - Package %s found.\\n" "${DEP_ARRAY[$i]}"
		continue
	fi
done
if [ "${COUNT}" -gt 1 ]; then
	printf "\\nThe following dependencies are required to install EOSIO:\\n"
	printf "${DISPLAY}\\n\\n"
	if [ $1 == 0 ]; then read -p "Do you wish to install these dependencies? (y/n) " answer; fi
	case ${answer} in
		1 | [Yy]* )
			if ! "${YUM}" -y install ${DEP}; then
				printf " - YUM dependency installation failed!\\n"
				exit 1;
			else
				printf " - YUM dependencies installed successfully.\\n"
			fi
		;;
		[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
		* ) echo "Please type 'y' for yes or 'n' for no.";;
	esac
else
	printf " - No required YUM dependencies to install.\\n"
fi

if [ -d /opt/rh/python33 ]; then
	printf "Enabling python33...\\n"
	source /opt/rh/python33/enable || exit 1
	printf " - Python33 successfully enabled!\\n"
fi
export CPATH="$CPATH:/opt/rh/python33/root/usr/include/python3.3m" # m on the end causes problems with boost finding python3


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
	printf " - CMAKE successfully installed @ ${HOME}/bin/cmake \\n"
else
	printf " - CMAKE found @ ${CMAKE}.\\n"
fi


printf "\\n"


printf "Checking Boost library (${BOOST_VERSION}) installation...\\n"
if [ ! -d $BOOST_ROOT ]; then
	printf "Installing Boost library...\\n"
	curl -LO https://dl.bintray.com/boostorg/release/${BOOST_VERSION_MAJOR}.${BOOST_VERSION_MINOR}.${BOOST_VERSION_PATCH}/source/boost_$BOOST_VERSION.tar.bz2 \
	&& tar -xvjf boost_$BOOST_VERSION.tar.bz2 \
	&& cd $BOOST_ROOT \
	&& ./bootstrap.sh --prefix=$BOOST_ROOT \
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
	&& tar -xzvf mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
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
	curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r$MONGO_CXX_DRIVER_VERSION.tar.gz -o mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz \
	&& tar -xzvf mongo-cxx-driver-r${MONGO_CXX_DRIVER_VERSION}.tar.gz \
	&& cd mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION/build \
	&& cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME .. \
	&& make -j"${JOBS}" VERBOSE=1 \
	&& make install \
	&& cd ../.. \
	&& rm -f mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz \
	|| exit 1
	printf " - MongoDB C++ driver successfully installed @ ${MONGO_CXX_DRIVER_ROOT}.\\n"
else
	printf " - MongoDB C++ driver found with correct version @ ${MONGO_CXX_DRIVER_ROOT}.\\n"
fi


printf "\\n"


printf "Checking LLVM 4 support...\\n"
if [ ! -d $LLVM_ROOT ]; then
	printf "Installing LLVM 4...\\n"
	cd ../opt \
	&& git clone --depth 1 --single-branch --branch $LLVM_VERSION https://github.com/llvm-mirror/llvm.git llvm && cd llvm \
	&& mkdir build \
	&& cd build \
	&& cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="${LLVM_ROOT}" -DLLVM_TARGETS_TO_BUILD="host" -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE="Release" .. \
	&& make -j"${JOBS}" \
	&& make install \
	&& cd ../.. \
	|| exit 1
	printf " - WASM compiler successfully installed @ ${LLVM_ROOT}\\n"
else
	printf " - WASM found @ ${LLVM_ROOT}.\\n"
fi

cd ..
printf "\\n"

function print_instructions()
{
	printf "Please ensure the following \$PATH and \$LD_LIBRARY_PATH stucture in the order specified, as well as scl enable commands within your ~/.bash_profile/rc file:\\n"
	printf "source /opt/rh/python33/enable\\n"
	printf "CPATH=\$CPATH:/opt/rh/python33/root/usr/include/python3.3m\\n" # Boost has trouble finding pyconfig.h 
	printf "source /opt/rh/devtoolset-7/enable\\n"
	# HOME/bin first to load proper cmake version over the one in /usr/bin.
	# llvm/bin last to prevent llvm/bin/clang from being used over /usr/bin/clang (We don't symlink into $HOME/bin)
	printf "export PATH=\$HOME/bin:\$PATH:$MONGODB_LINK_LOCATION/bin:\$HOME/opt/llvm/bin\\n"
	printf "export LD_LIBRARY_PATH=\$HOME/lib:\$HOME/lib64:\$HOME/opt/llvm/lib:\$LD_LIBRARY_PATH\\n" # libmongoc is installed into $HOME/lib64
	printf "export CPATH=\$HOME/include:\$CPLUS_INCLUDE_PATH\\n" # libmongoc is installed into $HOME/include
	printf "$( command -v mongod ) --dbpath ${MONGODB_DATA_LOCATION} -f ${MONGODB_CONF} --logpath ${MONGODB_LOG_LOCATION}/mongod.log &\\n"
	printf "cd ${BUILD_DIR} && make test\\n"
	return 0
}
