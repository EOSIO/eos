	OS_VER=`sw_vers -productVersion`
	OS_MAJ=`echo "${OS_VER}" | cut -d'.' -f1`
	OS_MIN=`echo "${OS_VER}" | cut -d'.' -f2`
	OS_PATCH=`echo "${OS_VER}" | cut -d'.' -f3`

	MEM_GIG=`bc <<< "($(sysctl -in hw.memsize) / 1024000000)"`

	CPU_SPEED=`bc <<< "scale=2; ($(sysctl -in hw.cpufrequency) / 100000000) / 10"`
	CPU_CORE=$( sysctl -in machdep.cpu.core_count )

	DISK_TOTAL=`df -H $PWD | grep /dev | tr -s ' ' | cut -d\  -f2 | sed 's/[^0-9]//'`
	DISK_AVAIL=`df -H $PWD | grep /dev | tr -s ' ' | cut -d\  -f4 | sed 's/[^0-9]//'`

	printf "\n\tOS name: $ARCH\n"
	printf "\tOS Version: ${OS_VER}\n"
	printf "\tCPU speed: ${CPU_SPEED}Ghz\n"
	printf "\tCPU cores: $CPU_CORE\n"
	printf "\tPhysical Memory: $MEM_GIG Gbytes\n"
	printf "\tDisk space total: ${DISK_TOTAL}G\n"
	printf "\tDisk space available: ${DISK_AVAIL}G\n\n"
	
	if [ $MEM_GIG -lt 8 ]; then
		printf "\tYour system must have 8 or more Gigabytes of physical memory installed.\n"
		printf "\tExiting now.\n"
		exit 1
	fi

	if [ $OS_MIN -lt 12 ]; then
		printf "\tYou must be running Mac OS 10.12.x or higher to install EOSIO.\n"
		printf "\tExiting now.\n"
		exit 1
	fi

	if [ $DISK_AVAIL -lt 100 ]; then
		printf "\tYou must have at least 100GB of available storage to install EOSIO.\n"
		printf "\tExiting now.\n"
		exit 1
	fi

	process_dep()
	{
		printf "\tChecking XCode installation\n"
		XCODESELECT=$(which xcode-select)
		if [ $? -ne 0 ]; then
			printf "\n\tXCode must be installed in order to proceed.\n\n"
			printf "\texiting now.\n"
			exit 1
		fi

		printf "\tXCode installation found.\n\n"
	
		printf "\tChecking Ruby installation\n"
		RUBY=$(which ruby)
		if [ $? -ne 0 ]; then
			printf "\nRuby must be installed in order to proceed.\n\n"
			printf "\texiting now.\n"
			exit 1
		fi

		printf "\tRuby installation found.\n\n"
		
		printf "\tChecking Home Brew installation\n"
		BREW=$(which brew)
		if [ $? -ne 0 ]; then
			printf "\tHomebrew must be installed to compile EOS.IO\n\n"
			printf "\tDo you wish to install Home Brew?\n"
			select yn in "Yes" "No"; do
				case $yn in
					[Yy]* ) 
					$XCODESELECT --install 2>/dev/null;
					$RUBY -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
					if [ $? -ne 0 ]; then
						echo "User aborted homebrew installation. Exiting now."
						exit 0;
					fi
					break;;
					[Nn]* ) echo "User aborted homebrew installation. Exiting now.";
							exit;;
					* ) echo "Please enter 1 for yes or 2 for no.";;
				esac
			done
		fi

		printf "\tHome Brew installation found.\n\n"
	# 	DEPS="git automake libtool openssl cmake wget boost llvm@4 gmp gettext"
		DCOUNT=0
		COUNT=1
		PERMISSION_GETTEXT=0
		DISPLAY=""
		DEP=""
	
		printf "\tChecking dependencies.\n"
		for line in `cat ${WORK_DIR}/scripts/eosio_build_dep`; do
			pkg=$( echo "${line}" | cut -d',' -f1 )
			printf "\tChecking $pkg ... "
			BIN=$(which $pkg)
			if [ $? -eq 0 ]; then
				 printf "\t$pkg found\n"
				continue
			fi
		
			LIB=$( ls -l /usr/local/lib/lib${pkg}* 2>/dev/null | wc -l)
			if [ ${LIB} -ne 0 ]; then
				 printf "\t$pkg found\n"
				continue
			else 
				let DCOUNT++
				
				if [ $pkg = "LLVM" ]; then
					pkg="llvm@4"
				fi

				if [ $pkg = "gettext" ]; then
					PERMISSION_GETTEXT=1
				fi
				
				DEP=$DEP" ${pkg} "
				DISPLAY="${DISPLAY}${COUNT}. ${pkg}\n\t"
				printf "\tPackage ${pkg} ${bldred}NOT${txtrst} found.\n"
				let COUNT++
			fi
		done

		if [ $DCOUNT -ne 0 ]; then
			printf "\n\tThe following dependencies are required to install EOSIO.\n"
			printf "\n\t$DISPLAY\n\n"
			echo "Do you wish to install these packages?"
			select yn in "Yes" "No"; do
				case $yn in
					[Yy]* ) 
					if [ $PERMISSION_GETTEXT -eq 1 ]; then
						sudo chown -R $(whoami) /usr/local/share
					fi

					$XCODESELECT --install 2>/dev/null;
					printf "\tUpdating Home Brew.\n"
					brew update
					printf "\tInstalling Dependencies.\n"
					brew install --force $DEP
					brew unlink $DEP && brew link --force $DEP
					break;;
					[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
					* ) echo "Please type 1 for yes or 2 for no.";;
				esac
			done
		else 
			printf "\n\tNo required Home Brew dependencies to install.\n"
		fi
		
	return 0
	}

	process_dep

	printf "\n\tChecking for secp256k1-zkp\n"
    # install secp256k1-zkp (Cryptonomex branch)
    if [ ! -e /usr/local/lib/libsecp256k1.a ]; then
		cd ${TEMP_DIR}
		git clone https://github.com/cryptonomex/secp256k1-zkp.git
		cd secp256k1-zkp
		./autogen.sh
		if [ $? -ne 0 ]; then
			printf "\tError running autogen\n"
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
		sudo rm -rf ${TEMP_DIR}/secp256k1-zkp
	else
		printf "\tsecp256k1 found at /usr/local/lib/\n"
	fi

	printf "\n\tChecking for binaryen\n"
	if [ ! -e /usr/local/binaryen/bin/binaryen.js ]; then
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
		sudo mkdir /usr/local/binaryen
		sudo mv ${TEMP_DIR}/binaryen/bin /usr/local/binaryen
		sudo ln -s /usr/local/binaryen/bin/* /usr/local
		sudo rm -rf ${TEMP_DIR}/binaryen
	else
		printf "\tBinaryen found at /usr/local/binaryen/bin/\n"
	fi

	printf "\n\tChecking for WASM\n"
	if [ ! -d /usr/local/wasm/bin ]; then
		# Build LLVM and clang for WASM:
		cd ${TEMP_DIR}
		mkdir wasm-compiler
		cd wasm-compiler
		git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
		cd llvm/tools
		git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
		cd ..
		mkdir build
		cd build
		sudo cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/usr/local/wasm \
		-DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly \
		-DCMAKE_BUILD_TYPE=Release ../
		if [ $? -ne 0 ]; then
			printf "\tError compiling WASM.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make -j4 install
		sudo rm -rf ${TEMP_DIR}/wasm-compiler
	else
		printf "\tWASM found at /usr/local/wasm/bin/\n"
	fi