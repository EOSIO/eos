#! /bin/bash

txtbld=$(tput bold)
bldred=${txtbld}$(tput setaf 1)
txtrst=$(tput sgr0)

binaries=(cleos
          eosio-abigen
          eosio-launcher
          eosio-s2wasm
          eosio-wast2wasm
          eosiocpp
          keosd
          nodeos
          eosio-applesdemo)

if [ -d "/usr/local/eosio" ]; then
   printf "\tDo you wish to remove this install? (requires sudo)\n"
   select yn in "Yes" "No"; do
      case $yn in
         [Yy]* )
            if [ "$(id -u)" -ne 0 ]; then
               printf "\n\tThis requires sudo, please run ./eosio_uninstall.sh with sudo\n\n"
               exit -1
            fi

            pushd /usr/local &> /dev/null
            rm -rf eosio
            pushd bin &> /dev/null
            for binary in ${binaries[@]}; do
               if [ -f "${binary}" ] || [ -L "${binary}" ]; then
                  printf "\t${txtbld}${binary}${txtrst} found, ${bldred}removing...${txtrst}\n"
                  rm ${binary}
               else
                  printf "\t${txtbld}${binary}${txtrst} not found, skipping...\n"
               fi
            done
            # Handle cleanup of directories created from installation
            if [ "$1" == "--full" ]; then
               if [ -d ~/Library/Application\ Support/eosio ]; then rm -rf ~/Library/Application\ Support/eosio; fi # Mac OS
               if [ -d ~/.local/share/eosio ]; then rm -rf ~/.local/share/eosio; fi # Linux
            fi
            popd &> /dev/null
            break;;
         [Nn]* )
            printf "\tAborting uninstall\n\n"
            exit -1;;
      esac
   done
fi

printf "\nEOSIO is successfully removed\n"
