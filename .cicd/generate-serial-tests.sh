#!/usr/bin/env bash
set -eo pipefail
. ./.cicd/helpers/general.sh
SERIAL_TESTS=$(cat tests/CMakeLists.txt | grep nonparallelizable_tests | awk -F" " '{ print $2 }')
# Use dockerfiles as source of truth for what platforms to use
## Linux
for DOCKERFILE in $(ls $CICD_DIR/docker); do
    [[ $DOCKERFILE =~ 'unpinned' ]] && continue
    DOCKERFILE_NAME=$(echo $DOCKERFILE | awk -F'.dockerfile' '{ print $1 }')
    PLATFORM_NAME=$(echo $DOCKERFILE_NAME | cut -d- -f1 | sed 's/os/OS/g')
    PLATFORM_NAME_UPCASE=$(echo $PLATFORM_NAME | tr a-z A-Z)
    VERSION_MAJOR=$(echo $DOCKERFILE_NAME | cut -d- -f2 | cut -d. -f1)
    VERSION_FULL=$(echo $DOCKERFILE_NAME | cut -d- -f2)
    OLDIFS=$IFS;IFS="_";set $PLATFORM_NAME;IFS=$OLDIFS
    PLATFORM_NAME_FULL="$(capitalize $1)$( [[ ! -z $2 ]] && echo "_$(capitalize $2)" || true ) $VERSION_FULL"
    [[ $DOCKERFILE_NAME =~ 'amazon' ]] && ICON=':aws:'
    [[ $DOCKERFILE_NAME =~ 'ubuntu' ]] && ICON=':ubuntu:'
    [[ $DOCKERFILE_NAME =~ 'centos' ]] && ICON=':centos:'
    for TEST_NAME in $SERIAL_TESTS; do
cat <<EOF
- label: "$ICON $PLATFORM_NAME_FULL - $TEST_NAME"
  command:
    - "buildkite-agent artifact download build.tar.gz . --step '$ICON $PLATFORM_NAME_FULL - Build' && tar -xzf build.tar.gz"
    - "./.cicd/serial-tests.sh $TEST_NAME"
    - "mv build/Testing/\$(ls build/Testing/ | grep '20' | tail -n 1)/Test.xml test-results.xml && buildkite-agent artifact upload test-results.xml"
  env:
    IMAGE_TAG: "$DOCKERFILE_NAME"
    BUILDKITE_AGENT_ACCESS_TOKEN:
  agents:
    queue: "automation-eos-builder-fleet"
  timeout: ${TIMEOUT:-10}
  skip: \${SKIP_${PLATFORM_NAME_UPCASE}_${VERSION_MAJOR}}\${SKIP_SERIAL_TESTS}
EOF
    done
done
# Darwin
for TEST_NAME in $SERIAL_TESTS; do
cat <<EOF
- label: ":darwin: macOS 10.14 - $TEST_NAME"
  command:
    - "brew install git graphviz libtool gmp llvm@4 pkgconfig python python@2 doxygen libusb openssl boost@1.70 cmake mongodb"
    - "git clone \$BUILDKITE_REPO eos && cd eos && git checkout \$BUILDKITE_COMMIT && git submodule update --init --recursive"
    - "cd eos && buildkite-agent artifact download build.tar.gz . --step ':darwin: macOS 10.14 - Build' && tar -xzf build.tar.gz"
    - "cd eos && ./.cicd/serial-tests.sh $TEST_NAME"
    - "cd eos && mv build/Testing/\$(ls build/Testing/ | grep '20' | tail -n 1)/Test.xml test-results.xml && buildkite-agent artifact upload test-results.xml"
  plugins:
    - chef/anka#v0.5.1:
        no-volume: true
        inherit-environment-vars: true
        vm-name: 10.14.4_6C_14G_40G
        vm-registry-tag: "clean::cicd::git-ssh::nas::brew::buildkite-agent"
        always-pull: true
        debug: true
        wait-network: true
  agents:
    - "queue=mac-anka-node-fleet"
  skip: \${SKIP_MOJAVE}\${SKIP_SERIAL_TESTS}
EOF
done