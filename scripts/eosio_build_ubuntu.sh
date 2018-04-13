	OS_VER=$( cat /etc/os-release | grep VERSION_ID | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' )
	OS_MAJ=`echo "${OS_VER}" | cut -d'.' -f1`
	OS_MIN=`echo "${OS_VER}" | cut -d'.' -f2`

	MEM_MEG=$( free -m | grep Mem | tr -s ' ' | cut -d\  -f2 )

	CPU_SPEED=$( lscpu | grep "MHz" | tr -s ' ' | cut -d\  -f3 | cut -d'.' -f1 )
	CPU_CORE=$( lscpu | grep "^CPU(s)" | tr -s ' ' | cut -d\  -f2 )

	DISK_INSTALL=`df -h . | tail -1 | tr -s ' ' | cut -d\  -f1`
	DISK_TOTAL_KB=`df . | tail -1 | awk '{print $2}'`
	DISK_AVAIL_KB=`df . | tail -1 | awk '{print $4}'`
	DISK_TOTAL=$(( $DISK_TOTAL_KB / 1048576 ))
	DISK_AVAIL=$(( $DISK_AVAIL_KB / 1048576 ))

	printf "\n\tOS name: $OS_NAME\n"
	printf "\tOS Version: ${OS_VER}\n"
	printf "\tCPU speed: ${CPU_SPEED}Mhz\n"
	printf "\tCPU cores: $CPU_CORE\n"
	printf "\tPhysical Memory: $MEM_MEG Mgb\n"
	printf "\tDisk install: ${DISK_INSTALL}\n"
	printf "\tDisk space total: ${DISK_TOTAL%.*}G\n"
	printf "\tDisk space available: ${DISK_AVAIL%.*}G\n"

	if [ $MEM_MEG -lt 7000 ]; then
		printf "\tYour system must have 7 or more Gigabytes of physical memory installed.\n"
		printf "\tExiting now.\n"
		exit 1
	fi

        case $OS_NAME in
			"Linux Mint")
			   if [ $OS_MAJ -lt 18 ]; then
				   printf "\tYou must be running Linux Mint 18.x or higher to install EOSIO.\n"
				   printf "\tExiting now.\n"
				   exit 1
			   fi
			;;
			"Ubuntu")
				if [ $OS_MIN -lt 4 ]; then
					printf "\tYou must be running Ubuntu 16.04.x or higher to install EOSIO.\n"
					printf "\tExiting now.\n"
					exit 1
				fi
			;;
        esac

	if [ ${DISK_AVAIL%.*} -lt $DISK_MIN ]; then
		printf "\tYou must have at least ${DISK_MIN}GB of available storage to install EOSIO.\n"
		printf "\tExiting now.\n"
		exit 1
	fi

	DEP_ARRAY=(clang-4.0 lldb-4.0 libclang-4.0-dev cmake make libbz2-dev libssl-dev \
	libgmp3-dev autotools-dev build-essential libbz2-dev libicu-dev python3-dev autoconf libtool curl)
	COUNT=1
	DISPLAY=""
	DEP=""

	printf "\n\tChecking for installed dependencies.\n\n"

	for (( i=0; i<${#DEP_ARRAY[@]}; i++ ));
	do
		pkg=$( dpkg -s ${DEP_ARRAY[$i]} 2>/dev/null | grep Status | tr -s ' ' | cut -d\  -f4 )
		if [ -z "$pkg" ]; then
			DEP=$DEP" ${DEP_ARRAY[$i]} "
			DISPLAY="${DISPLAY}${COUNT}. ${DEP_ARRAY[$i]}\n\t"
			printf "\tPackage ${DEP_ARRAY[$i]} ${bldred} NOT ${txtrst} found.\n"
			let COUNT++
		else
			printf "\tPackage ${DEP_ARRAY[$i]} found.\n"
			continue
		fi
	done		
	
	if [ ${COUNT} -gt 1 ]; then
		printf "\n\tThe following dependencies are required to install EOSIO.\n"
		printf "\n\t$DISPLAY\n\n"
		printf "\tDo you wish to install these packages?\n"
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* ) 
					printf "\n\n\tInstalling dependencies\n\n"
					sudo apt-get update
					sudo apt-get -y install ${DEP}
					if [ $? -ne 0 ]; then
						printf "\n\tDPKG dependency failed.\n"
						printf "\n\tExiting now.\n"
						exit 1
					else
						printf "\n\tDPKG dependencies installed successfully.\n"
					fi
				break;;
				[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else 
		printf "\n\tNo required dpkg dependencies to install.\n"
	fi

	printf "\n\tChecking boost library installation.\n"
	BVERSION=`cat "${BOOST_ROOT}/include/boost/version.hpp" 2>/dev/null | grep BOOST_LIB_VERSION \
	| tail -1 | tr -s ' ' | cut -d\  -f3 | sed 's/[^0-9\._]//gI'`
	if [ "${BVERSION}" != "1_66" ]; then
		printf "\tRemoving existing boost libraries in ${HOME}/opt/boost* .\n"
		rm -rf ${HOME}/opt/boost*
		if [ $? -ne 0 ]; then
			printf "\n\tUnable to remove deprecated boost libraries at this time.\n"
			printf "\n\tExiting now.\n"
			exit 1
		fi
		printf "\tInstalling boost libraries.\n"
		cd ${TEMP_DIR}
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2)
		if [ "${STATUS}" -ne 200 ]; then
			printf "\tUnable to download Boost libraries at this time.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		tar xf ${TEMP_DIR}/boost_1_66_0.tar.bz2
		rm -f  ${TEMP_DIR}/boost_1_66_0.tar.bz2
		cd ${TEMP_DIR}/boost_1_66_0/
		./bootstrap.sh "--prefix=$BOOST_ROOT"
		if [ $? -ne 0 ]; then
			printf "\n\tInstallation of boost libraries failed. 0\n"
			printf "\n\tExiting now.\n"
			exit 1
		fi
		./b2 install
		if [ $? -ne 0 ]; then
			printf "\n\tInstallation of boost libraries failed. 1\n"
			printf "\n\tExiting now.\n"
			exit 1
		fi
		rm -rf ${TEMP_DIR}/boost_1_66_0/
	else
		printf "\tBoost 1.66.0 found at ${HOME}/opt/boost_1_66_0.\n"
	fi

	printf "\n\tChecking MongoDB installation.\n"
    if [ ! -e ${MONGOD_CONF} ]; then
		printf "\n\tInstalling MongoDB 3.6.3.\n"
		cd ${HOME}/opt
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 https://fastdl.mongodb.org/linux/mongodb-linux-x86_64-3.6.3.tgz)
		if [ "${STATUS}" -ne 200 ]; then
			printf "\tUnable to download MongoDB at this time.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		tar xf mongodb-linux-x86_64-3.6.3.tgz
		rm -f mongodb-linux-x86_64-3.6.3.tgz
		ln -s ${HOME}/opt/mongodb-linux-x86_64-3.6.3/ mongodb
		mkdir ${HOME}/opt/mongodb/data
		mkdir ${HOME}/opt/mongodb/log
		touch ${HOME}/opt/mongodb/log/mongodb.log
		
tee > /dev/null ${MONGOD_CONF} <<mongodconf
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

	else
		printf "\tMongoDB configuration found at ${MONGOD_CONF}.\n"
	fi

	printf "\n\tChecking MongoDB C++ driver installation.\n"
    if [ ! -e /usr/local/lib/libmongocxx-static.a ]; then
		printf "\n\tInstalling MongoDB C & C++ drivers.\n"
		cd ${TEMP_DIR}
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 https://github.com/mongodb/mongo-c-driver/releases/download/1.9.3/mongo-c-driver-1.9.3.tar.gz)
		if [ "${STATUS}" -ne 200 ]; then
			rm -f ${TEMP_DIR}/mongo-c-driver-1.9.3.tar.gz
			printf "\tUnable to download MongoDB C driver at this time.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		tar xf mongo-c-driver-1.9.3.tar.gz
		rm -f ${TEMP_DIR}/mongo-c-driver-1.9.3.tar.gz
		cd mongo-c-driver-1.9.3
		./configure --enable-static --with-libbson=bundled --enable-ssl=openssl --disable-automatic-init-and-cleanup --prefix=/usr/local
		if [ $? -ne 0 ]; then
			printf "\tConfiguring MongoDB C driver has encountered the errors above.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		make -j${CPU_CORE}
		if [ $? -ne 0 ]; then
			printf "\tError compiling MongoDB C driver.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make install
		if [ $? -ne 0 ]; then
			printf "\tError installing MongoDB C driver.\nMake sure you have sudo privileges.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd ..
		rm -rf ${TEMP_DIR}/mongo-c-driver-1.9.3
		cd ${TEMP_DIR}
		sudo rm -rf ${TEMP_DIR}/mongo-cxx-driver
		git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
		if [ $? -ne 0 ]; then
			printf "\tUnable to clone MongoDB C++ driver at this time.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd mongo-cxx-driver/build
		cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
		if [ $? -ne 0 ]; then
			printf "\tCmake has encountered the above errors building the MongoDB C++ driver.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make -j${CPU_CORE}
		if [ $? -ne 0 ]; then
			printf "\tError compiling MongoDB C++ driver.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make install
		if [ $? -ne 0 ]; then
			printf "\tError installing MongoDB C++ driver.\nMake sure you have sudo privileges.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd
		sudo rm -rf ${TEMP_DIR}/mongo-cxx-driver
	else
		printf "\tMongo C++ driver found at /usr/local/lib/libmongocxx-static.a.\n"
	fi

	printf "\n\tChecking for secp256k1-zkp\n"
    # install secp256k1-zkp (Cryptonomex branch)
    if [ ! -e /usr/local/lib/libsecp256k1.a ]; then
		printf "\tInstalling secp256k1-zkp (Cryptonomex branch)\n"
		cd ${TEMP_DIR}
		git clone https://github.com/cryptonomex/secp256k1-zkp.git
		if [ $? -ne 0 ]; then
			printf "\tUnable to clone repo secp256k1-zkp @ https://github.com/cryptonomex/secp256k1-zkp.git.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd secp256k1-zkp
		./autogen.sh
		if [ $? -ne 0 ]; then
			printf "\tError running autogen for secp256k1-zkp.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		./configure
		make -j${CPU_CORE}
		if [ $? -ne 0 ]; then
			printf "\tError compiling secp256k1-zkp.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make -j${CPU_CORE} install
		rm -rf cd ${TEMP_DIR}/secp256k1-zkp
	else
		printf "\tsecp256k1 found\n"
	fi

	printf "\n\tChecking for LLVM with WASM support.\n"
	if [ ! -d ${HOME}/opt/wasm/bin ]; then
		# Build LLVM and clang for WASM:
		printf "\tInstalling LLVM & WASM\n"
		cd ${TEMP_DIR}
		mkdir llvm-compiler  2>/dev/null
		cd llvm-compiler
		git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
		if [ $? -ne 0 ]; then
			printf "\tUnable to clone llvm repo @ https://github.com/llvm-mirror/llvm.git.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd llvm/tools
		git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
		if [ $? -ne 0 ]; then
			printf "\tUnable to clone clang repo @ https://github.com/llvm-mirror/clang.git.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd ..
		mkdir build
		cd build
		cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=${HOME}/opt/wasm -DLLVM_TARGETS_TO_BUILD= \
		-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release ../
		if [ $? -ne 0 ]; then
			printf "\tError compiling LLVM and clang with EXPERIMENTAL WASM support.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		make -j${CPU_CORE} install
		if [ $? -ne 0 ]; then
			printf "\tError compiling LLVM and clang with EXPERIMENTAL WASM support.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		rm -rf ${TEMP_DIR}/llvm-compiler
	else
		printf "\tWASM found at ${HOME}/opt/wasm/bin\n"
	fi