if [ $1 == 1 ]; then answer=1; fi # NONINTERACTIVE

OS_VER=$(sw_vers -productVersion)
OS_MAJ=$(echo "${OS_VER}" | cut -d'.' -f1)
OS_MIN=$(echo "${OS_VER}" | cut -d'.' -f2)
OS_PATCH=$(echo "${OS_VER}" | cut -d'.' -f3)
MEM_GIG=$(bc <<< "($(sysctl -in hw.memsize) / 1024000000)")
CPU_SPEED=$(bc <<< "scale=2; ($(sysctl -in hw.cpufrequency) / 10^8) / 10")
CPU_CORE=$( sysctl -in machdep.cpu.core_count )
export JOBS=$(( MEM_GIG > CPU_CORE ? CPU_CORE : MEM_GIG ))

DISK_INSTALL=$(df -h . | tail -1 | tr -s ' ' | cut -d\  -f1 || cut -d' ' -f1)
blksize=$(df . | head -1 | awk '{print $2}' | cut -d- -f1)
gbfactor=$(( 1073741824 / blksize ))
total_blks=$(df . | tail -1 | awk '{print $2}')
avail_blks=$(df . | tail -1 | awk '{print $4}')
DISK_TOTAL=$((total_blks / gbfactor ))
DISK_AVAIL=$((avail_blks / gbfactor ))

export HOMEBREW_NO_AUTO_UPDATE=1

COUNT=1
PERMISSION_GETTEXT=0
DISPLAY=""
DEPS=""

printf "\\nOS name: ${OS_NAME}\\n"
printf "OS Version: ${OS_VER}\\n"
printf "CPU speed: ${CPU_SPEED}Mhz\\n"
printf "CPU cores: %s\\n" "${CPU_CORE}"
printf "Physical Memory: ${MEM_GIG} Gbytes\\n"
printf "Disk install: ${DISK_INSTALL}\\n"
printf "Disk space total: ${DISK_TOTAL}G\\n"
printf "Disk space available: ${DISK_AVAIL}G\\n"

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

printf "\\n"

printf "Checking xcode-select installation...\\n"
if ! XCODESELECT=$( command -v xcode-select)
then
	printf " - XCode must be installed in order to proceed!\\n\\n"
	exit 1
fi
printf " - XCode installation found @ ${XCODESELECT}\\n"

printf "Checking Ruby installation...\\n"
if ! RUBY=$( command -v ruby)
then
	printf " - Ruby must be installed in order to proceed!\\n"
	exit 1
fi
printf " - Ruby installation found @ ${RUBY}\\n"

printf "Checking CMAKE installation...\\n"
CMAKE=$(command -v cmake 2>/dev/null)
if [ -z $CMAKE ]; then
	printf "Installing CMAKE...\\n"
	curl -LO https://cmake.org/files/v$CMAKE_VERSION_MAJOR.$CMAKE_VERSION_MINOR/cmake-$CMAKE_VERSION.tar.gz \
	&& tar xf cmake-$CMAKE_VERSION.tar.gz \
	&& cd cmake-$CMAKE_VERSION \
	&& ./bootstrap --prefix=$HOME \
	&& make -j"${JOBS}" \
	&& make install \
	&& cd .. \
	&& rm -f cmake-$CMAKE_VERSION.tar.gz \
	|| exit 1
	printf " - CMAKE successfully installed @ ${HOME}/bin/cmake \\n"
else
	printf " - CMAKE found @ ${CMAKE}.\\n"
fi

printf "Checking Home Brew installation...\\n"
if ! BREW=$( command -v brew )
then
	printf "Homebrew must be installed to compile EOS.IO!\\n"
	if [ $1 == 0 ]; then read -p "Do you wish to install HomeBrew? (y/n)? " answer; fi
	case ${answer} in
		1 | [Yy]* )
			"${XCODESELECT}" --install 2>/dev/null;
			if ! "${RUBY}" -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"; then
				echo " - Unable to install homebrew at this time."
				exit 1;
			else
				BREW=$( command -v brew )
			fi
		;;
		[Nn]* ) echo "User aborted homebrew installation. Exiting now."; exit 1;;
		* ) echo "Please type 'y' for yes or 'n' for no.";;
	esac

