#!/usr/bin/env bash
set -eo pipefail
. ./.cicd/helpers/general.sh

rm -f /tmp/serial_tests

# Use dockerfiles as source of truth for what platforms to use
echo "/# SERIAL TESTS/ {\n "
for DOCKERFILE in $(ls $CICD_DIR/docker); do

    DOCKERFILE_NAME=$(echo $DOCKERFILE | cut -d. -f1)
    PLATFORM_NAME=$(echo $DOCKERFILE_NAME | cut -d- -f1)
    PLATFORM_NAME_UPCASE=$(echo $PLATFORM_NAME | tr a-z A-Z)
    VERSION=$(echo $DOCKERFILE_NAME | cut -d- -f2)
    OLDIFS=$IFS;IFS="_";set $PLATFORM_NAME;IFS=$OLDIFS
    PLATFORM_NAME_FULL="$(capitalize $1)$( [[ ! -z $2 ]] && echo "_$(capitalize $2)" || true ) $VERSION"
    [[ $PLATFORM_NAME =~ 'amazon' ]] && ICON=':aws:'
    [[ $PLATFORM_NAME =~ 'ubuntu' ]] && ICON=':ubuntu:'
    [[ $PLATFORM_NAME =~ 'centos' ]] && ICON=':centos:'

    for TEST_NAME in $(cat tests/CMakeLists.txt | grep nonparallelizable_tests | awk -F" " '{ print $2 }'); do
    
cat <<EOF
- label: "$ICON $PLATFORM_NAME_FULL - $TEST_NAME"
  command:
    - "buildkite-agent artifact download build.tar.gz . --step '$ICON $PLATFORM_NAME_FULL - Build' && tar -xzf build.tar.gz"
    - "bash ./.cicd/serial-tests.sh $TEST_NAME"
  env:
    IMAGE_TAG: "$DOCKERFILE_NAME"
    BUILDKITE_AGENT_ACCESS_TOKEN:
  agents:
    queue: "automation-eos-builder-fleet"
  timeout: ${TIMEOUT:-10}
  skip: \${SKIP_${PLATFORM_NAME_UPCASE}_${VERSION}}\${SKIP_SERIAL_TESTS}
EOF
        # replace "# SERIAL TESTS" in pipeline.yml with what we've generated
    done
done


#buildkite-agent pipeline upload .cicd/pipeline.yml