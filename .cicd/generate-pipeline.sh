#!/usr/bin/env bash
set -eo pipefail
. ./.cicd/helpers/general.sh

export MOJAVE_ANKA_TAG_BASE='clean::cicd::git-ssh::nas::brew::buildkite-agent'
export MOJAVE_ANKA_TEMPLATE_NAME='10.14.4_6C_14G_40G'

# Use files in platforms dir as source of truth for what platforms we need to generate steps for
export PLATFORMS_JSON_ARRAY='[]'
for FILE in $(ls $CICD_DIR/platforms); do

  # Ability to skip mac or linux by not even creating the json block
  ( [[ $SKIP_MAC == true ]] && [[ $FILE =~ 'macos' ]] ) && continue
  ( [[ $SKIP_LINUX == true ]] &&  [[ ! $FILE =~ 'macos' ]] ) && continue
  
  # Prevent using both platform files (only use unpinned or pinned)
  if [[ $PINNED == false || $UNPINNED == true ]] && [[ ! $FILE =~ 'macos' ]]; then
    [[ ! $FILE =~ 'unpinned' ]] && continue
  else
    [[ $FILE =~ 'unpinned' ]] && continue
  fi

  export FILE_NAME=$(echo $FILE | awk '{split($0,a,/\.(d|s)/); print a[1] }')
  export PLATFORM_NAME=$(echo $FILE_NAME | cut -d- -f1 | sed 's/os/OS/g')
  export PLATFORM_NAME_UPCASE=$(echo $PLATFORM_NAME | tr a-z A-Z)
  export VERSION_MAJOR=$(echo $FILE_NAME | cut -d- -f2 | cut -d. -f1)
  [[ $(echo $FILE_NAME | cut -d- -f2) =~ '.' ]] && export VERSION_MINOR="_$(echo $FILE_NAME | cut -d- -f2 | cut -d. -f2)" || export VERSION_MINOR=''
  export VERSION_FULL=$(echo $FILE_NAME | cut -d- -f2)
  OLDIFS=$IFS
  IFS="_"
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

oIFS="$IFS"
IFS=$'' 
nIFS=$IFS # Needed to fix array splitting (\n won't work)

###################
# Anka Ensure Tag #
echo $PLATFORMS_JSON_ARRAY | jq -cr ".[]" | while read -r PLATFORM_JSON; do
  if [[ $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME) =~ 'macos' ]]; then
  cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) Anka - Ensure $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) Template Dependency Tag"
    command:
      - "git clone git@github.com:EOSIO/mac-anka-fleet.git"
      - "cd mac-anka-fleet && . ./ensure_tag.bash -u 12 -r 25G -a '-n'"
    agents:
      - "queue=mac-anka-templater-fleet"
    env:
      REPO: ${BUILDKITE_PULL_REQUEST_REPO:-$BUILDKITE_REPO}
      REPO_COMMIT: $BUILDKITE_COMMIT
      TEMPLATE: $MOJAVE_ANKA_TEMPLATE_NAME
      TEMPLATE_TAG: $MOJAVE_ANKA_TAG_BASE
      TAG_COMMANDS: "git clone ${BUILDKITE_PULL_REQUEST_REPO:-$BUILDKITE_REPO} eos && cd eos && git checkout $BUILDKITE_COMMIT && git submodule update --init --recursive && ./.cicd/platforms/macos-10.14.sh && ./.cicd/build.sh && cd .. && rm -rf eos"
      PROJECT_TAG: $(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)
    timeout: ${TIMEOUT:-320}
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}\${SKIP_ENSURE_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}

EOF
  fi
done


echo "  - wait"; echo ""

###############
# BUILD STEPS #
echo $PLATFORMS_JSON_ARRAY | jq -cr ".[]" | while read -r PLATFORM_JSON; do
  if [[ ! $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME) =~ 'macos' ]]; then

  cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build"
    command:
      - "./.cicd/build.sh"
      - "tar -pczf build.tar.gz build && buildkite-agent artifact upload build.tar.gz"
    env:
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      BUILDKITE_AGENT_ACCESS_TOKEN:
    agents:
      queue: "automation-eos-builder-fleet"
    timeout: ${TIMEOUT:-60}
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}\${SKIP_BUILD}

EOF

  else

  cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && git checkout \$BUILDKITE_COMMIT && git submodule update --init --recursive"
      - "cd eos && ./.cicd/build.sh"
      - "cd eos && tar -pczf build.tar.gz build && buildkite-agent artifact upload build.tar.gz"
    plugins:
      - chef/anka#v0.5.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: ${MOJAVE_ANKA_TEMPLATE_NAME}
          vm-registry-tag: "${MOJAVE_ANKA_TAG_BASE}::$(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)"
          modify-cpu: 12
          modify-ram: 24
          always-pull: true
          debug: true
          wait-network: true
    agents:
      - "queue=mac-anka-large-node-fleet"
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}\${SKIP_BUILD}

EOF

  fi
done

echo "  - wait"; echo ""

