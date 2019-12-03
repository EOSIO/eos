#!/bin/bash
set -eo pipefail

# Parse docs file
export FILE_NAME_STRIPPED=$(echo $FILE_NAME | sed -E 's/(-pinned|-unpinned)//')
export POP_FILE_NAME="populated-$(echo $FILE)"

# Collect commands from code block, add RUN before the start of commands, and add it to temporary template
DOC_CODE_BLOCK=$(cat docs/dep-install-$FILE_NAME_STRIPPED.md | sed -n '/```/,/```/p')
SANITIZED_COMMANDS=$(echo "$DOC_CODE_BLOCK" | grep -v -e '```' -e '\#.*' -e '^$')
DOCKER_COMMANDS=$(echo "$SANITIZED_COMMANDS" | awk '{if ( $0 ~ /^[ ].*/ ) { print $0 } else if ( $0 ~ /^PATH/ ) { print "ENV " $0 } else { print "RUN " $0 } }')
echo "$DOCKER_COMMANDS" > /tmp/docker-commands
awk 'NR==3{print;system("cat /tmp/docker-commands");next} 1' .cicd/platform-templates/$PLATFORM_TYPE/$FILE > .cicd/platform-templates/$PLATFORM_TYPE/$POP_FILE_NAME