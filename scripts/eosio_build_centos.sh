if [ $1 == 1 ]; then ANSWER=1; else ANSWER=0; fi

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

	if [ "${OS_VER}" -lt 7 ]; then
		printf "\\n\\tYou must be running Centos 7 or higher to install EOSIO.\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi

	if [ "${DISK_AVAIL%.*}" -lt "${DISK_MIN}" ]; then
		printf "\\n\\tYou must have at least %sGB of available storage to install EOSIO.\\n" "${DISK_MIN}"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi

	printf "\\n"

        printf "\\tChecking Yum installation...\\n"
        if ! YUM=$( command -v yum 2>/dev/null ); then
                printf "\\t!! Yum must be installed to compile EOS.IO !!\\n"
                printf "\\tExiting now.\\n"
                exit 1;
        fi
        printf "\\t- Yum installation found at %s.\\n" "${YUM}"

        printf "\\tUpdating YUM repository...\\n"
        if ! sudo "${YUM}" -y update > /dev/null 2>&1; then
                printf "\\t!! YUM update failed !!\\n"
                printf "\\tExiting now.\\n"
                exit 1;
        fi
        printf "\\t - YUM repository successfully updated.\\n"

	printf "\\tChecking installation of Centos Software Collections Repository...\\n"
	SCL=$( rpm -qa | grep -E 'centos-release-scl-[0-9].*' )
	if [ -z "${SCL}" ]; then
		printf "\\t - Do you wish to install and enable this repository?\\n"
		if is_noninteractive; then exec <<< "1"; fi
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* )
					printf "\\tInstalling SCL...\\n"
					if ! sudo "${YUM}" -y --enablerepo=extras install centos-release-scl 2>/dev/null; then
						printf "\\t!! Centos Software Collections Repository installation failed !!\\n"
						printf "\\tExiting now.\\n\\n"
						exit 1;
					else
						printf "\\tCentos Software Collections Repository installed successfully.\\n"
					fi
				break;;
				[Nn]* ) echo "\\tUser aborting installation of required Centos Software Collections Repository, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else
		printf "\\t - ${SCL} found.\\n"
	fi

        printf "\\tChecking installation of devtoolset-7...\\n"
        DEVTOOLSET=$( rpm -qa | grep -E 'devtoolset-7-[0-9].*' )
        if [ -z "${DEVTOOLSET}" ]; then
                printf "\\tDo you wish to install devtoolset-7?\\n"
                if is_noninteractive; then exec <<< "1"; fi
                select yn in "Yes" "No"; do
                        case $yn in
                                [Yy]* )
                                        printf "\\tInstalling devtoolset-7...\\n"
                                        if ! sudo "${YUM}" install -y devtoolset-7 2>/dev/null; then
                                                printf "\\t!! Centos devtoolset-7 installation failed !!\\n"
                                                printf "\\tExiting now.\\n"
                                                exit 1;
                                        else
                                                printf "\\tCentos devtoolset installed successfully.\\n"
                                        fi
                                break;;
                                [Nn]* ) echo "User aborting installation of devtoolset-7. Exiting now."; exit;;
                                * ) echo "Please type 1 for yes or 2 for no.";;
                        esac
                done
        else
                printf "\\t - ${DEVTOOLSET} found.\\n"
        fi
	printf "\\tEnabling Centos devtoolset-7...\\n"
	if ! source "/opt/rh/devtoolset-7/enable" 2>/dev/null; then
		printf "\\t!! Unable to enable Centos devtoolset-7 at this time !!\\n"
		printf "\\tExiting now.\\n\\n"
		exit 1;
	fi
	printf "\\tCentos devtoolset-7 successfully enabled.\\n"

        printf "\\tChecking installation of python33...\\n"
        PYTHON33=$( rpm -qa | grep -E 'python33-[0-9].*' )
        if [ -z "${PYTHON33}" ]; then
                printf "\\tDo you wish to install python33?\\n"
                if is_noninteractive; then exec <<< "1"; fi
                select yn in "Yes" "No"; do
                        case $yn in
                                [Yy]* )
                                        printf "\\tInstalling Python33...\\n"
                                        if ! sudo "${YUM}" install -y python33.x86_64 2>/dev/null; then
                                                printf "\\t!! Centos Python33 installation failed !!\\n"
                                                printf "\\tExiting now.\\n"
                                                exit 1;
                                        else
                                                printf "\\n\\tCentos Python33 installed successfully.\\n"
                                        fi

                                break;;
                                [Nn]* ) echo "User aborting installation of python33. Exiting now."; exit;;
                                * ) echo "Please type 1 for yes or 2 for no.";;
                        esac
                done
        else
                printf "\\t - ${PYTHON33} found.\\n"
        fi

	printf "\\n"

	DEP_ARRAY=( git autoconf automake bzip2 libtool ocaml.x86_64 doxygen graphviz-devel.x86_64 \
	libicu-devel.x86_64 bzip2.x86_64 bzip2-devel.x86_64 openssl-devel.x86_64 gmp-devel.x86_64 \
	python-devel.x86_64 gettext-devel.x86_64)
	COUNT=1
	DISPLAY=""
	DEP=""

	printf "\\tChecking YUM for installed dependencies.\\n"

	for (( i=0; i<${#DEP_ARRAY[@]}; i++ ));
	do
		pkg=$( "${YUM}" info "${DEP_ARRAY[$i]}" 2>/dev/null | grep Repo | tr -s ' ' | cut -d: -f2 | sed 's/ //g' )
		if [ "$pkg" != "installed" ]; then
			DEP=$DEP" ${DEP_ARRAY[$i]} "
			DISPLAY="${DISPLAY}${COUNT}. ${DEP_ARRAY[$i]}\\n\\t"
			printf "\\t!! Package %s ${bldred} NOT ${txtrst} found !!\\n" "${DEP_ARRAY[$i]}"
			(( COUNT++ ))
		else
			printf "\\t - Package %s found.\\n" "${DEP_ARRAY[$i]}"
			continue
		fi
	done

	printf "\\n"

	if [ "${COUNT}" -gt 1 ]; then
		printf "\\tThe following dependencies are required to install EOSIO.\\n"
		printf "\\t${DISPLAY}\\n\\n"
		printf "\\tDo you wish to install these dependencies?\\n"
		if is_noninteractive; then exec <<< "1"; fi
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* )
					printf "\\tInstalling dependencies\\n\\n"
					if ! sudo "${YUM}" -y install ${DEP}; then
						printf "\\t!! YUM dependency installation failed !!\\n"
						printf "\\tExiting now.\\n"
						exit 1;
					else
						printf "\\tYUM dependencies installed successfully.\\n"
					fi
				break;;
				[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else
		printf "\\t - No required YUM dependencies to install.\\n"
	fi

	printf "\\n"

	if [ "${ENABLE_COVERAGE_TESTING}" = true ]; then
		printf "\\tChecking perl installation...\\n"
		perl_bin=$( command -v perl 2>/dev/null )
		if [ -z "${perl_bin}" ]; then
			printf "\\tInstalling perl...\\n"
			if ! sudo "${YUM}" -y install perl; then
				printf "\\t!! Unable to install perl at this time !!\\n"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
		else
			printf "\\t - Perl installation found at %s.\\n" "${perl_bin}"
		fi

		printf "\\n"

		printf "\\tChecking LCOV installation...\\n"
		lcov=$( command -v lcov 2>/dev/null )
		if [ -z  "${lcov}" ]; then
			printf "\\tInstalling LCOV...\\n"
			if ! cd "${TEMP_DIR}"; then
				printf "\\t!! Unable to enter directory %s !!\\n" "${TEMP_DIR}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			LCOVURL="https://github.com/linux-test-project/lcov.git"
			if ! git clone "${LCOVURL}"; then
				printf "\\t!! Unable to clone LCOV from ${LCOVURL} !!\\n"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			if ! cd "${TEMP_DIR}/lcov"; then
				printf "\\t!! Unable to enter directory %s/lcov !!\\n" "${TEMP_DIR}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			if ! sudo make install; then
				printf "\\t!! Unable to install LCOV at this time !!\\n"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			if ! cd "${CWD}"; then
				printf "\\t!! Unable to enter directory %s !!\\n" "${CWD}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			if ! rm -rf "${TEMP_DIR}/lcov"; then
				printf "\\t!! Unable to remove directory %s/lcov !!\\n" "${TEMP_DIR}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			printf "\\tSuccessfully installed LCOV.\\n"
		else
			printf "\\t - LCOV installation found @ %s.\\n" "${lcov}"
		fi
	fi

	printf "\\n"

	printf "\\tChecking CMAKE installation...\\n"
	if [ ! -e "${CMAKE}" ]; then
		printf "\\tInstalling CMAKE...\\n"
		if [ ! -d "${HOME}/opt" ]; then
			if ! mkdir "${HOME}/opt"; then
				printf "\\t!! Unable to create directory %s/opt !!\\n" "${HOME}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
		fi
		if ! cd "${HOME}/opt"; then
			printf "\\t!! Unable to enter directory %s/opt !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		CMAKETGZ="cmake-3.10.2.tar.gz"
		CMAKEURL="https://cmake.org/files/v3.10/${CMAKETGZ}"
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 "${CMAKEURL}")
		if [ "${STATUS}" -ne 200 ]; then
			printf "\\t!! Unable to download CMAKE from ${CMAKEURL} !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! tar xf "${HOME}/opt/${CMAKETGZ}"; then
			printf "\\t!! Unable to unarchive %s/opt/CMAKETGZ} !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! rm -f "${HOME}/opt/${CMAKETGZ}"; then
			printf "\\t!! Unable to remove %s/opt/${CMAKETGZ} !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		CMAKEFOLDER=$(echo $CMAKETGZ | sed 's/.tar.gz//g')
		if ! ln -s "${HOME}/opt/${CMAKEFOLDER}/" "${HOME}/opt/cmake"; then
			printf "\\t!! Unable to symlink %s/opt/${CMAKEFOLDER} to %s/opt/${CMAKEFOLDER}/cmake !!\\n" "${HOME}" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${HOME}/opt/cmake"; then
			printf "\\t!! Unable to enter directory %s/opt/cmake !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! ./bootstrap; then
			printf "\\t!! Error running bootstrap for CMAKE from $(pwd) !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! make -j"${JOBS}"; then
			printf "\\t!! Compiling CMAKE has exited with the above error !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		printf "\\tCMAKE successfully installed @ %s.\\n\\n" "${CMAKE}"
	else
		printf "\\t - CMAKE found @ %s.\\n" "${CMAKE}"
	fi

	BOOSTTGZ="boost_1_67_0.tar.bz2"
	BOOSTFOLDER=$(echo "${BOOSTTGZ}" | sed 's/.tar.bz2//g')
	if [ -d "${HOME}/opt/${BOOSTFOLDER}" ]; then
		if ! mv "${HOME}/opt/${BOOSTFOLDER}" "${BOOST_ROOT}"; then
			printf "\\t!! Unable to move directory %s/opt/${BOOSTFOLDER} to %s !!\\n" "${HOME}" "${BOOST_ROOT}"
			printf "\\tExiting now.\\n"
			exit 1
		fi
		if [ -d "$BUILD_DIR" ]; then
			if ! rm -rf "$BUILD_DIR"; then
				printf "\\t!! Unable to remove directory %s: Please delete it and try again !! 0\\n" "$BUILD_DIR" "${BASH_SOURCE[0]}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
		fi
	fi

if [ $ANSWER != 1 ]; then read -p "Do you wish to update YUM repositories? (y/n) " ANSWER; fi
case $ANSWER in
	1 | [Yy]* )
		if ! "${YUM}" -y update; then
			printf " - YUM update failed.\\n"
			exit 1;
		else
			printf " - YUM update complete.\\n"
		fi
	;;
	[Nn]* ) echo " - Proceeding without update!";;
	* ) echo "Please type 'y' for yes or 'n' for no."; exit;;