fi
printf " - Home Brew installation found @ ${BREW}\\n"

printf "\\nChecking dependencies...\\n"
var_ifs="${IFS}"
IFS=","
while read -r name tester testee brewname uri; do
	if [ "${tester}" "${testee}" ]; then
		printf " - %s found\\n" "${name}"
		continue
	fi
	# resolve conflict with homebrew glibtool and apple/gnu installs of libtool
	if [ "${testee}" == "/usr/local/bin/glibtool" ]; then
		if [ "${tester}" "/usr/local/bin/libtool" ]; then
			printf " - %s found\\n" "${name}"
			continue
		fi
	fi
	if [ "${brewname}" = "gettext" ]; then
		PERMISSION_GETTEXT=1
	fi
	DEPS=$DEPS"${brewname},"
	DISPLAY="${DISPLAY}${COUNT}. ${name}\\n"
	printf " - %s ${bldred}NOT${txtrst} found.\\n" "${name}"
	(( COUNT++ ))
done < "${CURRENT_DIR}/scripts/eosio_build_darwin_deps"
IFS="${var_ifs}"

if [ ! -d /usr/local/Frameworks ]; then
	printf "\\n${bldred}/usr/local/Frameworks is necessary to brew install python@3. Run the following commands as sudo and try again:${txtrst}\\n"
	printf "sudo mkdir /usr/local/Frameworks && sudo chown $(whoami):admin /usr/local/Frameworks\\n\\n"
	exit 1;
fi

if [  -z "$( python3 -c 'import sys; print(sys.version_info.major)' 2>/dev/null )" ]; then
	DEPS=$DEPS"python@3 "
	DISPLAY="${DISPLAY}${COUNT}. Python 3\\n"
	printf " - python3 ${bldred}NOT${txtrst} found.\\n"
	(( COUNT++ ))
else
	printf " - Python3 found\\n"
fi

if [ $COUNT -gt 1 ]; then
	printf "\\nThe following dependencies are required to install EOSIO:\\n"
	printf "${DISPLAY}\\n\\n"
	if [ $1 == 0 ]; then read -p "Do you wish to install these packages? (y/n) " answer; fi
	case ${answer} in
		1 | [Yy]* )
			"${XCODESELECT}" --install 2>/dev/null;
			if [ $1 == 0 ]; then read -p "Do you wish to update homebrew packages first? (y/n) " answer; fi
			case ${answer} in
				1 | [Yy]* )
					if ! brew update; then
						printf " - Brew update failed.\\n"
						exit 1;
					else
						printf " - Brew update complete.\\n"
					fi
				;;
				[Nn]* ) echo "Proceeding without update!";;
				* ) echo "Please type 'y' for yes or 'n' for no.";;
			esac
			brew tap eosio/eosio # Required to install mongo-cxx-driver with static library
			printf "\\nInstalling Dependencies...\\n"
			# Ignore cmake so we don't install a newer version.
			# Build from source to use local cmake; see homebrew-eosio repo for examples
			# DON'T INSTALL llvm@4 WITH --force!
			OIFS="$IFS"
			IFS=$','
			for DEP in $DEPS; do
				# Eval to support string/arguments with $DEP
				if ! eval $BREW install $DEP; then
					printf " - Homebrew exited with the above errors!\\n"
					exit 1;
				fi
			done
			IFS="$OIFS"
		;;
		[Nn]* ) echo "User aborting installation of required dependencies, Exiting now."; exit;;
		* ) echo "Please type 'y' for yes or 'n' for no.";;
	esac
else
	printf "\\n - No required Home Brew dependencies to install.\\n"
fi


printf "\\n"


