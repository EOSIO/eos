export ROOT_DIR=$( dirname "${BASH_SOURCE[0]}" )/../..
export BUILD_DIR=$ROOT_DIR/build
export CICD_DIR=$ROOT_DIR/.cicd
export HELPERS_DIR=$CICD_DIR/helpers
export JOBS=${JOBS:-"$(getconf _NPROCESSORS_ONLN)"}

. $HELPERS_DIR/logging.sh

function determine_eos_version() {
    export EOSIO_VERSION_MAJOR=$(cat ./CMakeLists.txt | grep -E "^[[:blank:]]*set[[:blank:]]*\([[:blank:]]*VERSION_MAJOR" | tail -1 | sed 's/.*VERSION_MAJOR //g' | sed 's/ //g' | sed 's/"//g' | cut -d\) -f1)
    export EOSIO_VERSION_MINOR=$(cat ./CMakeLists.txt | grep -E "^[[:blank:]]*set[[:blank:]]*\([[:blank:]]*VERSION_MINOR" | tail -1 | sed 's/.*VERSION_MINOR //g' | sed 's/ //g' | sed 's/"//g' | cut -d\) -f1)
    export EOSIO_VERSION_PATCH=$(cat ./CMakeLists.txt | grep -E "^[[:blank:]]*set[[:blank:]]*\([[:blank:]]*VERSION_PATCH" | tail -1 | sed 's/.*VERSION_PATCH //g' | sed 's/ //g' | sed 's/"//g' | cut -d\) -f1)
    export EOSIO_VERSION_SUFFIX=$(cat ./CMakeLists.txt | grep -E "^[[:blank:]]*set[[:blank:]]*\([[:blank:]]*VERSION_SUFFIX" | tail -1 | sed 's/.*VERSION_SUFFIX //g' | sed 's/ //g' | sed 's/"//g' | cut -d\) -f1)
}
