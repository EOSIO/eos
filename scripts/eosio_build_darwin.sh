echo "OS name: ${NAME}"
echo "OS Version: ${OS_VER}"
echo "CPU cores: ${CPU_CORES}"
echo "Physical Memory: ${MEM_GIG}G"
echo "Disk space total: ${DISK_TOTAL}G"
echo "Disk space available: ${DISK_AVAIL}G"
echo "Disk install: ${DISK_INSTALL}"

[[ "${OS_MIN}" -lt 12 ]] && echo "You must be running Mac OS 10.12.x or higher to install EOSIO." && exit 1

[[ $MEM_GIG -lt 7 ]] && echo "Your system must have 7 or more Gigabytes of physical memory installed." && exit 1
[[ "${DISK_AVAIL}" -lt "${DISK_MIN}" ]] && echo " - You must have at least ${DISK_MIN}GB of available storage to install EOSIO." && exit 1

export HOMEBREW_NO_AUTO_UPDATE=1

COUNT=1
DISPLAY=""
DEPS=""

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

printf "Checking Home Brew installation...\\n"
if ! BREW=$( command -v brew )
then
	printf "Homebrew must be installed to compile EOS.IO!\\n"
	if [ $ANSWER != 1 ]; then read -p "Do you wish to install HomeBrew? (y/n)? " ANSWER; fi
	case $ANSWER in
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
		* ) echo "Please type 'y' for yes or 'n' for no."; exit;;
	esac

fi
printf " - Home Brew installation found @ ${BREW}\\n"

printf "\\nChecking dependencies...\\n"
var_ifs="${IFS}"
IFS=","
while read -r name tester testee brewname uri; do
	if [ $tester $testee ]; then
		printf " - ${name} found!\\n"
		continue
	fi
	# resolve conflict with homebrew glibtool and apple/gnu installs of libtool
	if [ "${testee}" == "/usr/local/bin/glibtool" ]; then
		if [ "${tester}" "/usr/local/bin/libtool" ]; then
			printf " - ${name} found!\\n"
			continue
		fi
	fi
	DEPS=$DEPS"${brewname},"
	DISPLAY="${DISPLAY}${COUNT}. ${name}\\n"
	printf " - ${name} ${bldred}NOT${txtrst} found.\\n"
	(( COUNT++ ))
done < "${REPO_ROOT}/scripts/eosio_build_darwin_deps"
IFS="${var_ifs}"

if [ ! -d /usr/local/Frameworks ]; then
	printf "\\n${bldred}/usr/local/Frameworks is necessary to brew install python@3. Run the following commands as sudo and try again:${txtrst}\\n"
	printf "sudo mkdir /usr/local/Frameworks && sudo chown $(whoami):admin /usr/local/Frameworks\\n\\n"
	exit 1;
fi

if [ $COUNT -gt 1 ]; then
	printf "\\nThe following dependencies are required to install EOSIO:\\n"
	printf "${DISPLAY}\\n\\n"
	if [ $ANSWER != 1 ]; then read -p "Do you wish to install these packages? (y/n) " ANSWER; fi
	case $ANSWER in
		1 | [Yy]* )
			"${XCODESELECT}" --install 2>/dev/null;
			if [ $1 == 0 ]; then read -p "Do you wish to update homebrew packages first? (y/n) " ANSWER; fi
			case $ANSWER in
				1 | [Yy]* )
					if ! brew update; then
						printf " - Brew update failed.\\n"
						exit 1;
					else
						printf " - Brew update complete.\\n"
					fi
				;;
				[Nn]* ) echo "Proceeding without update!";;
				* ) echo "Please type 'y' for yes or 'n' for no."; exit;;
			esac
			brew tap eosio/eosio
			printf "\\nInstalling Dependencies...\\n"
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
		* ) echo "Please type 'y' for yes or 'n' for no."; exit;;
	esac
else
	printf " - No required Home Brew dependencies to install.\\n"
fi


printf "\\n"

### clean up force build before starting
if [ $FORCE_BUILD ];then
   rm -rf  \
   ${SRC_LOCATION}/llvm ${OPT_LOCATION}/llvm4 \
   ${TMP_LOCATION}/clang8 ${OPT_LOCATION}/clang8 \
   ${SRC_LOCATION}/zlib ${OPT_LOCATION}/zlib \
   ${SRC_LOCATION}/boost \
   ${SRC_LOCATION}/mongodb-linux-x86_64-amazon-$MONGODB_VERSION \
   ${SRC_LOCATION}/mongo-c-driver-$MONGO_C_DRIVER_VERSION \
   ${SRC_LOCATION}/mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION
