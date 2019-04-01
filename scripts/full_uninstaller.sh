#! /bin/bash
ANSWER=0
if [[ $1 =~ force-* ]]; then FORCED=1; else FORCED=0; fi
if [ -d "/usr/local/include/eosio" ] || [ -d "$HOME/opt/eosio" ] || [ $FORCED == 1 ]; then # use force for running the script directly
   printf "\nEOSIO installation (AND DEPENDENCIES) already found...\n"
   if [ $1 == 0 ]; then
      read -p "Do you wish to remove them? (this includes dependencies)? (y/n) " ANSWER
   elif [ $1 == 1 ] || [ $FORCED == 1 ]; then
      ANSWER=1
   fi
   echo "Uninstalling..."
   case $ANSWER in
      1 | [Yy]* )
         if [ -d "$HOME/opt/eosio" ] || [[ $1 == "force-new" ]]; then
            if [ $( uname ) == "Darwin" ]; then
               # gettext and other brew packages are not modified as they can be dependencies for things other than eosio
               if [ $ANSWER != 1 ]; then read -p "Do you wish to uninstall and unlink all brew installed llvm@4 versions? (y/n) " ANSWER; fi
               case $ANSWER in
                  1 | [Yy]* )
                     brew uninstall llvm@4 --force
                     brew cleanup -s llvm@4
                  ;;
                  [Nn]* ) ;;
                  * ) echo "Please type 'y' for yes or 'n' for no."; exit;;
               esac
               if [ $ANSWER != 1 ]; then read -p "Do you wish to uninstall and unlink all brew installed doxygen versions? (y/n) " ANSWER; fi
               case $ANSWER in
                  1 | [Yy]* )
                     brew uninstall doxygen --force
                     brew cleanup -s doxygen
                  ;;
                  [Nn]* ) ;;
                  * ) echo "Please type 'y' for yes or 'n' for no."; exit;;
               esac
               if [ $ANSWER != 1 ]; then read -p "Do you wish to uninstall and unlink all brew installed graphviz versions? (y/n) " ANSWER; fi
               case $ANSWER in
                  1 | [Yy]* )
                     brew uninstall graphviz --force
                     brew cleanup -s graphviz
                  ;;
                  [Nn]* ) ;;
                  * ) echo "Please type 'y' for yes or 'n' for no."; exit;;
               esac
               if [ $ANSWER != 1 ]; then read -p "Do you wish to uninstall and unlink all brew installed libusb versions? (y/n) " ANSWER; fi
               case $ANSWER in
                  1 | [Yy]* )
                     brew uninstall libusb --force
                     brew cleanup -s libusb
                  ;;
                  [Nn]* ) ;;
                  * ) echo "Please type 'y' for yes or 'n' for no."; exit;;
               esac
            fi
            rm -rf $HOME/opt/eosio
            rm -f $HOME/bin/eosio-launcher
            rm -rf $HOME/lib/cmake/eosios
            rm -rf $HOME/opt/llvm
            rm -f $HOME/opt/boost
            rm -rf $HOME/src/boost_*
            rm -rf $HOME/src/cmake-*
            rm -rf $HOME/share/cmake-*
            rm -rf $HOME/share/aclocal/cmake*
            rm -rf $HOME/doc/cmake*
            rm -f $HOME/bin/nodeos $HOME/bin/keosd $HOME/bin/cleos $HOME/bin/ctest $HOME/bin/*cmake* $HOME/bin/cpack
            rm -rf $HOME/src/mongo*
         fi

         if [ -d "/usr/local/include/eosio" ] || [[ $1 == "force-old" ]]; then
            if [ "$(id -u)" -ne 0 ]; then
               printf "\nCleanup requires sudo... Please manually run ./scripts/clean_old_install.sh with sudo.\n"
               exit -1
            fi
            pushd /usr/local &> /dev/null
            rm -rf wasm
            pushd include &> /dev/null
            rm -rf libbson-1.0 libmongoc-1.0 mongocxx bsoncxx appbase chainbase eosio.system eosiolib fc libc++ musl secp256k* 2>/dev/null
            rm -rf eosio 2>/dev/null
            popd &> /dev/null
            pushd bin &> /dev/null
            rm cleos eosio-abigen eosio-applesedemo eosio-launcher eosio-s2wasm eosio-wast2wasm eosiocpp keosd nodeos 2>/dev/null
            popd &> /dev/null
            libraries=(
               libeosio_testing
               libeosio_chain
               libfc
               libbinaryen
               libWAST
               libWASM
               libRuntime
               libPlatform
               libIR
               libLogging
               libsoftfloat
               libchainbase
               libappbase
               libbuiltins
               libbson-1.0
               libbson-static-1.0.a
               libbsoncxx-static
               libmongoc-1.0
               libmongoc-static-1.0.a
               libmongocxx-static
               libsecp256k*
            )
            pushd lib &> /dev/null
            for lib in ${libraries[@]}; do
               rm ${lib}.a ${lib}.dylib ${lib}.so 2>/dev/null
               rm pkgconfig/${lib}.pc 2>/dev/null
               rm cmake/${lib} 2>/dev/null
            done
            popd &> /dev/null
            pushd etc &> /dev/null
            rm eosio 2>/dev/null
            popd &> /dev/null
            pushd share &> /dev/null
            rm eosio 2>/dev/null
            popd &> /dev/null
            pushd usr/share &> /dev/null
            rm eosio 2>/dev/null
            popd &> /dev/null
            pushd var/lib &> /dev/null
            rm eosio 2>/dev/null
            popd &> /dev/null
            pushd var/log &> /dev/null
            rm eosio 2>/dev/null
            popd &> /dev/null
         fi
      ;;
      [Nn]* )
         printf "Skipping\n\n"
         exit 0
      ;;
   esac
fi
