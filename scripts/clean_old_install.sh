#! /bin/bash

OPT_LOCATION=${HOME}/opt

# ./build check for local cleanup while refactoring build scripts
if [ -d "./build" ]; then
   printf "\nEOSIO installation already found...\n"
   printf "Do you wish to remove this install?\n"
   select yn in "Yes" "No"; do
      case $yn in
         [Yy]* )
            if [ $( uname ) == "Darwin" ]; then
               brew uninstall mongo-cxx-driver mongo-c-driver llvm@4 libidn2 gettext --force
            fi
            rm -rf ./build
         break;;
         [Nn]* )
            printf "Skipping\n\n"
            exit 0
      esac
   done
fi
if [ -d "/usr/local/include/eosio" ] || [ $1 == "force" ]; then # use force for running the script directly
   printf "\nEOSIO installation already found...\n"
   printf "Do you wish to remove this install? (requires sudo)\n"
   select yn in "Yes" "No"; do
      case $yn in
         [Yy]* )
            if [ "$(id -u)" -ne 0 ]; then
               printf "\nThis requires sudo, please run ./scripts/clean_old_install.sh with sudo.\n"
               exit -1
            fi
            exit
            pushd /usr/local &> /dev/null

            rm -rf wasm

            if [ $( uname ) == "Darwin" ]; then
               brew uninstall mongo-cxx-driver mongo-c-driver llvm@4 gettext automake libtool gmp wget --force
            fi

            pushd include &> /dev/null
            rm -rf libbson-1.0 libmongoc-1.0 mongocxx bsoncxx appbase chainbase eosio eosio.system eosiolib fc libc++ musl secp256k* 2>/dev/null
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

            popd &> /dev/null
            break;;
         [Nn]* )
            printf "Skipping\n\n"
            exit 0
      esac
   done
fi
