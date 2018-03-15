	OS_VER=`sw_vers -productVersion`
	OS_MAJ=`echo "${OS_VER}" | cut -d'.' -f1`
	OS_MIN=`echo "${OS_VER}" | cut -d'.' -f2`
	OS_PATCH=`echo "${OS_VER}" | cut -d'.' -f3`

	MEM_GIG=`bc <<< "($(sysctl -in hw.memsize) / 1024000000)"`

	CPU_SPEED=`bc <<< "scale=2; ($(sysctl -in hw.cpufrequency) / 100000000) / 10"`
	CPU_CORE=$( sysctl -in machdep.cpu.core_count )

	blksize=`df . | head -1 | awk '{print $2}' | cut -d- -f1`
	gbfactor=$(( 1073741824 / $blksize ))
	total_blks=`df . | tail -1 | awk '{print $2}'`
	avail_blks=`df . | tail -1 | awk '{print $4}'`
	DISK_TOTAL=$(($total_blks / $gbfactor ))
	DISK_AVAIL=$(($avail_blks / $gbfactor ))

	printf "\n\tOS name: $ARCH\n"
	printf "\tOS Version: ${OS_VER}\n"
	printf "\tCPU speed: ${CPU_SPEED}Ghz\n"
	printf "\tCPU cores: $CPU_CORE\n"
	printf "\tPhysical Memory: $MEM_GIG Gbytes\n"
	printf "\tDisk space total: ${DISK_TOTAL}G\n"
	printf "\tDisk space available: ${DISK_AVAIL}G\n\n"
	
	if [ $MEM_GIG -lt 8 ]; then
		echo "Your system must have 8 or more Gigabytes of physical memory installed."
		echo "Exiting now."
		exit 1
	fi

	if [ $OS_MIN -lt 12 ]; then
		echo "You must be running Mac OS 10.12.x or higher to install EOSIO."
		echo "Exiting now."
		exit 1
	fi

	if [ $DISK_AVAIL -lt $DISK_MIN ]; then
		echo "You must have at least ${DISK_MIN}GB of available storage to install EOSIO."
		echo "Exiting now."
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
						echo "Unable to install homebrew at this time. Exiting now."
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
			
				if [ $pkg == "libtool" ] && [ $BIN == /usr/bin/libtool ]; then
					donothing=true
				else
					printf "\t$pkg found\n"
					continue
				fi
			fi
			
			LIB=$( ls -l /usr/local/lib/lib${pkg}* 2>/dev/null | wc -l)
			if [ ${LIB} -ne 0 ]; then
				 printf "\t$pkg found\n"
				continue
			else 
				let DCOUNT++
				
				if [ $pkg = "mongod" ]; then
					pkg="mongodb"
				fi

				if [ $pkg = "LLVM" ]; then
					pkg="llvm@4"
				fi

# 				if [ $pkg = "openssl" ]; then
# 					pkg="openssl@1.0"
# 				fi

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

	printf "\n\tChecking for MongoDB C++ driver\n"
    # install libmongocxx.dylib
    if [ ! -e /usr/local/lib/libmongocxx.dylib ]; then
		cd ${TEMP_DIR}
		brew install --force pkgconfig
		brew unlink pkgconfig && brew link --force pkgconfig
		curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/1.9.3/mongo-c-driver-1.9.3.tar.gz
		if [ $? -ne 0 ]; then
			rm -f ${TEMP_DIR}/mongo-c-driver-1.9.3.tar.gz
			printf "\tUnable to download MondgDB C driver at this time.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		tar xf mongo-c-driver-1.9.3.tar.gz
		rm -f ${TEMP_DIR}/mongo-c-driver-1.9.3.tar.gz
		cd mongo-c-driver-1.9.3
		./configure --enable-ssl=darwin --disable-automatic-init-and-cleanup --prefix=/usr/local
		if [ $? -ne 0 ]; then
			printf "\tConfiguring MondgDB C driver has encountered the errors above.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		make -j${CPU_CORE}
		if [ $? -ne 0 ]; then
			printf "\tError compiling MondgDB C driver.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make install
		if [ $? -ne 0 ]; then
			printf "\tError installing MondgDB C driver.\nMake sure you have sudo privileges.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd ..
		rm -rf ${TEMP_DIR}/mongo-c-driver-1.9.3
		cd ${TEMP_DIR}
		git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
		if [ $? -ne 0 ]; then
			printf "\tUnable to clone MondgDB C++ driver at this time.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd mongo-cxx-driver/build
		cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
		if [ $? -ne 0 ]; then
			printf "\tCmake has encountered the above errors building the MongoDB C++ driver.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		make -j${CPU_CORE}
		if [ $? -ne 0 ]; then
			printf "\tError compiling MondgDB C++ driver.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make install
		if [ $? -ne 0 ]; then
			printf "\tError installing MondgDB C++ driver.\nMake sure you have sudo privileges.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		cd ..
		rm -rf ${TEMP_DIR}/mongo-cxx-driver
	else
		printf "\tMongo C++ driver found at /usr/local/lib/libmongocxx.dylib.\n"
	fi

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
		make -j${CPU_CORE}
		if [ $? -ne 0 ]; then
			printf "\tError compiling secp256k1-zkp.\n"
			printf "\tExiting now.\n\n"
			exit;
		fi
		sudo make install
		rm -rf ${TEMP_DIR}/secp256k1-zkp
	else
		printf "\tsecp256k1 found at /usr/local/lib/\n"
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
		sudo make -j${CPU_CORE} install
		sudo rm -rf ${TEMP_DIR}/wasm-compiler
	else
		printf "\tWASM found at /usr/local/wasm/bin/\n"
	fi
