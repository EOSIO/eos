export ROOT_DIR=$( dirname "${BASH_SOURCE[0]}" )/../..
export BUILD_DIR=$ROOT_DIR/build
export CICD_DIR=$ROOT_DIR/.cicd
export HELPERS_DIR=$CICD_DIR/helpers
export JOBS=${JOBS:-"$(getconf _NPROCESSORS_ONLN)"}
export MOUNTED_DIR='/workdir'

# Obtain Repo URL (and support forks)
if [[ ! -z $BUILDKITE_PULL_REQUEST_REPO ]] && [[ ! $BUILDKITE_PULL_REQUEST_REPO =~ 'git://github.com/EOSIO' ]]; then ## Manually run builds have an empty PULL_REQUEST var + Check if it's a forked repo url
    REPO_URL_END=$(echo $BUILDKITE_PULL_REQUEST_REPO | awk -F'github.com' '{print $2}')
    export BUILDKITE_HTTPS_REPO_URL="https://github.com$(echo $BUILDKITE_PULL_REQUEST_REPO | awk -F'github.com' '{print $2}')"
elif [[ -z $BUILDKITE_HTTPS_REPO_URL ]]; then
    if [[ $BUILDKITE_REPO =~ github.com: ]]; then ## Support BUILDKITE_REPO not having github.com: and being the https url already (unpinned PRs): https://buildkite.com/EOSIO/eosio-build-unpinned/builds/331
        REPO_URL_END=$(echo $BUILDKITE_REPO | awk -F'github.com:' '{print $2}')
        export BUILDKITE_HTTPS_REPO_URL="https://${HTTPS_AUTH_TOKEN}github.com/${REPO_URL_END}"
    else
        REPO_URL_END=$(echo $BUILDKITE_REPO | awk -F'github.com/' '{print $2}')
        export BUILDKITE_HTTPS_REPO_URL="https://${HTTPS_AUTH_TOKEN}github.com/${REPO_URL_END}"
    fi
fi

function capitalize() {
    if [[ ! $1 =~ 'mac' ]]; then # Don't capitalize mac
        echo $1 | awk '{$1=toupper(substr($1,1,1))substr($1,2)}1'
    else
        echo $1
    fi
}