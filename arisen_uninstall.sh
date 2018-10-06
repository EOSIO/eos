#! /bin/bash

binaries=(arisecli
          arisen-abigen
          arisen-launcher
          arisen-s2wasm
          arisen-wast2wasm
          arisencpp
          awallet
          aos
          arisen-applesdemo)

if [ -d "/usr/local/arisen" ]; then
   printf "\tDo you wish to remove this install? (requires sudo)\n"
   select yn in "Yes" "No"; do
      case $yn in
         [Yy]* )
            if [ "$(id -u)" -ne 0 ]; then
               printf "\n\tThis requires sudo, please run ./arisen_uninstall.sh with sudo\n\n"
               exit -1
            fi

            pushd /usr/local &> /dev/null
            rm -rf arisen
            pushd bin &> /dev/null
            for binary in ${binaries[@]}; do
               rm ${binary}
            done
            # Handle cleanup of directories created from installation
            if [ "$1" == "--full" ]; then
               if [ -d ~/Library/Application\ Support/arisen ]; then rm -rf ~/Library/Application\ Support/arisen; fi # Mac OS
               if [ -d ~/.local/share/arisen ]; then rm -rf ~/.local/share/arisen; fi # Linux
            fi
            popd &> /dev/null
            break;;
         [Nn]* )
            printf "\tAborting uninstall\n\n"
            exit -1;;
      esac
   done
fi
