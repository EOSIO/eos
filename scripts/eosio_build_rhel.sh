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
		echo "Your system must have 8 or more Gigabytes of physical memory installed."
		echo "exiting now."
		exit 1
	fi

	if [ $OS_MIN -lt 4 ]; then
		echo "You must be running Ubuntu 16.04.x or higher to install EOSIO."
		echo "exiting now"
		exit 1
	fi

	if [ $DISK_AVAIL -lt 100 ]; then
		echo "You must have at least 100GB of available storage to install EOSIO."
		echo "exiting now"
		exit 1
	fi

	# EPEL Versions
	# clang 3.4.2
	
	printf "\n\tChecking Yum installation\n"
	
	YUM=$( which yum 2>/dev/null )
	if [ $? -ne 0 ]; then
		printf "\n\tYum must be installed to compile EOS.IO.\n"
		printf "\n\tExiting now.\n"
		exit 0
	fi
	
	printf "\tYum installation found at ${YUM}.\n"
	printf "\n\tChecking EPEL repository installation.\n"

	epel=$( sudo $YUM repoinfo epel | grep Repo-status | tr -s ' ' | cut -d: -f2 | sed 's/ //g' )

	if [ -z "$epel" ]; then
		printf "\n\tInstallation of EOS.IO requires the EPEL yum repository be installed.\n"
		printf "\tWould you like to install the EPEL yum repository?\n"
		
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* ) 
					printf "\n\n\tInstalling EPEL Repository.\n\n"
						sudo yum install https://dl.fedoraproject.org/pub/epel/epel-release-latest-$(rpm -E '%{rhel}').noarch.rpm					
						if [ $? -ne 0 ]; then
							printf "\n\tInstallation of EPEL repository was unsuccessful.\n"
							printf "\n\tExiting now.\n"
						exit 1;
					else
						printf "\n\tEPEL repository successfully installed.\n"
					fi
				break;;
				[Nn]* ) printf "\n\tUser aborting installation of required EPEL repository,\n\tExiting now.\n"; exit;;
				* ) printf "\n\tPlease type 1 for yes or 2 for no.\n";;
			esac
		done
	fi
	
	printf "\tEPEL repository found.\n"
	
# 	DEP_ARRAY=(clang-4.0 lldb-4.0 libclang-4.0-dev cmake make libbz2-dev libssl-dev libgmp3-dev autotools-dev build-essential libicu-dev python-dev autoconf libtool)
	DEP_ARRAY=(git autoconf automake libtool make gcc gcc-c++ bzip2 bzip2-devel.x86_64 openssl-devel.x86_64 gmp.x86_64 gmp-devel.x86_64 libstdc++-devel.x86_64 python python-devel libedit.x86_64 libxml2-devel.x86_64 ncurses-devel.x86_64 swig.x86_64 )
	DCOUNT=0
	COUNT=1
	DISPLAY=""
	DEP=""

	printf "\n\tChecking YUM for installed dependencies.\n\n"

	for (( i=0; i<${#DEP_ARRAY[@]}; i++ ));
	do
		pkg=$( sudo yum info ${DEP_ARRAY[$i]} 2>/dev/null | grep Repo | tr -s ' ' | cut -d: -f2 | sed 's/ //g' )

		if [ "$pkg" != "installed" ]; then
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
					sudo yum -y install ${DEP}
					if [ $? -ne 0 ]; then
						printf "\n\tYUM dependency installation failed.\n"
						printf "\n\tExiting now.\n"
						exit 1
					else
						printf "\n\tYUM dependencies installed successfully.\n"
					fi
				break;;
				[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else 
		printf "\n\tNo required YUM dependencies to install.\n"
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
		./b2 install
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
		make
		if [ $? -ne 0 ]; then
			printf "\tError compiling secp256k1-zkp.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make install
		rm -rf cd ${TEMP_DIR}/secp256k1-zkp
	else
		printf "\tsecp256k1 found\n"
	fi

	printf "\n\tChecking for CMAKE.\n"
	CMAKE=$( which cmake 2>/dev/null )
    # install CMAKE 3.10.2
    if [ -z $CMAKE ]; then
		printf "\tInstalling CMAKE\n"
		cd ${TEMP_DIR}
		curl -L -O https://cmake.org/files/v3.10/cmake-3.10.2.tar.gz
		tar xf cmake-3.10.2.tar.gz
		rm -f cmake-3.10.2.tar.gz
		cd cmake-3.10.2
		./bootstrap
		if [ $? -ne 0 ]; then
			printf "\tError running bootstrap for CMAKE.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		make
		if [ $? -ne 0 ]; then
			printf "\tError compiling CMAKE.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make install
		rm -rf cd ${TEMP_DIR}/cmake-3.10.2
	else
		printf "\tCMAKE found\n"
	fi
	
	printf "\n\tChecking for binaryen\n"
	if [ ! -d ${HOME}/opt/binaryen ]; then
		# Install binaryen v1.37.14:
		printf "\tInstalling binaryen v1.37.14:\n"
		cd ${TEMP_DIR}
		git clone https://github.com/WebAssembly/binaryen
		cd binaryen
		git checkout tags/1.37.14
		cmake . && make
		if [ $? -ne 0 ]; then
			printf "\tError compiling binaryen.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		mkdir -p ${HOME}/opt/binaryen/ 2>/dev/null
		mv ${TEMP_DIR}/binaryen/bin ${HOME}/opt/binaryen/
		rm -rf ${TEMP_DIR}/binaryen
	else
		printf "\tBinaryen found at ${HOME}/opt/binaryen\n"
	fi
	
	printf "\n\tChecking for WASM\n"
	if [ ! -d ${HOME}/opt/wasm ]; then
		# Build LLVM and clang for WASM:
		printf "\tInstalling LLVM & WASM\n"
		cd ${TEMP_DIR}
		mkdir wasm-compiler  2>/dev/null
		cd wasm-compiler
		git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
		cd llvm/tools
		git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
		cd ..
		mkdir build
		cd build
		cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=${HOME}/opt/wasm -DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly -DCMAKE_BUILD_TYPE=Release ../
		make -j $(nproc) install
		rm -rf ${TEMP_DIR}/wasm-compiler
	else
		printf "\tWASM found at ${HOME}/opt/wasm\n"
	fi