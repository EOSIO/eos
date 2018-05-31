	OS_VER=$( grep VERSION_ID /etc/os-release | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' )
	OS_MAJ=$(echo "${OS_VER}" | cut -d'.' -f1)
	OS_MIN=$(echo "${OS_VER}" | cut -d'.' -f2)

	MEM_MEG=$( free -m | sed -n 2p | tr -s ' ' | cut -d\  -f2 || cut -d' ' -f2 )
	CPU_SPEED=$( lscpu | grep -m1 "MHz" | tr -s ' ' | cut -d\  -f3 || cut -d' ' -f3 | cut -d'.' -f1 )
	CPU_CORE=$( lscpu | grep "^CPU(s)" | tr -s ' ' | cut -d\  -f2 || cut -d' ' -f2 )

	MEM_GIG=$(( ((MEM_MEG / 1000) / 2) ))
	JOBS=$(( MEM_GIG > CPU_CORE ? CPU_CORE : MEM_GIG ))

	DISK_INSTALL=$(df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 || cut -d' ' -f1)
	DISK_TOTAL_KB=$(df . | tail -1 | awk '{print $2}')
	DISK_AVAIL_KB=$(df . | tail -1 | awk '{print $4}')
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

	if [ "${MEM_MEG}" -lt 7000 ]; then
		printf "\\tYour system must have 7 or more Gigabytes of physical memory installed.\\n"
		printf "\\tExiting now.\\n"
		exit 1
	fi

	case "${OS_NAME}" in
		"Linux Mint")
		   if [ "${OS_MAJ}" -lt 18 ]; then
			   printf "\\tYou must be running Linux Mint 18.x or higher to install EOSIO.\\n"
			   printf "\\tExiting now.\\n"
			   exit 1
		   fi
		;;
		"Ubuntu")
			if [ "${OS_MAJ}" -lt 16 ]; then
				printf "\\tYou must be running Ubuntu 16.04.x or higher to install EOSIO.\\n"
				printf "\\tExiting now.\\n"
				exit 1
			fi
		;;
	esac

	if [ "${DISK_AVAIL%.*}" -lt "${DISK_MIN}" ]; then
		printf "\\tYou must have at least %sGB of available storage to install EOSIO.\\n" "${DISK_MIN}"
		printf "\\tExiting now.\\n"
		exit 1
	fi

	DEP_ARRAY=(clang-4.0 lldb-4.0 libclang-4.0-dev cmake make automake libbz2-dev libssl-dev \
	libgmp3-dev autotools-dev build-essential libicu-dev python2.7-dev python3-dev \
    autoconf libtool curl zlib1g-dev doxygen graphviz)
	COUNT=1
	DISPLAY=""
	DEP=""

	if [[ "${ENABLE_CODE_COVERAGE}" == true ]]; then
		DEP_ARRAY+=(lcov)
	fi

	printf "\\n\\tChecking for installed dependencies.\\n\\n"

	for (( i=0; i<${#DEP_ARRAY[@]}; i++ ));
	do
		pkg=$( dpkg -s "${DEP_ARRAY[$i]}" 2>/dev/null | grep Status | tr -s ' ' | cut -d\  -f4 )
		if [ -z "$pkg" ]; then
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
		printf "\\tDo you wish to install these packages?\\n"
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* ) 
					printf "\\n\\n\\tInstalling dependencies\\n\\n"
					sudo apt-get update
					if ! sudo apt-get -y install ${DEP}
					then
						printf "\\n\\tDPKG dependency failed.\\n"
						printf "\\n\\tExiting now.\\n"
						exit 1
					else
						printf "\\n\\tDPKG dependencies installed successfully.\\n"
					fi
				break;;
				[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else 
		printf "\\n\\tNo required dpkg dependencies to install.\\n"
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
	BVERSION=$( grep BOOST_LIB_VERSION "${BOOST_ROOT}/include/boost/version.hpp" 2>/dev/null \
	| tail -1 | tr -s ' ' | cut -d\  -f3 | sed 's/[^0-9\._]//gI')
	if [ "${BVERSION}" != "1_67" ]; then
		printf "\\tRemoving existing boost libraries in %s/opt/boost* .\\n" "${HOME}"
		if ! rm -rf "${HOME}"/opt/boost*
		then
			printf "\\n\\tUnable to remove deprecated boost libraries at this time.\\n"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		printf "\\tInstalling boost libraries.\\n"
		if ! cd "${TEMP_DIR}"
		then
			printf "\\n\\tUnable to enter directory %s.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.bz2)
		if [ "${STATUS}" -ne 200 ]; then
			printf "\\tUnable to download Boost libraries at this time.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! tar xf "${TEMP_DIR}/boost_1_67_0.tar.bz2"
		then
			printf "\\n\\tUnable to unarchive file %s/boost_1_67_0.tar.bz2.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -f "${TEMP_DIR}/boost_1_67_0.tar.bz2"
		then
			printf "\\n\\tUnable to remove file %s/boost_1_67_0.tar.bz2.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/boost_1_67_0/"
		then
			printf "\\n\\tUnable to enter directory %s/boost_1_67_0.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ./bootstrap.sh "--prefix=$BOOST_ROOT"
		then
			printf "\\n\\tInstallation of boost libraries failed. 0\\n"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1
		fi
		if ! ./b2 install
		then
			printf "\\n\\tInstallation of boost libraries failed. 1\\n"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1
		fi
		if ! rm -rf "${TEMP_DIR}"/boost_1_67_0
		then
			printf "\\n\\tUnable to remove %s/boost_1_67_0.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
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
		printf "\\n\\tInstalling MongoDB 3.6.3.\\n"
		if ! cd "${HOME}/opt"
		then
			printf "\\n\\tUnable to enter directory %s/opt.\\n" "${HOME}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-3.6.3.tgz)
		if [ "${STATUS}" -ne 200 ]; then
			printf "\\tUnable to download MongoDB at this time.\\n"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! tar xf "${HOME}/opt/mongodb-linux-x86_64-3.6.3.tgz"
		then
			printf "\\tUnable to unarchive file %s/opt/mongodb-linux-x86_64-3.6.3.tgz.\\n" "${HOME}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -f "${HOME}/opt/mongodb-linux-x86_64-3.6.3.tgz"
		then
			printf "\\tUnable to remove file %s/opt/mongodb-linux-x86_64-3.6.3.tgz.\\n" "${HOME}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ln -s "${HOME}/opt/mongodb-linux-x86_64-3.6.3/" "${HOME}/opt/mongodb"
		then
			printf "\\tUnable to symbolic link %s/opt/mongodb-linux-x86_64-3.6.3/ to %s/opt/mongodb.\\n" "${HOME}" "${HOME}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! mkdir "${HOME}/opt/mongodb/data"
		then
			printf "\\tUnable to create directory %s/opt/mongodb/data.\\n" "${HOME}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! mkdir "${HOME}/opt/mongodb/log"
		then
			printf "\\tUnable to create directory %s/opt/mongodb/log.\\n" "${HOME}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! touch "${HOME}/opt/mongodb/log/mongodb.log"
		then
			printf "\\tUnable to create file %s/opt/mongodb/log/mongodb.log.\\n" "${HOME}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
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
			printf "\\tUnable to write to file %s.\\n" "${MONGOD_CONF}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		printf "\\n\\tMongoDB successfully installed at %s/opt/mongodb.\\n" "${HOME}"
	else
		printf "\\tMongoDB configuration found at %s.\\n" "${MONGOD_CONF}"
	fi

	printf "\\n\\tChecking MongoDB C++ driver installation.\\n"
    if [ ! -e "/usr/local/lib/libmongocxx-static.a" ]; then
		printf "\\n\\tInstalling MongoDB C & C++ drivers.\\n"
		if ! cd "${TEMP_DIR}"
		then
			printf "\\n\\tUnable to enter directory %s.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 https://github.com/mongodb/mongo-c-driver/releases/download/1.9.3/mongo-c-driver-1.9.3.tar.gz)
		if [ "${STATUS}" -ne 200 ]; then
			if ! rm -f "${TEMP_DIR}/mongo-c-driver-1.9.3.tar.gz"
			then
				printf "\\n\\tUnable to remove file %s/mongo-c-driver-1.9.3.tar.gz.\\n" "${TEMP_DIR}"
			fi
			printf "\\tUnable to download MongoDB C driver at this time.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! tar xf "${TEMP_DIR}/mongo-c-driver-1.9.3.tar.gz"
		then
			printf "\\n\\tUnable to unarchive file %s/mongo-c-driver-1.9.3.tar.gz.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -f "${TEMP_DIR}/mongo-c-driver-1.9.3.tar.gz"
		then
			printf "\\n\\tUnable to remove file %s/mongo-c-driver-1.9.3.tar.gz.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/mongo-c-driver-1.9.3"
		then
			printf "\\n\\tUnable to enter directory %s/mongo-c-driver-1.9.3.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ./configure --enable-static --with-libbson=bundled --enable-ssl=openssl --disable-automatic-init-and-cleanup --prefix=/usr/local
		then
			printf "\\tConfiguring MongoDB C driver has encountered the errors above.\\n"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! make -j"${JOBS}"
		then
			printf "\\tError compiling MongoDB C driver.\\n"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo make install
		then
			printf "\\tError installing MongoDB C driver.\\nMake sure you have sudo privileges.\\n"
			printf "\\tExiting now.\\n\\n"
			exit;
		fi
		if ! cd "${TEMP_DIR}"
		then
			printf "\\n\\tUnable to enter directory %s.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -rf "${TEMP_DIR}/mongo-c-driver-1.9.3"
		then
			printf "\\n\\tUnable to remove directory %s/mongo-c-driver-1.9.3.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
		then
			printf "\\tUnable to clone MongoDB C++ driver at this time.\\n"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/mongo-cxx-driver/build"
		then
			printf "\\n\\tUnable to enter directory %s/mongo-cxx-driver/build.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
		then
			printf "\\tCmake has encountered the above errors building the MongoDB C++ driver.\\n"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo make -j"${JOBS}"
		then
			printf "\\tError compiling MongoDB C++ driver.\\n"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo make install
		then
			printf "\\tError installing MongoDB C++ driver.\\nMake sure you have sudo privileges.\\n"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${CWD}"
		then
			printf "\\n\\tUnable to enter directory %s.\\n" "${CWD}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo rm -rf "${TEMP_DIR}/mongo-cxx-driver"
		then
			printf "\\n\\tUnable to remove directory %s/mongo-cxx-driver.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n\\n"
			exit 1;
		fi
		printf "\\n\\tMongo C++ driver successfully installed @ /usr/local/lib/libmongocxx-static.a.\\n\\n"
	else
		printf "\\tMongo C++ driver found at /usr/local/lib/libmongocxx-static.a.\\n"
	fi

	printf "\\n\\tChecking secp256k1-zkp installation.\\n"
    # install secp256k1-zkp (Cryptonomex branch)
    if [ ! -e "/usr/local/lib/libsecp256k1.a" ]; then
		printf "\\tInstalling secp256k1-zkp (Cryptonomex branch).\\n"
		if ! cd "${TEMP_DIR}"
		then
			printf "\\n\\tUnable to cd into directory %s.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! git clone https://github.com/cryptonomex/secp256k1-zkp.git
		then
			printf "\\tUnable to clone repo secp256k1-zkp @ https://github.com/cryptonomex/secp256k1-zkp.git.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/secp256k1-zkp"
		then
			printf "\\n\\tUnable to cd into directory %s.\\n" "${TEMP_DIR}/secp256k1-zkp"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! ./autogen.sh
		then
			printf "\\tError running autogen for secp256k1-zkp.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ./configure
		then
			printf "\\tError running configure for secp256k1-zkp.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! make -j"${JOBS}"
		then
			printf "\\tError compiling secp256k1-zkp.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo make install
		then
			printf "\\tError installing secp256k1-zkp.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -rf "${TEMP_DIR}/secp256k1-zkp"
		then
			printf "\\tError removing directory %s.\\n" "${TEMP_DIR}/secp256k1-zkp"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		printf "\\n\\tsecp256k1 successfully installed @ /usr/local/lib.\\n\\n"
	else
		printf "\\tsecp256k1 found @ /usr/local/lib.\\n"
	fi

	printf "\\n\\tChecking for LLVM with WASM support.\\n"
	if [ ! -d "${HOME}/opt/wasm/bin" ]; then
		# Build LLVM and clang with WASM support:
		printf "\\tInstalling LLVM with WASM\\n"
		if ! cd "${TEMP_DIR}"
		then
			printf "\\n\\tUnable to cd into directory %s.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir "${TEMP_DIR}/llvm-compiler"  2>/dev/null
		then
			printf "\\n\\tUnable to create directory %s/llvm-compiler.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/llvm-compiler"
		then
			printf "\\n\\tUnable to enter directory %s/llvm-compiler.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
		then
			printf "\\tUnable to clone llvm repo @ https://github.com/llvm-mirror/llvm.git.\\n"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/llvm-compiler/llvm/tools"
		then
			printf "\\tUnable to enter directory %s/llvm-compiler/llvm/tools.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
		then
			printf "\\tUnable to clone clang repo @ https://github.com/llvm-mirror/clang.git.\\n"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/llvm-compiler/llvm"
		then
			printf "\\tUnable to enter directory %s/llvm-compiler/llvm.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! mkdir "${TEMP_DIR}/llvm-compiler/llvm/build"
		then
			printf "\\tUnable to create directory %s/llvm-compiler/llvm/build.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/llvm-compiler/llvm/build"
		then
			printf "\\tUnable to enter directory %s/llvm-compiler/llvm/build.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="${HOME}/opt/wasm" -DLLVM_TARGETS_TO_BUILD= \
		-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release ../
		then
			printf "\\tError compiling LLVM and clang with EXPERIMENTAL WASM support.0\\n"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! make -j"${JOBS}" install
		then
			printf "\\tError compiling LLVM and clang with EXPERIMENTAL WASM support.1\\n"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		if ! rm -rf "${TEMP_DIR}/llvm-compiler"
		then
			printf "\\tUnable to remove directory %s/llvm-compiler.\\n" "${TEMP_DIR}"
			printf "\\n\\tExiting now.\\n"
			exit 1;
		fi
		printf "\\n\\tWASM successffully installed @ %s/opt/wasm/bin.\\n\\n" "${HOME}"
	else
		printf "\\tWASM found at %s/opt/wasm/bin.\\n" "${HOME}"
	fi

	function print_instructions()
	{
		printf '\n\texport PATH=${HOME}/opt/mongodb/bin:$PATH\n'
		printf "\\t%s -f %s &\\n" "$( command -v mongod )" "${MONGOD_CONF}"
		printf "\\tcd %s; make test\\n\\n" "${BUILD_DIR}"
	return 0
	}
