#!/bin/bash
set -eo pipefail
export POP_FILE_NAME="populated-$(echo $FILE)"
# Collect commands from code block, add RUN before the start of commands, and add it to temporary template
DOC_CODE_BLOCK=$(cat docs/dep-install-$FILE_NAME.md | sed -n '/```/,/```/p')
SANITIZED_COMMANDS=$(echo "$DOC_CODE_BLOCK" | grep -v -e '```' -e '\#.*' -e '^$')
if [[ $FILE =~ '.dockerfile' ]]; then # Linux / Docker
    DOCKER_COMMANDS=$(echo "$SANITIZED_COMMANDS" | awk '{if ( $0 ~ /^[ ].*/ ) { print $0 } else if ( $0 ~ /^PATH/ ) { print "ENV " $0 } else { print "RUN " $0 } }')
else # Mac OSX
    DOCKER_COMMANDS=$(echo "$SANITIZED_COMMANDS")
fi
echo "$DOCKER_COMMANDS" > /tmp/docker-commands
awk 'NR==4{print;system("cat /tmp/docker-commands");next} 1' .cicd/platform-templates/$PLATFORM_TYPE/$FILE > /tmp/$POP_FILE_NAME