esac

printf "Checking installation of Centos Software Collections Repository...\\n"
SCL=$( rpm -qa | grep -E 'centos-release-scl-[0-9].*' )
if [ -z "${SCL}" ]; then
	if [ $ANSWER != 1 ]; then read -p "Do you wish to install and enable this repository? (y/n)? " ANSWER; fi
	case $ANSWER in
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
	* ) echo "Please type 'y' for yes or 'n' for no."; exit;;
	esac
else
	printf " - ${SCL} found.\\n"
fi

printf "Checking installation of devtoolset-7...\\n"
DEVTOOLSET=$( rpm -qa | grep -E 'devtoolset-7-[0-9].*' )
if [ -z "${DEVTOOLSET}" ]; then
	if [ $ANSWER != 1 ]; then read -p "Do you wish to install devtoolset-7? (y/n)? " ANSWER; fi
	case $ANSWER in
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
		* ) echo "Please type 'y' for yes or 'n' for no."; exit;;
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
	ocaml libicu-devel python python-devel python33 \
	gettext-devel file sudo libusbx-devel libcurl-devel
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
	if [ $ANSWER != 1 ]; then read -p "Do you wish to install these dependencies? (y/n) " ANSWER; fi
	case $ANSWER in
		1 | [Yy]* )
			if ! "${YUM}" -y install ${DEP}; then
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

