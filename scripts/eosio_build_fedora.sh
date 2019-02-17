if [ $1 == 1 ]; then ANSWER=1; else ANSWER=0; fi

CPU_SPEED=$( lscpu | grep "MHz" | tr -s ' ' | cut -d\  -f3 | cut -d'.' -f1 )
CPU_CORE=$( nproc )

OS_VER=$( grep VERSION_ID /etc/os-release | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' )
if [ "${OS_VER}" -lt 25 ]; then
	printf "You must be running Fedora 25 or higher to install EOSIO.\\n"
	printf "Exiting now.\\n"
	exit 1;
fi

# procps-ng includes free command
if [[ -z "$( rpm -qi "procps-ng" 2>/dev/null | grep Name )" ]]; then yum install -y procps-ng; fi
MEM_MEG=$( free -m | sed -n 2p | tr -s ' ' | cut -d\  -f2 )
if [ "${MEM_MEG}" -lt 7000 ]; then
	printf "Your system must have 7 or more Gigabytes of physical memory installed.\\n"
	printf "Exiting now.\\n"
	exit 1;
fi
MEM_GIG=$(( ((MEM_MEG / 1000) / 2) ))
export JOBS=$(( MEM_GIG > CPU_CORE ? CPU_CORE : MEM_GIG ))

DISK_INSTALL=$( df -h . | tail -1 | tr -s ' ' | cut -d\\ -f1 )
DISK_TOTAL_KB=$( df . | tail -1 | awk '{print $2}' )
DISK_AVAIL_KB=$( df . | tail -1 | awk '{print $4}' )
DISK_TOTAL=$(( DISK_TOTAL_KB / 1048576 ))
DISK_AVAIL=$(( DISK_AVAIL_KB / 1048576 ))
if [ "${DISK_AVAIL%.*}" -lt "${DISK_MIN}" ]; then
	printf "You must have at least %sGB of available storage to install EOSIO.\\n" "${DISK_MIN}"
	printf "Exiting now.\\n"
	exit 1;
fi

printf "\\nOS name: ${OS_NAME}\\n"
printf "OS Version: ${OS_VER}\\n"
printf "CPU speed: ${CPU_SPEED}Mhz\\n"
printf "CPU cores: ${CPU_CORE}\\n"
printf "Physical Memory: ${MEM_MEG} Mgb\\n"
printf "Disk space total: ${DISK_TOTAL%.*}G\\n"
printf "Disk space available: ${DISK_AVAIL%.*}G\\n"

# llvm is symlinked from /usr/lib64/llvm4.0 into user's home
DEP_ARRAY=( 
	git sudo procps-ng which gcc gcc-c++ autoconf automake libtool make \
	bzip2-devel wget bzip2 compat-openssl10 graphviz doxygen \
	openssl-devel gmp-devel libstdc++-devel python2 python2-devel python3 python3-devel \
	libedit ncurses-devel swig llvm4.0 llvm4.0-devel llvm4.0-libs llvm4.0-static libcurl-devel libusb-devel
)
COUNT=1
DISPLAY=""
DEP=""

printf "\\nChecking Yum installation...\\n"
if ! YUM=$( command -v yum 2>/dev/null ); then
		printf "!! Yum must be installed to compile EOS.IO !!\\n"
		printf "Exiting now.\\n"
		exit 1;
fi
printf " - Yum installation found at %s.\\n" "${YUM}"

	if [ "${DISK_AVAIL%.*}" -lt "${DISK_MIN}" ]; then
		printf "\\tYou must have at least %sGB of available storage to install EOSIO.\\n" "${DISK_MIN}"
		printf "\\tExiting now.\\n"
		exit 1;
	fi
	
	printf "\\n\\tChecking Yum installation\\n"
	
	YUM=$( command -v yum 2>/dev/null )
	if [ -z "${YUM}" ]; then
		printf "\\n\\tYum must be installed to compile EOS.IO.\\n"
		printf "\\n\\tExiting now.\\n"
		exit 1;
	fi
	
	printf "\\tYum installation found at %s.\\n" "${YUM}"
	printf "\\tUpdating YUM.\\n"
	if ! sudo yum -y update
	then
		printf "\\n\\tYUM update failed with the above errors.\\n"
		printf "\\n\\tExiting now.\\n"
		exit 1;
	fi
	
	DEP_ARRAY=( git gcc.x86_64 gcc-c++.x86_64 autoconf automake libtool make cmake.x86_64 \
	bzip2.x86_64 bzip2-devel.x86_64 openssl-devel.x86_64 gmp-devel.x86_64 libstdc++-devel.x86_64 \
	python2-devel.x86_64 python3-devel.x86_64 mongodb.x86_64 mongodb-server.x86_64 libedit.x86_64 \
	graphviz.x86_64 doxygen.x86_64 )
	COUNT=1
	DISPLAY=""
	DEP=""

	printf "\\n\\tChecking YUM for installed dependencies.\\n\\n"

	for (( i=0; i<${#DEP_ARRAY[@]}; i++ ));
	do
		pkg=$( "${YUM}" info "${DEP_ARRAY[$i]}" 2>/dev/null | grep Repo | tr -s ' ' | cut -d: -f2 | sed 's/ //g' )

		if [ "$pkg" != "@System" ]; then
			DEP=$DEP" ${DEP_ARRAY[$i]} "
			DISPLAY="${DISPLAY}${COUNT}. ${DEP_ARRAY[$i]}\\n\\t"
			printf "\\tPackage %s ${bldred} NOT ${txtrst} found.\\n" "${DEP_ARRAY[$i]}"
			(( COUNT++ ))
		else
			printf "\\tPackage %s found.\\n" "${DEP_ARRAY[$i]}"
			continue
		fi
	done		

	if [ ${COUNT} -gt 1 ]; then
		printf "\\n\\tThe following dependencies are required to install EOSIO.\\n"
		printf "\\n\\t${DISPLAY}\\n\\n"
		printf "\\tDo you wish to install these dependencies?\\n"
		if is_noninteractive; then exec <<< "1"; fi
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* ) 
					printf "\\n\\n\\tInstalling dependencies\\n\\n"
					if ! sudo yum -y install ${DEP}
					then
						printf "\\n\\tYUM dependency installation failed.\\n"
						printf "\\n\\tExiting now.\\n"
						exit 1;
					else
						printf "\\n\\tYUM dependencies installed successfully.\\n"
					fi
				break;;
				[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else 
		printf "\\n\\tNo required YUM dependencies to install.\\n"
	fi

	if [ "${ENABLE_COVERAGE_TESTING}" = true ]; then
		printf "\\n\\tCode coverage build requested."
		printf "\\n\\tChecking perl installation.\\n"
		perl_bin=$( command -v perl 2>/dev/null )
		if [ -z "${perl_bin}" ]; then
			printf "\\n\\tInstalling perl.\\n"
			if ! sudo "${YUM}" -y install perl
			then
				printf "\\n\\tUnable to install perl at this time.\\n"
				printf "\\n\\tExiting now.\\n\\n"
				exit 1;
			fi
		else
			printf "\\tPerl installation found at %s.\\n" "${perl_bin}"
		fi
		printf "\\n\\tChecking LCOV installation."
		if [ ! -e "/usr/local/bin/lcov" ]; then
			printf "\\n\\tLCOV installation not found.\\n"
			printf "\\tInstalling LCOV.\\n"
			if ! cd "${TEMP_DIR}"
			then
				printf "\\n\\tUnable to enter %s. Exiting now.\\n" "${TEMP_DIR}"
				exit 1;
			fi
			if ! git clone "https://github.com/linux-test-project/lcov.git"
			then
				printf "\\n\\tUnable to clone LCOV at this time.\\n"
				printf "\\tExiting now.\\n\\n"
				exit 1;
			fi
			if ! cd "${TEMP_DIR}/lcov"
			then
				printf "\\n\\tUnable to enter %s/lcov. Exiting now.\\n" "${TEMP_DIR}"
				exit 1;
			fi
			if ! sudo make install
			then
				printf "\\n\\tUnable to install LCOV at this time.\\n"
				printf "\\tExiting now.\\n\\n"
				exit 1;
			fi
			rm -rf "${TEMP_DIR}/lcov"
			printf "\\n\\tSuccessfully installed LCOV.\\n\\n"
		else
			printf "\\n\\tLCOV installation found @ /usr/local/bin.\\n"
		fi
	fi

if [ $ANSWER != 1 ]; then read -p "Do you wish to update YUM repositories? (y/n) " ANSWER; fi
case $ANSWER in
	1 | [Yy]* )
		if ! sudo $YUM -y update; then
			printf " - YUM update failed.\\n"
			exit 1;
		else
			printf " - YUM update complete.\\n"
		fi
	;;
	[Nn]* ) echo " - Proceeding without update!";;
	* ) echo "Please type 'y' for yes or 'n' for no."; exit;;
esac

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
	if [ $ANSWER != 1 ]; then read -p "Do you wish to install these dependencies? (y/n) " ANSWER; fi
	case $ANSWER in
		1 | [Yy]* )
			if ! sudo $YUM -y install ${DEP}; then
				printf " - YUM dependency installation failed!\\n"
				exit 1;
			else
				printf " - YUM dependencies installed successfully.\\n"
			fi
		;;
		[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
		* ) echo "Please type 'y' for yes or 'n' for no."; exit;;
	esac