fi

export CPATH="$(python-config --includes | awk '{print $1}' | cut -dI -f2):$CPATH" # Boost has trouble finding pyconfig.h
printf "Checking Boost library (${BOOST_VERSION}) installation...\\n"
BOOSTVERSION=$( grep "#define BOOST_VERSION" "$BOOST_ROOT/include/boost/version.hpp" 2>/dev/null | tail -1 | tr -s ' ' | cut -d\  -f3 )
if [ "${BOOSTVERSION}" != "${BOOST_VERSION_MAJOR}0${BOOST_VERSION_MINOR}0${BOOST_VERSION_PATCH}" ] || [ $FORCE_BUILD ]; then
	printf "Installing Boost library...\\n"
	curl -LO https://dl.bintray.com/boostorg/release/$BOOST_VERSION_MAJOR.$BOOST_VERSION_MINOR.$BOOST_VERSION_PATCH/source/boost_$BOOST_VERSION.tar.bz2 \
	&& tar -xjf boost_$BOOST_VERSION.tar.bz2 \
	&& cd $BOOST_ROOT \
	&& ./bootstrap.sh --prefix=$BOOST_ROOT \
	&& ./b2 -q -j$(sysctl -in machdep.cpu.core_count) --with-iostreams --with-date_time --with-filesystem \
	                                                  --with-system --with-program_options --with-chrono --with-test install \
	&& cd .. \
	&& rm -f boost_$BOOST_VERSION.tar.bz2 \
	&& rm -rf $BOOST_LINK_LOCATION \
	&& ln -s $BOOST_ROOT $BOOST_LINK_LOCATION \
	|| exit 1
	printf " - Boost library successfully installed @ ${BOOST_ROOT}.\\n"
else
	printf " - Boost library found with correct version @ ${BOOST_ROOT}.\\n"
fi
if [ $? -ne 0 ]; then exit -1; fi


printf "\\n"

if [ $BUILD_MONGO ]; then
   printf "Checking MongoDB installation...\\n"
   if [ ! -d $MONGODB_ROOT ] || [ $FORCE_BUILD ]; then
      printf "Installing MongoDB into ${MONGODB_ROOT}...\\n"
      curl -OL https://fastdl.mongodb.org/osx/mongodb-osx-ssl-x86_64-$MONGODB_VERSION.tgz \
      && tar -xzf mongodb-osx-ssl-x86_64-$MONGODB_VERSION.tgz \
      && mv $SRC_LOCATION/mongodb-osx-x86_64-$MONGODB_VERSION $MONGODB_ROOT \
      && touch $MONGODB_LOG_LOCATION/mongod.log \
      && rm -f mongodb-osx-ssl-x86_64-$MONGODB_VERSION.tgz \
      && cp -f $REPO_ROOT/scripts/mongod.conf $MONGODB_CONF \
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
   if [ $? -ne 0 ]; then exit -1; fi
   printf "Checking MongoDB C driver installation...\\n"
   if [ ! -d $MONGO_C_DRIVER_ROOT ] || [ $FORCE_BUILD ]; then
      printf "Installing MongoDB C driver...\\n"
      curl -LO https://github.com/mongodb/mongo-c-driver/releases/download/$MONGO_C_DRIVER_VERSION/mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
      && tar -xzf mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
      && cd mongo-c-driver-$MONGO_C_DRIVER_VERSION \
      && mkdir -p cmake-build \
      && cd cmake-build \
      && $CMAKE -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PREFIX -DENABLE_BSON=ON -DENABLE_SSL=DARWIN -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_STATIC=ON -DENABLE_ICU=OFF -DENABLE_SASL=OFF -DENABLE_SNAPPY=OFF .. \
      && make -j"${JOBS}" \
      && make install \
      && cd ../.. \
      && rm mongo-c-driver-$MONGO_C_DRIVER_VERSION.tar.gz \
      || exit 1
      printf " - MongoDB C driver successfully installed @ ${MONGO_C_DRIVER_ROOT}.\\n"
   else
      printf " - MongoDB C driver found with correct version @ ${MONGO_C_DRIVER_ROOT}.\\n"
   fi
   if [ $? -ne 0 ]; then exit -1; fi
   printf "Checking MongoDB C++ driver installation...\\n"
   if [ "$(grep "Version:" $PREFIX/lib/pkgconfig/libmongocxx-static.pc 2>/dev/null | tr -s ' ' | awk '{print $2}')" != $MONGO_CXX_DRIVER_VERSION ] || [ $FORCE_BUILD ]; then
      printf "Installing MongoDB C++ driver...\\n"
      curl -L https://github.com/mongodb/mongo-cxx-driver/archive/r$MONGO_CXX_DRIVER_VERSION.tar.gz -o mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz \
      && tar -xzf mongo-cxx-driver-r${MONGO_CXX_DRIVER_VERSION}.tar.gz \
      && cd mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION \
      && sed -i '' 's/"maxAwaitTimeMS", count/"maxAwaitTimeMS", static_cast<int64_t>(count)/' src/mongocxx/options/change_stream.cpp \
      && sed -i '' 's/add_subdirectory(test)//' src/mongocxx/CMakeLists.txt src/bsoncxx/CMakeLists.txt \
      && cd build \
      && $CMAKE -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_PREFIX_PATH=$PREFIX .. \
      && make -j"${JOBS}" VERBOSE=1 \
      && make install \
      && cd ../.. \
      && rm -f mongo-cxx-driver-r$MONGO_CXX_DRIVER_VERSION.tar.gz \
      || exit 1
      printf " - MongoDB C++ driver successfully installed @ ${MONGO_CXX_DRIVER_ROOT}.\\n"
   else
      printf " - MongoDB C++ driver found with correct version @ ${MONGO_CXX_DRIVER_ROOT}.\\n"
   fi
   if [ $? -ne 0 ]; then exit -1; fi

   printf "\\n"
