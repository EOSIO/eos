#!/bin/bash
set -eo pipefail
# environment
. ./.cicd/helpers/general.sh
export PLATFORMS_JSON_ARRAY='[]'
[[ -z "$ROUNDS" ]] && export ROUNDS='1'
[[ -z "$ROUND_SIZE" ]] && export ROUND_SIZE='1'
BUILDKITE_BUILD_AGENT_QUEUE='automation-eks-eos-builder-fleet'
BUILDKITE_TEST_AGENT_QUEUE='automation-eks-eos-tester-fleet'
# attach pipeline documentation
export DOCS_URL="https://github.com/EOSIO/eos/blob/$(git rev-parse HEAD)/.cicd"
export RETRY="$([[ "$BUILDKITE" == 'true' ]] && buildkite-agent meta-data get pipeline-upload-retries --default '0' || echo "${RETRY:-0}")"
if [[ "$BUILDKITE" == 'true' && "$RETRY" == '0' ]]; then
    echo "This documentation is also available on [GitHub]($DOCS_URL/README.md)." | buildkite-agent annotate --append --style 'info' --context 'documentation'
    cat .cicd/README.md | buildkite-agent annotate --append --style 'info' --context 'documentation'
    if [[ "$BUILDKITE_PIPELINE_SLUG" == 'eosio-test-stability' ]]; then
        echo "This documentation is also available on [GitHub]($DOCS_URL/eosio-test-stability.md)." | buildkite-agent annotate --append --style 'info' --context 'test-stability'
        cat .cicd/eosio-test-stability.md | buildkite-agent annotate --append --style 'info' --context 'test-stability'
    fi
fi
[[ "$BUILDKITE" == 'true' ]] && buildkite-agent meta-data set pipeline-upload-retries "$(( $RETRY + 1 ))"
# guard against accidentally spawning too many jobs
if (( $ROUNDS > 1 || $ROUND_SIZE > 1 )) && [[ "$BUILDKITE_PIPELINE_SLUG" != 'eosio-test-stability' ]]; then
    echo '+++ :no_entry: WARNING: Your parameters will spawn a very large number of jobs!' 1>&2
    echo "Setting ROUNDS='$ROUNDS' and/or ROUND_SIZE='$ROUND_SIZE' in the environment will cause ALL tests to be run $(( $ROUNDS * $ROUND_SIZE )) times, which will consume a large number of agents!" 1>&2
    [[ "$BUILDKITE" == 'true' ]] && cat | buildkite-agent annotate --append --style 'error' --context 'no-TEST' <<-MD
