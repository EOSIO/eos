	OS_VER=$(sw_vers -productVersion)
	OS_MAJ=$(echo "${OS_VER}" | cut -d'.' -f1)
	OS_MIN=$(echo "${OS_VER}" | cut -d'.' -f2)
	OS_PATCH=$(echo "${OS_VER}" | cut -d'.' -f3)

	MEM_GIG=$(bc <<< "($(sysctl -in hw.memsize) / 1024000000)")

	CPU_SPEED=$(bc <<< "scale=2; ($(sysctl -in hw.cpufrequency) / 10^8) / 10")
	CPU_CORE=$( sysctl -in machdep.cpu.core_count )

	DISK_INSTALL=$(df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 || cut -d' ' -f1)
	blksize=$(df . | head -1 | awk '{print $2}' | cut -d- -f1)
	gbfactor=$(( 1073741824 / blksize ))
	total_blks=$(df . | tail -1 | awk '{print $2}')
	avail_blks=$(df . | tail -1 | awk '{print $4}')
	DISK_TOTAL=$((total_blks / gbfactor ))
	DISK_AVAIL=$((avail_blks / gbfactor ))

	printf "\\n\\tOS name: %s\\n" "${ARCH}"
	printf "\\tOS Version: %s\\n" "${OS_VER}"
	printf "\\tCPU speed: %sGhz\\n" "${CPU_SPEED}"
	printf "\\tCPU cores: %s\\n" "${CPU_CORE}"
	printf "\\tPhysical Memory: %s Gbytes\\n" "${MEM_GIG}"
	printf "\\tDisk install: %s\\n" "${DISK_INSTALL}"
	printf "\\tDisk space total: %sG\\n" "${DISK_TOTAL}"
	printf "\\tDisk space available: %sG\\n\\n" "${DISK_AVAIL}"

	if [ "${MEM_GIG}" -lt 7 ]; then
		echo "Your system must have 7 or more Gigabytes of physical memory installed."
		echo "Exiting now."
		exit 1
	fi

	if [ "${OS_MIN}" -lt 12 ]; then
		echo "You must be running Mac OS 10.12.x or higher to install EOSIO."
		echo "Exiting now."
		exit 1
	fi

	if [ "${DISK_AVAIL}" -lt "$DISK_MIN" ]; then
		echo "You must have at least ${DISK_MIN}GB of available storage to install EOSIO."
		echo "Exiting now."
		exit 1
	fi

	printf "\\tChecking xcode-select installation\\n"
	if ! XCODESELECT=$( command -v xcode-select)
	then
		printf "\\n\\tXCode must be installed in order to proceed.\\n\\n"
		printf "\\tExiting now.\\n"
		exit 1
	fi

	printf "\\txcode-select installation found @ \\n"
	printf "\\t%s \\n\\n" "${XCODESELECT}"

	printf "\\tChecking Ruby installation.\\n"
	if ! RUBY=$( command -v ruby)
	then
		printf "\\nRuby must be installed in order to proceed.\\n\\n"
		printf "\\tExiting now.\\n"
		exit 1
	fi

	printf "\\tRuby installation found @ \\n"
	printf "\\t%s \\n\\n" "${RUBY}"

	printf "\\tChecking Home Brew installation\\n"
	if ! BREW=$( command -v brew )
	then
		printf "\\tHomebrew must be installed to compile EOS.IO\\n\\n"
		printf "\\tDo you wish to install Home Brew?\\n"
		select yn in "Yes" "No"; do
			case "${yn}" in
				[Yy]* ) 
				"${XCODESELECT}" --install 2>/dev/null;
				if ! "${RUBY}" -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
				then
					echo "Unable to install homebrew at this time. Exiting now."
					exit 1;
				else
					BREW=$( command -v brew )
				fi
				break;;
				[Nn]* ) echo "User aborted homebrew installation. Exiting now.";
						exit 1;;
				* ) echo "Please enter 1 for yes or 2 for no.";;
			esac
		done
	fi

	printf "\\tHome Brew installation found @\\n"
	printf "\\t%s\\n\\n" "${BREW}"
	
	COUNT=1
	PERMISSION_GETTEXT=0
	DISPLAY=""
	DEP=""

	printf "\\tChecking dependencies.\\n"
	var_ifs="${IFS}"
	IFS=","
	while read -r name tester testee brewname uri
	do
		printf "\\tChecking %s ... " "${name}"
		if [ "${tester}" "${testee}" ]; then
			printf "\\t\\t %s found\\n" "${name}"
			continue
		fi
		# resolve conflict with homebrew glibtool and apple/gnu installs of libtool
		if [ "${testee}" == "/usr/local/bin/glibtool" ]; then
			if [ "${tester}" "/usr/local/bin/libtool" ]; then
				printf "\\t\\t %s found\\n" "${name}"
				continue
			fi
		fi
		if [ "${brewname}" = "gettext" ]; then
			PERMISSION_GETTEXT=1
		fi
		DEP=$DEP"${brewname} "
		DISPLAY="${DISPLAY}${COUNT}. ${name}\\n\\t"
		printf "\\t\\t %s ${bldred}NOT${txtrst} found.\\n" "${name}"
		(( COUNT++ ))
	done < scripts/eosio_build_dep
	IFS="${var_ifs}"
		
	printf "\\tChecking Python3 ... "
	if [  -z "$( python3 -c 'import sys; print(sys.version_info.major)' 2>/dev/null )" ]; then
		DEP=$DEP"python@3 "
		DISPLAY="${DISPLAY}${COUNT}. Python 3\\n\\t"
		printf "\\t\\t python3 ${bldred}NOT${txtrst} found.\\n"
		(( COUNT++ ))
	else
		printf "\\t\\t Python3 found\\n"
	fi

	if [ $COUNT -gt 1 ]; then
		printf "\\n\\tThe following dependencies are required to install EOSIO.\\n"
		printf "\\n\\t%s\\n\\n" "${DISPLAY}"
		echo "Do you wish to install these packages?"
		select yn in "Yes" "No"; do
			case $yn in
				[Yy]* ) 
					if [ $PERMISSION_GETTEXT -eq 1 ]; then
						sudo chown -R "$(whoami)" /usr/local/share
					fi
					"${XCODESELECT}" --install 2>/dev/null;
					printf "\\tUpdating Home Brew.\\n"
					if ! brew update
					then
						printf "\\tUnable to update Home Brew at this time.\\n"
						printf "\\tExiting now.\\n\\n"
						exit 1;
					fi
					printf "\\tInstalling Dependencies.\\n"
					if ! "${BREW}" install --force ${DEP}
					then
						printf "\\tHomebrew exited with the above errors.\\n"
						printf "\\tExiting now.\\n\\n"
						exit 1;
					fi
					if ! "${BREW}" unlink {DEP} && "${BREW}" link --force ${DEP}
					then
						printf "\\tHomebrew exited with the above errors.\\n"
						printf "\\tExiting now.\\n\\n"
						exit 1;
					fi
				break;;
				[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
				* ) echo "Please type 1 for yes or 2 for no.";;
			esac
		done
	else 
		printf "\\n\\tNo required Home Brew dependencies to install.\\n"
	fi

		
	printf "\\n\\tChecking boost library installation.\\n"
	BVERSION=$( grep "#define BOOST_VERSION" "/usr/local/include/boost/version.hpp" 2>/dev/null | tail -1 | tr -s ' ' | cut -d\  -f3 )
	if [ "${BVERSION}" != "106700" ]; then
		if [ ! -z "${BVERSION}" ]; then
			printf "\\tFound Boost Version %s.\\n" "${BVERSION}"
			printf "\\tEOS.IO requires Boost version 1.67.\\n"
			printf "\\tWould you like to uninstall version %s and install Boost version 1.67.\\n" "${BVERSION}"
			select yn in "Yes" "No"; do
				case $yn in
					[Yy]* )
						if "${BREW}" list | grep "boost"
						then 
							printf "\\tUninstalling Boost Version %s.\\n" "${BVERSION}"
							if ! "${BREW}" uninstall --force boost
							then
								printf "\\tUnable to remove boost libraries at this time. 0\\n"
								printf "\\tExiting now.\\n\\n"
								exit 1;
							fi
						else
							printf "\\tRemoving Boost Version %s.\\n" "${BVERSION}"
							if ! sudo rm -rf "/usr/local/include/boost"
							then
								printf "\\tUnable to remove boost libraries at this time. 1\\n"
								printf "\\tExiting now.\\n\\n"
								exit 1;
							fi
							if ! sudo rm -rf /usr/local/lib/libboost*
							then
								printf "\\tUnable to remove boost libraries at this time. 2\\n"
								printf "\\tExiting now.\\n\\n"
								exit 1;
							fi
						fi
					break;;
					[Nn]* ) echo "User cancelled installation of Boost libraries, Exiting now."; exit;;
					* ) echo "Please type 1 for yes or 2 for no.";;
				esac
			done
		fi
		printf "\\tInstalling boost libraries.\\n"
		if ! "${BREW}" install boost
		then
			printf "\\tUnable to install boost 1.67 libraries at this time. 0\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if [ -d "$BUILD_DIR" ]; then
			if ! rm -rf "$BUILD_DIR"
			then
			printf "\\tUnable to remove directory %s. Please remove this directory and run this script %s again. 0\\n" "$BUILD_DIR" "${BASH_SOURCE[0]}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
			fi
		fi
		printf "\\tBoost 1.67.0 successfully installed @ /usr/local.\\n"
	else
		printf "\\tBoost 1.67.0 found at /usr/local.\\n"
	fi

	printf "\\n\\tChecking MongoDB C++ driver installation.\\n"
    if [ ! -e "/usr/local/lib/libmongocxx-static.a" ]; then
		if ! cd "${TEMP_DIR}"
		then
			printf "\\tUnable to enter directory %s.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! pkgconfig=$( "${BREW}" list | grep pkg-config )
		then
			if ! "${BREW}" install --force pkg-config
			then
				printf "\\tHomebrew returned an error installing pkg-config.\\n"
				printf "\\tExiting now.\\n\\n"
				exit 1;
			fi
			if ! "${BREW}" unlink pkg-config && "${BREW}" link --force pkg-config
			then
				printf "\\tHomebrew returned an error linking pkgconfig.\\n"
				printf "\\tExiting now.\\n\\n"
				exit 1;
			fi
		fi
		STATUS=$(curl -LO -w '%{http_code}' --connect-timeout 30 https://github.com/mongodb/mongo-c-driver/releases/download/1.9.3/mongo-c-driver-1.9.3.tar.gz)
		if [ "${STATUS}" -ne 200 ]; then
			if ! rm -f "${TEMP_DIR}/mongo-c-driver-1.9.3.tar.gz"
			then
				printf "\\tUnable to remove file %s/mongo-c-driver-1.9.3.tar.gz.\\n" "${TEMP_DIR}"
			fi
			printf "\\tUnable to download MongoDB C driver at this time.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! tar xf mongo-c-driver-1.9.3.tar.gz
		then
			printf "\\tUnable to unarchive file mongo-c-driver-1.9.3.tar.gz.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -f "${TEMP_DIR}/mongo-c-driver-1.9.3.tar.gz"
		then
			printf "\\tUnable to remove file mongo-c-driver-1.9.3.tar.gz.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd mongo-c-driver-1.9.3
		then
			printf "\\tUnable to cd into directory %s/mongo-c-driver-1.9.3.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ./configure --enable-static --with-libbson=bundled --enable-ssl=darwin --disable-automatic-init-and-cleanup --prefix=/usr/local
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
		if ! rm -rf "${TEMP_DIR}/mongo-c-driver-1.9.3"
		then
			printf "\\tUnable to remove directory %s/mongo-c-driver-1.9.3.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! git clone https://github.com/mongodb/mongo-cxx-driver.git --branch releases/stable --depth 1
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
		if ! cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
		then
			printf "\\tCmake has encountered the above errors building the MongoDB C++ driver.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! make -j"${CPU_CORE}"
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
		if ! rm -rf "${TEMP_DIR}/mongo-cxx-driver"
		then
			printf "\\tUnable to remove directory %s/mongo-cxx-driver.\\n" "${TEMP_DIR}" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		printf "\\tMongo C++ driver installed at /usr/local/lib/libmongocxx-static.a.\\n"
	else
		printf "\\tMongo C++ driver found at /usr/local/lib/libmongocxx-static.a.\\n"
	fi

	printf "\\n\\tChecking secp256k1-zkp installation.\\n"
    # install secp256k1-zkp (Cryptonomex branch)
    if [ ! -e "/usr/local/lib/libsecp256k1.a" ]; then
		if ! cd "${TEMP_DIR}"
		then
			printf "\\tUnable to enter directory %s.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
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
			printf "\\tUnable to enter directory %s/secp256k1-zkp.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ./autogen.sh
		then
			printf "\\tError running autogen.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! ./configure
		then
			printf "\\tConfiguring secp256k1-zkp has returned the above error.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! make -j"${CPU_CORE}"
		then
			printf "\\tError compiling secp256k1-zkp.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo make install
		then
			printf "\\tInstalling secp256k1-zkp has returned the above error.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! rm -rf "${TEMP_DIR}/secp256k1-zkp"
		then
			printf "\\tUnable to remove directory %s/secp256k1-zkp56k1-zkp.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		printf "\\n\\n\\tSuccessffully installed secp256k1 @ /usr/local/lib/.\\n\\n"
	else
		printf "\\tsecp256k1 found at /usr/local/lib/.\\n"
	fi
  
	printf "\\n\\tChecking LLVM with WASM support.\\n"
	if [ ! -d /usr/local/wasm/bin ]; then
		if ! cd "${TEMP_DIR}"
		then
			printf "\\tUnable to enter directory %s.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! mkdir "${TEMP_DIR}/wasm-compiler"
		then
			printf "\\tUnable to create directory %s/wasm-compiler.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/wasm-compiler"
		then
			printf "\\tUnable to enter directory %s/wasm-compiler.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/llvm.git
		then
			printf "\\tUnable to clone llvm repo @ https://github.com/llvm-mirror/llvm.git.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/wasm-compiler/llvm/tools"
		then
			printf "\\tUnable to enter directory %s/wasm-compiler/llvm/tools.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! git clone --depth 1 --single-branch --branch release_40 https://github.com/llvm-mirror/clang.git
		then
			printf "\\tUnable to clone clang repo @ https://github.com/llvm-mirror/clang.git.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/wasm-compiler/llvm"
		then
			printf "\\tUnable to enter directory %s/wasm-compiler/llvm.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! mkdir "${TEMP_DIR}/wasm-compiler/llvm/build"
		then
			printf "\\tUnable to create directory %s/wasm-compiler/llvm/build.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cd "${TEMP_DIR}/wasm-compiler/llvm/build"
		then
			printf "\\tUnable to enter directory %s/wasm-compiler/llvm/build.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/usr/local/wasm \
		-DLLVM_TARGETS_TO_BUILD= -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=WebAssembly \
		-DCMAKE_BUILD_TYPE=Release ../
		then
			printf "\\tError compiling LLVM/Clang with WASM support.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo make -j"${CPU_CORE}" install
		then
			printf "\\tCompiling LLVM/Clang with WASM support has exited with the error above.\\n"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		if ! sudo rm -rf "${TEMP_DIR}/wasm-compiler"
		then
			printf "\\tUnable to remove directory %s/wasm-compiler.\\n" "${TEMP_DIR}"
			printf "\\tExiting now.\\n\\n"
			exit 1;
		fi
		printf "\\tSuccessfully installed LLVM/Clang with WASM support @ /usr/local/wasm/bin/.\\n"
	else
		printf "\\tWASM found at /usr/local/wasm/bin/.\\n"
	fi

	function print_instructions()
	{
		printf "\\n\\t%s -f %s &\\n" "$( command -v mongod )" "${MONGOD_CONF}"
		printf "\\tcd %s; make test\\n\\n" "${BUILD_DIR}"
	return 0
	}
