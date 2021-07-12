export ROOT_DIR=$( dirname "${BASH_SOURCE[0]}" )/../..
export BUILD_DIR="$ROOT_DIR/build"
export CICD_DIR="$ROOT_DIR/.cicd"
export HELPERS_DIR="$CICD_DIR/helpers"
export JOBS=${JOBS:-"$(getconf _NPROCESSORS_ONLN)"}
export MOUNTED_DIR='/eos'
export DOCKER_CLI_EXPERIMENTAL='enabled'
export DOCKERHUB_CI_REGISTRY="docker.io/eosio/ci"
export DOCKERHUB_CONTRACTS_REGISTRY="docker.io/eosio/ci-contracts-builder"
if [[ "$BUILDKITE_PIPELINE_SLUG" =~ "-sec" ]] ; then
    export CI_REGISTRIES=("$MIRROR_REGISTRY")
    export CONTRACT_REGISTRIES=("$MIRROR_REGISTRY")
else
    export CI_REGISTRIES=("$DOCKERHUB_CI_REGISTRY" "$MIRROR_REGISTRY")
    export CONTRACT_REGISTRIES=("$DOCKERHUB_CONTRACTS_REGISTRY" "$MIRROR_REGISTRY")
fi

# capitalize each word in a string
function capitalize()
{
    if [[ ! "$1" =~ 'mac' ]]; then # Don't capitalize mac
        echo "$1" | awk '{$1=toupper(substr($1,1,1))substr($1,2)}1'
    else
        echo "$1"
    fi
}

# load buildkite intrinsic environment variables for use in docker run
function buildkite-intrinsics()
{
    BK_ENV=''
    if [[ -f "$BUILDKITE_ENV_FILE" ]]; then
        while read -r var; do
            BK_ENV="$BK_ENV --env ${var%%=*}"
        done < "$BUILDKITE_ENV_FILE"
    fi
    echo "$BK_ENV"
}

            ##### sanitize branch names for use in URIs (docker containers) #####
# tr '/' '_'                       # convert forward-slashes '/' to underscores '_'
# sed -E 's/[^-_.a-zA-Z0-9]+/-/g'  # convert invalid docker chars to '-'
# sed -E 's/-+/-/g'                # replace multiple dashes in a series "----" with a single dash '-'
# sed -E 's/-*_+-*/_/g'            # replace dashes '-' and underscores '_' in-series with a single underscore '_'
# sed -E 's/_+/_/g'                # replace multiple underscores in a row "___" with a single underscore '_'
# sed -E 's/(^[-_.]+|[-_.]+$)//g'  # ensure tags do not begin or end with separator characters [-_.]
function sanitize()
{
    echo "$1" | tr '/' '_' | sed -E 's/[^-_.a-zA-Z0-9]+/-/g' | sed -E 's/-+/-/g' | sed -E 's/-*_+-*/_/g' | sed -E 's/_+/_/g' | sed -E 's/(^[-_.]+|[-_.]+$)//g'
}
