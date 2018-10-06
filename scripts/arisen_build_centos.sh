	OS_VER=$( grep VERSION_ID /etc/os-release | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' \
	| cut -d'.' -f1 )

	MEM_MEG=$( free -m | sed -n 2p | tr -s ' ' | cut -d\  -f2 )
	CPU_SPEED=$( lscpu | grep "MHz" | tr -s ' ' | cut -d\  -f3 | cut -d'.' -f1 )
	CPU_CORE=$( lscpu -pCPU | grep -v "#" | wc -l )
	MEM_GIG=$(( ((MEM_MEG / 1000) / 2) ))
	JOBS=$(( MEM_GIG > CPU_CORE ? CPU_CORE : MEM_GIG ))

	DISK_INSTALL=$( df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 )
	DISK_TOTAL_KB=$( df . | tail -1 | awk '{print $2}' )
	DISK_AVAIL_KB=$( df . | tail -1 | awk '{print $4}' )
	DISK_TOTAL=$(( DISK_TOTAL_KB / 1048576 ))
	DISK_AVAIL=$(( DISK_AVAIL_KB / 1048576 ))

	printf "\\n\\tOS name: %s\\n" "${OS_NAME}"
	printf "\\tOS Version: %s\\n" "${OS_VER}"
	printf "\\tCPU speed: %sMhz\\n" "${CPU_SPEED}"
	printf "\\tCPU cores: %s\\n" "${CPU_CORE}"
	printf "\\tPhysical Memory: %s Mgb\\n" "${MEM_MEG}"
	printf "\\tDisk install: %s\\n" "${DISK_INSTALL}"
	printf "\\tDisk space total: %sG\\n" "${DISK_TOTAL%.*}"
	printf "\\tDisk space available: %sG\\n" "${DISK_AVAIL%.*}"
	printf "\\tConcurrent Jobs (make -j): ${JOBS}\\n"

	if [ "${MEM_MEG}" -lt 7000 ]; then
		printf "\\n\\tYour system must have 7 or more Gigabytes of physical memory installed.\\n"
		printf "\\tExiting now.\\n\\n"
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
		pkg=$( sudo "${YUM}" info "${DEP_ARRAY[$i]}" 2>/dev/null | grep Repo | tr -s ' ' | cut -d: -f2 | sed 's/ //g' )
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

	printf "\\tChecking boost library installation...\\n"
	BOOSTVERSION=$( grep "#define BOOST_VERSION" "${BOOST_ROOT}/include/boost/version.hpp" 2>/dev/null \
	| tail -1 | tr -s ' ' | cut -d\  -f3)
	if [ "${BOOSTVERSION}" != "106700" ]; then
		printf "\\tRemoving existing boost libraries in %s/opt/boost*...\\n" "${HOME}"
		if ! rm -rf "${HOME}"/opt/boost*; then
			printf "\\t!! Unable to remove deprecated boost libraries at %s/opt/boost* !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		printf "\\tInstalling boost libraries...\\n"
		if ! cd "${TEMP_DIR}"; then
			printf "\\t!! Unable to enter directory %s !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		BOOSTURL="https://dl.bintray.com/boostorg/release/1.67.0/source/${BOOSTTGZ}"
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 "${BOOSTURL}")
		if [ "${STATUS}" -ne 200 ]; then
			printf "\\t!! Unable to download Boost libraries from ${BOOSTURL} !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! tar xf "${TEMP_DIR}/${BOOSTTGZ}"; then
			printf "\\t!! Unable to unarchive file %s/${BOOSTTGZ} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -f  "${TEMP_DIR}/${BOOSTTGZ}"; then
			printf "\\t!! Unable to remove file %s/${BOOSTTGZ} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/${BOOSTFOLDER}/"; then
			printf "\\t!! Unable to enter directory %s/${BOOSTFOLDER} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! ./bootstrap.sh --prefix=$BOOST_ROOT; then
			printf "\\t!! Installation of boost libraries failed with the above error !! 0\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! $TEMP_DIR/$BOOSTFOLDER/b2 -j"${JOBS}" install; then
			printf "\\t!! Installation of boost libraries in ${BOOST_ROOT} failed with the above error !! 1\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! rm -rf "${TEMP_DIR}/${BOOSTFOLDER}/"; then
			printf "\\t!! Unable to remove directory %s/boost_1_67_0 !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if [ -d "$BUILD_DIR" ]; then
			if ! rm -rf "$BUILD_DIR"; then
				printf "\\t!!Unable to remove directory %s: Please manually remove and try again !! 0\\n" "$BUILD_DIR" "${BASH_SOURCE[0]}"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
		fi
		printf "\\tBoost successfully installed @ %s.\\n" "${BOOST_ROOT}"
	else
		printf "\\t - Boost ${BOOSTVERSION} found at %s.\\n" "${BOOST_ROOT}"
	fi

	printf "\\n"

	printf "\\tChecking MongoDB installation.\\n"
	if [ ! -e "${MONGOD_CONF}" ]; then
		printf "\\tInstalling MongoDB 3.6.3...\\n"
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
		MONGOTGZ="mongodb-linux-x86_64-3.6.3.tgz"
                MONGOURL="https://fastdl.mongodb.org/linux/${MONGOTGZ}"
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 "${MONGOURL}")
		if [ "${STATUS}" -ne 200 ]; then
			printf "\\t!! Unable to download MongoDB from ${MONGOURL} !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! tar xf "${HOME}/opt/${MONGOTGZ}"; then
			printf "\\t!! Unable to unarchive file %s/opt/${MONGOTGZ} !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! rm -f "${HOME}/opt/${MONGOTGZ}"; then
			printf "\\t!! Unable to remove file %s/opt/${MONGOTGZ} !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		MONGOFOLDER=$(echo "${MONGOTGZ}" | sed 's/.tgz//g')
		if ! ln -s "${HOME}/opt/${MONGOFOLDER}/" "${HOME}/opt/mongodb"; then
			printf "\\t!! Unable to symlink file %s/opt/${MONGOFOLDER} to %s/opt/mongodb !!\\n" "${HOME}" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir "${HOME}/opt/mongodb/data"; then
			printf "\\t!! Unable to create directory %s/opt/mongodb/data !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir "${HOME}/opt/mongodb/log"; then
			printf "\\t!! Unable to create directory %s/opt/mongodb/log !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! touch "${HOME}/opt/mongodb/log/mongodb.log"; then
			printf "\\t!! Unable to create file %s/opt/mongodb/log/mongodb.log !!\\n" "${HOME}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi

		printf "\\n"

		if ! tee > /dev/null "${MONGOD_CONF}" <<mongodconf
