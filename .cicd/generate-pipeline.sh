#!/bin/bash
set -eo pipefail
# environment
. ./.cicd/helpers/general.sh
export PLATFORMS_JSON_ARRAY='[]'
[[ -z "$ROUNDS" ]] && export ROUNDS='1'
BUILDKITE_BUILD_AGENT_QUEUE='automation-eks-eos-builder-fleet'
BUILDKITE_TEST_AGENT_QUEUE='automation-eks-eos-tester-fleet'

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
for FILE in $(ls "$CICD_DIR/platforms/$PLATFORM_TYPE"); do
    # skip mac or linux by not even creating the json block
    ( [[ $SKIP_MAC == true ]] && [[ $FILE =~ 'macos' ]] ) && continue
    ( [[ $SKIP_LINUX == true ]] && [[ ! $FILE =~ 'macos' ]] ) && continue
    # use pinned or unpinned, not both sets of platform files
    if [[ $PINNED == false ]]; then
        export SKIP_CONTRACT_BUILDER=${SKIP_CONTRACT_BUILDER:-true}
        export SKIP_PACKAGE_BUILDER=${SKIP_PACKAGE_BUILDER:-true}
    fi
    export FILE_NAME="$(echo "$FILE" | awk '{split($0,a,/\.(d|s)/); print a[1] }')"
    # macos-10.14
    # ubuntu-16.04
    # skip Mojave if it's anything but the post-merge build
    if [[ "$FILE_NAME" =~ 'macos-10.14' && "$SKIP_MACOS_10_14" != 'false' && "$RUN_ALL_TESTS" != 'true' && ( "$BUILDKITE_SOURCE" != 'webhook' || "$BUILDKITE_PULL_REQUEST" != 'false' || ! "$BUILDKITE_MESSAGE" =~ 'Merge pull request' ) ]]; then
        export SKIP_MACOS_10_14='true'
        continue
    fi
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
    # Anka Template and Tags
    export ANKA_TAG_BASE='clean::cicd::git-ssh::nas::brew::buildkite-agent'
    if [[ $FILE_NAME =~ 'macos-10.14' ]]; then
      export ANKA_TEMPLATE_NAME='10.14.6_6C_14G_80G'
    elif [[ $FILE_NAME =~ 'macos-10.15' ]]; then
      export ANKA_TEMPLATE_NAME='10.15.5_6C_14G_80G'
    else # Linux
      export ANKA_TAG_BASE=''
      export ANKA_TEMPLATE_NAME=''
    fi
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
echo '  - wait'
echo ''
# build steps
echo '    # builds'
echo $PLATFORMS_JSON_ARRAY | jq -cr '.[]' | while read -r PLATFORM_JSON; do
    if [[ ! "$(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)" =~ 'macos' ]]; then
        cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build"
    command: "./.cicd/build.sh"
    env:
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      PLATFORM_TYPE: $PLATFORM_TYPE
    agents:
      queue: "$BUILDKITE_BUILD_AGENT_QUEUE"
    timeout: ${TIMEOUT:-180}
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}${SKIP_BUILD}