else
	printf " - No required YUM dependencies to install.\\n"
fi

printf "\\n"


printf "Checking CMAKE installation...\\n"
if [ ! -e $CMAKE ]; then
	printf "Installing CMAKE...\\n"
	curl -LO https://cmake.org/files/v$CMAKE_VERSION_MAJOR.$CMAKE_VERSION_MINOR/cmake-$CMAKE_VERSION.tar.gz \
	&& tar -xzf cmake-$CMAKE_VERSION.tar.gz \
	&& cd cmake-$CMAKE_VERSION \
	&& ./bootstrap --prefix=$HOME \
	&& make -j"${JOBS}" \
	&& make install \
	&& cd .. \
	&& rm -f cmake-$CMAKE_VERSION.tar.gz \
	|| exit 1
	printf " - CMAKE successfully installed @ ${CMAKE} \\n"
else
	printf " - CMAKE found @ ${CMAKE}.\\n"
fi
if [ $? -ne 0 ]; then exit -1; fi


printf "\\n"


printf "Checking Boost library (${BOOST_VERSION}) installation...\\n"
BOOSTVERSION=$( grep "#define BOOST_VERSION" "$HOME/opt/boost/include/boost/version.hpp" 2>/dev/null | tail -1 | tr -s ' ' | cut -d\  -f3 )
if [ "${BOOSTVERSION}" != "${BOOST_VERSION_MAJOR}0${BOOST_VERSION_MINOR}0${BOOST_VERSION_PATCH}" ]; then
	printf "Installing Boost library...\\n"
	curl -LO https://dl.bintray.com/boostorg/release/${BOOST_VERSION_MAJOR}.${BOOST_VERSION_MINOR}.${BOOST_VERSION_PATCH}/source/boost_$BOOST_VERSION.tar.bz2 \
	&& tar -xjf boost_$BOOST_VERSION.tar.bz2 \
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
if [ $? -ne 0 ]; then exit -1; fi


