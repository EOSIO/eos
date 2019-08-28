export ROOT_DIR=$( dirname "${BASH_SOURCE[0]}" )/../..
export BUILD_DIR=$ROOT_DIR/build
export CICD_DIR=$ROOT_DIR/.cicd
export HELPERS_DIR=$CICD_DIR/helpers
export JOBS=${JOBS:-"$(getconf _NPROCESSORS_ONLN)"}
export MOUNTED_DIR='/workdir'

export MOJAVE_ANKA_TAG_BASE='clean::cicd::git-ssh::nas::brew::buildkite-agent'
export MOJAVE_ANKA_TEMPLATE_NAME='10.14.4_6C_14G_40G'

function capitalize() {
    if [[ ! $1 =~ 'mac' ]]; then # Don't capitalize mac
        echo $1 | awk '{$1=toupper(substr($1,1,1))substr($1,2)}1'
    else
        echo $1
    fi
}