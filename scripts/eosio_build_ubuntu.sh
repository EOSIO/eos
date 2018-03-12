	OS_VER=$( cat /etc/os-release | grep VERSION_ID | cut -d'=' -f2 | sed 's/[^0-9\.]//gI' )
	OS_MAJ=`echo "${OS_VER}" | cut -d'.' -f1`
	OS_MIN=`echo "${OS_VER}" | cut -d'.' -f2`

	MEM_MEG=$( free -m | grep Mem | tr -s ' ' | cut -d\  -f2 )

	CPU_SPEED=$( lscpu | grep "MHz" | tr -s ' ' | cut -d\  -f3 | cut -d'.' -f1 )
	CPU_CORE=$( lscpu | grep "^CPU(s)" | tr -s ' ' | cut -d\  -f2 )

	DISK_TOTAL=`df -h / | grep /dev | tr -s ' ' | cut -d\  -f2 | sed 's/[^0-9]//'`
	DISK_AVAIL=`df -h / | grep /dev | tr -s ' ' | cut -d\  -f4 | sed 's/[^0-9]//'`

	printf "\n\tOS name: $OS_NAME\n"
	printf "\tOS Version: ${OS_VER}\n"
	printf "\tCPU speed: ${CPU_SPEED}Mhz\n"
	printf "\tCPU cores: $CPU_CORE\n"
	printf "\tPhysical Memory: $MEM_MEG Mgb\n"
	printf "\tDisk space total: ${DISK_TOTAL}G\n"
	printf "\tDisk space available: ${DISK_AVAIL}G\n"

	if [ $MEM_MEG -lt 4000 ]; then
		printf "\tYour system must have 8 or more Gigabytes of physical memory installed.\n"
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

	if [ $DISK_AVAIL -lt $DISK_MIN ]; then
		printf "\tYou must have at least ${DISK_MIN}GB of available storage to install EOSIO.\n"
		printf "\tExiting now.\n"
		exit 1
	fi

	DEP_ARRAY=(clang-4.0 lldb-4.0 libclang-4.0-dev cmake make libbz2-dev libssl-dev libgmp3-dev autotools-dev build-essential libbz2-dev libicu-dev python-dev autoconf libtool curl)
	DCOUNT=0
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
			let DCOUNT++
		else
			printf "\tPackage ${DEP_ARRAY[$i]} found.\n"
			continue
		fi
	done		
	
	if [ ${DCOUNT} -ne 0 ]; then
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

	printf "\n\tChecking for boost libraries\n"
	if [ ! -d ${HOME}/opt/boost_1_66_0 ]; then
		# install boost
		printf "\tInstalling boost libraries\n"
		cd ${TEMP_DIR}
		curl -L https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.bz2 > boost_1.66.0.tar.bz2
		tar xf boost_1.66.0.tar.bz2
		cd boost_1_66_0/
		./bootstrap.sh "--prefix=$BOOST_ROOT"
		./b2 -j${CPU_CORE} install
		rm -rf ${TEMP_DIR}/boost_1_66_0/
		rm -f  ${TEMP_DIR}/boost_1.66.0.tar.bz2
	else
		printf "\tBoost 1.66 found at ${HOME}/opt/boost_1_66_0\n"
	fi

	printf "\n\tChecking for secp256k1-zkp\n"
    # install secp256k1-zkp (Cryptonomex branch)
    if [ ! -e /usr/local/lib/libsecp256k1.a ]; then
		printf "\tInstalling secp256k1-zkp (Cryptonomex branch)\n"
		cd ${TEMP_DIR}
		git clone https://github.com/cryptonomex/secp256k1-zkp.git
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

   printf "\n\tChecking for SoftFloat\n"
   if [ ! -d ${HOME}/opt/berkeley-softfloat-3 ]; then
      # clone the library
		cd ${TEMP_DIR}
      mkdir softfloat
      cd softfloat
      git clone --depth 1 --single-branch --branch master https://github.com/ucb-bar/berkeley-softfloat-3.git
      cd berkeley-softfloat-3/build/Linux-x86_64-GCC
      make -j${CPU_CORE} SPECIALIZE_TYPE="8086-SSE" SOFTFLOAT_OPS="-DSOFTFLOAT_ROUND_EVEN -DINLINE_LEVEL=5 -DSOFTFLOAT_FAST_DIV32TO16 -DSOFTFLOAT_FAST_DIV64TO32"
      if [ $? -ne 0 ]; then
         printf "\tError compiling softfloat.\n"
         printf "\tExiting now.\n\n"
         exit;
      fi
      # no install target defined for this library
      mkdir -p ${HOME}/opt/berkeley-softfloat-3
      cp softfloat.a ${HOME}/opt/berkeley-softfloat-3/libsoftfloat.a
      mv ${TEMP_DIR}/softfloat/berkeley-softfloat-3/source/include ${HOME}/opt/berkeley-softfloat-3/include
		rm -rf ${TEMP_DIR}/softfloat
	else
		printf "\tsoftfloat found at /usr/local/berkeley-softfloat-3/\n"
   fi

	printf "\n\tChecking for LLVM with WASM support.\n"
	if [ ! -d ${HOME}/opt/wasm/bin ]; then
		# Build LLVM and clang for WASM:
		printf "\tInstalling LLVM & WASM\n"
		cd ${TEMP_DIR}
		mkdir llvm-compiler  2>/dev/null
		cd llvm-compiler
		git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
		cd llvm/tools
		git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
		cd ..
		mkdir build
		cd build
		cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=${HOME}/opt/wasm -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release ../
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
