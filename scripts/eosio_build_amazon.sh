	OS_VER=$( grep VERSION_ID /etc/os-release | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' | cut -d'.' -f1 )

	MEM_MEG=$( free -m | sed -n 2p | tr -s ' ' | cut -d\  -f2 )
	CPU_SPEED=$( lscpu | grep "MHz" | tr -s ' ' | cut -d\  -f3 | cut -d'.' -f1 )
	CPU_CORE=$( lscpu -pCPU | grep -v "#" | wc -l )
	MEM_GIG=$(( ((MEM_MEG / 1000) / 2) ))
	JOBS=$(( MEM_GIG > CPU_CORE ? CPU_CORE : MEM_GIG ))

	DISK_TOTAL=$( df -h . | grep /dev | tr -s ' ' | cut -d\  -f2 | sed 's/[^0-9]//' )
	DISK_AVAIL=$( df -h . | grep /dev | tr -s ' ' | cut -d\  -f4 | sed 's/[^0-9]//' )

	printf "\\n\\tOS name: %s\\n" "${OS_NAME}"
	printf "\\tOS Version: %s\\n" "${OS_VER}"
	printf "\\tCPU speed: %sMhz\\n" "${CPU_SPEED}"
	printf "\\tCPU cores: %s\\n" "${CPU_CORE}"
	printf "\\tPhysical Memory: %sMgb\\n" "${MEM_MEG}"
	printf "\\tDisk space total: %sGb\\n" "${DISK_TOTAL}"
	printf "\\tDisk space available: %sG\\n" "${DISK_AVAIL}"

	if [ "${MEM_MEG}" -lt 7000 ]; then
		printf "\\tYour system must have 7 or more Gigabytes of physical memory installed.\\n"
		printf "\\texiting now.\\n"
		exit 1
	fi

	if [ "${OS_VER}" -lt 2017 ]; then
		printf "\\tYou must be running Amazon Linux 2017.09 or higher to install EOSIO.\\n"
		printf "\\texiting now.\\n"
		exit 1
	fi

	if [ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]; then
		printf "\\tYou must have at least %sGB of available storage to install EOSIO.\\n" "${DISK_MIN}"
		printf "\\texiting now.\\n"
		exit 1
	fi

	printf "\\n\\tChecking Yum installation.\\n"
	if ! YUM=$( command -v yum 2>/dev/null )
	then
		printf "\\n\\tYum must be installed to compile EOS.IO.\\n"
		printf "\\n\\tExiting now.\\n"
		exit 1
	fi
	
	printf "\\tYum installation found at %s.\\n" "${YUM}"
	printf "\\tUpdating YUM.\\n"
	if ! UPDATE=$( sudo "$YUM" -y update )
	then
		printf "\\n\\tYUM update failed.\\n"
		printf "\\n\\tExiting now.\\n"
		exit 1
	fi
	printf "\\t%s\\n" "${UPDATE}"

	DEP_ARRAY=( git gcc72.x86_64 gcc72-c++.x86_64 autoconf automake libtool make bzip2 \
	bzip2-devel.x86_64 openssl-devel.x86_64 gmp-devel.x86_64 libstdc++72.x86_64 \
	python27.x86_64 python36-devel.x86_64 libedit-devel.x86_64 doxygen.x86_64 graphviz.x86_64)
	COUNT=1
	DISPLAY=""
	DEP=""

	printf "\\n\\tChecking YUM for installed dependencies.\\n\\n"

	for (( i=0; i<${#DEP_ARRAY[@]}; i++ ));
	do
		pkg=$( sudo "$YUM" info "${DEP_ARRAY[$i]}" 2>/dev/null | grep Repo | tr -s ' ' | cut -d: -f2 | sed 's/ //g' )

		if [ "$pkg" != "installed" ]; then
			DEP=$DEP" ${DEP_ARRAY[$i]} "
			DISPLAY="${DISPLAY}${COUNT}. ${DEP_ARRAY[$i]}\\n\\t"
			printf "\\tPackage %s ${bldred} NOT ${txtrst} found.\\n" "${DEP_ARRAY[$i]}"
			(( COUNT++ ))
		else
			printf "\\tPackage %s found.\\n" "${DEP_ARRAY[$i]}"
			continue
		fi
	done		

	if [ "${COUNT}" -gt 1 ]; then
		printf "\\n\\tThe following dependencies are required to install EOSIO.\\n"
		printf "\\n\\t${DISPLAY}\\n\\n"
		printf "\\tDo you wish to install these dependencies?\\n"
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* ) 
					printf "\\n\\n\\tInstalling dependencies.\\n\\n"
					if ! sudo "${YUM}" -y install ${DEP}
					then
						printf "\\n\\tYUM dependency installation failed.\\n"
						printf "\\n\\tExiting now.\\n"
						exit 1
					else
						printf "\\n\\tYUM dependencies installed successfully.\\n"
					fi
				break;;
				[Nn]* ) printf "\\nUser aborting installation of required dependencies,\\n Exiting now.\\n"; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else 
		printf "\\n\\tNo required YUM dependencies to install.\\n"
	fi

	if [ "${ENABLE_COVERAGE_TESTING}" = true ]; then
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
		printf "\\n\\tChecking LCOV installation.\\n"
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

	printf "\\n\\tChecking CMAKE installation.\\n"
    if [ ! -e "${CMAKE}" ]; then
		printf "\\tInstalling CMAKE.\\n"
		if ! mkdir -p "${HOME}/opt/" 2>/dev/null
		then
			printf "\\n\\tUnable to create directory %s/opt.\\n" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${HOME}/opt"
		then
			printf "\\n\\tUnable to enter directory %s/opt.\\n" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		STATUS=$( curl -LO -w '%{http_code}' --connect-timeout 30 "https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz" )
		if [ "${STATUS}" -ne 200 ]; then
			printf "\\tUnable to clone CMAKE repo.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! tar xf "${HOME}/opt/cmake-3.10.2.tar.gz"
		then
			printf "\\tUnable to unarchive file %s/opt/cmake-3.10.2.tar.gz at this time.\\n" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -f "${HOME}/opt/cmake-3.10.2.tar.gz"
		then
			printf "\\tUnable to remove file %s/opt/cmake-3.10.2.tar.gz.\\n" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ln -s "${HOME}/opt/cmake-3.10.2/" "${HOME}/opt/cmake"
		then
			printf "\\tUnable to symlink directory %s/opt/cmake-3.10.2/ to %s/opt/cmake at this time.\\n" "${HOME}" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${HOME}/opt/cmake/"
		then
			printf "\\n\\tUnable to change directory into %s/opt/cmake.\\n" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ./bootstrap
		then
			printf "\\tRunning bootstrap for CMAKE exited with the above error.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! make -j"${JOBS}"
		then
			printf "\\tError compiling CMAKE.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		printf "\\tCMAKE successfully installed @ %s.\\n" "${CMAKE}"
	else
		printf "\\tCMAKE found @ %s.\\n" "${CMAKE}"
	fi

	if [ -d "${HOME}/opt/boost_1_67_0" ]; then
		if ! mv "${HOME}/opt/boost_1_67_0" "$BOOST_ROOT"
		then
			printf "\\n\\tUnable to move directory %s/opt/boost_1_67_0 to %s.\\n" "${HOME}" "${BOOST_ROOT}"
			printf "\\n\\tExiting now.\\n"
			exit 1
		fi
		if [ -d "$BUILD_DIR" ]; then
			if ! rm -rf "$BUILD_DIR"
			then
			printf "\\tUnable to remove directory %s. Please remove this directory and run this script %s again. 0\\n" "$BUILD_DIR" "${BASH_SOURCE[0]}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
			fi
		fi
	fi
	
	printf "\\n\\tChecking boost library installation.\\n"
	BVERSION=$( grep "BOOST_LIB_VERSION" "${BOOST_ROOT}/include/boost/version.hpp" 2>/dev/null \
	| tail -1 | tr -s ' ' | cut -d\  -f3 | sed 's/[^0-9\._]//gI' )
	if [ "${BVERSION}" != "1_67" ]; then
		printf "\\tRemoving existing boost libraries in %s/opt/boost*.\\n" "${HOME}"
		if ! rm -rf "${HOME}"/opt/boost*
		then
			printf "\\n\\tUnable to remove deprecated boost libraries at this time.\\n"
			printf "\\n\\tExiting now.\\n"
			exit 1
		fi
		printf "\\tInstalling boost libraries.\\n"
		if ! cd "${TEMP_DIR}"
		then
			printf "\\n\\tUnable to cd into directory %s at this time.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1
		fi
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 \
		"https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.bz2" )
		if [ "${STATUS}" -ne 200 ]; then
			printf "\\tUnable to download Boost libraries at this time.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! tar xf "${TEMP_DIR}/boost_1_67_0.tar.bz2"
		then
			printf "\\tUnable to decompress Boost libraries @ %s/boost_1_67_0.tar.bz2 at this time.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -f  "${TEMP_DIR}/boost_1_67_0.tar.bz2"
		then
			printf "\\tUnable to remove Boost libraries @ %s/boost_1_67_0.tar.bz2 at this time.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/boost_1_67_0/"
		then
			printf "\\tUnable to change directory into %s/boost_1_67_0/ at this time.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ./bootstrap.sh "--prefix=${BOOST_ROOT}"
		then
			printf "\\n\\tInstallation of boost libraries failed. 0\\n"
			printf "\\n\\tExiting now.\\n"
			exit 1
		fi
		if ! "${TEMP_DIR}"/boost_1_67_0/b2 -j"${CPU_CORE}" install
		then
			printf "\\n\\tInstallation of boost libraries failed. 1\\n"
			printf "\\n\\tExiting now.\\n"
			exit 1
		fi
		if ! rm -rf "${TEMP_DIR}/boost_1_67_0/"
		then
			printf "\\n\\tUnable to remove boost libraries directory @ %s/boost_1_67_0/.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1
		fi
		if [ -d "$BUILD_DIR" ]; then
			if ! rm -rf "$BUILD_DIR"
			then
			printf "\\tUnable to remove directory %s. Please remove this directory and run this script %s again. 0\\n" "$BUILD_DIR" "${BASH_SOURCE[0]}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
			fi
		fi
		printf "\\tBoost successfully installed @ %s.\\n" "${BOOST_ROOT}"
	else
		printf "\\tBoost found at %s.\\n" "${BOOST_ROOT}"
	fi

	printf "\\n\\tChecking MongoDB installation.\\n"
	if [ ! -e "${MONGOD_CONF}" ]; then
		printf "\\tInstalling MongoDB 3.6.3.\\n"
		if ! cd "${HOME}/opt"
		then
			printf "\\n\\tUnable to cd into directory %s/opt.\\n" "${HOME}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		STATUS=$( curl -LO -w '%{http_code}' --connect-timeout 30 \
		"https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-amazon-3.6.3.tgz" )
		if [ "${STATUS}" -ne 200 ]; then
			printf "\\tUnable to download MongoDB at this time.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! tar xf "${HOME}/opt/mongodb-linux-x86_64-amazon-3.6.3.tgz"
		then
			printf "\\tUnable to decompress file %s at this time.\\n" \
			"${HOME}/opt/mongodb-linux-x86_64-amazon-3.6.3.tgz"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -f "${HOME}/opt/mongodb-linux-x86_64-amazon-3.6.3.tgz"
		then
			printf "\\tUnable to remove file %s/opt/mongodb-linux-x86_64-amazon-3.6.3.tgz at this time.\\n" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ln -s "${HOME}/opt/mongodb-linux-x86_64-amazon-3.6.3/" "${HOME}/opt/mongodb"
		then
			printf "\\tUnable to symlink directory %s/opt/mongodb-linux-x86_64-amazon-3.6.3/ to directory %s/opt/mongodb at this time.\\n" \
			"${HOME}" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! mkdir "${HOME}/opt/mongodb/data"
		then
			printf "\\tUnable to create directory %s/opt/mongodb/data at this time.\\n" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! mkdir "${HOME}/opt/mongodb/log"
		then
			printf "\\tUnable to make directory %s/opt/mongodb/log at this time.\\n" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! touch "${HOME}/opt/mongodb/log/mongodb.log"
		then
			printf "\\tUnable to create log file @ %s/opt/mongodb/log/mongodb.log at this time.\\n" "${HOME}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		
if ! tee > "/dev/null" "${MONGOD_CONF}" <<mongodconf
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
	printf "\\tUnable to create mongodb config file @ %s/opt/mongodb/log/mongodb.log at this time.\\n" "${HOME}"
	printf "\\tExiting now.\\n\\n"
	exit 1;
fi
		printf "\\n\\n\\tMongoDB successfully installed.\\n\\tConfiguration file @ %s.\\n\\n" "${MONGOD_CONF}"

	else
		printf "\\tMongoDB configuration found at %s.\\n" "${MONGOD_CONF}"
	fi

	printf "\\n\\tChecking MongoDB C++ driver installation.\\n"
	MONGO_INSTALL=true
    if [ -e "/usr/local/lib64/libmongocxx-static.a" ]; then
		MONGO_INSTALL=false
		if [ ! -f /usr/local/lib64/pkgconfig/libmongocxx-static.pc ]; then
			MONGO_INSTALL=true
		else
			if ! version=$( grep "Version:" /usr/local/lib64/pkgconfig/libmongocxx-static.pc | tr -s ' ' | awk '{print $2}' )
			then
				printf "\\tUnable to determine mongodb-cxx-driver version.\\n"
				printf "\\tExiting now.\\n\\n"
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
		if ! cd "${TEMP_DIR}"
		then
			printf "\\tUnable to enter directory %s.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		STATUS=$( curl -LO -w '%{http_code}' --connect-timeout 30 https://github.com/mongodb/mongo-c-driver/releases/download/1.10.2/mongo-c-driver-1.10.2.tar.gz )
		if [ "${STATUS}" -ne 200 ]; then
			if ! rm -f "${TEMP_DIR}/mongo-c-driver-1.10.2.tar.gz"
			then
				printf "\\tUnable to remove file %s/mongo-c-driver-1.10.2.tar.gz.\\n" "${TEMP_DIR}"
			fi
			printf "\\tUnable to download MongoDB C driver at this time.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! tar xf mongo-c-driver-1.10.2.tar.gz
		then
			printf "\\tUnable to unarchive file %s/mongo-c-driver-1.10.2.tar.gz.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -f "${TEMP_DIR}/mongo-c-driver-1.10.2.tar.gz"
		then
			printf "\\tUnable to remove file mongo-c-driver-1.10.2.tar.gz.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}"/mongo-c-driver-1.10.2
		then
			printf "\\tUnable to cd into directory %s/mongo-c-driver-1.10.2.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! mkdir cmake-build
		then
			printf "\\tUnable to create directory %s/mongo-c-driver-1.10.2/cmake-build.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd cmake-build
		then
			printf "\\tUnable to enter directory %s/mongo-c-driver-1.10.2/cmake-build.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! "${CMAKE}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_BSON=ON \
		-DENABLE_SSL=OPENSSL -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=ON -DENABLE_STATIC=ON ..
		then
			printf "\\tConfiguring MongoDB C driver has encountered the errors above.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! make -j"${CPU_CORE}"
		then
			printf "\\tError compiling MongoDB C driver.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo make install
		then
			printf "\\tError installing MongoDB C driver.\\nMake sure you have sudo privileges.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}"
		then
			printf "\\tUnable to enter directory %s.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -rf "${TEMP_DIR}/mongo-c-driver-1.10.2"
		then
			printf "\\tUnable to remove directory %s/mongo-c-driver-1.10.2.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/v3.3 --depth 1
		then
			printf "\\tUnable to clone MongoDB C++ driver at this time.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/mongo-cxx-driver/build"
		then
			printf "\\tUnable to enter directory %s/mongo-cxx-driver/build.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! "${CMAKE}" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
		then
			printf "\\tCmake has encountered the above errors building the MongoDB C++ driver.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo make -j"${CPU_CORE}"
		then
			printf "\\tError compiling MongoDB C++ driver.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo make install
		then
			printf "\\tError installing MongoDB C++ driver.\\nMake sure you have sudo privileges.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}"
		then
			printf "\\tUnable to enter directory %s.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo rm -rf "${TEMP_DIR}/mongo-cxx-driver"
		then
			printf "\\tUnable to remove directory %s/mongo-cxx-driver.\\n" "${TEMP_DIR}" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		printf "\\tMongo C++ driver installed at /usr/local/lib64/libmongocxx-static.a.\\n"
	else
		printf "\\tMongo C++ driver found at /usr/local/lib64/libmongocxx-static.a.\\n"
	fi

	printf "\\n\\tChecking LLVM with WASM support.\\n"
	if [ ! -d "${HOME}/opt/wasm/bin" ]; then
		printf "\\tInstalling LLVM & WASM.\\n"
		if ! cd "${TEMP_DIR}"
		then
			printf "\\n\\tUnable to cd into directory %s.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir "${TEMP_DIR}/llvm-compiler"  2>/dev/null
		then
			printf "\\n\\tUnable to make directory %s/llvm-compiler.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/llvm-compiler"
		then
			printf "\\n\\tUnable to change directory into %s/llvm-compiler.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
		then
			printf "\\tUnable to clone llvm repo @ https://github.com/llvm-mirror/llvm.git.\\n"
			printf "\\tExiting now.\\n\\n"
			exit;
		fi
		if ! cd "${TEMP_DIR}/llvm-compiler/llvm/tools"
		then
			printf "\\n\\tUnable to change directory into %s/llvm-compiler/llvm/tools.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
		then
			printf "\\tUnable to clone clang repo @ https://github.com/llvm-mirror/clang.git.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/llvm-compiler/llvm"
		then
			printf "\\n\\tUnable to change directory into %s/llvm-compiler/llvm.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir "${TEMP_DIR}/llvm-compiler/llvm/build" 2>/dev/null
		then
			printf "\\n\\tUnable to create directory %s/llvm-compiler/llvm/build.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/llvm-compiler/llvm/build"
		then
			printf "\\n\\tUnable to change directory into %s/llvm-compiler/llvm/build.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! "$CMAKE" -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="${HOME}/opt/wasm" \
		-DLLVM_ENABLE_RTTI=1 -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="WebAssembly" \
		-DCMAKE_BUILD_TYPE="Release" ..
		then
			printf "\\tError compiling LLVM and clang with EXPERIMENTAL WASM support.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! make -j"${JOBS}"
		then
			printf "\\tError compiling LLVM and clang with EXPERIMENTAL WASM support.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! make install
		then
			printf "\\tError installing LLVM and clang with EXPERIMENTAL WASM support.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -rf "${TEMP_DIR}/llvm-compiler" 2>/dev/null
		then
			printf "\\tError removing directory %s/llvm-compiler.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		printf "\\tWASM successfully installed at %s/opt/wasm.\\n" "${HOME}"
	else
		printf "\\tWASM found at %s/opt/wasm.\\n" "${HOME}"
	fi

	function print_instructions()
	{
		printf "\\n\\t%s -f %s &\\n" "$( command -v mongod )" "${MONGOD_CONF}"
		printf '\texport PATH=${HOME}/opt/mongodb/bin:$PATH \n'
		printf "\\tcd %s; make test\\n\\n" "${BUILD_DIR}"
	return 0
	}