fi


# We install llvm into /usr/local/opt using brew install llvm@4
printf "Checking LLVM 4 support...\\n"
if [ ! -d $LLVM_ROOT ]; then
	ln -s /usr/local/opt/llvm@4 $LLVM_ROOT \
	|| exit 1
	printf " - LLVM successfully linked from /usr/local/opt/llvm@4 to ${LLVM_ROOT}\\n"
else
	printf " - LLVM found @ ${LLVM_ROOT}.\\n"
fi


cd ..
printf "\\n"

if $BUILD_CLANG8; then
   printf "Checking Clang 8 support...\\n"
   if [ ! -d $CLANG8_ROOT ] || [ $FORCE_BUILD ]; then
      printf "Installing Clang 8...\\n"
      cd ${TMP_LOCATION} \
      && rm -rf clang8 \
      && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/llvm.git clang8 \
      && cd clang8 && git checkout $PINNED_COMPILER_LLVM_COMMIT \
      && cd tools \
      && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/lld.git \
      && cd lld && git checkout $PINNED_COMPILER_LLD_COMMIT && cd ../ \
      && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/polly.git \
      && cd polly && git checkout $PINNED_COMPILER_POLLY_COMMIT && cd ../ \
      && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/clang.git clang && cd clang/tools \
      && git checkout $PINNED_COMPILER_CLANG_VERSION \
      && mkdir extra && cd extra \
      && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/clang-tools-extra.git \
      && cd clang-tools-extra && git checkout $PINNED_COMPILER_CLANG_TOOLS_EXTRA_COMMIT && cd .. \
      && cd ../../../../projects \
      && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/libcxx.git \
      && cd libcxx && git checkout $PINNED_COMPILER_LIBCXX_COMMIT && cd ../ \
      && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/libcxxabi.git \
      && cd libcxxabi && git checkout $PINNED_COMPILER_LIBCXXABI_COMMIT && cd ../ \
      && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/libunwind.git \
      && cd libunwind && git checkout $PINNED_COMPILER_LIBUNWIND_COMMIT && cd ../ \
      && git clone --single-branch --branch $PINNED_COMPILER_BRANCH https://git.llvm.org/git/compiler-rt.git \
      && cd compiler-rt && git checkout $PINNED_COMPILER_COMPILER_RT_COMMIT && cd ../ \
      && cd ${TMP_LOCATION}/clang8 \
      && mkdir build && cd build \
      && $CMAKE -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="${CLANG8_ROOT}" -DLLVM_BUILD_EXTERNAL_COMPILER_RT=ON -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_ENABLE_LIBCXX=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_INCLUDE_DOCS=OFF -DLLVM_OPTIMIZED_TABLEGEN=ON -DLLVM_TARGETS_TO_BUILD=all -DCMAKE_BUILD_TYPE=Release .. \
      && make -j"${JOBS}" \
      && make install \
      && cd ../.. \
      || exit 1
      printf " - Clang 8 successfully installed @ ${CLANG8_ROOT}\\n"
   else
      printf " - Clang 8 found @ ${CLANG8_ROOT}.\\n"
   fi
   if [ $? -ne 0 ]; then exit -1; fi

   printf "\\n"
fi

function print_instructions() {
	return 0
}