printf "Checking Boost library (${BOOST_VERSION}) installation...\\n"
if [ ! -d $BOOST_ROOT ]; then
	printf "Installing Boost library...\\n"
	curl -LO https://dl.bintray.com/boostorg/release/$BOOST_VERSION_MAJOR.$BOOST_VERSION_MINOR.$BOOST_VERSION_PATCH/source/boost_$BOOST_VERSION.tar.bz2 \
	&& tar -xvf boost_$BOOST_VERSION.tar.bz2 \
	&& cd $BOOST_ROOT \
	&& ./bootstrap.sh --prefix=$BOOST_ROOT \
	&& ./b2 -q -j$(sysctl -in machdep.cpu.core_count) install \
	&& cd .. \
	&& rm -f boost_$BOOST_VERSION.tar.bz2 \
	&& rm -rf $BOOST_LINK_LOCATION \
	&& ln -s $BOOST_ROOT $BOOST_LINK_LOCATION \
	|| exit 1
	printf " - Boost library successfully installed @ ${BOOST_ROOT}.\\n"
else
	printf " - Boost library found with correct version @ ${BOOST_ROOT}.\\n"
fi


printf "\\n"


printf "\\nChecking MongoDB installation...\\n"
if [ ! -d $MONGODB_ROOT ]; then
	printf "Installing MongoDB into ${MONGODB_ROOT}...\\n"
	curl -OL https://fastdl.mongodb.org/osx/mongodb-osx-ssl-x86_64-$MONGODB_VERSION.tgz \
	&& tar -xzvf mongodb-osx-ssl-x86_64-$MONGODB_VERSION.tgz \
	&& mv $SRC_LOCATION/mongodb-osx-x86_64-$MONGODB_VERSION $MONGODB_ROOT \
	&& touch $MONGODB_LOG_LOCATION/mongod.log \
	&& rm -f mongodb-osx-ssl-x86_64-$MONGODB_VERSION.tgz \
	&& cp -f $CURRENT_DIR/scripts/mongod.conf $MONGODB_CONF \
	&& mkdir -p $MONGODB_DATA_LOCATION \
	&& rm -rf $MONGODB_LINK_LOCATION \
	&& rm -rf $BIN_LOCATION/mongod \
	&& ln -s $MONGODB_ROOT $MONGODB_LINK_LOCATION \
	&& ln -s $MONGODB_LINK_LOCATION/bin/mongod $BIN_LOCATION/mongod \
	|| exit 1
	printf " - MongoDB successfully installed @ ${MONGODB_ROOT}\\n"
else
	printf " - MongoDB found with correct version @ ${MONGODB_ROOT}.\\n"
fi

# We install llvm into /usr/local/opt using brew install llvm@4
printf "\\nChecking LLVM 4 support...\\n"
if [ ! -d $LLVM_ROOT ]; then
	ln -s /usr/local/opt/llvm@4 $LLVM_ROOT \
	|| exit 1
	printf " - LLVM (WASM compiler) successfully linked from /usr/local/opt/llvm@4 to ${LLVM_ROOT}\\n"
else
	printf " - LLVM (WASM compiler) found @ ${LLVM_ROOT}.\\n"
fi

cd ..
printf "\\n"

function print_instructions()
{
printf "Please ensure the following \$PATH and \$LD_LIBRARY_PATH stucture, in the order specified, within your ~/.bash_profile/rc file:\\n"
	# HOME/bin first to load proper cmake version over the one in /usr/bin.
	# llvm/bin last to prevent llvm/bin/clang from being used over /usr/bin/clang + We don't symlink into $HOME/bin
	printf "export PATH=\$HOME/bin:\$PATH:$MONGODB_LINK_LOCATION/bin:\$HOME/opt/llvm/bin\\n"
	printf "export LD_LIBRARY_PATH=\$HOME/opt/llvm/lib:\$LD_LIBRARY_PATH\\n"
	printf "${BIN_LOCATION}/mongod --dbpath ${MONGODB_DATA_LOCATION} -f ${MONGODB_CONF} --logpath ${MONGODB_LOG_LOCATION}/mongod.log &\\n"
	printf "cd ${BUILD_DIR} && make test\\n"
	return 0
}
