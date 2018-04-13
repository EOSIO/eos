	OS_VER=`sw_vers -productVersion`
	OS_MAJ=`echo "${OS_VER}" | cut -d'.' -f1`
	OS_MIN=`echo "${OS_VER}" | cut -d'.' -f2`
	OS_PATCH=`echo "${OS_VER}" | cut -d'.' -f3`

	MEM_GIG=`bc <<< "($(sysctl -in hw.memsize) / 1024000000)"`

	CPU_SPEED=`bc <<< "scale=2; ($(sysctl -in hw.cpufrequency) / 10^6) / 10"`
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

	if [ $MEM_GIG -lt 7 ]; then
		echo "Your system must have 7 or more Gigabytes of physical memory installed."
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
	COUNT=1
	PERMISSION_GETTEXT=0
	BOOST_CHECK=0
	DISPLAY=""
	DEP=""

	printf "\tChecking dependencies.\n"
	var_ifs=${IFS}
	IFS=','
	while read -r name tester testee brewname uri
	do
		printf "\tChecking $name ... "
		if [ ${tester} ${testee} ]; then
			# check boost version, it should be 1_66
			if [ "${brewname}" = "boost" ]; then
				BVERSION=`cat "${testee}" 2>/dev/null | grep BOOST_LIB_VERSION | tail -1 \
				| tr -s ' ' | cut -d\  -f3 | sed 's/[^0-9_]//g'`
				if [ "${BVERSION}" != "1_66" ]; then
					BOOST_CHECK=1
					DEP=$DEP"${brewname} "
					DISPLAY="${DISPLAY}${COUNT}. ${name}\n\t"
					printf "\t\t ${name} ${bldred}needs updating.${txtrst}.\n"
					let COUNT++
					continue
				fi
			fi
			printf '\t\t %s found\n' "$name"
			continue
		fi
		# resolve conflict with homebrew glibtool and apple/gnu installs of libtool
		if [ ${testee} == "/usr/local/bin/glibtool" ]; then
			if [ ${tester} "/usr/local/bin/libtool" ]; then
				printf '\t\t %s found\n' "$name"
				continue
			fi
		fi
		if [ $brewname = "gettext" ]; then
			PERMISSION_GETTEXT=1
		fi
		DEP=$DEP"${brewname} "
		DISPLAY="${DISPLAY}${COUNT}. ${name}\n\t"
		printf "\t\t ${name} ${bldred}NOT${txtrst} found.\n"
		let COUNT++
	done < scripts/eosio_build_dep
	IFS=${var_ifs}
		
	printf "\tChecking Python3 ... "
	if [  -z `python3 -c 'import sys; print(sys.version_info.major)' 2>/dev/null` ]; then
		DEP=$DEP"python@3 "
		DISPLAY="${DISPLAY}${COUNT}. Python 3\n\t"
		printf "\t\t python3 ${bldred}NOT${txtrst} found.\n"
		let DCOUNT++
	else
		printf "\t\t Python3 found\n"
	fi

	if [ $COUNT -gt 1 ]; then
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
					if [ $? -ne 0 ]; then
						printf "\tUnable to update Home Brew at this time.\n"
						printf "\tExiting now.\n\n"
						exit;
					fi
					printf "\tInstalling Dependencies.\n"
					if [ "${BOOST_CHECK}" = 1 ]; then
						brew remove boost
					fi 
					brew install --force $DEP
					if [ $? -ne 0 ]; then
						printf "\tHomebrew exited with the above errors.\n"
						printf "\tExiting now.\n\n"
						exit;
					fi
					brew unlink $DEP && brew link --force $DEP
					if [ $? -ne 0 ]; then
						printf "\tHomebrew exited with the above errors.\n"
						printf "\tExiting now.\n\n"
						exit;
					fi
				break;;
				[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else 
		printf "\n\tNo required Home Brew dependencies to install.\n"
	fi
		
	printf "\n\tChecking MongoDB C++ driver installation.\n"
    if [ ! -e /usr/local/lib/libmongocxx-static.a ]; then
		cd ${TEMP_DIR}
		brew install --force pkgconfig
		brew unlink pkgconfig && brew link --force pkgconfig
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
		./configure --enable-static --with-libbson=bundled --enable-ssl=darwin --disable-automatic-init-and-cleanup --prefix=/usr/local
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
		rm -rf ${TEMP_DIR}/mongo-cxx-driver
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
		make -j${CPU_CORE}
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
		rm -rf ${TEMP_DIR}/mongo-cxx-driver
	else
		printf "\tMongo C++ driver found at /usr/local/lib/libmongocxx-static.a.\n"
	fi

	printf "\n\tChecking secp256k1-zkp installation.\n"
    # install secp256k1-zkp (Cryptonomex branch)
    if [ ! -e /usr/local/lib/libsecp256k1.a ]; then
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
  
	printf "\n\tChecking LLVM with WASM support.\n"
	if [ ! -d /usr/local/wasm/bin ]; then
		cd ${TEMP_DIR}
		mkdir wasm-compiler
		cd wasm-compiler
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
