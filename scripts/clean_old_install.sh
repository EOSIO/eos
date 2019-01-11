#! /bin/bash

OPT_LOCATION=${HOME}/opt

# ./build check for local cleanup while refactoring build scripts
if [ -d "/usr/local/include/eosio" ] || [ -d "$OPT_LOCATION/eosio" ] || [ -d "./build" ]; then
   printf "\n\tOld eosio install needs to be removed.\n\n"
   printf "\tDo you wish to remove this install? (requires sudo)\n"
   select yn in "Yes" "No"; do
      case $yn in
         [Yy]* )
            if [ "$(id -u)" -ne 0 ]; then
               printf "\n\tThis requires sudo, please run ./scripts/clean_old_install.sh with sudo\n\n"
               exit -1
            fi
            pushd /usr/local &> /dev/null

            rm -rf wasm

            if [ $( uname ) == "Darwin" ]; then
               brew uninstall mongo-c-driver --force
               brew uninstall mongo-cxx-driver --force
               brew uninstall llvm@4 --force
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
            printf "\tAborting uninstall\n\n"
            exit -1;;
      esac
   done
fi
