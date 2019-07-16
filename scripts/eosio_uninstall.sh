#!/usr/bin/env bash
set -eo pipefail
VERSION=2.1
# User input handling
DEP_PROCEED=false
FULL=false

function usage() {
   printf "Usage: $0 OPTION...
  -i DIR      Directory where eosio is installed)
  -y          Noninteractive mode (answers yes to every prompt)
  -f          Removal of data directory (be absolutely sure you want to delete it before using this!)
   \\n" "$0" 1>&2
   exit 1
}

TIME_BEGIN=$( date -u +%s )
if [ $# -ne 0 ]; then
   while getopts "i:yf" opt; do
      case "${opt}" in
         i )
            INSTALL_LOCATION=$OPTARG
         ;;
         y )
            NONINTERACTIVE=true
            PROCEED=true
            DEP_PROCEED=true
         ;;
         f )
            FULL=true
         ;;
         h )
            usage
         ;;
         ? )
            echo "Invalid Option!" 1>&2
            usage
         ;;
         : )
            echo "Invalid Option: -${OPTARG} requires an argument." 1>&2
            usage
         ;;
         * )
            usage
         ;;
      esac
   done
fi

export CURRENT_WORKING_DIR=$(pwd) # relative path support

# Ensure we're in the repo root and not inside of scripts
cd $( dirname "${BASH_SOURCE[0]}" )/..

# Load bash script helper functions
. ./scripts/helpers/eosio.sh

# Support relative paths : https://github.com/EOSIO/eos/issues/7560
( [[ ! -z $INSTALL_LOCATION ]] && [[ ! $INSTALL_LOCATION =~ ^\/ ]] ) && export INSTALL_LOCATION="${CURRENT_WORKING_DIR}/$INSTALL_LOCATION"

INSTALL_PATHS=()

# -y alone should not remove the data directories and not prompt the user for anything. 
# -f alone should remove the data directories after prompting the user if they are sure. 
# -f -y should proceed forward with removing the data directories without prompting the user.
if [[ $NONINTERACTIVE == false ]] && $FULL; then
   while true; do
      read -p "By specifying -f, removal of the eosio data directory will require a resync of data which can take days. Do you wish to proceed? (y/n) " PROCEED
      case $PROCEED in
         "" ) echo "What would you like to do?";;
         0 | true | [Yy]* ) break;;
         1 | false | [Nn]* ) exit 0;;
         * ) echo "Please type 'y' for yes or 'n' for no.";;
      esac
   done
fi

export EOSIO_INSTALL_DIR=${INSTALL_LOCATION:-$EOSIO_INSTALL_DIR}

if [[ ! -d "${EOSIO_INSTALL_DIR}" ]]; then
   echo "[EOSIO installation ${COLOR_YELLOW}NOT${COLOR_NC} found in ${EOSIO_INSTALL_DIR}]"
else
   # As of 1.8.0, we're using a versioned directories under home: https://github.com/EOSIO/eos/issues/6940
   echo "[EOSIO installation found: ${EOSIO_INSTALL_DIR}]" && INSTALL_PATHS+=("${EOSIO_INSTALL_DIR}") # EOSIO_INSTALL_DIR set in .environment
   while true; do
      [[ $NONINTERACTIVE == false ]] && read -p "Do you wish to remove the installation in ${EOSIO_INSTALL_DIR}? (y/n) " PROCEED
      case $PROCEED in
         "" ) echo "What would you like to do?";;
         0 | true | [Yy]* )
            # Handle cleanup of data directory
            if $FULL; then
               ## Add both just to be safe
               [[ $ARCH == "Darwin" ]] && INSTALL_PATHS+=("${HOME}/Library/Application\ Support/eosio")
               [[ $ARCH != "Darwin" ]] && INSTALL_PATHS+=("${HOME}/.local/share/eosio")
            fi
            # Version < 1.8.0; Before we started using ~/eosio/1.8.x
            # Arrays should return with newlines (IFS=\n;helpers.sh) as Application\ Support will split into two
            for INSTALL_PATH in ${INSTALL_PATHS[@]}; do
               execute rm -rf $INSTALL_PATH
            done
            echo " - EOSIO Removal Complete"
            break;;
         1 | false | [Nn]* ) echo " - Cancelled EOSIO Removal!"; exit 1;;
         * ) echo "Please type 'y' for yes or 'n' for no.";;
      esac
   done
fi

echo "[Removing EOSIO Dependencies]"
if [[ $ARCH == "Darwin" ]]; then
   for package in $(cat scripts/eosio_build_darwin_deps | cut -d, -f1 2>/dev/null); do
      while true; do
         [[ $NONINTERACTIVE == false ]] && read -p "Do you wish to uninstall and unlink all brew installed ${package} versions? (y/n) " DEP_PROCEED
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
