#!/usr/bin/env bash
set -eo pipefail

# Load bash script helper functions
. ./scripts/helpers/eosio.sh

usage() {
   printf "Usage --- \\n $ %s [ --full ] [ --force ]\\n
     --full: Removal of data directory (be absolutely sure you want to delete it before using this!)\\n
     --force: Unattended uninstall which works regardless of the eosio directory existence.\\n              This helps cleanup dependencies and start fresh if you need to.
   \\n" "$0"
}

 INSTALL_PATHS=(
   $HOME/bin/eosio-launcher
   $HOME/lib/cmake/eosios
   $HOME/opt/llvm*
   $HOME/opt/boost
   $HOME/src/boost_*
   $HOME/src/cmake-*
   $HOME/share/cmake-*
   $HOME/share/aclocal/cmake*
   $HOME/doc/cmake*
   $HOME/bin/nodeos 
   $HOME/bin/keosd 
   $HOME/bin/cleos 
   $HOME/bin/ctest 
   $HOME/bin/*cmake* 
   $HOME/bin/cpack
   $HOME/src/mongo*
   $HOME/opt/mongo*
   $HOME/bin/mongo*
   $HOME/lib/cmake
   $HOME/lib/libbson*
   $HOME/lib/libmongo*
   $HOME/lib/pkgconfig
   $HOME/include/bsoncxx*
   $HOME/include/libbson*
   $HOME/include/libmongo*
   $HOME/include/mongocxx*
   $HOME/share/mongo*
)

# User input handling
PROCEED=false
DEP_PROCEED=false
FORCED=false
FULL=false
if [[ $@ =~ [[:space:]]?--force[[:space:]]? ]]; then
   echo "[Forcing Unattended Removal: Enabled]"
   FORCED=true
   PROCEED=true
   DEP_PROCEED=true
fi
if [[ $@ =~ [[:space:]]?--full[[:space:]]? ]]; then
   echo "[Full removal (nodeos generated state, etc): Enabled]"
   if $FORCED; then
      FULL=true
   elif [[ $FORCED == false ]]; then
      while true; do
         read -p "Removal of the eosio data directory will require a resync of data which can take days. Do you wish to proceed? (y/n) " PROCEED
         case $PROCEED in
            "" ) echo "What would you like to do?";;
            0 | true | [Yy]* )
               FULL=true
            break;;
            1 | false | [Nn]* ) break;;
            * ) echo "Please type 'y' for yes or 'n' for no.";;
         esac
      done
   fi
fi
if [[ ! -z $@ ]] && [[ ! $@ =~ [[:space:]]?--force[[:space:]]? ]] && [[ ! $@ =~ [[:space:]]?--full[[:space:]]? ]]; then usage && exit; fi

# If eosio folder exist, add it to the INSTALL_PATHS for deletion
[[ -d "${HOME}/opt/eosio" ]] && INSTALL_PATHS+=("$HOME/opt/eosio")
# As of 1.8.0, we're using a versioned directories under home: https://github.com/EOSIO/eos/issues/6940
[[ -d "${EOSIO_INSTALL_DIR}" ]] && INSTALL_PATHS+=("${EOSIO_INSTALL_DIR}") # EOSIO_INSTALL_DIR set in .environment

# Removal
[[ ! -z "${EOSIO_LOCATION}" ]] && printf "[EOSIO Installation Found: ${EOSIO_LOCATION}]\n"
while true; do
   [[ $FORCED == false ]] && read -p "Do you wish to remove the installation? (y/n) " PROCEED
   case $PROCEED in
      "" ) echo "What would you like to do?";;
      0 | true | [Yy]* )
         echo "[Removing EOSIO and Dependencies]"
         if [[ $ARCH == "Darwin" ]]; then
            for package in $(cat scripts/eosio_build_darwin_deps | cut -d, -f1 2>/dev/null); do
               while true; do
                  [[ $FORCED == false ]] && read -p "Do you wish to uninstall and unlink all brew installed ${package} versions? (y/n) " DEP_PROCEED
                  case $DEP_PROCEED in
                     "") echo "What would you like to do?";;
                     0 | true | [Yy]* )
                        execute brew uninstall $package --force || true
                        execute brew cleanup -s $package || true
                        break;;
                     1 | false | [Nn]* ) break;;
                     * ) echo "Please type 'y' for yes or 'n' for no.";;
                  esac
               done
            done
         fi
         # Handle cleanup of data directory
         if $FULL; then
            ## Add both just to be safe
            INSTALL_PATHS+=("${HOME}/Library/Application\ Support/eosio")
            INSTALL_PATHS+=("${HOME}/.local/share/eosio")
         fi
         # Version < 1.8.0; Before we started using ~/eosio/1.8.x
         # Arrays should return with newlines (IFS=\n;helpers.sh) as Application\ Support will split into two
         for INSTALL_PATH in ${INSTALL_PATHS[@]}; do
            execute rm -rf $INSTALL_PATH
         done
         # Cleanup directories if they're empty (else the user had the directory there already for a reason)
         execute rmdir $HOME/src 2>/dev/null
         execute rmdir $HOME/opt 2>/dev/null
         execute rmdir $HOME/var/log/mongodb 2>/dev/null
         execute rmdir $HOME/var/log 2>/dev/null
         execute rmdir $HOME/var 2>/dev/null
         execute rmdir $HOME/bin 2>/dev/null
         execute rmdir $HOME/etc 2>/dev/null
         execute rmdir $HOME/lib 2>/dev/null
         execute rmdir $HOME/data/mongodb 2>/dev/null
         execute rmdir $HOME/data 2>/dev/null
         echo "[EOSIO Removal Complete]"
         break;;
      1 | false | [Nn]* ) echo " - Cancelled EOSIO Removal!"; exit 1;;
      * ) echo "Please type 'y' for yes or 'n' for no.";;
   esac
done