#!/bin/bash
set -eo pipefail
# environment
. ./.cicd/helpers/general.sh
export MOJAVE_ANKA_TAG_BASE=${MOJAVE_ANKA_TAG_BASE:-'clean::cicd::git-ssh::nas::brew::buildkite-agent'}
export MOJAVE_ANKA_TEMPLATE_NAME=${MOJAVE_ANKA_TEMPLATE_NAME:-'10.14.4_6C_14G_40G'}
export PLATFORMS_JSON_ARRAY='[]'
[[ -z "$ROUNDS" ]] && export ROUNDS='1'
LINUX_CONCURRENCY='8'
MAC_CONCURRENCY='2'
LINUX_CONCURRENCY_GROUP='eos-scheduled-build'
MAC_CONCURRENCY_GROUP='eos-scheduled-build-mac'

# Determine if it's a forked PR and make sure to add git fetch so we don't have to git clone the forked repo's url
if [[ $BUILDKITE_BRANCH =~ ^pull/[0-9]+/head: ]]; then
  PR_ID=$(echo $BUILDKITE_BRANCH | cut -d/ -f2)
  export GIT_FETCH="git fetch -v --prune origin refs/pull/$PR_ID/head &&"
fi

for FILE in $(ls $CICD_DIR/platforms); do
    # skip mac or linux by not even creating the json block
    ( [[ $SKIP_MAC == true ]] && [[ $FILE =~ 'macos' ]] ) && continue
    ( [[ $SKIP_LINUX == true ]] && [[ ! $FILE =~ 'macos' ]] ) && continue
    # use pinned or unpinned, not both sets of platform files
    if [[ $PINNED == false || $UNPINNED == true ]] && [[ ! $FILE =~ 'macos' ]]; then
        export SKIP_CONTRACT_BUILDER=${SKIP_CONTRACT_BUILDER:-true}
        export SKIP_PACKAGE_BUILDER=${SKIP_PACKAGE_BUILDER:-true}
        [[ ! $FILE =~ 'unpinned' ]] && continue
    else
        [[ $FILE =~ 'unpinned' ]] && continue
    fi
    export FILE_NAME="$(echo $FILE | awk '{split($0,a,/\.(d|s)/); print a[1] }')"
    export PLATFORM_NAME="$(echo $FILE_NAME | cut -d- -f1 | sed 's/os/OS/g')"
    export PLATFORM_NAME_UPCASE="$(echo $PLATFORM_NAME | tr a-z A-Z)"
    export VERSION_MAJOR="$(echo $FILE_NAME | cut -d- -f2 | cut -d. -f1)"
    [[ "$(echo $FILE_NAME | cut -d- -f2)" =~ '.' ]] && export VERSION_MINOR="_$(echo $FILE_NAME | cut -d- -f2 | cut -d. -f2)" || export VERSION_MINOR=''
    export VERSION_FULL="$(echo $FILE_NAME | cut -d- -f2)"
    OLDIFS=$IFS
    IFS='_'
    set $PLATFORM_NAME
    IFS=$OLDIFS
    export PLATFORM_NAME_FULL="$(capitalize $1)$( [[ ! -z $2 ]] && echo "_$(capitalize $2)" || true ) $VERSION_FULL"
    [[ $FILE_NAME =~ 'amazon' ]] && export ICON=':aws:'
    [[ $FILE_NAME =~ 'ubuntu' ]] && export ICON=':ubuntu:'
    [[ $FILE_NAME =~ 'centos' ]] && export ICON=':centos:'
    [[ $FILE_NAME =~ 'macos' ]] && export ICON=':darwin:'
    . $HELPERS_DIR/file-hash.sh $CICD_DIR/platforms/$FILE # returns HASHED_IMAGE_TAG, etc
    export PLATFORMS_JSON_ARRAY=$(echo $PLATFORMS_JSON_ARRAY | jq -c '. += [{ 
        "FILE_NAME": env.FILE_NAME, 
        "PLATFORM_NAME": env.PLATFORM_NAME,
        "PLATFORM_NAME_UPCASE": env.PLATFORM_NAME_UPCASE,
        "VERSION_MAJOR": env.VERSION_MAJOR,
        "VERSION_MINOR": env.VERSION_MINOR,
        "VERSION_FULL": env.VERSION_FULL,
        "PLATFORM_NAME_FULL": env.PLATFORM_NAME_FULL,
        "DOCKERHUB_FULL_TAG": env.FULL_TAG,
        "HASHED_IMAGE_TAG": env.HASHED_IMAGE_TAG,
        "ICON": env.ICON
        }]')
done
# set build_source whether triggered or not
if [[ ! -z ${BUILDKITE_TRIGGERED_FROM_BUILD_ID} ]]; then
    export BUILD_SOURCE="--build \$BUILDKITE_TRIGGERED_FROM_BUILD_ID"
fi
export BUILD_SOURCE=${BUILD_SOURCE:---build \$BUILDKITE_BUILD_ID}
# set trigger_job if master/release/develop branch and webhook
if [[ $BUILDKITE_BRANCH =~ ^release/[0-9]+\.[0-9]+\.x$ || $BUILDKITE_BRANCH =~ ^master$ || $BUILDKITE_BRANCH =~ ^develop$ ]]; then
    [[ $BUILDKITE_SOURCE == 'webhook' ]] && export TRIGGER_JOB=true
fi
oIFS="$IFS"
IFS=$''
nIFS=$IFS # fix array splitting (\n won't work)
# start with a wait step
echo '  - wait'
echo ''

# pipeline tail
cat <<EOF
  
  - label: ":git: Git Submodule Regression Check"
    command: "./.cicd/submodule-regression-check.sh"
    agents:
      queue: "automation-basic-builder-fleet"
    timeout: ${TIMEOUT:-5}

EOF
IFS=$oIFS