EOF
    else
        cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && $GIT_FETCH git checkout -f \$BUILDKITE_COMMIT && git submodule update --init --recursive"
      - "cd eos && ./.cicd/build.sh"
    plugins:
      - EOSIO/anka#v0.6.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TEMPLATE_NAME)
          vm-registry-tag: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TAG_BASE)::$(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)
          modify-cpu: 12
          modify-ram: 24
          always-pull: true
          debug: true
          wait-network: true
          failover-registries:
            - 'registry_1'
            - 'registry_2'
          pre-commands: 
            - "rm -rf mac-anka-fleet; git clone git@github.com:EOSIO/mac-anka-fleet.git && cd mac-anka-fleet && . ./ensure-tag.bash -u 12 -r 25G -a '-n'"
      - EOSIO/skip-checkout#v0.1.1:
          cd: ~
    env:
      REPO: ${BUILDKITE_PULL_REQUEST_REPO:-$BUILDKITE_REPO}
      REPO_COMMIT: $BUILDKITE_COMMIT
      TEMPLATE: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TEMPLATE_NAME)
      TEMPLATE_TAG: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TAG_BASE)
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      PLATFORM_TYPE: $PLATFORM_TYPE
      TAG_COMMANDS: "git clone ${BUILDKITE_PULL_REQUEST_REPO:-$BUILDKITE_REPO} eos && cd eos && $GIT_FETCH git checkout -f \$BUILDKITE_COMMIT && git submodule update --init --recursive && export IMAGE_TAG=$(echo "$PLATFORM_JSON" | jq -r .FILE_NAME) && export PLATFORM_TYPE=$PLATFORM_TYPE && . ./.cicd/platforms/$PLATFORM_TYPE/$(echo "$PLATFORM_JSON" | jq -r .FILE_NAME).sh && cd ~/eos && cd .. && rm -rf eos"
      PROJECT_TAG: $(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)
    timeout: ${TIMEOUT:-180}
    agents: "queue=mac-anka-large-node-fleet"
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}${SKIP_BUILD}
EOF
    fi
done
cat <<EOF

  - label: ":docker: Docker - Build and Install"
    command: "./.cicd/installation-build.sh"
    env:
      IMAGE_TAG: "ubuntu-18.04-unpinned"
      PLATFORM_TYPE: "unpinned"
    agents:
      queue: "$BUILDKITE_BUILD_AGENT_QUEUE"
    timeout: ${TIMEOUT:-180}
    skip: ${SKIP_INSTALL}${SKIP_LINUX}${SKIP_DOCKER}

  - wait

EOF
# tests
IFS=$oIFS
for ROUND in $(seq 1 $ROUNDS); do
    IFS=$''
    echo "    # round $ROUND of $ROUNDS"
    # parallel tests
    echo '    # parallel tests'
    echo $PLATFORMS_JSON_ARRAY | jq -cr '.[]' | while read -r PLATFORM_JSON; do
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
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}${SKIP_UNIT_TESTS}

EOF
        else
            cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Unit Tests"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && $GIT_FETCH git checkout -f \$BUILDKITE_COMMIT && git submodule update --init --recursive"
      - "cd eos && buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "cd eos && ./.cicd/test.sh scripts/parallel-test.sh"
    plugins:
      - EOSIO/anka#v0.6.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TEMPLATE_NAME)
          vm-registry-tag: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TAG_BASE)::$(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)
          always-pull: true
          debug: true
          wait-network: true
          failover-registries:
            - 'registry_1'
            - 'registry_2'
      - EOSIO/skip-checkout#v0.1.1:
          cd: ~
    agents: "queue=mac-anka-node-fleet"
    retry:
      manual:
        permit_on_passed: true
    timeout: ${TIMEOUT:-60}
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}${SKIP_UNIT_TESTS}

EOF
        fi
    echo
    done
    # wasm spec tests
    echo '    # wasm spec tests'
    echo $PLATFORMS_JSON_ARRAY | jq -cr '.[]' | while read -r PLATFORM_JSON; do
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
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}${SKIP_WASM_SPEC_TESTS}

EOF
        else
            cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - WASM Spec Tests"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && $GIT_FETCH git checkout -f \$BUILDKITE_COMMIT && git submodule update --init --recursive"
      - "cd eos && buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "cd eos && ./.cicd/test.sh scripts/wasm-spec-test.sh"
    plugins:
      - EOSIO/anka#v0.6.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TEMPLATE_NAME)
          vm-registry-tag: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TAG_BASE)::$(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)
          always-pull: true
          debug: true
          wait-network: true
          failover-registries:
            - 'registry_1'
            - 'registry_2'
      - EOSIO/skip-checkout#v0.1.1:
          cd: ~
    agents: "queue=mac-anka-node-fleet"
    retry:
      manual:
        permit_on_passed: true
    timeout: ${TIMEOUT:-60}
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}${SKIP_WASM_SPEC_TESTS}