systemLog:
 destination: file
 path: ${HOME}/opt/mongodb/log/mongodb.log
 logAppend: true
 logRotate: reopen
net:
 bindIp: 127.0.0.1,::1
 ipv6: true
storage:
 dbPath: ${HOME}/opt/mongodb/data
mongodconf
		then
			printf "\\t!! Unable to write to file %s !!\\n" "${MONGOD_CONF}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		printf "\\tMongoDB successfully installed @ %s/opt/mongodb.\\n" "${HOME}"
	else
		printf "\\t - MongoDB config found at %s.\\n" "${MONGOD_CONF}"
	fi

	printf "\\tChecking MongoDB C++ driver installation...\\n"
	MONGO_INSTALL=true
	if [ -e "/usr/local/lib64/libmongocxx-static.a" ]; then
		MONGO_INSTALL=false
		if [ ! -f /usr/local/lib64/pkgconfig/libmongocxx-static.pc ]; then
			MONGO_INSTALL=true
		else
			if ! version=$( grep "Version:" /usr/local/lib64/pkgconfig/libmongocxx-static.pc | tr -s ' ' | awk '{print $2}' ); then
				printf "\\t!! Unable to determine mongodb-cxx-driver version !!\\n"
				printf "\\tExiting now.\\n"
				exit 1;
			fi
			maj=$( echo "${version}" | cut -d'.' -f1 )
			min=$( echo "${version}" | cut -d'.' -f2 )
			if [ "${maj}" -gt 3 ]; then
				MONGO_INSTALL=true
			elif [ "${maj}" -eq 3 ] && [ "${min}" -lt 3 ]; then
				MONGO_INSTALL=true
			fi
		fi
	fi

	if [ $MONGO_INSTALL == "true" ]; then
		if ! cd "${TEMP_DIR}"; then
			printf "\\t!! Unable to enter directory %s !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		MONGODRIVERTGZ="mongo-c-driver-1.10.2.tar.gz"
		MONGODRIVERURL="https://github.com/mongodb/mongo-c-driver/releases/download/1.10.2/${MONGODRIVERTGZ}"
		STATUS=$( curl -LO -w '%{http_code}' --connect-timeout 30 "${MONGODRIVERURL}" )
		if [ "${STATUS}" -ne 200 ]; then
			if ! rm -f "${TEMP_DIR}/${MONGODRIVERTGZ}"; then
				printf "\\t!! Unable to remove file %s/${MONGODRIVERTGZ} !!\\n" "${TEMP_DIR}"
			fi
			printf "\\t!! Unable to download MongoDB C driver at this time !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! tar xf "${MONGODRIVERTGZ}"; then
			printf "\\t!! Unable to unarchive file %s/${MONGODRIVERTGZ} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! rm -f "${TEMP_DIR}/${MONGODRIVERTGZ}"; then
			printf "\\t!! Unable to remove file ${MONGODRIVERTGZ} !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		MONGODRIVERFOLDER=$(echo $MONGODRIVERTGZ | sed 's/.tar.gz//g')
		if ! cd "${TEMP_DIR}/${MONGODRIVERFOLDER}"; then
			printf "\\t!! Unable to cd into directory %s/${MONGODRIVERFOLDER} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir cmake-build; then
			printf "\\t!! Unable to create directory %s/${MONGODRIVERFOLDER}/cmake-build !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd cmake-build; then
			printf "\\t!! Unable to enter directory %s/${MONGODRIVERFOLDER}/cmake-build !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! "${CMAKE}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_BSON=ON \
		-DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON ..;then
			printf "\\t!! Configuring MongoDB C driver has encountered the errors above !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! make -j"${JOBS}"; then
			printf "\\t!! Error compiling MongoDB C driver !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! sudo make install; then
			printf "\\t!! Error installing MongoDB C driver: Make sure you have sudo privileges !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}"; then
			printf "\\t!! Unable to enter directory %s !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! rm -rf "${TEMP_DIR}/${MONGODRIVERFOLDER}"; then
			printf "\\t!! Unable to remove directory %s/${MONGODRIVERFOLDER} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/v3.3 --depth 1; then
			printf "\\t!! Unable to clone MongoDB C++ driver at this time. !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/mongo-cxx-driver/build"; then
			printf "\\t!! Unable to enter directory %s/mongo-cxx-driver/build !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! "${CMAKE}" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..;then
			printf "\\t!! Cmake has encountered the above errors building the MongoDB C++ driver !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! sudo make -j"${JOBS}"; then
			printf "\\t!! Error compiling MongoDB C++ driver !!\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo make install; then
			printf "\\t!! Error installing MongoDB C++ driver.\\nMake sure you have sudo privileges !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}"; then
			printf "\\t!! Unable to enter directory %s !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! sudo rm -rf "${TEMP_DIR}/mongo-cxx-driver"; then
			printf "\\t!! Unable to remove directory %s/mongo-cxx-driver !!\\n" "${TEMP_DIR}" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		printf "\\tMongo C++ driver installed at /usr/local/lib64/libmongocxx-static.a.\\n"
	else
		printf "\\t - Mongo C++ driver found at /usr/local/lib64/libmongocxx-static.a.\\n"
	fi

	printf "\\n"

	printf "\\tChecking LLVM with WASM support installation...\\n"
	if [ ! -d "${HOME}/opt/wasm/bin" ]; then
		printf "\\tInstalling LLVM with WASM...\\n"
		if ! cd "${TEMP_DIR}"; then
			printf "\\t!! Unable to enter directory %s !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir "${TEMP_DIR}/llvm-compiler"  2>/dev/null; then
			printf "\\t!! Unable to create directory %s/llvm-compiler !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/llvm-compiler"; then
			printf "\\t!! Unable to enter directory %s/llvm-compiler !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		LLVMURL="https://github.com/llvm-mirror/llvm.git"
		if ! git clone --depth 1 --single-branch --branch release_40 "${LLVMURL}"; then
			printf "\\t!! Unable to clone llvm repo from ${LLVMURL} !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		LLVMLOCATION="llvm-compiler/llvm/tools"
		if ! cd "${TEMP_DIR}/${LLVMLOCATION}"; then
			printf "\\t!! Unable to enter directory %s/${LLVMLOCATION} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		CLANGURL="https://github.com/llvm-mirror/clang.git"
		if ! git clone --depth 1 --single-branch --branch release_40 "${CLANGURL}"; then
			printf "\\t!! Unable to clone clang repo from ${CLANGURL} !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		LLVMMIDLOCATION=$(echo $LLVMLOCATION | sed 's/\/tools//g')
		if ! cd "${TEMP_DIR}/${LLVMMIDLOCATION}"; then
			printf "\\t!! Unable to enter directory %s/${LLVMMIDLOCATION} !!\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir "${TEMP_DIR}/${LLVMMIDLOCATION}/build" 2>/dev/null; then
			printf "\\t!! Unable to create directory %s/${LLVMMIDLOCATION}/build !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/${LLVMMIDLOCATION}/build"; then
			printf "\\t!! Unable to enter directory %s/${LLVMMIDLOCATION}/build !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! "${CMAKE}" -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="${HOME}/opt/wasm" \
		-DLLVM_TARGETS_TO_BUILD="host" -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="WebAssembly" \
		-DLLVM_ENABLE_RTTI=1 -DCMAKE_BUILD_TYPE="Release" ..; then
			printf "\\t!! CMake has exited with the above error !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! make -j"${JOBS}"; then
			printf "\\t!! Compiling LLVM with EXPERIMENTAL WASM support has exited with the above errors !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		if ! make install; then
			printf "\\t!! Installing LLVM with EXPERIMENTAL WASM support has exited with the above errors !!\\n"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		LLVMFOLDER=$(echo $LLVMMIDLOCATION | | sed 's/\/llvm//g')
		if ! rm -rf "${TEMP_DIR}/${LLVMFOLDER}" 2>/dev/null; then
			printf "\\t!! Unable to remove directory %s/${LLVMFOLDER} !!\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n"
			exit 1;
		fi
		printf "\\tWASM compiler successfully installed at %s/opt/wasm\\n" "${HOME}"
	else
		printf "\\t - WASM found at %s/opt/wasm\\n" "${HOME}"
	fi

	printf "\\n"

	function print_instructions()
	{
		printf "\\t%s -f %s &\\n" "$( command -v mongod )" "${MONGOD_CONF}"
		printf "\\tsource /opt/rh/python33/enable\\n"
		printf '\texport PATH=${HOME}/opt/mongodb/bin:$PATH\n'
		printf "\\tcd %s; make test\\n\\n" "${BUILD_DIR}"
		return 0
	}
