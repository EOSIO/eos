export ROOT_DIR=$( dirname "${BASH_SOURCE[0]}" )/../..
export BUILD_DIR=$ROOT_DIR/build
export CICD_DIR=$ROOT_DIR/.cicd
export SCRIPTS_DIR=$ROOT_DIR/scripts
export HELPERS_DIR=$CICD_DIR/helpers
export JOBS=${JOBS:-"$(getconf _NPROCESSORS_ONLN)"}
export MOUNTED_DIR='/workdir'

# capitalize each word in a string
function capitalize()
{
    if [[ ! $1 =~ 'mac' ]]; then # Don't capitalize mac
        echo $1 | awk '{$1=toupper(substr($1,1,1))substr($1,2)}1'
    else
        echo $1
    fi
}

# load buildkite intrinsic environment variables for use in docker run
function buildkite-intrinsics()
{
    BK_ENV=''
    if [[ -f $BUILDKITE_ENV_FILE ]]; then
        while read -r var; do
            BK_ENV="$BK_ENV --env ${var%%=*}"
        done < "$BUILDKITE_ENV_FILE"
    fi
    echo "$BK_ENV"
}