EOF
        fi
    echo
    done
    # serial tests
    echo '    # serial tests'
    echo $PLATFORMS_JSON_ARRAY | jq -cr '.[]' | while read -r PLATFORM_JSON; do
        IFS=$oIFS
        SERIAL_TESTS="$(cat tests/CMakeLists.txt | grep nonparallelizable_tests | grep -v "^#" | awk -F" " '{ print $2 }')"
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
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}${SKIP_SERIAL_TESTS}

EOF
            elif [[ "$TEST_NAME" != 'rodeos_test_eosvmoc' ]]; then
                cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - $TEST_NAME"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && $GIT_FETCH git checkout -f \$BUILDKITE_COMMIT && git submodule update --init --recursive"
      - "cd eos && buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "cd eos && ./.cicd/test.sh scripts/serial-test.sh $TEST_NAME"
    plugins:
      - EOSIO/anka#v0.6.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TEMPLATE_NAME)
          vm-registry-tag: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TAG_BASE)::$(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)
          always-pull: true
          debug: true
          wait-network: true
          failover-registries:
            - 'registry_1'
            - 'registry_2'
      - EOSIO/skip-checkout#v0.1.1:
          cd: ~
    agents: "queue=mac-anka-node-fleet"
    retry:
      manual:
        permit_on_passed: true
    timeout: ${TIMEOUT:-60}
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}${SKIP_SERIAL_TESTS}
EOF
            fi
            echo
        done
        IFS=$nIFS
    done
    # long-running tests
    echo '    # long-running tests'
    echo $PLATFORMS_JSON_ARRAY | jq -cr '.[]' | while read -r PLATFORM_JSON; do
        IFS=$oIFS
        LR_TESTS="$(cat tests/CMakeLists.txt | grep long_running_tests | grep -v "^#" | awk -F" " '{ print $2 }')"
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
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}${SKIP_LONG_RUNNING_TESTS:-true}

EOF
            else
                cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - $TEST_NAME"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && $GIT_FETCH git checkout -f \$BUILDKITE_COMMIT && git submodule update --init --recursive"
      - "cd eos && buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' ${BUILD_SOURCE} && tar -xzf build.tar.gz"
      - "cd eos && ./.cicd/test.sh scripts/long-running-test.sh $TEST_NAME"
    plugins:
      - EOSIO/anka#v0.6.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TEMPLATE_NAME)
          vm-registry-tag: $(echo "$PLATFORM_JSON" | jq -r .ANKA_TAG_BASE)::$(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)
          always-pull: true
          debug: true
          wait-network: true
          failover-registries:
            - 'registry_1'
            - 'registry_2'
      - EOSIO/skip-checkout#v0.1.1:
          cd: ~
    agents: "queue=mac-anka-node-fleet"
    retry:
      manual:
        permit_on_passed: true
    timeout: ${TIMEOUT:-180}
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}${SKIP_LONG_RUNNING_TESTS:-true}
EOF
            fi
            echo
        done
        IFS=$nIFS
    done
    IFS=$oIFS
    if [[ "$ROUND" != "$ROUNDS" ]]; then
        echo '  - wait'
        echo ''
    fi
done
# Execute multiversion test
if [[ ! "$PINNED" == 'false' || "$SKIP_MULTIVERSION_TEST" == 'false' ]]; then
        cat <<EOF
  - label: ":pipeline: Multiversion Test"
    command: 
      - "buildkite-agent artifact download build.tar.gz . --step ':ubuntu: Ubuntu 18.04 - Build' && tar -xzf build.tar.gz"
      - ./.cicd/test.sh .cicd/multiversion.sh
    env:
      IMAGE_TAG: "ubuntu-18.04-pinned"
      PLATFORM_TYPE: "pinned"
    agents:
      queue: "$BUILDKITE_TEST_AGENT_QUEUE"
    timeout: ${TIMEOUT:-30}
    skip: ${SKIP_LINUX}${SKIP_UBUNTU_18_04}${SKIP_MULTIVERSION_TEST}