Your build was cancelled because you set \`ROUNDS\` and/or \`ROUND_SIZE\` outside the [eosio-test-stability](https://buildkite.com/EOSIO/eosio-test-stability) pipeline.
MD
    exit 255
fi
# Determine if it's a forked PR and make sure to add git fetch so we don't have to git clone the forked repo's url
if [[ $BUILDKITE_BRANCH =~ ^pull/[0-9]+/head: ]]; then
    PR_ID=$(echo $BUILDKITE_BRANCH | cut -d/ -f2)
    export GIT_FETCH="git fetch -v --prune origin refs/pull/$PR_ID/head &&"
fi
# Determine which dockerfiles/scripts to use for the pipeline.
if [[ $PINNED == false ]]; then
    export PLATFORM_TYPE="unpinned"
else
    export PLATFORM_TYPE="pinned"
fi

for FILE in basename $(ls "$CICD_DIR/platforms/$PLATFORM_TYPE/ubuntu-18*"); do
    # use pinned or unpinned, not both sets of platform files
    if [[ $PINNED == false ]]; then
        export SKIP_CONTRACT_BUILDER=${SKIP_CONTRACT_BUILDER:-true}
        export SKIP_PACKAGE_BUILDER=${SKIP_PACKAGE_BUILDER:-true}
    fi
    export FILE_NAME="$(echo "$FILE" | awk '{split($0,a,/\.(d|s)/); print a[1] }')"
    export PLATFORM_NAME="$(echo $FILE_NAME | cut -d- -f1 | sed 's/os/OS/g')"
    # macOS
    # ubuntu
    export PLATFORM_NAME_UPCASE="$(echo $PLATFORM_NAME | tr a-z A-Z)"
    # MACOS
    # UBUNTU
    export VERSION_MAJOR="$(echo $FILE_NAME | cut -d- -f2 | cut -d. -f1)"
    # 10
    # 16
    [[ "$(echo $FILE_NAME | cut -d- -f2)" =~ '.' ]] && export VERSION_MINOR="_$(echo $FILE_NAME | cut -d- -f2 | cut -d. -f2)" || export VERSION_MINOR=''
    # _14
    # _04
    export VERSION_FULL="$(echo $FILE_NAME | cut -d- -f2)"
    # 10.14
    # 16.04
    OLDIFS=$IFS
    IFS='_'
    set $PLATFORM_NAME
    IFS=$OLDIFS
    export PLATFORM_NAME_FULL="$(capitalize $1)$( [[ ! -z $2 ]] && echo "_$(capitalize $2)" || true ) $VERSION_FULL"
    [[ $FILE_NAME =~ 'amazon' ]] && export ICON=':aws:'
    [[ $FILE_NAME =~ 'ubuntu' ]] && export ICON=':ubuntu:'
    [[ $FILE_NAME =~ 'centos' ]] && export ICON=':centos:'
    [[ $FILE_NAME =~ 'macos' ]] && export ICON=':darwin:'
    . "$HELPERS_DIR/file-hash.sh" "$CICD_DIR/platforms/$PLATFORM_TYPE/$FILE" # returns HASHED_IMAGE_TAG, etc
    export PLATFORM_SKIP_VAR="SKIP_${PLATFORM_NAME_UPCASE}_${VERSION_MAJOR}${VERSION_MINOR}"

    export ANKA_TAG_BASE=''
    export ANKA_TEMPLATE_NAME=''

    export PLATFORMS_JSON_ARRAY=$(echo $PLATFORMS_JSON_ARRAY | jq -c '. += [{
        "FILE_NAME": env.FILE_NAME,
        "PLATFORM_NAME": env.PLATFORM_NAME,
        "PLATFORM_SKIP_VAR": env.PLATFORM_SKIP_VAR,
        "PLATFORM_NAME_UPCASE": env.PLATFORM_NAME_UPCASE,
        "VERSION_MAJOR": env.VERSION_MAJOR,
        "VERSION_MINOR": env.VERSION_MINOR,
        "VERSION_FULL": env.VERSION_FULL,
        "PLATFORM_NAME_FULL": env.PLATFORM_NAME_FULL,
        "DOCKERHUB_FULL_TAG": env.FULL_TAG,
        "HASHED_IMAGE_TAG": env.HASHED_IMAGE_TAG,
        "ICON": env.ICON,
        "ANKA_TAG_BASE": env.ANKA_TAG_BASE,
        "ANKA_TEMPLATE_NAME": env.ANKA_TEMPLATE_NAME
        }]')
done

# set build_source whether triggered or not
if [[ ! -z ${BUILDKITE_TRIGGERED_FROM_BUILD_ID} ]]; then
    export BUILD_SOURCE="--build \$BUILDKITE_TRIGGERED_FROM_BUILD_ID"
fi
export BUILD_SOURCE=${BUILD_SOURCE:---build \$BUILDKITE_BUILD_ID}
# set trigger_job if master/release/develop branch and webhook
if [[ ! $BUILDKITE_PIPELINE_SLUG =~ 'lrt' ]] && [[ $BUILDKITE_BRANCH =~ ^release/[0-9]+\.[0-9]+\.x$ || $BUILDKITE_BRANCH =~ ^master$ || $BUILDKITE_BRANCH =~ ^develop$ || "$SKIP_LONG_RUNNING_TESTS" == 'false' ]]; then
    [[ $BUILDKITE_SOURCE != 'schedule' ]] && export TRIGGER_JOB=true
fi
# run LRTs synchronously when running full test suite
if [[ "$RUN_ALL_TESTS" == 'true' && "$SKIP_LONG_RUNNING_TESTS" != 'true' ]]; then
    export BUILD_SOURCE="--build \$BUILDKITE_BUILD_ID"
    export SKIP_LONG_RUNNING_TESTS='false'
    export TRIGGER_JOB='false'
fi
oIFS="$IFS"
IFS=$''
nIFS=$IFS # fix array splitting (\n won't work)
# start with a wait step
echo 'steps:'
echo '  - wait'
echo ''
# build steps
[[ -z "$DCMAKE_BUILD_TYPE" ]] && export DCMAKE_BUILD_TYPE='Release'
export LATEST_UBUNTU="$(echo "$PLATFORMS_JSON_ARRAY" | jq -c 'map(select(.PLATFORM_NAME == "ubuntu")) | sort_by(.VERSION_MAJOR) | .[-1]')" # isolate latest ubuntu from array
if [[ "$DEBUG" == 'true' ]]; then
    echo '# PLATFORMS_JSON_ARRAY'
    echo "# $(echo "$PLATFORMS_JSON_ARRAY" | jq -c '.')"
    echo '# LATEST_UBUNTU'
    echo "# $(echo "$LATEST_UBUNTU" | jq -c '.')"
    echo ''
fi
echo '    # builds'
echo $PLATFORMS_JSON_ARRAY | jq -cr '.[]' | while read -r PLATFORM_JSON; do
    if [[ ! "$(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)" =~ 'macos' ]]; then
        cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build"
    command: "./.cicd/build.sh"
    env:
      DCMAKE_BUILD_TYPE: $DCMAKE_BUILD_TYPE
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      PLATFORM_TYPE: $PLATFORM_TYPE
    agents:
      queue: "$BUILDKITE_BUILD_AGENT_QUEUE"
    timeout: ${TIMEOUT:-180}
    skip: $(echo "$PLATFORM_JSON" | jq -r '.PLATFORM_SKIP_VAR | env[.] // empty')${SKIP_BUILD}

EOF
    fi
done
[[ -z "$TEST" ]] && cat <<EOF
  - label: ":docker: Docker - Build and Install"
    command: "./.cicd/installation-build.sh"
    env:
      IMAGE_TAG: "ubuntu-18.04-unpinned"
      PLATFORM_TYPE: "unpinned"
    agents:
      queue: "$BUILDKITE_BUILD_AGENT_QUEUE"
    timeout: ${TIMEOUT:-180}
    skip: ${SKIP_INSTALL}${SKIP_LINUX}${SKIP_DOCKER}${SKIP_CONTRACT_BUILDER}

EOF
cat <<EOF
  - wait

EOF
# tests
IFS=$oIFS
if [[ "$DCMAKE_BUILD_TYPE" != 'Debug' ]]; then
    for ROUND in $(seq 1 $ROUNDS); do
        IFS=$''
        echo "    # round $ROUND of $ROUNDS"
        # parallel tests
        echo '    # parallel tests'
        [[ -z "$TEST" ]] && echo $PLATFORMS_JSON_ARRAY | jq -cr '.[]' | while read -r PLATFORM_JSON; do
            if [[ ! "$(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)" =~ 'macos' ]]; then
                cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Unit Tests"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "./.cicd/test.sh scripts/parallel-test.sh"
    env:
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      PLATFORM_TYPE: $PLATFORM_TYPE
    agents:
      queue: "$BUILDKITE_BUILD_AGENT_QUEUE"
    retry:
      manual:
        permit_on_passed: true
    timeout: ${TIMEOUT:-30}
    skip: $(echo "$PLATFORM_JSON" | jq -r '.PLATFORM_SKIP_VAR | env[.] // empty')${SKIP_UNIT_TESTS}

EOF
            fi
        done
        # wasm spec tests
        echo '    # wasm spec tests'
        [[ -z "$TEST" ]] && echo $PLATFORMS_JSON_ARRAY | jq -cr '.[]' | while read -r PLATFORM_JSON; do
            if [[ ! "$(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)" =~ 'macos' ]]; then
                cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - WASM Spec Tests"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "./.cicd/test.sh scripts/wasm-spec-test.sh"
    env:
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      PLATFORM_TYPE: $PLATFORM_TYPE
    agents:
      queue: "$BUILDKITE_BUILD_AGENT_QUEUE"
    retry:
      manual:
        permit_on_passed: true
    timeout: ${TIMEOUT:-30}
    skip: $(echo "$PLATFORM_JSON" | jq -r '.PLATFORM_SKIP_VAR | env[.] // empty')${SKIP_WASM_SPEC_TESTS}

EOF
            fi
        done
        # serial tests
        echo '    # serial tests'
        echo $PLATFORMS_JSON_ARRAY | jq -cr '.[]' | while read -r PLATFORM_JSON; do
            IFS=$oIFS
            if [[ -z "$TEST" ]]; then
                SERIAL_TESTS="$(cat tests/CMakeLists.txt | grep nonparallelizable_tests | grep -v "^#" | awk -F ' ' '{ print $2 }' | sort | uniq)"
            else
                SERIAL_TESTS="$(cat tests/CMakeLists.txt | grep -v "^#" | awk -F ' ' '{ print $2 }' | sort | uniq | grep -P "^$TEST$" | awk "{while(i++<$ROUND_SIZE)print;i=0}")"
            fi
            for TEST_NAME in $SERIAL_TESTS; do
                if [[ ! "$(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)" =~ 'macos' ]]; then
                    cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - $TEST_NAME"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "./.cicd/test.sh scripts/serial-test.sh $TEST_NAME"
    env:
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      PLATFORM_TYPE: $PLATFORM_TYPE
    agents:
      queue: "$BUILDKITE_TEST_AGENT_QUEUE"
    retry:
      manual:
        permit_on_passed: true
    timeout: ${TIMEOUT:-20}
    skip: $(echo "$PLATFORM_JSON" | jq -r '.PLATFORM_SKIP_VAR | env[.] // empty')${SKIP_SERIAL_TESTS}

EOF
            fi
        done
            IFS=$nIFS
        done
        # long-running tests
        echo '    # long-running tests'
        [[ -z "$TEST" ]] && echo $PLATFORMS_JSON_ARRAY | jq -cr '.[]' | while read -r PLATFORM_JSON; do
            IFS=$oIFS
            LR_TESTS="$(cat tests/CMakeLists.txt | grep long_running_tests | grep -v "^#" | awk -F" " '{ print $2 }' | sort | uniq)"
            for TEST_NAME in $LR_TESTS; do
                if [[ ! "$(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)" =~ 'macos' ]]; then
                    cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - $TEST_NAME"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' ${BUILD_SOURCE} && tar -xzf build.tar.gz"
      - "./.cicd/test.sh scripts/long-running-test.sh $TEST_NAME"
    env:
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      PLATFORM_TYPE: $PLATFORM_TYPE
    agents:
      queue: "$BUILDKITE_TEST_AGENT_QUEUE"
    retry:
      manual:
        permit_on_passed: true
    timeout: ${TIMEOUT:-180}
    skip: $(echo "$PLATFORM_JSON" | jq -r '.PLATFORM_SKIP_VAR | env[.] // empty')${SKIP_LONG_RUNNING_TESTS:-true}

EOF
                fi
            done
            IFS=$nIFS
        done
        IFS=$oIFS        