printf "\\n"


printf "Checking MongoDB installation...\\n"
if [ ! -d $MONGODB_ROOT ]; then
	printf "Installing MongoDB into ${MONGODB_ROOT}...\\n"
	curl -OL https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-amazon-$MONGODB_VERSION.tgz \
	&& tar -xzf mongodb-linux-x86_64-amazon-$MONGODB_VERSION.tgz \
	&& mv $SRC_LOCATION/mongodb-linux-x86_64-amazon-$MONGODB_VERSION $MONGODB_ROOT \
	&& touch $MONGODB_LOG_LOCATION/mongod.log \
	&& rm -f mongodb-linux-x86_64-amazon-$MONGODB_VERSION.tgz \
	&& cp -f $REPO_ROOT/scripts/mongod.conf $MONGODB_CONF \
	&& mkdir -p $MONGODB_DATA_LOCATION \
	&& rm -rf $MONGODB_LINK_LOCATION \
	&& rm -rf $BIN_LOCATION/mongod \
	&& ln -s $MONGODB_ROOT $MONGODB_LINK_LOCATION \
	&& ln -s $MONGODB_LINK_LOCATION/bin/mongod $BIN_LOCATION/mongod \
	|| exit 1
	printf " - MongoDB successfully installed @ ${MONGODB_ROOT}\\n"
else
	printf " - MongoDB found with correct version @ ${MONGODB_ROOT}.\\n"
fi
if [ $? -ne 0 ]; then exit -1; fi
printf "Checking MongoDB C driver installation...\\n"
if [ ! -d $MONGO_C_DRIVER_ROOT ]; then
	printf "Installing MongoDB C driver...\\n"
	curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/$MONGO_C_DRIVER_VERSION/mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
	&& tar -xzf mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
	&& cd mongo-c-driver-$MONGO_C_DRIVER_VERSION \
	&& mkdir -p cmake-build \
	&& cd cmake-build \
	&& $CMAKE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME -DENABLE_BSON=ON -DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON .. \
	&& make -j"${JOBS}" \
	&& make install \
	&& cd ../.. \
	&& rm mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
	|| exit 1
	printf " - MongoDB C driver successfully installed @ ${MONGO_C_DRIVER_ROOT}.\\n"
else
	printf " - MongoDB C driver found with correct version @ ${MONGO_C_DRIVER_ROOT}.\\n"
fi
if [ $? -ne 0 ]; then exit -1; fi
printf "Checking MongoDB C++ driver installation...\\n"
if [ ! -d $MONGO_CXX_DRIVER_ROOT ]; then
	printf "Installing MongoDB C++ driver...\\n"
	curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r$MONGO_CXX_DRIVER_VERSION.tar.gz -o mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz \
	&& tar -xzf mongo-cxx-driver-r${MONGO_CXX_DRIVER_VERSION}.tar.gz \
	&& cd mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION/build \
	&& $CMAKE -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME .. \
	&& make -j"${JOBS}" VERBOSE=1 \
	&& make install \
	&& cd ../.. \
	&& rm -f mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz \
	|| exit 1
	printf " - MongoDB C++ driver successfully installed @ ${MONGO_CXX_DRIVER_ROOT}.\\n"
else
	printf " - MongoDB C++ driver found with correct version @ ${MONGO_CXX_DRIVER_ROOT}.\\n"
fi
if [ $? -ne 0 ]; then exit -1; fi


printf "\\n"


printf "Checking LLVM 4 support...\\n"
if [ ! -d $LLVM_ROOT ]; then
	ln -s /usr/lib64/llvm4.0 $LLVM_ROOT \
	|| exit 1
	printf " - LLVM successfully linked from /usr/lib64/llvm4.0 to ${LLVM_ROOT}\\n"
else
	printf " - LLVM found @ ${LLVM_ROOT}.\\n"
fi
if [ $? -ne 0 ]; then exit -1; fi


cd ..
printf "\\n"

function print_instructions() {
	return 0
}