EOF
fi
# trigger eosio-lrt post pr
if [[ -z $BUILDKITE_TRIGGERED_FROM_BUILD_ID && $TRIGGER_JOB == "true" ]]; then
    if ( [[ ! $PINNED == false ]] ); then
        cat <<EOF
  - label: ":pipeline: Trigger Long Running Tests"
    trigger: "eosio-lrt"
    async: true
    build:
      message: "Triggered by $BUILDKITE_PIPELINE_SLUG build $BUILDKITE_BUILD_NUMBER"
      commit: "${BUILDKITE_COMMIT}"
      branch: "${BUILDKITE_BRANCH}"
      env:
        BUILDKITE_PULL_REQUEST: "${BUILDKITE_PULL_REQUEST}"
        BUILDKITE_PULL_REQUEST_BASE_BRANCH: "${BUILDKITE_PULL_REQUEST_BASE_BRANCH}"
        BUILDKITE_PULL_REQUEST_REPO: "${BUILDKITE_PULL_REQUEST_REPO}"
        BUILDKITE_TRIGGERED_FROM_BUILD_URL: "${BUILDKITE_BUILD_URL}"
        SKIP_BUILD: "true"
        SKIP_WASM_SPEC_TESTS: "true"
        PINNED: "${PINNED}"

EOF
    fi
fi
# trigger eosio-sync-from-genesis for every build
if [[ "$BUILDKITE_PIPELINE_SLUG" == 'eosio' && -z "${SKIP_INSTALL}${SKIP_LINUX}${SKIP_DOCKER}${SKIP_SYNC_TESTS}" ]]; then
    cat <<EOF
  - label: ":chains: Sync from Genesis Test"
    trigger: "eosio-sync-from-genesis"
    async: false
    if: build.env("BUILDKITE_TAG") == null
    build:
      message: "Triggered by $BUILDKITE_PIPELINE_SLUG build $BUILDKITE_BUILD_NUMBER"
      commit: "${BUILDKITE_COMMIT}"
      branch: "${BUILDKITE_BRANCH}"
      env:
        BUILDKITE_TRIGGERED_FROM_BUILD_URL: "${BUILDKITE_BUILD_URL}"
        SKIP_JUNGLE: "${SKIP_JUNGLE}"
        SKIP_KYLIN: "${SKIP_KYLIN}"
        SKIP_MAIN: "${SKIP_MAIN}"
        TIMEOUT: "${TIMEOUT}"

EOF
fi
# trigger eosio-resume-from-state for every build
if [[ "$BUILDKITE_PIPELINE_SLUG" == 'eosio' && -z "${SKIP_INSTALL}${SKIP_LINUX}${SKIP_DOCKER}${SKIP_SYNC_TESTS}" ]]; then
    cat <<EOF
  - label: ":outbox_tray: Resume from State Test"
    trigger: "eosio-resume-from-state"
    async: false
    if: build.env("BUILDKITE_TAG") == null
    build:
      message: "Triggered by $BUILDKITE_PIPELINE_SLUG build $BUILDKITE_BUILD_NUMBER"
      commit: "${BUILDKITE_COMMIT}"
      branch: "${BUILDKITE_BRANCH}"
      env:
        BUILDKITE_TRIGGERED_FROM_BUILD_URL: "${BUILDKITE_BUILD_URL}"
        SKIP_JUNGLE: "${SKIP_JUNGLE}"
        SKIP_KYLIN: "${SKIP_KYLIN}"
        SKIP_MAIN: "${SKIP_MAIN}"
        TIMEOUT: "${TIMEOUT}"