if [ -d /opt/rh/python33 ]; then
	printf "Enabling python33...\\n"
	source /opt/rh/python33/enable || exit 1
	printf " - Python33 successfully enabled!\\n"
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


export CPATH="$CPATH:/opt/rh/python33/root/usr/include/python3.3m" # m on the end causes problems with boost finding python3
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
	printf " - MongoDB successfully installed @ ${MONGODB_ROOT} (Symlinked to ${MONGODB_LINK_LOCATION}).\\n"
else
	printf " - MongoDB found with correct version @ ${MONGODB_ROOT} (Symlinked to ${MONGODB_LINK_LOCATION}).\\n"
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
	printf "Installing LLVM 4...\\n"
	cd ../opt \
	&& git clone --depth 1 --single-branch --branch $LLVM_VERSION https://github.com/llvm-mirror/llvm.git llvm && cd llvm \
	&& mkdir build \
	&& cd build \
	&& $CMAKE -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="${LLVM_ROOT}" -DLLVM_TARGETS_TO_BUILD="host" -DLLVM_BUILD_TOOLS=false -DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE="Release" .. \
	&& make -j"${JOBS}" \
	&& make install \
	&& cd ../.. \
	|| exit 1
	printf " - LLVM successfully installed @ ${LLVM_ROOT}\\n"
else
	printf " - LLVM found @ ${LLVM_ROOT}.\\n"
fi
if [ $? -ne 0 ]; then exit -1; fi


cd ..
printf "\\n"

function print_instructions() {
	printf "source /opt/rh/python33/enable\\n"
	printf "source /opt/rh/devtoolset-7/enable\\n"
	return 0
}