##############
# UNIT TESTS #
echo $PLATFORMS_JSON_ARRAY | jq -cr ".[]" | while read -r PLATFORM_JSON; do
  if [[ ! $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME) =~ 'macos' ]]; then

  cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Unit Tests"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "./.cicd/parallel-tests.sh"
    env:
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      BUILDKITE_AGENT_ACCESS_TOKEN:
    agents:
      queue: "automation-eos-builder-fleet"
    timeout: ${TIMEOUT:-10}
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}\${SKIP_UNIT_TESTS}

EOF

  else

  cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Unit Tests"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && git checkout \$BUILDKITE_COMMIT && git submodule update --init --recursive"
      - "cd eos && buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "cd eos && ./.cicd/parallel-tests.sh"
    plugins:
      - chef/anka#v0.5.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: ${MOJAVE_ANKA_TEMPLATE_NAME}
          vm-registry-tag: "${MOJAVE_ANKA_TAG_BASE}::$(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)"
          always-pull: true
          debug: true
          wait-network: true
    agents:
      - "queue=mac-anka-node-fleet"
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}\${SKIP_UNIT_TESTS}

EOF

  fi
done

################
# SERIAL TESTS #
echo $PLATFORMS_JSON_ARRAY | jq -cr ".[]" | while read -r PLATFORM_JSON; do
  IFS=$oIFS
  SERIAL_TESTS=$(cat tests/CMakeLists.txt | grep nonparallelizable_tests | awk -F" " '{ print $2 }')

  for TEST_NAME in $SERIAL_TESTS; do

    if [[ ! $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME) =~ 'macos' ]]; then

  cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - $TEST_NAME"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "./.cicd/parallel-tests.sh"
    env:
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      BUILDKITE_AGENT_ACCESS_TOKEN:
    agents:
      queue: "automation-eos-builder-fleet"
    timeout: ${TIMEOUT:-10}
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}\${SKIP_SERIAL_TESTS}

EOF

    else

  cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - $TEST_NAME"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && git checkout \$BUILDKITE_COMMIT && git submodule update --init --recursive"
      - "cd eos && buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "cd eos && ./.cicd/serial-tests.sh $TEST_NAME"
      - "cd eos && mv build/Testing/\$(ls build/Testing/ | grep '20' | tail -n 1)/Test.xml test-results.xml && buildkite-agent artifact upload test-results.xml"
    plugins:
      - chef/anka#v0.5.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: ${MOJAVE_ANKA_TEMPLATE_NAME}
          vm-registry-tag: "${MOJAVE_ANKA_TAG_BASE}::$(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)"
          always-pull: true
          debug: true
          wait-network: true
    agents:
      - "queue=mac-anka-node-fleet"
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}\${SKIP_SERIAL_TESTS}
EOF

    fi

  done
  IFS=$nIFS


done


#############
# LRT TESTS #
echo $PLATFORMS_JSON_ARRAY | jq -cr ".[]" | while read -r PLATFORM_JSON; do
  if [[ ! $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME) =~ 'macos' ]]; then

  cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Long-Running Tests"
    command:
      - "buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "./.cicd/parallel-tests.sh"
    env:
      IMAGE_TAG: $(echo "$PLATFORM_JSON" | jq -r .FILE_NAME)
      BUILDKITE_AGENT_ACCESS_TOKEN:
    agents:
      queue: "automation-eos-builder-fleet"
    timeout: ${TIMEOUT:-10}
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}\${SKIP_LONG_RUNNING_TESTS:-true}

EOF

  else

  cat <<EOF
  - label: "$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Long-Running Tests"
    command:
      - "git clone \$BUILDKITE_REPO eos && cd eos && git checkout \$BUILDKITE_COMMIT && git submodule update --init --recursive"
      - "cd eos && buildkite-agent artifact download build.tar.gz . --step '$(echo "$PLATFORM_JSON" | jq -r .ICON) $(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_FULL) - Build' && tar -xzf build.tar.gz"
      - "cd eos && ./.cicd/long-running-tests.sh"
      - "cd eos && mv build/Testing/\$(ls build/Testing/ | grep '20' | tail -n 1)/Test.xml test-results.xml && buildkite-agent artifact upload test-results.xml"
    plugins:
      - chef/anka#v0.5.1:
          no-volume: true
          inherit-environment-vars: true
          vm-name: ${MOJAVE_ANKA_TEMPLATE_NAME}
          vm-registry-tag: "${MOJAVE_ANKA_TAG_BASE}::$(echo "$PLATFORM_JSON" | jq -r .HASHED_IMAGE_TAG)"
          modify-cpu: 12
          modify-ram: 24
          always-pull: true
          debug: true
          wait-network: true
    agents:
      - "queue=mac-anka-large-node-fleet"
    skip: \${SKIP_$(echo "$PLATFORM_JSON" | jq -r .PLATFORM_NAME_UPCASE)_$(echo "$PLATFORM_JSON" | jq -r .VERSION_MAJOR)$(echo "$PLATFORM_JSON" | jq -r .VERSION_MINOR)}\${SKIP_LONG_RUNNING_TESTS:-true}

EOF

  fi
done

IFS=$oIFS