EOF
fi
# pipeline tail
cat <<EOF
  - wait:
    continue_on_failure: true

  - label: ":bar_chart: Test Metrics"
    command:
      - "ssh-keyscan -H github.com >> ~/.ssh/known_hosts"
      - "git clone \$BUILDKITE_REPO ."
      - "$GIT_FETCH git checkout -f \$BUILDKITE_COMMIT"
      - "echo '+++ :compression: Extracting Test Metrics Code'"
      - "tar -zxf .cicd/metrics/test-metrics.tar.gz"
      - "echo '+++ :javascript: Running test-metrics.js'"
      - "node --max-old-space-size=32768 test-metrics.js"
    plugins:
      - EOSIO/skip-checkout#v0.1.1:
          cd: ~
    agents:
      queue: "$BUILDKITE_TEST_AGENT_QUEUE"
    timeout: ${TIMEOUT:-10}
    soft_fail: true

  - wait

    # packaging
  - label: ":centos: CentOS 7.7 - Package Builder"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step ':centos: CentOS 7.7 - Build' && tar -xzf build.tar.gz"
      - "./.cicd/package.sh"
    env:
      IMAGE_TAG: "centos-7.7-$PLATFORM_TYPE"
      PLATFORM_TYPE: $PLATFORM_TYPE
      OS: "el7" # OS and PKGTYPE required for lambdas
      PKGTYPE: "rpm"
    agents:
      queue: "$BUILDKITE_TEST_AGENT_QUEUE"
    timeout: ${TIMEOUT:-10}
    skip: ${SKIP_CENTOS_7_7}${SKIP_PACKAGE_BUILDER}${SKIP_LINUX}

  - label: ":centos: CentOS 8 - Package Builder"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step ':centos: CentOS 8 - Build' && tar -xzf build.tar.gz"
      - "./.cicd/package.sh"
    env:
      IMAGE_TAG: "centos-8-$PLATFORM_TYPE"
      PLATFORM_TYPE: $PLATFORM_TYPE
      OS: "el8" # OS and PKGTYPE required for lambdas
      PKGTYPE: "rpm"
    agents:
      queue: "$BUILDKITE_TEST_AGENT_QUEUE"
    timeout: ${TIMEOUT:-10}
    skip: ${SKIP_CENTOS_8}${SKIP_PACKAGE_BUILDER}${SKIP_LINUX}

  - label: ":ubuntu: Ubuntu 16.04 - Package Builder"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step ':ubuntu: Ubuntu 16.04 - Build' && tar -xzf build.tar.gz"
      - "./.cicd/package.sh"
    env:
      IMAGE_TAG: "ubuntu-16.04-$PLATFORM_TYPE"
      PLATFORM_TYPE: $PLATFORM_TYPE
      OS: "ubuntu-16.04" # OS and PKGTYPE required for lambdas
      PKGTYPE: "deb"
    agents:
      queue: "$BUILDKITE_TEST_AGENT_QUEUE"
    timeout: ${TIMEOUT:-10}
    skip: ${SKIP_UBUNTU_16_04}${SKIP_PACKAGE_BUILDER}${SKIP_LINUX}

  - label: ":ubuntu: Ubuntu 18.04 - Package Builder"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step ':ubuntu: Ubuntu 18.04 - Build' && tar -xzf build.tar.gz"
      - "./.cicd/package.sh"
    env:
      IMAGE_TAG: "ubuntu-18.04-$PLATFORM_TYPE"
      PLATFORM_TYPE: $PLATFORM_TYPE
      OS: "ubuntu-18.04" # OS and PKGTYPE required for lambdas
      PKGTYPE: "deb"
    agents:
      queue: "$BUILDKITE_TEST_AGENT_QUEUE"
    timeout: ${TIMEOUT:-10}
    skip: ${SKIP_UBUNTU_18_04}${SKIP_PACKAGE_BUILDER}${SKIP_LINUX}

  - label: ":ubuntu: Ubuntu 20.04 - Package Builder"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step ':ubuntu: Ubuntu 20.04 - Build' && tar -xzf build.tar.gz"
      - "./.cicd/package.sh"
    env:
      IMAGE_TAG: "ubuntu-20.04-$PLATFORM_TYPE"
      PLATFORM_TYPE: $PLATFORM_TYPE
      OS: "ubuntu-20.04" # OS and PKGTYPE required for lambdas
      PKGTYPE: "deb"
    agents:
      queue: "$BUILDKITE_TEST_AGENT_QUEUE"
    timeout: ${TIMEOUT:-10}
    skip: ${SKIP_UBUNTU_20_04}${SKIP_PACKAGE_BUILDER}${SKIP_LINUX}

  - label: ":darwin: macOS 10.14 - Package Builder"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && $GIT_FETCH git checkout -f \$BUILDKITE_COMMIT"
      - "cd eos && buildkite-agent artifact download build.tar.gz . --step ':darwin: macOS 10.14 - Build' && tar -xzf build.tar.gz"
      - "cd eos && ./.cicd/package.sh"
    plugins:
      - EOSIO/anka#v0.6.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: 10.14.6_6C_14G_80G
          vm-registry-tag: "clean::cicd::git-ssh::nas::brew::buildkite-agent"
          always-pull: true
          debug: true
          wait-network: true
          failover-registries:
            - 'registry_1'
            - 'registry_2'
      - EOSIO/skip-checkout#v0.1.1:
          cd: ~
    agents:
      - "queue=mac-anka-node-fleet"
    timeout: ${TIMEOUT:-30}
    skip: ${SKIP_MACOS_10_14}${SKIP_PACKAGE_BUILDER}${SKIP_MAC}

  - label: ":darwin: macOS 10.15 - Package Builder"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && $GIT_FETCH git checkout -f \$BUILDKITE_COMMIT"
      - "cd eos && buildkite-agent artifact download build.tar.gz . --step ':darwin: macOS 10.15 - Build' && tar -xzf build.tar.gz"
      - "cd eos && ./.cicd/package.sh"
    plugins:
      - EOSIO/anka#v0.6.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: 10.15.5_6C_14G_80G
          vm-registry-tag: "clean::cicd::git-ssh::nas::brew::buildkite-agent"
          always-pull: true
          debug: true
          wait-network: true
          failover-registries:
            - 'registry_1'
            - 'registry_2'
      - EOSIO/skip-checkout#v0.1.1:
          cd: ~
    agents:
      - "queue=mac-anka-node-fleet"
    timeout: ${TIMEOUT:-30}
    skip: ${SKIP_MACOS_10_15}${SKIP_PACKAGE_BUILDER}${SKIP_MAC}

  - label: ":docker: Docker - Label Container with Git Branch and Git Tag"
    command: .cicd/docker-tag.sh
    env:
      IMAGE_TAG: "ubuntu-18.04-unpinned"
      PLATFORM_TYPE: "unpinned"
    agents:
      queue: "$BUILDKITE_BUILD_AGENT_QUEUE"
    timeout: ${TIMEOUT:-10}
    skip: ${SKIP_INSTALL}${SKIP_LINUX}${SKIP_DOCKER}${SKIP_PACKAGE_BUILDER}

  - wait

  - label: ":git: Git Submodule Regression Check"
    command: "./.cicd/submodule-regression-check.sh"
    agents:
      queue: "automation-basic-builder-fleet"
    timeout: ${TIMEOUT:-5}

  - label: ":beer: Brew Updater"
    command:
      - "buildkite-agent artifact download eosio.rb . --step ':darwin: macOS 10.14 - Package Builder'"
      - "buildkite-agent artifact upload eosio.rb"
    agents:
      queue: "automation-basic-builder-fleet"
    timeout: "${TIMEOUT:-5}"
    skip: ${SKIP_PACKAGE_BUILDER}${SKIP_MAC}${SKIP_MACOS_10_14}

  - label: ":docker: :ubuntu: Docker - Build 18.04 Docker Image"
    command:  "./.cicd/create-docker-from-binary.sh"
    agents:
      queue: "$BUILDKITE_BUILD_AGENT_QUEUE"
    skip: ${SKIP_INSTALL}${SKIP_LINUX}${SKIP_DOCKER}${SKIP_PACKAGE_BUILDER}
    timeout: ${TIMEOUT:-10}

EOF
IFS=$oIFS
